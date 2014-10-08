#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<fcntl.h>
#include<string.h>
#include<sys/select.h>
#include<netinet/in.h>
#include<unistd.h>
#include<errno.h>
#include<sys/uio.h>

#include "protocol_binary.h"
#include "ssl_helper.h"

#define MOCK_MEMCACHED_PORT 11310
#define MOCK_MEMCACHED_SSL_PORT 11311
#define KEYSTORE_SIZE 256
#define BACKLOG 10
#define MAX_CLIENT_CONNS 20
#define BUFFER_SIZE  512
#define IDENTITY "MOCK_MEMD"

#define LOG_ERR "Error"
#define LOG_TRACE "Trace"
#define LOG_DEBUG "Debug"

#define LOG(severity,srcfile,srcline,msg) \
            do {  fprintf(stderr, "%s:<%s:%s> %s", severity, srcfile, srcline, msg); }while(0);


typedef struct IOBuf {
    void *buf;
    int consumed;
    int size;
} IOBuf;

typedef struct conn {
    int fd;
    int saslAuthenticated;
    IOBuf *inbuf;
    IOBuf *outbuf;
    SSL_CTX *ctx;
}conn;

struct memcachedServer {
    int port;
    int sockfd;
    int startfd;
    time_t *start_time;
    SSL_CTX *ctx;
};

/* Connection routines */
conn* conn_new(int);
int allocate_iobuffer(IOBuf *);
void destroy_conn(conn *);


extern struct memcachedServer server;
extern struct conn clientConns[MAX_CLIENT_CONNS];
extern SSL* sslObjs[MAX_CLIENT_CONNS];

