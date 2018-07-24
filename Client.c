#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h>
#include <stdbool.h>

#define MAXDATASIZE 100 /* max number of bytes we can get at once */

int sockfd, numbytes, i=0;  
struct hostent *he;
struct sockaddr_in their_addr; /* connector's address information */
int buf2[MAXDATASIZE];
char buf[MAXDATASIZE];
char recbuf[MAXDATASIZE];
int recbuf2[MAXDATASIZE];

void command();


//Receivefunctions
void receiveInt() {
	recv(sockfd,buf2,MAXDATASIZE,0);
	
}

void receiveChar() {
	recv(sockfd,buf,MAXDATASIZE,0);
	strcpy(buf,buf);
}

// Sendfunctions
void sendChar(char buf[MAXDATASIZE]) {
	send(sockfd,buf,MAXDATASIZE,0);
}

void sendInt(int *buf2) {
	send(sockfd,buf2,MAXDATASIZE,0);
}


bool authenticate() {
	char *username = (char *)malloc(sizeof(char) * MAXDATASIZE);
	char *password = (char *)malloc(sizeof(char) * MAXDATASIZE);
	bool next;

	printf("===========================================\n\n\n");
	printf("Welcome to the Online Hangman Gaming System\n\n\n");
	printf("===========================================\n\n\n");

	printf("\nYou are required to logon with your registered Username and Password\n");

	printf("\nPlease enter your username-->");
	scanf("%s",username);
	sendChar(username);

	printf("Please enter your password-->");
	scanf("%s",password);
	sendChar(password);

	//receive whether the authentication was accepted or denied 
	receiveChar();
	if (strcmp(buf, "Confirmed") == 0) {
		return true;
	} else if (strcmp(buf, "Denied") == 0) {
		printf("\nYou entered either an incorrect username or password - disconnecting\n");
		return false;
	}
	

	free(password);
	free(username);
}

//this function is the main menu where the user inputs what they wish to do
int mainMenu() {
	char selection = 0;
	int check = 0;
	printf("\n\nWelcome to the Hangman Gaming System\n\n\n\n");
	printf("Please enter a selection\n");
	printf("<1> Play Hangman\n");
	printf("<2> Show Leaderboard\n");
	printf("<3> Quit\n\n");

	while (1) { 
		if (check >= 1 && check <= 3) {
			break;
		} else {
			printf("\n\nplease enter a valid menu option");
			printf("\n\nSelection option 1-3 ->");
			scanf("%s", &selection);
			check = atoi(&selection);
		}
	}
	return check;
}

//this is the function that handles anything to do with running the hangman game
void playHangman() {
	//setting all the neccessary variables
	int typeLength = 0;
	int objectLength = 0;
	char guessedLetters[MAXDATASIZE];
	char tempbuf[MAXDATASIZE];
	char places1[MAXDATASIZE][MAXDATASIZE];
	char places2[MAXDATASIZE][MAXDATASIZE];
	char name[MAXDATASIZE];
	int objectCorrect = 0;
	int typeCorrect = 0;
	int guessesLeft = 0;
	char guess = 0;
	int subtractGuess = 0;
	int cnt = 0;
	bool finished = false;
	bool underscore = false;
	bool new = false;
	char pnt = 0;
	int tmp1 = 0;

	//initialise to zero
	for (int i = 0; i < MAXDATASIZE; i++) {
		guessedLetters[i] = 0;
	    tempbuf[i] = 0;
		for (int j = 0; j < MAXDATASIZE; j++) {
			places1[i][j] = 0;
			places2[i][j] = 0;
		}
		
	}

	//receiving the length of the object and object type
	receiveInt();
	objectLength = *buf2; 
	
	receiveInt();
	typeLength = *buf2;
	
	receiveChar();
	strcpy(name, buf); //receiving the users name for postgame message

	//while the game isn't finished keep iterating
	while (finished != true) {
		guessesLeft = (objectLength + typeLength + 10) - subtractGuess;
		if (guessesLeft > 26) {
			guessesLeft = 26;
		}

		//check to see whethe the game is won or lost every turn
		if (guessesLeft == 0) {
			printf("\n\nGame over");
			printf("\n\n\nBad luck %s! You have run out of guesses. The Hangman got you!\n\n\n",name);
			sendChar("GameLost");
			break;//go back to menu
		} else if (objectCorrect >= objectLength && typeCorrect >= typeLength) {
			printf("\n\nGame over");
			printf("\n\n\nWell done %s! You won this round of Hangman!\n\n\n",name);
			sendChar("GameWon");
			break;//go back to menu
		} else {
			sendChar("Empty");
		}

		printf("\n\n\nGuessed letters: %s",guessedLetters);
		printf("\n\nNumber of guesses left: %d", guessesLeft);
		printf("\n\nWord:");


		for (int k = 0; k < objectLength; k++) {
			underscore = true;
			for (int i = 0; i < cnt; i++) {
				for (int j = 0; j < (int)strlen(places1[i]); j++) { 
					char pnt = places1[i][j];
					int tmp1 = atoi(&pnt);
					if (tmp1 == k) { 
						printf(" %c",guessedLetters[i]);
						underscore = false;
					}
				}
				
			}
			if (underscore == true) {
				printf(" _");
			}
		}
		printf("   ");
		
		for (int k = 0; k < typeLength; k++) {
			underscore = true;
			for (int i = 0; i < cnt; i++) {
				for (int j = 0; j < (int)strlen(places2[i]); j++) { 
					char pnt = places2[i][j];
                	int tmp1 = atoi(&pnt);
					if (tmp1 == k) {
						printf(" %c",guessedLetters[i]);
						underscore = false;
					}
				}
			}
			if (underscore == true) {
				printf(" _");
			}
		}
		
		//receive the users guess and make sure it is a single lowercase character
		while (1) {
			if((int)strlen(&guess) == 1 && guess >= 'a' && guess <= 'z') {
				break;
			} else {
				printf("\n\nEnter your new guess - ");
				scanf("%s", &guess);
			}
		}

		/* if the current guess is the same as any previous guess then
		set a boolean to true which will essentially re-input the last guess */
		new = true;
		for (int i = 0; i < (int)strlen(guessedLetters); i++) {
			if (guess == guessedLetters[i]) {
				new = false;
			} 
	
		}
		
		printf("\n\n---------------------------");
		char *tempsend = &guess;
		sendChar(tempsend);

		receiveChar();
		strcpy(places1[cnt], buf);

		//check to see if the character entered for was correct
		receiveChar();
		if (strcmp(buf, "Correct") == 0) {
			for (int i = 0; i < (int)strlen(places1[cnt]); i++) {
				objectCorrect += 1;
			}
		} 

		receiveChar();		
		strcpy(places2[cnt], buf);

		//check to see if the character entered was correct
		receiveChar();
		if (strcmp(buf, "Correct") == 0) {
			for (int i = 0; i < (int)strlen(places2[cnt]); i++) {
				typeCorrect += 1;
			}
		} 
		
		if (new == true) {
			strcat(guessedLetters, &guess);
			cnt += 1;
		}
		guess = 0;
		
		//counter for guesses left
		subtractGuess += 1;
	}
	//once the game is complete return to the main menu
	command();
}

//displays the high score of only the current user
void leaderBoard() {
	char username[MAXDATASIZE];
	int gamesWon = 0;
	int gamesPlayed = 0;

	receiveInt();
	gamesPlayed = *buf2;

	receiveInt();
	gamesWon = *buf2;

	receiveChar();
	strcpy(username,buf);
	
	if (gamesPlayed == 0) {	
		printf("\n\n=============================================================================");
		printf("\n\nThere is no information currently stored in the Leader Board. Try again later");
		printf("\n\n=============================================================================");
	} else {
		printf("\n\n===============================================");
		printf("\n\nPlayer - %s", username);
		printf("\nNumber of games won - %d", gamesWon);
		printf("\nNumer of games played - %d", gamesPlayed);
		printf("\n\n===============================================");
	}
	

	command();
}

//this function quits the game and exits the client
void quit() {
	close(sockfd);
}

/*this function receives input from the user and informs the server regarding
the menu*/
void command() {
	switch(mainMenu()) {
		case 1:
			sendChar("playHangman");
			playHangman();
		case 2:
			sendChar("leaderBoard");
			leaderBoard();
		case 3:
			sendChar("Terminate");
			quit();
	}
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr,"usage: client_hostname port_number\n");
		exit(1);
	}

	if ((he=gethostbyname(argv[1])) == NULL) {  /* get the host info */
		herror("gethostbyname");
		exit(1);
	}

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}


	their_addr.sin_family = AF_INET;      /* host byte order */
	their_addr.sin_port = htons(atoi(argv[2]));    /* short, network byte order */
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

	if (connect(sockfd, (struct sockaddr *)&their_addr, \
	sizeof(struct sockaddr)) == -1) {
		perror("connect");
		exit(1);
	}

		//START HERE

		if (authenticate() == true) {
			command();
		} 
		close(sockfd);
	return 0;
}
