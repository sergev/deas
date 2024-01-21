#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include "mpilib.h"
#include "mpiio.h"
#include "idea.h"
#include "md5.h"
#include "pgplib.h"
#include "randpool.h"

#define convert(x) convert_byteorder((byteptr)&(x), sizeof(x))

char *crypt_pubfile = "pubring.pgp";
char *crypt_secfile = "secring.pgp";

/* Version byte for data structures created by this version of PGP */
#define	VERSION_BYTE_OLD	2	/* PGP2 */
#define	VERSION_BYTE_NEW	3

/* Cipher Type Byte (CTB) definitions follow...*/
#define CTB_TYPE_MASK           0x7c
#define CTB_LLEN_MASK 0x03
#define is_ctb_type(ctb,type)   (((ctb) & CTB_TYPE_MASK)==(4*type))

/* "length of length" field of packet, in bytes (1, 2, 4, 8 bytes): */
#define ctb_llength(ctb) ((int) 1 << (int) ((ctb) & CTB_LLEN_MASK))

#define CTB_SKE_TYPE            2       /* packet signed with RSA secret key */
#define CTB_CERT_SECKEY_TYPE    5       /* secret key certificate */
#define CTB_CERT_PUBKEY_TYPE    6       /* public key certificate */
#define CTB_KEYCTRL_TYPE        12      /* key control packet */
#define CTB_USERID_TYPE         13      /* user id packet */
#define CTB_COMMENT_TYPE        14      /* comment packet */
#define CTB_EOF                 0x1a

#define CTB_BYTE(type,llen)     (0x80 + (4*type) + llen)
#define CTB_USERID              CTB_BYTE(CTB_USERID_TYPE,0)
#define CTB_CERT_SECKEY         CTB_BYTE(CTB_CERT_SECKEY_TYPE,1)
#define CTB_CERT_PUBKEY         CTB_BYTE(CTB_CERT_PUBKEY_TYPE,1)
#define CTB_KEYCTRL             CTB_BYTE(CTB_KEYCTRL_TYPE,0)
#define CTB_SKE                 CTB_BYTE(CTB_SKE_TYPE,1)

#define MPILEN             (2+MAX_BYTE_PRECISION)
#define MAX_KEYCERT_LENGTH (1+2+1+4+2+1 +(2*MPILEN) +1+8 +(4*MPILEN) +2)
#define KEYFRAGSIZE        8   /* # of bytes in key ID modulus fragment */
#define SIZEOF_TIMESTAMP   4   /* 32-bit timestamp */

#define RSA_ALGORITHM_BYTE      1       /* use RSA */
#define IDEA_ALGORITHM_BYTE     1       /* use the IDEA cipher */

//int rsa_public_encrypt (void *outbuf, void *inbuf, short bytes,
//	void *E, void *N);
int rsa_private_encrypt (void *outbuf, void *inbuf, short bytes,
	void *E, void *D, void *P, void *Q, void *U, void *N);
int rsa_public_decrypt (void *outbuf, void *inbuf,
	void *E, void *N);
//int rsa_private_decrypt (void *outbuf, void *inbuf,
//	void *E, void *D, void *P, void *Q, void *U, void *N);

/* Returns the length of a packet according to the CTB and
 * the length field. */
word32 getpastlength(byte ctb, FILE *f)
{
	word32 length;
	unsigned int llength;	/* length of length */
	byte buf[8];

	fill0(buf,sizeof(buf));
	length = 0L;
	/* Use ctb length-of-length field... */
	llength = ctb_llength(ctb);	/* either 1, 2, 4, or 8 */
	if (llength==8) /* 8 means no length field, assume huge length */
		return -1L;	/* return huge length */

	/* now read in the actual length field... */
	if (fread((byteptr) buf,1,llength,f) < llength)
		return (-2L); /* error -- read failure or premature eof */
	/* convert length from external byteorder... */
	if (llength==1)
		length = (word32) buf[0];
	if (llength==2)
		length = (word32) fetch_word16(buf);
	if (llength==4)
		length = fetch_word32(buf);
	return length;
} /* getpastlength */

/* Checksum maintained as a running sum by read_mpi and write_mpi.
 * The checksum is maintained based on the plaintext values being
 * read and written.  To use it, store a 0 to it before doing a set
 * of read_mpi's or write_mpi's.  Then read it aftwerwards.
 */
word16	mpi_checksum;

/*
 * Read a mutiprecision integer from a file.
 * adjust_precision is TRUE iff we should call set_precision to the
 * size of the number read in.
 * scrambled is TRUE iff field is encrypted (protects secret key fields).
 * Returns the bitcount of the number read in, or returns a negative
 * number if an error is detected.
 */
int read_mpi(unitptr r, FILE *f, boolean adjust_precision,
             struct IdeaCfbContext *cfb)
{
	byte buf[MAX_BYTE_PRECISION+2];
	unsigned int count;
	word16 bytecount,bitcount;

	mp_init(r,0);

	if ((count = fread(buf,1,2,f)) < 2)
		return (-1); /* error -- read failure or premature eof */

	bitcount = fetch_word16(buf);
	if (bits2units(bitcount) > global_precision)
		return -1;	/* error -- possible corrupted bitcount */

	bytecount = bits2bytes(bitcount);

	count = fread(buf+2,1,bytecount,f);
	if (count < bytecount)
		return -1;	/* error -- premature eof */

	if (cfb) {	/* decrypt the field */
		ideaCfbSync(cfb);
		ideaCfbDecrypt(cfb, buf+2, buf+2, bytecount);
	}

	/* Update running checksum, in case anyone cares... */
	mpi_checksum += checksum (buf, bytecount+2);

	/*	We assume that the bitcount prefix we read is an exact
		bitcount, not rounded up to the next byte boundary.
		Otherwise we would have to call mpi2reg, then call
		countbits, then call set_precision, then recall mpi2reg
		again.
	*/
	if (adjust_precision && bytecount) {
		/* set the precision to that specified by the number read. */
		if (bitcount > MAX_BIT_PRECISION-SLOP_BITS)
			return -1;
		set_precision(bits2units(bitcount+SLOP_BITS));
		/* Now that precision is optimally set, call mpi2reg */
	}

	if (mpi2reg(r,buf) == -1)	/* convert to internal format */
		return -1;
	burn(buf);	/* burn sensitive data on stack */
	return (bitcount);
}	/* read_mpi */

/*
 * Reads a key certificate from the current file position of file f.
 * Depending on the certificate type, it will set the proper fields
 * of the return arguments.  Other fields will not be set.
 * pctb is always set.
 * If the packet is CTB_CERT_PUBKEY or CTB_CERT_SECKEY, it will
 * return timestamp, n, e, and if the secret key components are
 * present and d is not NULL, it will read, decrypt if hidekey is
 * true, and return d, p, q, and u.
 * If the packet is CTB_KEYCTRL, it will return keyctrl as that byte.
 * If the packet is CTB_USERID, it will return userid.
 * If the packet is CTB_COMMENT_TYPE, it won't return anything extra.
 * The file pointer is left positioned after the certificate.
 *
 * If the key could not be read because of a version error or bad
 * data, the return value is -6 or -4, the file pointer will be
 * positioned after the certificate, only the arguments pctb and
 * userid will valid in this case, other arguments are undefined.
 * Return value -3 means the error is unrecoverable.
 */
short readkeypacket (FILE *f, struct IdeaCfbContext *cfb, byte *pctb,
	char *userid, void *n, void *e, void *d, void *p, void *q, void *u,
	byte *sigkeyID, byte *keyctrl)
{
    byte ctb;
    word16 cert_length;
    int count;
    byte version, alg, mdlen;
    word16 validity;
    word16 chksum;
    extern word16 mpi_checksum;
    long next_packet;
    byte iv[8];

/*** Begin certificate header fields ***/
    count = fread(&ctb, 1, 1, f);	/* read key certificate CTB byte */
    if (count == 0)
	return -1;		/* premature eof */

    if (pctb)
	*pctb = ctb;            /* returns type to caller */
    if ((ctb != CTB_CERT_PUBKEY) && (ctb != CTB_CERT_SECKEY) &&
	(ctb != CTB_USERID) && (ctb != CTB_KEYCTRL) &&
	!is_ctb_type(ctb, CTB_SKE_TYPE) &&
	!is_ctb_type(ctb, CTB_COMMENT_TYPE))
	/* Either bad key packet or X/Ymodem padding detected */
	return (ctb == CTB_EOF) ? -1 : -2;

    cert_length = getpastlength(ctb, f);	/* read certificate length */

    if (cert_length > MAX_KEYCERT_LENGTH - 3)
	return -3;		/* bad length */

    next_packet = ftell(f) + cert_length;

    /*
     * skip packet and return, keeps us in sync when we hit a
     * version error or bad data.  Implemented oddly to make it
     * only one statement.
     */
#define SKIP_RETURN(x) return fseek(f, next_packet, SEEK_SET), x

    if (ctb == CTB_USERID) {
	if (cert_length > 255)
	    return -3;		/* Bad length error */
	if (userid) {
	    userid[0] = cert_length;	/* Save user ID length */
	    fread(userid + 1, 1, cert_length, f);    /* read rest of user ID */
	} else
	    fseek(f, (long) cert_length, SEEK_CUR);
	return 0;		/* normal return */

    } else if (is_ctb_type(ctb, CTB_SKE_TYPE)) {

	if (sigkeyID) {
	    fread(&version, 1, 1, f);	/* Read version of sig packet */
	    if (version != VERSION_BYTE_OLD && version != VERSION_BYTE_NEW)
		SKIP_RETURN(-6);	/* Need a later version */
	    /* Skip timestamp, validity period, and type byte */
	    fread(&mdlen, 1, 1, f);
	    fseek(f, (long) mdlen, SEEK_CUR);
	    /* Read and return KEY ID */
	    fread(sigkeyID, 1, KEYFRAGSIZE, f);
	}
	SKIP_RETURN(0);		/* normal return */

    } else if (ctb == CTB_KEYCTRL) {

	if (cert_length != 1)
	    return -3;		/* Bad length error */
	if (keyctrl)
	    fread(keyctrl, 1, cert_length, f);	/* Read key control byte */
	else
	    fseek(f, (long) cert_length, SEEK_CUR);
	return 0;		/* normal return */

    } else if (ctb != CTB_CERT_PUBKEY && ctb != CTB_CERT_SECKEY) /* comment or other packet */
	SKIP_RETURN(0);		/* normal return */

    /* Here we have a key packet */
    if (n != NULL)
	set_precision(MAX_UNIT_PRECISION);	/* safest opening assumption */
    fread(&version, 1, 1, f);	/* read and check version */
    if (version != VERSION_BYTE_OLD && version != VERSION_BYTE_NEW)
	SKIP_RETURN(-6);	/* Need a later version */
    fseek(f, (long) SIZEOF_TIMESTAMP, SEEK_CUR);
    fread(&validity, 1, sizeof(validity), f);	/* Read validity period */
    convert(validity);		/* convert from external byteorder */
    /* We don't use validity period yet */
    fread(&alg, 1, 1, f);
    if (alg != RSA_ALGORITHM_BYTE)
	SKIP_RETURN(-6);	/* Need a later version */
/*** End certificate header fields ***/

    /* We're past certificate headers, now look at some key material... */

    cert_length -= 1 + SIZEOF_TIMESTAMP + 2 + 1;

    if (n == NULL)		/* Skip key certificate data */
	SKIP_RETURN(0);

    if (read_mpi(n, f, TRUE, FALSE) < 0)
	SKIP_RETURN(-4);	/* data corrupted, return error */

    /* Note that precision was adjusted for n */

    if (read_mpi(e, f, FALSE, FALSE) < 0)
	SKIP_RETURN(-4);	/* data corrupted, error return */

    cert_length -= (countbytes(n) + 2) + (countbytes(e) + 2);

    if (d == NULL) {		/* skip rest of this key certificate */
	if (cert_length && !is_ctb_type(ctb,CTB_CERT_SECKEY_TYPE))
	    SKIP_RETURN(-4);	/* key w/o userID */
        else
	    SKIP_RETURN(0);	/* Normal return */
    }

    if (is_ctb_type(ctb,CTB_CERT_SECKEY_TYPE)) {
	fread(&alg, 1, 1, f);
	if (alg && alg != IDEA_ALGORITHM_BYTE)
	    SKIP_RETURN(-6);	/* Unknown version */

	if (!cfb && alg)
	    /* Don't bother trying if hidekey is false and alg is true */
	    SKIP_RETURN(-5);
	if (alg) {		/* if secret components are encrypted... */
	    /* process encrypted CFB IV before reading secret components */
	    count = fread(iv, 1, 8, f);
	    if (count < 8)
		return -4;	/* data corrupted, error return */

	    ideaCfbDecrypt(cfb, iv, iv, 8);
	    cert_length -= 8;	/* take IV length into account */
	}
	/* Reset checksum before these reads */
	mpi_checksum = 0;

	if (read_mpi(d, f, FALSE, cfb) < 0)
	    return -4;		/* data corrupted, error return */
	if (read_mpi(p, f, FALSE, cfb) < 0)
	    return -4;		/* data corrupted, error return */
	if (read_mpi(q, f, FALSE, cfb) < 0)
	    return -4;		/* data corrupted, error return */

	/* use register 'u' briefly as scratchpad */
	mp_mult(u, p, q);	/* compare p*q against n */
	if (mp_compare(n, u) != 0)      /* bad pass phrase? */
	    return -5;		/* possible bad pass phrase, error return */

	/* now read in real u */
	if (read_mpi(u, f, FALSE, cfb) < 0)
	    return -4;		/* data corrupted, error return */

	/* Read checksum, compare with mpi_checksum */
	fread(&chksum, 1, sizeof(chksum), f);
	convert(chksum);
	if (chksum != mpi_checksum)
	    return -5;		/* possible bad pass phrase */

	cert_length -= 1 + (countbytes(d) + 2) + (countbytes(p) + 2)
	    + (countbytes(q) + 2) + (countbytes(u) + 2) + 2;

    } else {			/* not a secret key */

	mp_init(d, 0);
	mp_init(p, 0);
	mp_init(q, 0);
	mp_init(u, 0);
    }

    if (cert_length != 0)
	SKIP_RETURN(-4);	/* data corrupted, error return */
    return 0;			/* normal return */
}

/*
 * Userid contains a C string search target of
 * userid to find in keyfile.
 * keyfile is the file to begin search in, and it may be modified
 * to indicate true filename of where the key was found.  It can be
 * either a public key file or a secret key file.
 * file_position is returned as the byte offset within the keyfile
 * that the key was found at.  pktlen is the length of the key packet.
 * These values are for the key packet itself, not including any
 * following userid, control, signature, or comment packets.
 *
 * Returns -6 if the key was found but the key was not read because of a
 * version error or bad data.  The arguments timestamp, n and e are
 * undefined in this case.
 */
int getpublickey (char *keyfile, long *_file_position, char *userid,
	void *n, void *e)
{
	byte ctb;                       /* returned by readkeypacket */
	FILE *f;
	int status, keystatus = -1;
	char matchid[256];
	long fpos;
	long file_position = 0;

	/* open file f for read, in binary (not text) mode... */
	f = fopen (keyfile, "rb");
	if (! f)
		return -1;
	strcpy (matchid, (char*) userid);
	for (;;) {
		fpos = ftell (f);
		status = readkeypacket (f, 0, &ctb, userid, n, e, 0, 0, 0, 0, 0, 0);
		if (status < 0 && status != -4 && status != -6) {
			fclose (f);
			return status;
		}

		/* Only check for matches when we find a USERID packet */
		if (ctb == CTB_USERID && strstr (userid, matchid)) {
			/* found key, normal return */
			if (_file_position)
				*_file_position = file_position;
			fclose (f);
			return keystatus;
		}

		/* Remember packet position and size for last key packet */
		if (ctb == CTB_CERT_PUBKEY || ctb == CTB_CERT_SECKEY) {
			file_position = fpos;
			keystatus = status;
		}
	}
}

/*
 * keyID contains key fragment we expect to find in keyfile.
 * If keyID is NULL, then userid contains search target of
 * userid to find in keyfile.
 * giveup controls whether we ask the user for the name of the
 * secret key file on failure.  showkey controls whether we print
 * out the key information when we find it.  keyfile, if non-NULL,
 * is the name of the secret key file; if NULL, we use the
 * default.  hpass and hkey, if non-NULL, get returned with a copy
 * of the hashed password buffer and hidekey variable.
 */
int getsecretkey (unsigned char *ideakey, char *keyfile, char *userid,
	void *n, void *e, void *d, void *p, void *q, void *u)
{
	FILE *f;
	long file_position;
	int status;

	status = getpublickey (keyfile, &file_position, userid, n, e);
	if (status < 0)
		return status;

	/* open file f for read, in binary (not text) mode... */
	f = fopen (keyfile, "rb");
	if (! f)
		return -1;

	/* First guess is no password */
	fseek (f, file_position, SEEK_SET);  /* reposition file to key */
	status = readkeypacket (f, 0, 0, userid, n, e, d, p, q, u, 0, 0);

	if (status == -5) {          /* Anything except bad password */
		struct IdeaCfbContext cfb;

		ideaCfbInit (&cfb, ideakey);
		fseek (f, file_position, SEEK_SET);
		status = readkeypacket (f, &cfb, 0, userid, n, e, d, p, q, u, 0, 0);
		ideaCfbDestroy (&cfb);
	}
	fclose (f);

	if (status < 0)
	    return -1;

	/* No effective check of pass phrase if d is NULL */
	if (d && testeq ((unitptr)d, 0))
		return -1;      /* didn't get secret key components */
	return 0;
}

int cryptRandByte ()
{
	return randPoolGetByte ();
}

int public_encrypt (void *fp, void *tp, int len, char *username)
{
	char *from = fp, *to = tp;
	char userid[MAX_BYTE_PRECISION];
	char N [MAX_BYTE_PRECISION],  E [MAX_BYTE_PRECISION];
	int err, outlen, q, n;

	set_precision (bytes2units (64));
	strncpy (userid, username, MAX_BYTE_PRECISION);
	err = getpublickey (crypt_pubfile, 0, userid, N, E);
	if (err < 0)
		return err;

	q = countbytes ((void*) N);
	for (outlen=0; len>0; outlen+=q) {
		n = q-4;
		if (n > len)
			n = len;
		err = rsa_public_encrypt (to, from, n, E, N);
		if (err)
			return -1;
		to += q;
		from += n;
		len -= n;
	}
	return outlen;
}

int private_encrypt (void *fp, void *tp, int len, char *username, char *password)
{
	char *from = fp, *to = tp;
	char userid[MAX_BYTE_PRECISION], passwd[16];
	char N [MAX_BYTE_PRECISION], E [MAX_BYTE_PRECISION];
	char D [MAX_BYTE_PRECISION], P [MAX_BYTE_PRECISION];
	char Q [MAX_BYTE_PRECISION], U [MAX_BYTE_PRECISION];
	int err, outlen, q, n;
	struct MD5Context mdContext;

	/* Calculate the hash */
	MD5Init(&mdContext);
	MD5Update(&mdContext, password, strlen (password));
	MD5Final(passwd, &mdContext);

	set_precision (bytes2units (64));
	strncpy (userid, username, MAX_BYTE_PRECISION);
	err = getsecretkey (passwd, crypt_secfile, userid, N, E, D, P, Q, U);
	if (err < 0)
		return err;

	q = countbytes ((void*) N);
	for (outlen=0; len>0; outlen+=q) {
		n = q-4;
		if (n > len)
			n = len;
		err = rsa_private_encrypt (to, from, n, E, D, P, Q, U, N);
		if (err)
			return -1;
		to += q;
		from += n;
		len -= n;
	}
	return outlen;
}

int public_decrypt (void *fp, void *tp, int len, char *E, char *N)
{
	char *from = fp, *to = tp;
	int n, outlen, q;

	set_precision (bytes2units (64));
	q = countbytes ((void*) N);
	for (outlen=0; len>0; outlen+=n) {
		n = rsa_public_decrypt (to, from, E, N);
		if (n <= 0)
			return -1;
		to += n;
		from += q;
		len -= q;
	}
	return outlen;
}

int private_decrypt (void *fp, void *tp, int len,
	char *E, char *D, char *P, char *Q, char *U, char *N)
{
	char *from = fp, *to = tp;
	int n, outlen, q;

	set_precision (bytes2units (64));
	q = countbytes ((void*) N);
	for (outlen=0; len>0; outlen+=n) {
		n = rsa_private_decrypt (to, from, E, D, P, Q, U, N);
		if (n <= 0)
			return -1;
		to += n;
		from += q;
		len -= q;
	}
	return outlen;
}

void block_encrypt (void *from, void *to, int len, char *key)
{
	struct IdeaCfbContext cfb;

	ideaCfbInit (&cfb, key);
	ideaCfbEncrypt (&cfb, from, to, len);
	ideaCfbDestroy (&cfb);
}

void block_decrypt (void *from, void *to, int len, char *key)
{
	struct IdeaCfbContext cfb;

	ideaCfbInit (&cfb, key);
	ideaCfbDecrypt (&cfb, from, to, len);
	ideaCfbDestroy (&cfb);
}

static struct IdeaCfbContext iocfb;

void crypt_init (char *key)
{
	ideaCfbInit (&iocfb, key);
}

int crypt_read (int fd, void *buf, int len)
{
	char iobuf [512];

	if (len > sizeof (iobuf))
		len = sizeof (iobuf);
	len = read (fd, iobuf, len);
	if (len > 0) {
		ideaCfbReinit (&iocfb, 0);
		ideaCfbDecrypt (&iocfb, iobuf, buf, len);
	}
	return len;
}

int crypt_write (int fd, void *buf, int len)
{
	char iobuf [512];

	if (len > sizeof (iobuf))
		len = sizeof (iobuf);
	ideaCfbReinit (&iocfb, 0);
	ideaCfbEncrypt (&iocfb, buf, iobuf, len);
	return write (fd, iobuf, len);
}

#ifdef TEST
int main (int argc, char **argv)
{
	char inbuf [512], outbuf [512], *key;
	int q, n;
	int decodeflag = 0;
	extern int optind;

	for (;;) {
		switch (getopt (argc, argv, "kd")) {
		case EOF:
			break;
		case 'd':
			++decodeflag;
			continue;
		default:
usage:                  fprintf (stderr, "Usage:\n");
			fprintf (stderr, "\tpgptest key < infile > outfile\n");
			fprintf (stderr, "\t\t- encrypt file\n");
			fprintf (stderr, "\tpgptest -d key < infile > outfile\n");
			fprintf (stderr, "\t\t- decrypt file\n");
			return 1;
		}
		break;
	}
	argc -= optind;
	argv += optind;

	if (argc != 1)
		goto usage;
	key = argv[0];

	q = 512;
	for (;;) {
		n = read (0, inbuf, q);
		if (n < 0)
			perror ("input");
		if (n <= 0)
			break;
		if (decodeflag)
			block_decrypt (inbuf, outbuf, n, key);
		else
			block_encrypt (inbuf, outbuf, n, key);
		if (write (1, outbuf, n) != n) {
			perror ("output");
			break;
		}
	}
	return 0;
}
#endif
