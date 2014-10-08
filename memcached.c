#include "memcached.h"
#include <pthread.h>

struct memcachedServer mserver;
struct memcachedServer sslServer;
SSL* sslObjs[MAX_CLIENT_CONNS];
int counter_ssl_objects = 0;

int
makeSocketNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL);
    if (flags < 0) {
        exit(1);
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}


static void
config_handler() {
    fprintf(stderr, "\n Got request for config");
    //Load the config from file
}


int
decode_request(char *buf, size_t size) {
    protocol_binary_request_header *hdr;
    hdr = (protocol_binary_request_header*)malloc(sizeof(*hdr));

    if(hdr) {
        printf("unable to set memory");

    }
    memset((void *)hdr, '0', sizeof(*hdr));
    memcpy((void *)hdr, (const void *)buf, sizeof(*hdr));

    if(hdr->request.opcode == PROTOCOL_BINARY_CMD_GET_CLUSTER_CONFIG) {
        config_handler();
    } else {
        fprintf(stderr, "Got unknown request %d", hdr->request.opcode);
    }
    return 0;
}


int
handleClientRead(int fd) {
    char buf[4096];
    memset((void *)&buf, '\0', sizeof(buf));
    int nbytes = 0;

    fprintf(stderr, "\n Handling client read");

    while((nbytes = recv(fd, buf, sizeof(buf), 0)) > 0) {
        fprintf(stderr, "\n Number of bytes on the socket: %d", nbytes);
    }
    
    decode_request(buf, nbytes);
    char *msg = "hello";
    
    if(send(fd, msg, strlen(msg), 0) < 0) {
        return -1;
    }
    return 0;
}

int
handleSSLClientRead(SSL *ssl){
    char buf[4096];
    memset((void *)&buf, '\0', sizeof(buf));

    int nbytes = 0;
    fprintf(stderr, "\n Handling SSL client read");

    nbytes = SSL_read(ssl, buf, sizeof(buf));

    //connection closed
    if (nbytes == 0) {
        return -1;
    }

    //error 
    if (nbytes < 0) {
        int ret = nbytes;
        int err = SSL_get_error(ssl, ret);
        fprintf(stderr, "Error while reading from ssl connection %d\n", err);
        return -1;
    }

    while(nbytes > 0) {
        fprintf(stderr, "\n Number of bytes received on ssl socket: %d", nbytes);
        nbytes = SSL_read(ssl, buf, sizeof(buf));
    }

    fprintf(stderr, "\n Received Nbytes: %d", nbytes);

    decode_request(buf, nbytes);
    char *msg = "Received Message";
    SSL_write(ssl, msg, strlen(msg));
    return 0;
}

void
runServer(void *arg) {
    struct memcachedServer *server = (struct memcachedServer *)arg;

    fd_set rfds, master;
    FD_ZERO(&rfds);
    FD_ZERO(&master);

    FD_SET(server->sockfd, &master);

    int sfd, new_fd, fd_max, first_conn_fd = 0; 
    int ii;
    fd_max = server->sockfd;
    makeSocketNonBlocking(server->sockfd);
    rfds = master;

    while(1) {
        if(select(fd_max+1, &rfds, NULL, NULL, NULL) < 0){
            fprintf(stderr, "error on select");
            exit(1);
        }

        fprintf(stderr, "\n Got a new event on the server socket!!");

        for (ii = 0; ii <= fd_max; ii++) {

            if (FD_ISSET(ii, &rfds)) {
                if (ii == server->sockfd) {
                    fprintf(stderr, "\n Accepting a new connection on port %d", server->port);

                    //Accept new client connection
                    struct sockaddr_in caddr;
                    socklen_t addrlen;

                    new_fd = accept(server->sockfd, (struct sockaddr *)&caddr, &addrlen);
                    if(new_fd < 0) {
                        fprintf(stderr, "Unable to accept new connection");
                        exit(1);
                    } else {
                        fprintf(stderr, "\n New connection %d", new_fd);
                    }

                    //store the starting fd for connections
                    if(server->startfd == 0) {
                        server->startfd = new_fd;
                    }

                    conn *c = conn_new(new_fd);

                    if(server->ctx) {
                        SSL *ssl = SSL_new(server->ctx);
                        SSL_set_accept_state(ssl);
                        SSL_set_fd(ssl, new_fd);

                        fprintf(stderr, "\n Trying the SSL Handshake on fd %d", new_fd);

                        //Doing a blocking SSL handshake 
                        int ret = SSL_accept(ssl);
                        if (ret != 1) {
                            int err = SSL_get_error(ssl, ret);
                            while (err == SSL_ERROR_WANT_READ) {
                                ret = SSL_accept(ssl);
                                err = SSL_get_error(ssl, ret);
                            }
                            if(err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_SYSCALL) {
                                fprintf(stderr, "\n SSL library errored");
                                //destroy_conn(c);
                                close(new_fd);
                                continue;
                            }

                            if(err == SSL_ERROR_NONE) {
                                fprintf(stderr, "\n Successfully completed SSL handshake");
                            }

                            sslObjs[counter_ssl_objects] = ssl;
                            fprintf(stderr, "\n Successfully loaded fd %d", new_fd - server->startfd);
                        }
                    }
                    makeSocketNonBlocking(new_fd);
                    if (new_fd > fd_max) {
                        fd_max = new_fd;
                    }
                    FD_SET(new_fd, &master);
                } else {
                    fprintf(stderr, "Got an event from the connections \n");
                    //close(ii);
                    //FD_CLR(ii, &master);

                    //read request from client socket 
                    if(server->ctx) {
                        int j = 0;
                        for(j = 0; j < MAX_CLIENT_CONNS ; j++) { 
                            if(SSL_get_fd(sslObjs[j])  == ii) {
                                if(handleSSLClientRead(sslObjs[j]) < 0) {
                                    fprintf(stderr, "\n Client read failed");
                                    close(ii);
                                    FD_CLR(ii, &master);
                                }
                                break;
                            }
                        }
                    } else { 
                        if(handleClientRead(ii) < 0) {
                            close(ii);
                            FD_CLR(ii, &master);
                        }
                    }

                }
            }
        }

        //reload the read fds as select changes the fds
        rfds = master;
    }
}



void 
initServer(struct memcachedServer *server) {
    server->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (server->sockfd == -1) {
        exit(1);
    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(server->port);
    serveraddr.sin_addr.s_addr = INADDR_ANY;

    //bind the socket to the port 
    if(bind(server->sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        close(server->sockfd);
        exit(1);
    }

    //listen on the socket
    if(listen(server->sockfd, BACKLOG) < 0) {
        close(server->sockfd);
        exit(1);
    }

    if(makeSocketNonBlocking(server->sockfd) < 0 ) {
        exit(1);
    }
}


int 
main(int argc, char **argv)
{
    memset(&mserver, 0, sizeof server);
    memset(&sslServer, 0, sizeof sslServer);

    if (argc > 1) {
        mserver.port = atoi(argv[1]) + 1;
        sslServer.port = atoi(argv[1]);
    } else {
        mserver.port = MOCK_MEMCACHED_PORT;
        sslServer.port = MOCK_MEMCACHED_SSL_PORT;
    }

    if (argc > 2) {
        sslServer.ctx = CreateAndLoadContext(argv[2]); 
    }

    //create a new thread for non ssl memcached port
    pthread_t serverProc, sslserverProc;

    initServer(&mserver);
    initServer(&sslServer);
    
    #if 0
    int mserver_tid = 0;
    mserver_tid = pthread_create(&serverProc, NULL, (void *)&runServer, (void *) &mserver);
    if (mserver_tid) {
        fprintf(stderr, "Unable to create a new server thread");
    }
    #endif

    int sslmserver_tid = 0;
    sslmserver_tid = pthread_create(&sslserverProc, NULL, (void *)&runServer, (void *) &sslServer);

    //pthread_join(serverProc, NULL);
    pthread_join(sslserverProc, NULL);

    return 0;
}
