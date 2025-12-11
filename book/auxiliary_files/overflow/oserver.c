/* Program name:	oserver4.c
 * this program calls a function to process the connection,
 * making it branch to fishkill on return, after the buffer overflow.
 *
 * Alas, we want to overflow the *procedure* buffer, and we've arranged
 * to print out the address of the main() buffer.
 * So we have to precompute the offset.

 * The stalk protocol was changed in this version to support "quit",
 * which closes a session (and thus causes a return via an overwritten stack frame).

 * To simplify the shellcode, the child process handling a connection 
 * changes its stdin and stdout to the network connection. 
 * This also allows the child process to read network data with gets()
 * Normal terminal output is therefore printed to stderr.
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define SERVER_PORT 5431
#define MAX_PENDING 1
#define BUFSIZE 80
#define MBUFSIZE 1
#define OVERBUF 1000

#define TRUE 1

void setstdinout(int);
void stackpeek(void);
void process_connection (int csock);
int  cutword(char * p, int size);

int mainaddr; 
int gsock;

char * mbufp;

#define USE_GETS

int main(int argc, char ** argv) {
	char mbuf[MBUFSIZE];		// to help find the stack spacing
	int sock, new_s;
	struct sockaddr_in sin;
	struct hostent *hp;
	int len = sizeof sin;
	mbufp = mbuf;
	mainaddr = (int) &main;
	/* Create socket on which to send. */
	sock = socket(PF_INET,SOCK_STREAM,0);
	if (sock < 0) {
		perror("opening datagram socket");
		exit(1);
	}
	memset((void*) &sin, 0, sizeof sin);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr  = INADDR_ANY;
	sin.sin_port = htons(SERVER_PORT);

	if (bind(sock, (struct sockaddr *) &sin, sizeof sin) < 0) {
		perror("bind failed");
		exit(1);
	}

	fprintf(stderr, "buffer starts at address %x , size=%d\n", (unsigned int) mbuf, BUFSIZE);
	//fprintf(stderr, "main is at %x\n", mainaddr);       // useful for guessing locations containing return addresses
  	//stackpeek();			// use as desired
	listen(sock, MAX_PENDING);

	while (TRUE) {
		// handle new connection
		if ((new_s = accept(sock, (struct sockaddr *) &sin, &len)) < 0) {
		    perror("accept failed");
		}

		fprintf(stderr, "\nconnection established; new socket = %d, addr=%s\n",
				 new_s, inet_ntoa(sin.sin_addr) );
		if (fork() == 0) {		// child
			process_connection(new_s);
			exit(0);
		}
		close(new_s);

	}
	return 0;
}

// The following sets the stdin and stdout to be fd (the socket),
// so that a shell would in fact look like a normal shell.
// A real server might also do this, but if not this could be done in the shellcode.

void setstdinout(int fd) {
	dup2(fd,0);
	dup2(fd,1);
	fcntl(0,F_SETFD, 0);
	fcntl(1,F_SETFD, 0);
}

/*
 * process the connection
 * csock = the connected socket
 */

void process_connection (int csock) {
	int len;
	char pcbuf[BUFSIZE];

	gsock = csock;	// we must avoid clobbering local variables when the overflow hits

	fprintf(stderr, "mbuf - pcbuf = %d\n", mbufp-pcbuf);	// print this out to help the exploit

	// this means our shellcode will have its stdin and stdout exactly as needed
	setstdinout(gsock);

	while(TRUE) {
		//read much more into pcbuf than should fit
		fprintf(stderr, "waiting to read\n");
#ifdef USE_GETS
		gets(pcbuf);
		len = strlen(pcbuf);
#else
		len = recv(gsock,  pcbuf, OVERBUF, 0);	// note the wrong value OVERBUF > sizeof(pcbuf)
#endif
		fprintf(stderr,	"just read %d bytes into buffer of size %d\n",
			len, sizeof(pcbuf));

		// now print out the buffer
		if (len <= 0) {
			fprintf(stderr, "quitting!\n");
			return;
		}
		int cappedlen = (len<BUFSIZE) ? len : BUFSIZE;
		pcbuf[cappedlen]=0;	// when we print the string, we want to avoid dumping all of memory

		for (int i=0; i<len; i++) if (pcbuf[i]=='\r') pcbuf[i] = '\0';	// avoid problems with \r

		fprintf(stderr, "the string is \"%s\"\n", pcbuf);

		char * QUITSTR = "quit";
		len = cutword(pcbuf, BUFSIZE);

		if (strncmp(pcbuf, QUITSTR, len) == 0) {
			fprintf(stderr, "quitting!");
			return;			// this will trigger the overflow!
		}
	}
	// don't close connection until AFTER return! Otherwise the shellcode shell will have no connection
}

// for exploring the stack layout
void stackpeek() {
    int n = 0xdeadbeef;
    int i = 0x411;
    int * pn = &n;
    fprintf(stderr, "address of n: %x\n", (unsigned) &n);
    for ( i = 0; i<20; i++) {
	fprintf(stderr, "pn[%d] = %x\n", i, pn[i]);
    }
}
 
// takes a string in a buffer pointed to by p, of at least size bytes,
// and returns the index of the first char that is null,blank,tab,or newline.
// That is, it returns the length of the first "word"
// If 1st char is blank/etc, word is zero-length

int cutword(char * p, int size) {
	p[size-1]='\0';
    char * q = p;
	while (*q != ' ' && *q != '\t' && *q != '\n' && *q != '\r' && *q != '\0') q++;
	return (q-p);
}
