#include "memcached.h"

conn*
conn_new(int fd) {
    fprintf(stderr, "Creating new connection \n");
    conn *c = NULL;
    c = (conn *)malloc(sizeof(*c));
    if (!c) {
        fprintf(stderr, "Out of memory!!");
        exit(1);
    }
    return c;
}

int
allocate_iobuffer(IOBuf *iobuf) {
    if(!iobuf) {
        iobuf = (IOBuf *)malloc(sizeof(*iobuf));
        iobuf->buf = NULL;
        iobuf->consumed = 0;
        iobuf->size = 0;
    }
    iobuf->buf = (IOBuf *)realloc(iobuf, BUFFER_SIZE + iobuf->size);

    if (!iobuf->buf) {
        fprintf(stderr, "Out of memory!!");
        exit(1);
    }

    iobuf->size = BUFFER_SIZE + iobuf->size;
}

void 
destry_conn(conn *c) {
    free(c->inbuf);
    free(c->outbuf);
    free(c);
}


