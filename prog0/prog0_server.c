/* David Shagam
Yulo Leake
CSCI 367 Program 0 server */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

main (int argc, char **argv ){
	struct protoent *protocol;
	struct sockaddr_in sad;    //server adress
	struct sockaddr_in cad;    //client adress
	int sock1, sock2; 				 //sockets used, 1 is main, 2 is connected clients
	int port;
	int addresslen; 					 //length of address

  unsigned int toGuess   = 0;		// number to guess
	unsigned int prevGuess = 0;		// previous user guess
	int firstguess = 0;						// flag to indicate it is user's first guess

	if( argc != 3) {
		fprintf(stderr, "Wrong Number of Arg's\n");
		fprintf(stderr, "Needs 2 arguments: \n");
		fprintf(stderr, "./prog0_server server_port number_to_guess\n");
		exit(EXIT_FAILURE);
	}
	
	memset((char *)&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = INADDR_ANY;

	port = atoi(argv[1]);

	if(port > 0) {
		sad.sin_port = htons((u_short) port);
	} else {
		fprintf(stderr, "Bad port\n");
		exit(EXIT_FAILURE);
	}
	
	/*   Get number to guess from the command argument.   */
	toGuess = atoi(argv[2]);

	if ( ((long int) (protocol = getprotobyname("tcp")))==0){
		fprintf(stderr, "can't map TCP\n");
		exit(EXIT_FAILURE);
	}

	sock1 = socket(PF_INET, SOCK_STREAM, protocol->p_proto);
	if (sock1 < 0) {
		fprintf(stderr, "socket failed\n");
		exit(EXIT_FAILURE);
	}

	//bind
	if (bind(sock1, (struct sockaddr *)&sad, sizeof(sad)) < 0){
		fprintf(stderr, "couldn't bind\n");
		exit(EXIT_FAILURE);
	}

	//request queue
	if (listen(sock1, 6) < 0) {
		fprintf(stderr, "Sorry I'm deaf\n");
		exit(EXIT_FAILURE);
	}

	while(1) {
		addresslen = sizeof(cad);
		if ( (sock2 = accept(sock1, (struct sockaddr *)&cad, &addresslen)) < 0) {
			fprintf(stderr, "Couldn't accept\n");
			exit(EXIT_FAILURE);
		}

		/*   Forking and zombie prevention*/
		signal(SIGCHLD, SIG_IGN);
		pid_t p = fork();

		if ( p < 0 ) {
			fprintf(stderr, "Fork failed\n");
			exit(EXIT_FAILURE);
		} else if ( p == 0 ) {
			close(sock1);
			char retChar;
			unsigned long  prevDist;
			unsigned long  curDist;
			unsigned long curGuess;
			/*   Main game loop on the server side   */
			while( retChar !='0' ){
				if(recv(sock2, &curGuess, sizeof(curGuess), 0) > 0){
					curGuess = ntohl(curGuess);
					/* handle first guess, runs only once */
					if (firstguess == 0){
						if(curGuess == toGuess){
							retChar = '0';
						} else {
							retChar = '1';
						}
						firstguess = 1; 
				} else {		/*   handle rest of guesses   */
						prevDist = abs(toGuess - prevGuess); // calculate previous distance
						curDist  = abs(toGuess - curGuess);  // calculate current  distance
						if (curDist == 0) {
							retChar = '0';
						} else if (curDist < prevDist){
							retChar = '1';
						} else if (curDist == prevDist){
							retChar = '3';
						} else {
							retChar = '2';
						}
					}
					prevGuess = curGuess;
					send(sock2, &retChar, sizeof(retChar), 0);
				} else {
					/*   Receiving done, close and exit child.   */
					close (sock1);
					exit(EXIT_FAILURE);
				}
			}
			/*   Child process done, close and exit.   */
			close(sock2);
			exit(EXIT_SUCCESS);
		} else {
			/*   close second socket to open up for new clients   */
			close(sock2);
		}
	}
	close(sock1);
	exit(EXIT_SUCCESS);
}
