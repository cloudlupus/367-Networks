/* David Shagam
CSCI 367 Progam 1 Client*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

main(int argc, char *argv[]){
	int sockdesc; // socket descrptor
	int port; // port num
	char *host; // host name

	struct sockaddr_in sad; // holds ip
	struct hostent *hostpath; //host pointer
	struct protoent *protocol; // protocl to use.
	char buf[42]; //data buffer
	char gamemode;
	char gamestate[42]; // gamestate

	/* argument count */
	if(argc != 3){
		fprintf(stderr, "Wrong Number of Args\n");
		fprintf(stderr, "usage: \n");
		fprintf(stderr, "./prog1_client host_ip port");
		exit(EXIT_FAILURE);
	}

	memset(( char*)&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;

	/*grab port*/
	port = atoi(argv[2]); // grabs port ascii to int
	if(port> 0 ){
		sad.sin_port = htons((u_short)port);
	} else {
		fprintf(stderr, "Bad Port %s please use a port that is valid \n",argv[2]);
		exit(EXIT_FAILURE);
	}


	/* grab host info*/
	host = argv[1];
	hostpath = gethostbyname(host);

	if(hostpath == NULL){
		fprintf(stderr, "Invalid host %s \n", host);
		exit(EXIT_FAILURE);
	}

	memcpy(&sad.sin_addr, hostpath->h_addr, hostpath->h_length);

	/* TCP setup */

	if( ((long int) (protocol = getprotobyname("tcp")))==0){
		fprintf(stderr, "cannot map \"TCP\" to protocl num\n");
		exit(EXIT_FAILURE);
	}

	/* socket creation*/ 
	sockdesc = socket(PF_INET, SOCK_STREAM, protocol->p_proto);
	if(sockdesc< 0){
		fprintf(stderr, "Socket creation failed\n");
		exit(EXIT_FAILURE);
	}

	/*connect to server */
	if (connect(sockdesc, (struct sockaddr *)&sad, sizeof(sad)) < 0){
		fprintf(stderr, "connection failure \n");
		exit(EXIT_FAILURE);
	}

	/*server logic*/
	int looplogic =1;
	while(looplogic ==1){
		int recvsize;
		recvsize = recv(sockdesc, &buf, 1,MSG_WAITALL);
		char temp;
		
		temp = buf[0];
		// print what game mode we are playing
		if( temp == 'S'){
			gamemode = temp;
			printf("Game mode set to Standard\n");
		} else if (temp == 'P'){
			gamemode = temp;
			printf("Game mode set to Popout\n");
		} else if(temp == 'K'){
			gamemode = temp;
			printf("Game mode set to Antistack\n");
		}

		recvsize = recv(sockdesc, &buf, 1,MSG_WAITALL);
		temp = buf[0];
		// are we waiting on a 2nd player? if so we need to recv information for when we start playing.
		if(temp=='2'){
			printf("Waiting on player 2\n");
			recvsize= recv(sockdesc, &buf, 1, MSG_WAITALL);
			temp = buf[0];
		}
		if(temp =='Y' || temp == 'H' ){
			//intiatlized for the first iteration of the game.
			char isturn = temp;
			// game logic//
			int playing = 1;
			while(playing == 1 ){
				// game is going
				if (isturn == '0'){
					// logic for grabbing new info. probably not needed.
					recv(sockdesc, &buf, 1, MSG_WAITALL);
					isturn = buf[0];

				}
				char usermove[2];
				if(isturn == 'Y'){
					// your turn, print the board, print to make a move, request move.
					recvsize = recv(sockdesc, &buf, sizeof(buf), 0);
					printboard(buf);
					printf("Make a move\n");
					fgets(usermove, sizeof( usermove)+2, stdin);
					send(sockdesc, &usermove, sizeof(usermove), 0);
					isturn = '0';


				} else if(isturn == 'H') {
					// hold print the board and tell to wait.
					recvsize =  recv(sockdesc, &buf, sizeof(buf), 0);
					printboard(buf);
					printf("Please wait, other player's turn.\n");
					// not your turn.
					isturn ='0';

				} else if(isturn =='I'){
					// invalid move recieved, your turn continues. send your new guess and loop through logic again.
					printf("Move was invalid please make a valid move.\n");
					fgets(usermove, sizeof(usermove)+2, stdin);
					send(sockdesc, &usermove, sizeof(usermove), 0);
					isturn = '0';
				} else if(isturn == 'W'){
					printf("YOU ARE WINRAR!\n");
					playing = 0;
					close(sockdesc);
					exit(EXIT_SUCCESS);
				} else if(isturn =='L'){
					printf("You lost sadpanda :( \n");
					playing = 0;
					close(sockdesc);
					close(EXIT_SUCCESS);
				} else if(isturn == 'T'){
					printf("You tied! no winners or losers were had this day\n");
					playing= 0;
					close(sockdesc);
					close(EXIT_SUCCESS);
				}
			}
		}
	
		// exit game
		looplogic =0;
	}
	// finished game close.
	close(sockdesc);
	exit(EXIT_SUCCESS);


}

// takes the string that is the game state and prints it.
printboard(char buf[]){
	int i;
	for( i = 0; i < 42; i++){
		if (i > 0 && ((i +1)% 7) == 0){ 
			// printed the last character in a row make new row
			printf("%c \n",buf[i]);
		}else { 
			// not the end of a row print char.
			printf("%c ", buf[i]);
		}
	}
	// print formating
	printf("-------------\n");
	printf("0 1 2 3 4 5 6\n");
	printf("\n");
}
