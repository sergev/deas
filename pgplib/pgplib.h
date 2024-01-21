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
