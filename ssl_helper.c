#include "ssl_helper.h"

SSL_CTX* CreateAndLoadContext(char *certPath) {
    SSL_CTX* ctx = InitSSLCTX();
    if (ctx) {
        LoadCertificates(ctx, certPath);
    }
    return ctx;
}

SSL_CTX*
InitSSLCTX() {
    OpenSSL_add_ssl_algorithms();
    SSL_load_error_strings();

    SSL_METHOD *method;
    method = SSLv23_server_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        exit(1);
    }
    return ctx;
}

void
LoadCertificates(SSL_CTX *ctx, char *certFile) {
    int ret;
    if((ret = SSL_CTX_use_certificate_file(ctx, "cert.pem", SSL_FILETYPE_PEM)) <= 0) {
        fprintf(stderr, "Unable to load the file %d",ret);
    }
    if((ret = SSL_CTX_use_PrivateKey_file(ctx, "key.pem", SSL_FILETYPE_PEM)) <= 0) {
        fprintf(stderr, "Unable to load the priv key file %d", ret);
    }

}

