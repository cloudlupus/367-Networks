/* David Shagam
CSCI 367, Program 3 Participant */

#include <stdio.h> 
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

main (int argc, char *argv[]){

	int sock;
	int port;
	char *host;

	struct sockaddr_in sad;
	struct hostent *hostpath;
	struct protoent *protocol;

	char buf[1024];

	//setup

	if(argc !=3 ){
		fprintf(stderr, "Wrong num args needs 2 \n");
		exit(EXIT_FAILURE);

	}

	memset (( char*)&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;

	port = atoi(argv[2]);
	if(port > 0){
		sad.sin_port = htons((u_short)port);
	} else {
		fprintf(stderr, "bad port please use valid port\n");
		exit(EXIT_FAILURE);
	}

	host = argv[1];
	hostpath = gethostbyname(host);
	if(hostpath == NULL){
		fprintf(stderr,"invalid host\n");
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, hostpath->h_addr, hostpath ->h_length);

	if( ((long int) (protocol = getprotobyname("tcp")))==0){
		fprintf(stderr, "cannot map TCP \n");
		exit(EXIT_FAILURE);
	}

	sock = socket(PF_INET, SOCK_STREAM, protocol->p_proto);
	if(sock< 0){
		fprintf(stderr, "SOCKET FAILED\n");
		exit(EXIT_FAILURE);
	}

	if(connect (sock, (struct sockaddr *)&sad, sizeof(sad))<0){
		fprintf(stderr, "connection fialed \n");
		exit(EXIT_FAILURE);
	}

	//read logic
	char recieved;
	printf("pleaes type a username\n");
	while(1){
		memset(buf, 0, sizeof(buf));
		fgets(buf, sizeof(buf),stdin);
		int lenstr = strlen(buf);
		if(lenstr >1 && lenstr <18){
			int len = send(sock, buf, lenstr, 0);
			recv(sock, &recieved, sizeof(recieved), 0);
			printf("recived %c back\n", recieved);
			if (recieved == 'F'){
				printf("Server is full try again later, Program now closing\n");
				close(sock);
				exit(EXIT_FAILURE);
			} else if(recieved == 'I'){
				printf("Username you picked was invalid or in use please input a new username\n");
			} else if (recieved == 'V'){
				//play game.
				while(1){
					memset(buf, 0, sizeof(buf));
					fgets(buf, sizeof(buf), stdin);
					int len = send(sock, buf, strlen(buf),0);
					
				}
			}
		} else {
			printf("Name was of inappropriate length, Name should be between 1 and 16 characters long\n");
		}
		//printf("\nsend called with %d chars\n",len);
		
	}
}
