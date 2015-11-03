/*David Shagam
CSCI 367 Program 3 Server*/

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/select.h>

fd_set managedSet;
int partSock;
#define MAX_PARTICIPANT 8
#define MAX_OBSERVERS 16
#define KICK_THRESHOLD 3
int largestSock;
int obsSock;
int tempPartSock;
//int tempSock;
int observers[MAX_OBSERVERS];
char buf[1024];
char *answers[1024];
struct sockaddr_in cad;
int numObs =0;
int numPart = 0;
int indexOfQuestion;
int inround = 0;
int gameStart = 0;

typedef struct {
	int socket;
	int score;
	int idle;
	int didAns;
	char name[16];
	char answer[1024];
} playerst;

typedef struct{
	int value;
	char catagory[100];
	char question[500];
	char answerHuman[500];
	char answer[500];

} question;

question questiontoAsk;

playerst participants[MAX_PARTICIPANT];

//load the questions
loadQuestion(){
	char temparray[1024];
	if(fgets(temparray, sizeof(temparray), stdin) != NULL){
		char *delim = "	";

		char* token = strtok(temparray, delim);
		memset(questiontoAsk.catagory, 0, sizeof(questiontoAsk.catagory));
		strncpy(questiontoAsk.catagory, token, strlen(token));

		token = strtok(NULL, delim);
		questiontoAsk.value = atoi(token+1);

		token = strtok(NULL, delim);

		memset(questiontoAsk.question, 0, sizeof(questiontoAsk.question));
		strncpy(questiontoAsk.question, token, strlen(token));
		
		token = strtok(NULL, delim);
		memset(questiontoAsk.answer, 0, sizeof(questiontoAsk.answer));
		memset(questiontoAsk.answerHuman, 0 , sizeof(questiontoAsk.answerHuman));
		strncpy(questiontoAsk.answerHuman, token, strlen(token));
		char temp[1024];
		sprintf(temp,"%s" ,token);
		int it;
		int placement=0;
		for(it = 0; it < strlen(token); it++){
			if(isspace(temp[it]) == 0 && temp[it] != '\n'){
				questiontoAsk.answer[placement]= tolower(temp[it]);
				placement++;
			}
		}

	} else {
		printf("Out of questions quitting.\n");
		exit(EXIT_SUCCESS);

	}
}

askQuestion(){
	loadQuestion();
	char temp[1024];
	sprintf(temp, "The catagory is %s worth $%d points\nQuestion: %s\n", questiontoAsk.catagory, questiontoAsk.value, questiontoAsk.question );
	printf("%s\n",temp);
	messageObs(temp);

}

getAnswers(int pSocket, int index){
	if(participants[index].didAns == 0 && gameStart == 1){
		participants[index].didAns=1;
		char temp[1024];
		memset(temp, 0, sizeof(temp));
		int sizeRec= recv(pSocket, temp, sizeof(temp), 0);
		if(sizeRec > 0){
			int i;
			int placement = 0;
			participants[index].idle = 0;
			memset(participants[index].answer, 0, sizeof(participants[index].answer));
			//recv(pSocket, participants[index].answer, sizeof(participants[index].answer), 0);
			int lenstr = strlen(temp);
			for(i = 0; i < lenstr; i++){
				if(isspace(temp[i])==0 && temp[i] != '\n'){
					participants[index].answer[placement] = tolower(temp[i]);
					placement++;
				}
			}
			printf("message recieved is %s\n", participants[index].answer);
		} else if (sizeRec <0){
			printf("Player has disconnected");
			kickPlayer(index);
		}
		
	} else if(participants[index].didAns == 1) {
		//dump the data
		char temp[1024];
		recv(pSocket, temp, sizeof(temp),0);
		memset(temp, 0, sizeof(temp));
	}
}

kickPlayer(int index){
	close(participants[index].socket);
	participants[index].idle = 0;
	participants[index].score = 2000;
	participants[index].didAns = 0;
	participants[index].socket = -1;
	memset(participants[index].name, 0, sizeof(participants[index].name));
	memset(participants[index].answer, 0, sizeof(participants[index].name));
	numPart--;
	printf("kicked a participant\n");
}

checkAnswers(){
	int it;
	memset(buf, 0, sizeof(buf));
	for(it = 0; it<MAX_PARTICIPANT; it++){
		if(participants[it].didAns ==1 && participants[it].socket!= -1){
			if(strcasecmp(participants[it].answer, questiontoAsk.answer) == 0 ){
				// matching case
				participants[it].score += questiontoAsk.value;
				char temp[1024];
				sprintf(temp, "Player %s answered correctly.\n", participants[it].name);
				strcat(buf, temp);
			} else {
				// not matching case.
				participants[it].score -= (questiontoAsk.value / 2);
				char temp[1024];
				sprintf(temp, "Player %s answered incorrectly.\n", participants[it].name);
				strcat(buf, temp);

			}
		}
		//idle or low score kick.
		if( participants[it].idle >= KICK_THRESHOLD && participants[it].socket != -1){
			char temp[1024];
			sprintf(temp, "Player %s has been kicked from the game.\n", participants[it].name );
			strcat(buf, temp);
			kickPlayer(it);

		}
	}
	char temp2[1024];
	sprintf(temp2, "the correct answer was: %s\n",questiontoAsk.answerHuman);
	strcat(buf, temp2);
	messageObs(buf);
	printScores();
}

printScores(){
	char message[1024];
	memset(message, 0, sizeof(message));
	strcat(message,"---------------------------\n");

	strcat(message, "|     Player     | Score  |\n");
	strcat(message,"---------------------------\n");


	int it1;
	for(it1=0; it1 < MAX_PARTICIPANT; it1++){
		if(participants[it1].socket != -1){
			strcat(message, "|");
			strcat(message, participants[it1].name );

			int numPad = 16 - strlen(participants[it1].name);
			int it2;
			char pad1[16];
			for(it2 = 0; it2 < numPad; it2++){
				strcat(message, " ");
			}
			char scoreval[100];

			strcat(message, "|");


			sprintf(scoreval, "%d", participants[it1].score);

			strcat(message, scoreval);

			numPad = 7 - strlen( scoreval);
			for(it2 = 0; it2< numPad; it2++){
				strcat(message," ");
			}
			strcat(message, "|\n");
		}
		

	}
	messageObs(message);
	printf("%s",message);

}

checkConnected(){
	int i;
	char temp[1024] = "ping";
	for(i = 0; i <MAX_PARTICIPANT; i++){
		if( participants[i].socket > -1){
			
			int retval = write(participants[i].socket,temp, strlen(temp), 0);
			if (retval < 0){
				kickPlayer(i);
			}
		}
	}
	//checkObs();


}

// builds and rebuilds the managed sockets for select calls
buildFDSet(){
	checkConnected();
	//printf("build fd set called\n");
	largestSock = 0;
	FD_ZERO(&managedSet);
	if(inround == 0){
		if (largestSock < partSock){
			largestSock = partSock;
		}
		FD_SET(partSock, &managedSet);
		if(largestSock < obsSock){
			largestSock = obsSock;
		}
		FD_SET(obsSock, &managedSet);
		if( tempPartSock !=0){
			if(largestSock < tempPartSock){
				largestSock = tempPartSock;
			}
			FD_SET(tempPartSock, &managedSet);
		}
	}
	int i;
	for(i = 0; i<MAX_OBSERVERS; i++){
		if(observers[i]!= 0){
			if(largestSock< observers[i]){
				largestSock = observers[i];
			}
			FD_SET(observers[i], &managedSet);
		}
	}

	for (i = 0; i<MAX_PARTICIPANT; i++){
		if(participants[i].socket != -1){
			if(largestSock < participants[i].socket){
				largestSock = participants[i].socket;
			}
			FD_SET(participants[i].socket, &managedSet);
			//printf("added participant %d to list of managed sockets\n", i);
		}
	}
}

// messaes all observers 
messageObs(char *message){
	//printf("messageObs called\n");
	//write(1, message, strlen(message));
	int i;
	for( i = 0; i < MAX_OBSERVERS; i++){
		if(observers[i]!= 0){
			int len = strlen(message);
			if ( sizeof(buf)< len){
				len = sizeof(buf);
			}
			int valid = send(observers[i], message, len,0);
			if (len>=1023){
				char * temp2= "\n";
				send(observers[i], temp2, strlen(temp2),0);
			}
			//printf("\n VALID IS %d and stringlength is %d \n \n ", valid, len);
			if (valid < 0){
				//printf("socket is %d is closed.\n", i);
				//shutdown(observers[i], SHUT_RDWR);
				close(observers[i]);
				observers[i] = 0;
				numObs--;
				printf("kicked an observer\n");
			}
		} 
	}

}

// connect a new observer
connectObs(){
	//printf("connectObs called\n");
	int alen = sizeof(cad);
	int tempSock = accept(obsSock, (struct sockaddr *)&cad, &alen);
	int i = 0;
	while(observers[i] != 0){
		i++;
	}
	if( i >=MAX_OBSERVERS){
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "No room please try again later\n");
		send(tempSock, buf, sizeof(buf), 0 );
		close(tempSock);
	} else {
		//printf("spot for observer found spot is %d and socket num is %d\n",i, tempSock);
		observers[i]= tempSock;
		numObs++;
		memset(buf, 0, sizeof(buf));
		//sprintf(buf, "A new observer has joined\n");
		//messageObs(buf);
		//printf("num obs is %d\n",numObs);
	}
	tempSock = 0;
}


// connect a new participant
connectPart(){
	//printf("connectPart called\n");
	int alen = sizeof(cad);
	tempPartSock = accept(partSock, (struct sockaddr *)&cad, &alen);
	validateConPart();

}

newRound(){
	int i;
	for(i=0; i<MAX_PARTICIPANT; i++){
		memset(participants[i].answer, 0 , sizeof(participants[i].answer));
		participants[i].didAns = 0;
	}
}

validateConPart(){
	printf("validate participant called\n");
	int i = 0;
	char tempName[16];
	memset(tempName, 0, sizeof(tempName));
	int len = recv(tempPartSock, tempName, sizeof(tempName), 0);
	printf("name to check is %s \n", tempName);
	while(participants[i].socket!= -1 && i < MAX_PARTICIPANT){
		i++;
	}
	if(len >=0){
		if (i >= MAX_PARTICIPANT ){
			//printf("no participant room");
			printf("sending an F \n");
		  char tempchar = 'F';
		  send(tempPartSock, &tempchar, sizeof(tempchar),0 );
		  close(tempPartSock);
		  tempPartSock = 0;
		} else {
			if( len>1 && len < 18 ){
				int it;
				char tempchar = ' ';
				for(it = 0; it <MAX_PARTICIPANT; it++ ){
					if(strcasecmp(participants[it].name, tempName) == 0){
						tempchar = 'I';
						printf("found a duplicate name\n");
						it = MAX_PARTICIPANT;
					}
				}
				if (tempchar != 'I'){
					printf("no duplicate found placing user\n");
					printf("spot found for participant spot is %d and socket number is %d\n", i, tempPartSock);
					participants[i].socket = tempPartSock;
					memset(participants[i].name, 0, sizeof(participants[i].name));
					int a;
					int pntr=0;
					for(a=0; a <16; a++){
						if(tempName[a] != '\n'){
							participants[i].name[pntr] = tempName[a];
							pntr++;
						}

					}
					//strcpy(participants[i].name, tempName);
					participants[i].score = 2000;
					participants[i].idle = 0;
					participants[i].didAns = 0;
					memset(participants[i].answer, 0, sizeof(participants[i].answer));

					tempchar = 'V';
					 int val = send(tempPartSock, &tempchar, sizeof(tempchar), 0);
					 /*if(val <= 0){
					 	close(tempPartSock);
					 	tempPartSock = 0;
					 }*/
					char temp2[1024];
					memset(temp2, 0, sizeof(temp2));
					sprintf(temp2, "User: %s has joined the game\n", participants[i].name);
					messageObs(temp2);
					tempPartSock = 0;
					numPart++;
				} else {
					send(tempPartSock, &tempchar, sizeof(tempchar), 0);
				}
			} else {
				char tempchar = 'I';
				int val = send(tempPartSock, &tempchar, sizeof(tempchar), 0 );
				if( val < 0 ){
					close(tempPartSock);
					tempPartSock = 0;
				}
			}
		}
	} else {
		close(tempPartSock);
		tempPartSock=0;
	}
}


//check unused observers
checkObs(){
	int i;
	for(i = 0; i <MAX_OBSERVERS; i++){
		if( observers[i]!=0){
			char temp[1024];
			memset(temp,0, sizeof(temp));
			int retval = recv(observers[i],temp, sizeof(temp), MSG_DONTWAIT);
			//printf("check obs ran and retval is %d \n ",retval);
			if (retval <= 0){
				close(observers[i]);
				observers[i]=0;
				numObs--;
			}
		}
	}
}

manageFDSet(){
	//printf("Managed FD Set called\n");
	printf("\nmanaging fd set\n");
	if(inround == 0){
		if( FD_ISSET(partSock, &managedSet)!= 0 && tempPartSock == 0){
			printf("calling connectPart\n");
			connectPart();
		}
		if(FD_ISSET(obsSock, &managedSet)!= 0){
			connectObs();
		}
		if(FD_ISSET(tempPartSock, &managedSet) !=0 && tempPartSock != 0){
			printf("calling validateconpart\n");
			validateConPart();
		}
	}

	printf("\nnow handling observers\n");
	int i;
	for( i = 0; i < MAX_OBSERVERS; i++){
		if(observers[i] != 0 && FD_ISSET(observers[i], &managedSet)){
			memset(buf, 0, sizeof(buf));
			if(recv(observers[i], buf, sizeof(buf),0 ) <= 0){
				close(observers[i]);
				observers[i]=0;
				numObs--;
			}
		}
	}
	printf("\nnow handling participants\n");
	for(i = 0; i < MAX_PARTICIPANT; i++){
		if( participants[i].socket!= -1 && FD_ISSET(participants[i].socket, &managedSet)!= 0){	
			//printf("Participant with data found user number is %d\n",i);
			getAnswers(participants[i].socket, i);
		}
	}

}

noAnswers(){
	int i;
	for(i = 0; i< MAX_PARTICIPANT; i++){
		if(participants[i].didAns == 0){
			participants[i].idle++;
		}
	}
}

main (int argc, char *argv[]){
	signal(SIGPIPE, SIG_IGN);
	struct protoent *protocol;
	struct sockaddr_in sad;
	struct sockaddr_in sad2;
	int portObs;
	int portPart;
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 100;

	int iter;
	for (iter = 0; iter < MAX_OBSERVERS; iter++){
		observers[iter]= 0;
	}
	for (iter = 0; iter < MAX_PARTICIPANT; iter++){
		participants[iter].socket =-1;
		participants[iter].score = 2000;
		participants[iter].idle = 0;
		participants[iter].didAns = 0;
	}

	if (argc !=3){
		fprintf(stderr, "WRONG Num args needs 2 args \n First arg, Participant port, 2nd arg Observer port\n");
		exit(EXIT_FAILURE);
	}
	// first listen port
	memset((char *)&sad, 0, sizeof(sad));
	sad.sin_family = AF_INET;
	sad.sin_addr.s_addr = INADDR_ANY;
	// 2nd listen port
	memset((char *)&sad2, 0, sizeof(sad2));
	sad2.sin_family = AF_INET;
	sad2.sin_addr.s_addr = INADDR_ANY;

	portPart = atoi(argv[1]);
	portObs = atoi(argv[2]);
	// make sur enot same port.
	if (portPart == portObs){
		fprintf(stderr, "PORTS ARE THE SAME PLEASE USE DIFFERNT PORTS\n");
		exit(EXIT_FAILURE);
	}
	//participant port
	if(portPart > 0){
		sad.sin_port = htons((u_short)portPart);
	} else {
		fprintf(stderr, "First argument bad port\n");
		exit(EXIT_FAILURE);
	}
	// obs port
	if(portObs > 0 ){
		sad2.sin_port = htons((u_short)portObs);
	} else {
		fprintf(stderr, "Second argument bad port\n");
		exit(EXIT_FAILURE);
	}

	//setup protocol
	if ( ((long int)(protocol = getprotobyname("tcp")))==0){
		fprintf(stderr, "can't map tcp\n");
		exit(EXIT_FAILURE);
	}

	//create listen participant sock
	partSock = socket(PF_INET, SOCK_STREAM, protocol->p_proto);
	if(partSock < 0){
		fprintf(stderr, "participant socket failed\n");
		exit(EXIT_FAILURE);
	}
	//create observer socket
	obsSock = socket(PF_INET, SOCK_STREAM, protocol->p_proto);
	if(obsSock < 0){
		fprintf(stderr, "Observer socket failed\n");
		exit(EXIT_FAILURE);
	}

	//bind particiapnt
	if(bind(partSock, (struct sockaddr *)&sad, sizeof(sad)) <0){
		fprintf(stderr, "Participant failed to bind\n");
		exit(EXIT_FAILURE);
	}
	// bind observer
	if(bind(obsSock, (struct sockaddr *)&sad2, sizeof(sad2)) < 0){
		fprintf(stderr, "Observer failed to bind\n");
		exit(EXIT_FAILURE);
	}
	//2 queues.
	if(listen(partSock, 65)<0){
		fprintf(stderr, "Can't listen for Participants\n");
		exit(EXIT_FAILURE);
	}

	if(listen(obsSock, 65)<0){
		fprintf(stderr, "Can't listen for Observers\n");
		exit(EXIT_FAILURE);
	}

	while(1){
		inround = 0;
		gameStart = 0;
		if(numObs < 1 || numPart < 1){
			buildFDSet();
			printf(" number of obs = %d, number of part = %d \n", numObs, numPart);
			int num = select(largestSock+1, &managedSet, (fd_set *) 0, (fd_set *) 0,NULL);
			//printf("the value of num is %d\n",num);

			if(num > 0){
				manageFDSet();
			}
		} else {
			//play game.
			gameStart = 1;
			printf("round started\n");
			askQuestion();
			inround= 1;
			printf("sleeping for 15 seconds \n");
			int numsleep =0;
			while(numsleep< 15){
				buildFDSet();
				sleep(1);
				numsleep++;
				//printf("running select \n");
				int num = select(largestSock+1, &managedSet, (fd_set *)0, (fd_set *)0, &timeout);
				//printf("select returned %d \n",num );
				while(num > 0){
					if(numPart > 0 && numObs > 0){
						manageFDSet();
						//printf("managed the set\n");
						buildFDSet();
						//printf("build set finished\n");
						num = select(largestSock+1, &managedSet, (fd_set *)0, (fd_set *)0, &timeout);
						//printf(" num is %d in while loop \n", num);
						}	
					}
			}
			//printf("got to checking answers\n");
			noAnswers();
			checkAnswers();
			//printf("made it past checking answers\n");
			inround = 0;
			int iter2;
			printf("intermission \n");
			for(iter2 =0; iter2 < 5; iter2++){
				buildFDSet();
				sleep(1);
				int num2 = select(largestSock+1, &managedSet, (fd_set *)0, (fd_set *)0,&timeout);
				while(num2 > 0){
					manageFDSet();
					buildFDSet();
					num2 = select(largestSock+1, &managedSet, (fd_set *)0, (fd_set *)0, &timeout);
				}
			}
			//printf("made it to new round\n");
			newRound();
			//printf("made it past new round\n");
			//if(num> 0)
		}



	}
}
