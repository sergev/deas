#define CBLK            8
#define CALIGN(n)       (((n) + CBLK - 1) / CBLK * CBLK)

extern char *crypt_pubfile;
extern char *crypt_secfile;

int public_encrypt (void *from, void *to, int len, char *username);
int private_encrypt (void *from, void *to, int len, char *username, char *password);
int public_decrypt (void *from, void *to, int len, char *e, char *n);
int private_decrypt (void *from, void *to, int len,
	char *e, char *d, char *p, char *q, char *u, char *n);
void block_encrypt (void *from, void *to, int len, char *key);
void block_decrypt (void *from, void *to, int len, char *key);

void crypt_init (char *key);
int crypt_read (int fd, void *buf, int len);
int crypt_write (int fd, void *buf, int len);

int rsa_public_encrypt(unsigned char *outbuf, unsigned char *inbuf, short bytes, unsigned char *E, unsigned char *N);
int rsa_private_decrypt(unsigned char *outbuf, unsigned char *inbuf, unsigned char *E, unsigned char *D, unsigned char *P, unsigned char *Q, unsigned char *U, unsigned char *N);

int getpublickey (char *keyfile, long *file_position, char *userid, void *n, void *e);
int getsecretkey (unsigned char *ideakey, char *keyfile, char *userid, void *n, void *e, void *d, void *p, void *q, void *u);
