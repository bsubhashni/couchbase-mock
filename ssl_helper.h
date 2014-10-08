#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/bio.h>


/* Creates a new SSL context */
SSL_CTX* InitSSLCTX();

/* Loads the PEM certificate to the SSL Context */
void LoadCertificates();

SSL_CTX* CreateAndLoadContext(char *);
