/*David Shagam
CSCI 367 Program 2 Server*/

#include <stdio.h>
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
int largestSock;
int obsSock;
//int tempSock;
int observers[64];
int participants[64];
char buf[1024];
struct sockaddr_in cad;


// builds and rebuilds the managed sockets for select calls
buildFDSet(){
	checkConnected();
	//printf("build fd set called\n");
	FD_ZERO(&managedSet);
	if (largestSock < partSock){
		largestSock = partSock;
	}
	FD_SET(partSock, &managedSet);
	if(largestSock < obsSock){
		largestSock = obsSock;
	}
	FD_SET(obsSock, &managedSet);

	int i;
	for (i = 0; i<64; i++){
		if(participants[i] != 0){
			if(largestSock < participants[i]){
				largestSock = participants[i];
			}
			FD_SET(participants[i], &managedSet);
			//printf("added participant %d to list of managed sockets\n", i);
		}
	}
}

// messaes all observers 
messageObs(char *message){
	//printf("messageObs called\n");
	//write(1, message, strlen(message));
	int i;
	for( i = 0; i < 64; i++){
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
			if (valid < 1){
				//printf("socket is %d is closed.\n", i);
				shutdown(observers[i], SHUT_RDWR);
				close(observers[i]);
				observers[i] = 0;
				char *temp = "An Observer has left\n";
				messageObs(temp);
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
	if( i >=64){
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "No room please try again later\n");
		send(tempSock, buf, sizeof(buf), 0 );
		close(tempSock);
	} else {
		//printf("spot for observer found spot is %d and socket num is %d\n",i, tempSock);
		observers[i]= tempSock;
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "A new observer has joined\n");
		messageObs(buf);
	}
	tempSock = 0;
}

// connect a new participant
connectPart(){
	//printf("connectPart called\n");
	int alen = sizeof(cad);
	int tempSock = accept(partSock, (struct sockaddr *)&cad, &alen);
	int i = 0;
	while(participants[i]!= 0){
		i++;
	}
	if (i >=64){
		//printf("no participant room");
	  close(tempSock);
	} else {
		//printf("spot found for participant spot is %d and socket number is %d\n", i, tempSock);
		participants[i] = tempSock;
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "User %d has joined\n", i);
		messageObs(buf);
	}
	tempSock = 0;

}

// clear up unused participants
checkConnected(){
	int i;
	for(i = 0; i <64; i++){
		if( participants[i]>0){
			char temp[1024] = "ping";
			int retval = write(participants[i],temp, strlen(temp), 0);
			if (retval < 0){
				shutdown(participants[i], SHUT_RDWR);
				close(participants[i]);
				memset(temp, 0, sizeof(temp));
				sprintf(temp, "User %d has left\n", i);
				participants[i]=0;
				messageObs(temp);
			}
		}
	}

}

//check unused observers
checkObs(){
	int i;
	for(i = 0; i <64; i++){
		if( observers[i]>0){
			char temp[1024]="ping\n";
			int retval = send(observers[i],temp, 0, 0);
			//printf("check obs ran and retval is %d \n ",retval);
			if (retval < 0){
				shutdown(observers[i], SHUT_RDWR);
				close(observers[i]);
				memset(temp, 0, sizeof(temp));
				sprintf(temp, "An Observer has left\n");
				observers[i]=0;
				messageObs(temp);
			}
		}
	}
}

// reads select
readData(int sockToRead, int NumPart){
	//printf("read data called\n");
	//to do int to ascii
	char temp[1024];
	memset(temp, 0, sizeof(temp));
	int n = recv(sockToRead, temp, sizeof(buf), 0);
	if(n >0){
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%d: ",NumPart);
		messageObs(buf);
		//printf("size of n is %d \n",n);
		messageObs(temp);
	}

}

manageFDSet(){
	//printf("Managed FD Set called\n");
	if( FD_ISSET(partSock, &managedSet)){
		connectPart();
	}
	if(FD_ISSET(obsSock, &managedSet)){
		connectObs();
	}
	int i;
	for(i = 0; i < 64; i++){
		if( participants[i]!= 0){	
			if( FD_ISSET(participants[i], &managedSet)){
				//printf("Participant with data found user number is %d\n",i);
				readData(participants[i], i);
			}
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
	timeout.tv_sec = 1;
	timeout.tv_usec = 0;

	int iter;
	for (iter = 0; iter < 64; iter++){
		observers[iter]= 0;
		participants[iter]= 0;
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
		//printf("loop\n");
		buildFDSet();
		checkObs();
		int num = select(largestSock+1, &managedSet, (fd_set *) 0, (fd_set *) 0,NULL);
		//printf("the value of num is %d\n",num);
		if(num > 0){
			manageFDSet();
		}
	}
}
