/*
David Shagam
CSCI 367 Program1 server
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

char gamestate[42]; //game state
void gamelogic(int curplayer, int p1sock, int p2sock, char gametoplay);

main( int argc, char *argv[]){
	struct protoent *protocol; // protocol
	struct sockaddr_in sad; //server address
	struct sockaddr_in cad1; //client 1 address
	struct sockaddr_in cad2; //client 2 adress
	int acceptsock, p1sock, p2sock; // sockets accept for getting clients, p1 for palyer 1 and p2 for player 2
	int port;
	int addresslen; // length of address
	char gametype; // 1 for standard, 2 for popout, 3 for antistack
	int curplayer;


	if( argc != 3){
		fprintf(stderr, "Wrong num of args\n needs 2 argumetns: \n ./prog1_server server_port gametype");
		exit(EXIT_FAILURE);
	}

	memset((char *)&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = INADDR_ANY;

	port = atoi(argv[1]);

	if(port > 0){
		sad.sin_port = htons((u_short) port);
	} else {
		fprintf(stderr, "bad port please use valid port \n");
		exit(EXIT_FAILURE);
	}

	/* gets game type*/
	//char* gamein = argv[2];
	if(strcmp(argv[2], "standard") ==0 ){
		gametype = 'S';

	} else if (strcmp(argv[2],"popout")== 0){
		gametype = 'P';

	} else if (strcmp(argv[2],"antistack") == 0){
		gametype = 'K';
	} else {
		fprintf(stderr, "%s is not a valid game type, valid game types are standard popout or antistack\n",argv[2]);
		exit(EXIT_FAILURE);
	}

	/* define protocol*/
		if( ((long int)( protocol = getprotobyname("tcp")))==0){
			fprintf(stderr, "can't map TCP\n");
			exit(EXIT_SUCCESS);
		}

 	/* create listen socket*/
	acceptsock = socket(PF_INET, SOCK_STREAM, protocol->p_proto);
	if(acceptsock < 0){
		fprintf(stderr, "socket failed\n");
		exit(EXIT_FAILURE);
	}
	/* bind listen socket*/
	if(bind(acceptsock, (struct sockaddr *)&sad, sizeof(sad))<0){
		fprintf(stderr, "can't bind\n");
		exit(EXIT_FAILURE);
	}
	/* queue*/
	if(listen(acceptsock, 80)<0){
		fprintf(stderr, "couldn't listen \n");
	}
	/* game logic*/
	while(1){
		addresslen = sizeof(cad1);
		if ( (p1sock = accept( acceptsock, (struct sockaddr *)&cad1, &addresslen)) < 0){
			fprintf(stderr, "Couldn't accept p1 \n");
			exit(EXIT_FAILURE);
		}
		// send information to player 1
		send(p1sock, &gametype, sizeof(gametype),0);
		char temp = '2';
		send(p1sock, &temp, sizeof(temp),0);

		addresslen = sizeof(cad2);
		if ( (p2sock = accept(acceptsock, (struct sockaddr *)&cad2, &addresslen))<0){
			fprintf(stderr, "couldn't accept p2\n");
			exit(EXIT_FAILURE);
		}
		//send information to player 2
		send(p2sock, &gametype, sizeof(gametype),0);

		//fork
		signal (SIGCHLD, SIG_IGN);
		pid_t p = fork();

		if( p < 0 ){
			fprintf(stderr, "Forking failed\n");
			exit(EXIT_FAILURE);
		} else if (p == 0){
			//child
			close(acceptsock);
			char retChar;
			memset(&gamestate,(char)'0', 42);
			curplayer = 1;
			// which game we should play based on server.
			gamelogic(curplayer, p1sock, p2sock, gametype);	
			//game is done close the child
			close(p1sock);
			close(p2sock);
			exit(EXIT_SUCCESS);
		} else {
			//parent we don't need the player sockets.
			close(p1sock);
			close(p2sock);

		}

	}
	close(acceptsock);
	exit(EXIT_SUCCESS);
}

// helper function to consider a 1d array as a 2d array.
// returns 0 if out of x y bounds essentially.
char grab2d(int x, int y){
	// make sure we are in bounds treat out of bounds cases as empty slots.
	if (x <0 || y < 0 || x > 6 || y > 5){
		return '0';
	}
	// return the character at gamestate[col][row];
	return gamestate[x + 7*y];
}

//updates the gamestate
// takes a string if it was a move or remove.
// takes an int for the column
// gets what player it is to fill the slot with
updateboard(char move[], int col, char player){
	//add a piece
	if(move[0]=='A'){
		int i;
		// find the first open spot starting from the bottom of the array.
		for(i = 5;i>=0; i--){
			if(gamestate[col + i * 7] == '0'){
				//place piece
				gamestate[col + i * 7] = player;
				// quit for loop
				i = -1;
			}
		}
	} else if(move[0]=='P'){
		// we are removing a piece
		// shuffle all pieces down.
		int i;
		for(i= 5; i>=1; i--){
			gamestate[col + i*7] = gamestate[col + (i-1)* 7];
		}

	}

}

// checks if the move is valid 
// if move is valid returns 1 else returns 0.
int validmove(char move[], char gametype, char player){
	// what column was it added in?
	// giant if switch to converting the char to an int.
	int col;
	if (move[1]=='0'){
		col = 0;
	} else if (move[1] == '1'){
		col = 1;
	} else if (move[1] == '2'){
		col = 2;
	} else if (move[1] == '3'){
		col = 3;
	} else if (move[1] == '4'){
		col = 4; 
	} else if (move[1] == '5'){
		col = 5;
	} else if (move[1] == '6'){
		col = 6;
	} else {
		// not a valid move because it's not one of the predesignated columns
		return 0;
	}
	if (gametype != 'P'){
		// game isn't antistack meaning we only care about adding
		if(move[0] == 'A' && gamestate[col]=='0'){ // move is adding, AND he column has an available spot
			// update the board
			updateboard(move, col, player);
			// send valid
			return 1;
		} else {
			// not valid
			return 0;
		}
	} else {
		// game mode is popout
		if((move[0]== 'A' && gamestate[col] == '0' ) || (move[0]=='P' && gamestate[col + 7*5] == player)){
			//Move is add and column not full OR move is remove and the very last spot is filled by the player
			//update boad
			updateboard(move,col, player);
			return 1;
		}else {
			return 0;
		}

	}
}

// returns 0 for a tie, returns 1 for player 1, returns 2 for player2. and -1 for no victor. returns 3 for p1 and p2 win simul.
// takes char game type, int x position, int y position
int victoryCheck(char gametype, int x, int y){
	//logic for standard game,
	/*
			All of the checks are sliding windows, it starts with checking if the most recent addition is on one end
			of a 4 in a row. Then it moves over one until it gets to the point where it's checking if the most recent addition
			is on the other side of a 4 in a row. This encompasses all possible.
		*/
	if (gametype == 'S'){
		// game type S requires 4 in a row to win.
		// windows are 4
		//horizotal
		int i; 
		char temp = grab2d(x,y);
		for(i =0; i<4; i++){
			if(grab2d(x +(-3+i),y) == temp && grab2d(x+(-2+i),y)==temp && grab2d(x+(-1+i),y)==temp && grab2d(x+i,y)==temp ){
				if(temp == '1'){
					return 1;
				} else if(temp == '2') {
					return 2;
				}
			}

		}
		//vertical
		for(i =0; i<4; i++){
			if(grab2d(x,y +(-3+i)) == temp && grab2d(x,y+(-2+i))==temp && grab2d(x,y+(-1+i))==temp && grab2d(x,y+i)==temp ){
				if(temp == '1'){
					return 1;
				} else if (temp =='2') {
					return 2;
				}
			}

		}
		// /diag
		for(i =0; i<4; i++){
			if(grab2d(x + (-3+i) ,y +(3-i)) == temp && grab2d(x+(-2+i),y+(2-i))==temp && grab2d(x+(-1+i),y+(1-i))==temp && grab2d(x+i,y-i)==temp ){
				if(temp == '1'){
					return 1;
				} else if (temp =='2') {
					return 2;
				}
			}

		}

		// \diag
		for(i =0; i<4; i++){
			if(grab2d(x + (-3+i) ,y +(-3+i)) == temp && grab2d(x+(-2+i),y+(-2+i))==temp && grab2d(x+(-1+i),y+(-1+i))==temp && grab2d(x+i,y+i)==temp ){
				if(temp == '1'){
					return 1;
				} else if (temp =='2') {
					return 2;
				}
			}

		}
		// no winner
		//check for tie
		if(grab2d(0,0) != '0' && grab2d(1,0)!='0' && grab2d(2,0)!='0' && grab2d(3,0)!='0' && grab2d(4,0)!='0' && grab2d(5,0)!='0' && grab2d(6,0)!='0' ){
			return 0;
		} else {
			//no winner or tie, continue game.
			return -1;
		}

	}else if( gametype == 'K'){
		// game mode antistack
		// almost identicle to Standard however the windows are of size 3 instead of 4
		// also who is a winner and loser is flipped of Standard.
		//horizotal
		int i;
		char temp = grab2d(x,y);
		for(i =0; i<3; i++){
			if(grab2d(x+(-2+i),y)==temp && grab2d(x+(-1+i),y)==temp && grab2d(x+i,y)==temp ){
				if(temp == '1'){
					return 2;
				} else if(temp == '2') {
					return 1;
				}
			}

		}
		//vertical
		for(i =0; i<3; i++){
			if(grab2d(x,y+(-2+i))==temp && grab2d(x,y+(-1+i))==temp && grab2d(x,y+i)==temp ){
				if(temp == '1'){
					return 2;
				} else if (temp =='2') {
					return 1;
				}
			}

		}
		// /diag
		for(i =0; i<3; i++){
			if(grab2d(x+(-2+i),y+(2-i))==temp && grab2d(x+(-1+i),y+(1-i))==temp && grab2d(x+i,y-i)==temp ){
				if(temp == '1'){
					return 2;
				} else if (temp =='2') {
					return 1;
				}
			}

		}

		// \diag
		for(i =0; i<3; i++){
			if(grab2d(x+(-2+i),y+(-2+i))==temp && grab2d(x+(-1+i),y+(-1+i))==temp && grab2d(x+i,y+i)==temp ){
				if(temp == '1'){
					return 2;
				} else if (temp =='2') {
					return 1;
				}
			}

		}

		if(grab2d(0,0) != '0' && grab2d(1,0)!='0' && grab2d(2,0)!='0' && grab2d(3,0)!='0' && grab2d(4,0)!='0' && grab2d(5,0)!='0' && grab2d(6,0)!='0' ){
			return 0;
		} else {
			return -1;
		}

	} else if (gametype == 'P'){
		// popout has the same checks style as standard with sliding windows of 4.
		// however there is the possability that both players win based on a pop
		// we need a special return case if both players are winners based on a pop.
		int j;
		int p1 = 0;
		int p2 = 0;
		//horizotal
		for(j = 0; j<6; j++){
			int i;
			char temp = grab2d(x,j);
			for(i =0; i<4; i++){
				if(grab2d(x +(-3+i),j) == temp && grab2d(x+(-2+i),j)==temp && grab2d(x+(-1+i),j)==temp && grab2d(x+i,j)==temp ){
					if(temp == '1'){
						p1 = 1;
					} else if(temp == '2') {
						p2 = 2;
					}
				}

			}
			//vertical
			for(i =0; i<4; i++){
				if(grab2d(x,j +(-3+i)) == temp && grab2d(x,j+(-2+i))==temp && grab2d(x,j+(-1+i))==temp && grab2d(x,j+i)==temp ){
					if(temp == '1'){
						p1 = 1;
					} else if (temp =='2') {
						p2 = 2;
					}
				}

			}
			// /diag
			for(i =0; i<4; i++){
				if(grab2d(x + (-3+i) ,j +(3-i)) == temp && grab2d(x+(-2+i),j+(2-i))==temp && grab2d(x+(-1+i),j+(1-i))==temp && grab2d(x+i,j-i)==temp ){
					if(temp == '1'){
						p1 = 1;
					} else if (temp =='2') {
						p2 = 2;
					}
				}

			}

			// \diag
			for(i =0; i<4; i++){
				if(grab2d(x + (-3+i) ,j +(-3+i)) == temp && grab2d(x+(-2+i),j+(-2+i))==temp && grab2d(x+(-1+i),j+(-1+i))==temp && grab2d(x+i,j+i)==temp ){
					if(temp == '1'){
						p1 = 1;
					} else if (temp =='2') {
						p2 = 2;
					}
				}

			}
		}
		int ret = p1 + p2;
		if(ret != 0){
			return ret;
		}else if(grab2d(0,0) != '0' && grab2d(1,0)!='0' && grab2d(2,0)!='0' && grab2d(3,0)!='0' && grab2d(4,0)!='0' && grab2d(5,0)!='0' && grab2d(6,0)!='0' ){
			return 0;
		} else {
			return -1;
		}

	}
}

// game logic for standard
// takes the int curent player, takes int player 1 socket, player 2 socket, and char game played.
int minusConversion = 48;
void gamelogic(int curplayer, int p1sock, int p2sock, char gametoplay){
	int looplogic = 1;
	while(looplogic ==1){
		char temp;
		char usermove[2];
		// player 1 turn
		if(curplayer == 1){
			// send messages and game state
			temp = 'Y';
			send(p1sock,&temp, 1, 0);
			send(p1sock, &gamestate, sizeof(gamestate), 0);
			temp = 'H';
			send(p2sock, &temp,1,0);
			send(p2sock, &gamestate, sizeof(gamestate), 0);
			int invalid = 1;
			//loop for if invalid turn
			while (invalid ==1){
				//get message from player
				recv(p1sock, &usermove, 2,MSG_WAITALL);
				// check validity of move. Can only pop your own, can only pop on popout
				// makes sure you aren't popping an empty column, or adding to a full column
				if (validmove(usermove, gametoplay, '1')==1){
					invalid = 0;
					//get x column through trickery
					int x = usermove[1]-minusConversion;
					int y;
					int i;
					//calculate the y position.
					for(i=0; i<6; i++){
						if(gamestate[x+ 7*i]!='0'){
							y=i;
							i=7;
						}

					}

					//check for a victory
					int wincheck;
					wincheck = victoryCheck(gametoplay,x, y);
					if(wincheck == 1){
						// player 1 wins
						temp = 'W';
						send(p1sock, &temp, 1, 0);
						temp = 'L';
						send(p2sock, &temp, 1, 0);
						looplogic = 0;
					} else if (wincheck == 2){
						//player 2 wins
						temp = 'W';
						send(p2sock, &temp, 1, 0);
						temp = 'L';
						send(p1sock, &temp, 1, 0);
						looplogic = 0;
					} else if (wincheck == 0){
						// game is a tie
						temp = 'T';
						send(p1sock, &temp, 1, 0);
						send(p2sock, &temp, 1, 0);
						looplogic = 0;
					} else if (wincheck == 3){
						// only valid for popout, there is a tie however we designate the player who made the move to be the winner.
						temp = 'W';
						send(p1sock, &temp, 1, 0);
						temp = 'L';
						send(p2sock, &temp, 1, 0);
						looplogic =0;
					}
				} else {
					// invalid move, loop until valid move is made.
					temp = 'I';
					send(p1sock, &temp, 1, 0);
					invalid = 1;
				}
			}
			// change player
			curplayer = 2;
		} else if(curplayer == 2) {
			//playe 2 turn send turn and gamestate;
			temp = 'H';
			send(p1sock, &temp,1,0);
			send(p1sock, &gamestate, sizeof(gamestate),0);
			temp = 'Y';
			send(p2sock, &temp,1,0);
			send(p2sock, &gamestate, sizeof(gamestate),0);
			int invalid = 1;
			// loop for invalid turn
			while (invalid ==1){
				//get users move
				recv(p2sock, &usermove, 2,MSG_WAITALL);
				// check validity
				if (validmove(usermove, gametoplay, '2')==1){
					invalid = 0;
					//calculate x
					int x = usermove[1]-minusConversion;
					int y;
					int i;
					// calculate y
					for(i=0; i<6; i++){
						if(gamestate[x+ 7*i]!='0'){
							y=i;
							i=7;
						}

					}
					// check victory conditions, exit game if victory/loss/tie
					int wincheck;
					wincheck = victoryCheck(gametoplay,x, y);
					if(wincheck == 1){
						temp = 'W';
						send(p1sock, &temp, 1, 0);
						temp = 'L';
						send(p2sock, &temp, 1, 0);
						looplogic = 0;
					} else if (wincheck == 2){
						temp = 'W';
						send(p2sock, &temp, 1, 0);
						temp = 'L';
						send(p1sock, &temp, 1, 0);
						looplogic = 0;
					} else if (wincheck == 0){
						temp = 'T';
						send(p1sock, &temp, 1, 0);
						send(p2sock, &temp, 1, 0);
						looplogic = 0;
					} else if (wincheck ==3){
						temp = 'W';
						send(p2sock, &temp, 1, 0);
						temp = 'L';
						send(p2sock, &temp, 1, 0);
						looplogic =0;
					}
				} else {
					// invalid move loop until valid.
					temp = 'I';
					send(p2sock, &temp, 1, 0);
					invalid = 1;
				}
			}
			// change player
			curplayer = 1;
		}
	}

}