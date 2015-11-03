/* David Shagam
Yulo Leake
CSCI 367 Program 0 client */

//includes
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>


main (int argc, char **argv){
	int sd; 		// socket descriptor
	int port; 	// used port
	char *host; // host name

 	unsigned long user_guess;
	char server_response;

	struct sockaddr_in sad;    // holds ip
	struct hostent *hostpath;  // host pointer
	struct protoent *protocol; // protocol

	/*   Check argumetn counts; if not 3, print error message*/
	if(argc != 3){
		fprintf(stderr, "Wrong number of arguments\n");
		fprintf(stderr, "usage:\n");
		fprintf(stderr, "./prog0_client host_ip port\n");
		exit(EXIT_FAILURE);
	}
	
	memset(( char*)&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;

	/*   Grab port number of host and see if it valid; if invalid, print error message*/
	port = atoi(argv[2]); //grabs port number of host
	if (port > 0){
		sad.sin_port = htons((u_short)port);
	} else {
		fprintf(stderr, "Error: Bad port %s \n", argv[2]);
		exit(EXIT_FAILURE);
	}

	host = argv[1]; // grabs ip of host
	
	hostpath = gethostbyname(host);
	if(hostpath == NULL){
		fprintf(stderr,"Error: Invalid host: %s\n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, hostpath->h_addr, hostpath->h_length);

	/* Map TCP transport protocol name to protocol number. */
	if ( ((long int)(protocol = getprotobyname("tcp"))) == 0) {
		fprintf(stderr, "Error: Cannot map \"tcp\" to protocol number");
		exit(EXIT_FAILURE);
	}

	/* Create a socket. */
	sd = socket(PF_INET, SOCK_STREAM, protocol->p_proto);
	if (sd < 0) {
		fprintf(stderr, "Error: Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/* Connect the socket to the specified server. */
	if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
		fprintf(stderr,"Error: Connection failed\n");
		exit(EXIT_FAILURE);
	}

	/*   Main game loop - loop until user guesses correctly.   */
	server_response = '1';
	while(server_response != '0') {
		fprintf(stderr, "Enter Guess: ");
		scanf("%lu", &user_guess);
		user_guess = htonl(user_guess);
		send(sd, &user_guess, sizeof(user_guess), 0); 
		if(recv(sd, &server_response, 1, 0) > 0){
			if( server_response == '0'){
				fprintf(stderr, "You Win!\n");
			} else if ( server_response == '1'){
				fprintf(stderr, "Warmer\n");
			} else if ( server_response == '2'){
				fprintf(stderr, "Colder\n");
			} else {
				fprintf(stderr, "Same\n");
			}
		} else {
			// receiving done, close and exit child.
			close (sd);
			exit(EXIT_FAILURE);
		}
	}
	close(sd);
	exit(EXIT_SUCCESS);
}
