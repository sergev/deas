#include "pgplib.h"

int public_encrypt (void *from, void *to, int len, char *username)
{
	memcpy (to, from, len);
	return len;
}

int private_encrypt (void *from, void *to, int len, char *username, char *password)
{
	memcpy (to, from, len);
	return len;
}

int public_decrypt (void *from, void *to, int len, char *e, char *n)
{
	memcpy (to, from, len);
	return len;
}

int private_decrypt (void *from, void *to, int len,
	char *e, char *d, char *p, char *q, char *u, char *n)
{
	memcpy (to, from, len);
	return len;
}

void block_encrypt (void *from, void *to, int len, char *key)
{
	memcpy (to, from, len);
}

void block_decrypt (void *from, void *to, int len, char *key)
{
	memcpy (to, from, len);
}
