// Peter Dordal, 2017, based on https://wiki.openssl.org/index.php/Simple_TLS_Server
//gcc -o tlsserver tlsserver.c -lssl -lcrypto
// docs: https://www.openssl.org/docs/man1.0.2/ssl
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

int createSocket(int port);
void printCiphers(const SSL * theSSL);

int main(int argc, char **argv) {
    char * certfilename; char * keyfilename;
    certfilename = "appcert.pem"; keyfilename = "appkey.pem";
    fprintf(stderr, "certificate file: %s; key file: %s\n", certfilename, keyfilename);

    int sock;
    SSL_CTX *ctx;
    const SSL_METHOD *method;

    SSL_library_init();   // "SSL_library_init() always returns '1'
    SSL_load_error_strings();   // pld: useful for diagnostics

    method = SSLv23_server_method();      // for openssl 1.0.2
    //method = TLS_server_method();       // for openssl 1.1.0
    ctx = SSL_CTX_new(method);
    if (ctx == NULL) {
	      ERR_print_errors_fp(stderr);   // prints entire error stack
	      exit(1);
    }
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);

    /* Set the APPLICATION certificate and key*/
    int cfile_result = SSL_CTX_use_certificate_file(ctx, certfilename, SSL_FILETYPE_PEM);
    if (cfile_result != 1) {
        ERR_print_errors_fp(stderr);
	      exit(1);
    }
/* */   // what happens if this is commented out?
    int kfile_result = SSL_CTX_use_PrivateKey_file(ctx, keyfilename, SSL_FILETYPE_PEM);
    if ( kfile_result != 1 ) {
        ERR_print_errors_fp(stderr);
	      exit(1);
    }
    /* */
    sock = createSocket(4433);      // standard TCP connection

    /* Handle connections */
    while(1) {
        struct sockaddr_in addr;
        unsigned int len = sizeof(addr);
        SSL *ssl;
        const char reply[] = "You have reached the Simple TLS Server\n";

        int childsock = accept(sock, (struct sockaddr*)&addr, &len);
        if (childsock < 0) {
            perror("Unable to accept");
            exit(EXIT_FAILURE);
        }
        fprintf(stderr, "connection accepted from %s/%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        ssl = SSL_new(ctx);
        SSL_set_fd(ssl, childsock);
        //print_ciphers(ssl);
        if (SSL_accept(ssl) != 1) {
            fprintf(stderr, "SSL_accept failed\n");
            ERR_print_errors_fp(stderr);
        }
        else {
            SSL_write(ssl, reply, strlen(reply));
        }

        SSL_free(ssl);
        close(childsock);
    }

    close(sock);
    SSL_CTX_free(ctx);
    EVP_cleanup();
}

int createSocket(int port) {
    int sock;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Unable to create socket");
	      exit(1);
    }

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Unable to bind");
        exit(1);
    }

    if (listen(sock, 1) < 0) {
        perror("Unable to listen");
        exit(1);
    }

    return sock;
}

void printCiphers(const SSL * theSSL) {
    const char * cipher;
    int priority = 0;
    while ((cipher = SSL_get_cipher_list(theSSL, priority) ) != NULL) {
        fprintf(stderr, "cipher %d: %s\n", priority, cipher);
        priority++;
    }
}
