#include "memcached.h"
#include<arpa/inet.h>
#include<strings.h>


SSL_CTX* InitCTX(void) {
    SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_ssl_algorithms();
    method = SSLv2_client_method();
    ctx = SSL_CTX_new(method);

    if(ctx == NULL) {
        fprintf(stderr, "Error getting new context");
        abort();
    }
    return ctx;
}

void setHeaderForGetConfigRequest(protocol_binary_request_get_cluster_config *gcmd) {
    protocol_binary_request_header *hdr = &gcmd->message.header;
    hdr->request.magic = PROTOCOL_BINARY_REQ;
    hdr->request.datatype = PROTOCOL_BINARY_RAW_BYTES;
    hdr->request.opcode = PROTOCOL_BINARY_CMD_GET;
}


int main(int argc, char **argv) {
    //connect to a server
    int sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    struct sockaddr_in server;

    if (sockfd < 0) {
        printf("Unable to create socket");
        exit(1);
    }

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons(argv[1] ? atoi(argv[1]) : 11311);

    //SSL connection 
    SSL_CTX *ctx = InitCTX();

    if (!ctx) {
        fprintf(stderr, "Could not create SSL Context \n");
        exit(1);
    }

    if(connect(sockfd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Unable to connect to server");
        close(sockfd);
        exit(1);
    } else {
        printf("Successfully connected to the ssl server \n");
        SSL *ssl = SSL_new(ctx); 
        SSL_set_fd(ssl, sockfd);

        int ret = SSL_connect(ssl);
        if (ret != 1) {
            fprintf(stderr, "Cannot connect exiting \n");
            /*int err = SSL_get_error(ssl, ret);
            while(err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                //retry stuff here
                ret = SSL_connect(ssl);
                err = SSL_get_error(ssl, ret); 
            }*/
        }


        
        protocol_binary_request_get_cluster_config gcmd;
        memset(&gcmd, '0', sizeof(gcmd)); 

        protocol_binary_request_header *hdr = &gcmd.message.header;
        hdr->request.magic = PROTOCOL_BINARY_REQ;
        hdr->request.datatype = PROTOCOL_BINARY_RAW_BYTES;
        hdr->request.opcode = PROTOCOL_BINARY_CMD_GET_CLUSTER_CONFIG;
        

        fprintf(stderr, "Writing %lu on the socket\n", sizeof(gcmd));
        ret = SSL_write(ssl, (const void *)&gcmd, sizeof(gcmd));
        

        //get the status of the ssl write
        int err = SSL_get_error(ssl, ret);
        fprintf(stderr, "\n Write error on the socket %d",err);
        sleep(10);
        char outbuf[512]; 
        int count;
        
        /* 
        while(err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            if (err == SSL_ERROR_WANT_READ) {
                fprintf(stderr, "\n want read error on the socket");
            }
            if (err == SSL_ERROR_WANT_WRITE) {
                fprintf(stderr, "\n want write error on the socket");
            }
            fprintf(stderr, "\n Write error on the socket %d", err);

            count = SSL_read(ssl, (void *)&outbuf, sizeof(outbuf));
            if (count>0) {
                fprintf(stderr, "Bytes read from the socket %d", count);
            }
            ret = SSL_write(ssl, (void *)&gcmd, sizeof(gcmd));
            err = SSL_get_error(ssl, ret); 
        }
        */
        if (err == SSL_ERROR_NONE) {
            printf("\n Writen %d bytes to the server successfully", ret);
        }
        fprintf(stderr, "\n Write error on the socket %d",err);
        
        /*char outbuf[512];
        int bytesRead = SSL_read(ssl, outbuf, sizeof(outbuf));
        printf("%d", bytesRead);*/
        sleep(300);

        if(ssl) {
            SSL_free(ssl);
        }
    }

    if (ctx) SSL_CTX_free(ctx);
    close(sockfd);
    return 0; 
}
