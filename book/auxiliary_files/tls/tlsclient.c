// Peter Dordal, 2017, based largely on code at https://ubuntuforums.org/archive/index.php/t-2217101.html

//gcc -o tlsclient tlsclient.c -lssl -lcrypto
// docs: https://www.openssl.org/docs/man1.1.0/ssl
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

void printCertName(FILE *, char *);
int  openConnection(char * hostname, unsigned short port);
void parseArgs(int argc, char *argv[]); // "usage: tlsclient [hostname [port]]"

char * hostname = "localhost";
int    port     = 4433;   // traditional openssl demo port; 443 is the HTTPS port

int main(int argc, char *argv[]){
    int sock;
    char cafile[] = "CAcert.pem";
    char * certsdir = "/etc/ssl/certs";
    char request[200];
    int req_len;
    char buf[2000];
    int ret;  // for return codes
    const SSL_METHOD *method;
    SSL_CTX *ctx;
    SSL     *ssl;
    X509    *cert = NULL;
    X509_NAME *certname = NULL;

    parseArgs(argc, argv);
    snprintf(request, sizeof request, "GET / HTTP/1.1\r\nHost: %s\r\n\r\n", hostname);
    req_len = strlen(request);

    SSL_library_init();   // "SSL_library_init() always returns '1'
    ERR_load_crypto_strings();
    SSL_load_error_strings();

    method = SSLv23_client_method();
    //method = TLSv1_1_client_method();   // try uncommenting one of these instead
    //method = TLSv1_2_client_method();   // you will then have to comment out SSL_CTX_set_options()
    ctx = SSL_CTX_new(method);
    if (ctx == NULL) {
        ERR_print_errors_fp(stderr);    // failed to create SSL context
        exit(1);
    }
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3|SSL_OP_NO_TLSv1|SSL_OP_NO_TLSv1_1);

    /* load the trust store */
    if( SSL_CTX_load_verify_locations(ctx, NULL, certsdir) != 1) {    // load certs in a directory
        ERR_print_errors_fp(stderr);
    }
    if( SSL_CTX_load_verify_locations(ctx, cafile, NULL) != 1) {      // load a single cert
        ERR_print_errors_fp(stderr);
    }

    sock = openConnection(hostname, port);   // standard TCP connection

    ssl = SSL_new(ctx);
    if (ssl == NULL) {fprintf(stderr, "SSL_new() failed\n"); exit(1);}
    ret = SSL_set_fd(ssl, sock);  // returns 0 on failure, 1 on success
    if (ret !=1) {fprintf(stderr, "SSL_set_fd() failed\n"); exit(1);}

    ret = SSL_connect(ssl);
    if (ret != 1) {
        ret = SSL_get_error(ssl, ret);
        ERR_print_errors_fp(stderr);          // may be the wrong error printer
        fprintf(stderr, "SSL_connect failed with SSL_get_error code %d\n", ret);
        exit(1);
    }

    /* Get the remote certificate into the X509 structure */
    cert = SSL_get_peer_certificate(ssl);
    if (cert == NULL) {
         fprintf(stderr, "Error: Could not get a certificate from: %s.\n", hostname);
         exit(1);
    } else {
         fprintf(stderr, "Retrieved the server's certificate from: %s.\n", hostname);
    }

    /* extract various certificate information */
    certname = X509_get_subject_name(cert);

    /*  display the cert subject here */
    fprintf(stderr, "Printing the certificate name:\n");
    X509_NAME_oneline(certname, buf, sizeof buf);   // use "is strongly discouraged in new applications"
    printCertName(stderr, buf);
    ret = SSL_get_verify_result(ssl);

    if(X509_V_OK != ret) {
        fprintf(stderr, "verify_result error: \"%s\" [%d]\n", X509_verify_cert_error_string(ret), ret);
    } else {
        fprintf(stderr, "certificate verify result is X509_V_OK: all is well!\n");
    }

    SSL_write(ssl, request, req_len);     // ignored by tlsserver program
    int bytesread;
    do {
        bytesread = SSL_read(ssl, buf, sizeof(buf));
        fprintf(stderr, "read %d bytes\n", bytesread);
        fwrite(buf, bytesread, 1, stdout);
    } while(bytesread > 0);
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);
}

// prints the certname string replacing the '/' separator with \n\t
void printCertName(FILE * f, char * name){
    int i = 0; char ch;
    while ((ch = name[i++]) != 0) {
        if (ch == '/') fprintf(f, "\n\t");
        else fprintf(f, "%c", ch);
    }
    fprintf(f, "\n");
}

int  openConnection(char * hostname, unsigned short port) {
    struct hostent *host;
    struct sockaddr_in addr;
    int sock;
    fprintf(stderr, "looking up %s...", hostname);
    host = gethostbyname(hostname);
    if (host == NULL) {
         fprintf(stderr, "can't find host %s\n", hostname);
         exit(1);
    }
    fprintf(stderr, ". got it!\n");
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("can't create socket");
        exit(1);
    }
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = *(long*)(host->h_addr);

    if ( connect(sock, (struct sockaddr*) &addr, sizeof(addr)) == -1 ) {
        fprintf(stderr, "Cannot connect to host %s [%s] on port %d.\n", hostname, inet_ntoa(addr.sin_addr), port);
        exit(1);
    }
    return sock;
}

void parseArgs(int argc, char * argv[]) {
    if (argc >= 2) {
        hostname = argv[1];
        fprintf(stderr, "setting hostname to %s\n", hostname);
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
        fprintf(stderr, "setting port to %d\n", port);
    }
}
