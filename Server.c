#define _GNU_SOURCE
#include <arpa/inet.h>
#include <time.h>
#include <stdio.h> 
#include <stdlib.h> 
#include <errno.h> 
#include <string.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/wait.h> 
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#include <signal.h>



/* number of threads used to service requests */
#define THREADNO 10
#define BACKLOG 10     /* size of queue to hold pending connections */
#define RETURNED_ERROR -1 /* value of returned error */
#define PORT_NO 12345  /* port number to conect to CHANGEE!!!! */
#define MAXDATASIZE 100


pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
pthread_cond_t  got_request   = PTHREAD_COND_INITIALIZER;

pthread_t  p_threads[THREADNO],sighandler;   /* thread's structures   */
int thr_id[THREADNO];      /* thread IDs            */
int num_requests = 0;   /* number of pending requests, initially none */

/* format of a single request. */
typedef struct request request_t;
struct request {
    int number;             /* number of the request                  */
	int fd;					/* clients_fd							  */
	char *username;
    int gamesPlayed;
    int gamesWon;
    request_t* next;   /* pointer to next request, NULL if none. */
};

struct request* requests = NULL;     /* head of linked list of requests. */
struct request* last_request = NULL; /* pointer to last request.         */
int sockfd, new_fd;
int buf2[MAXDATASIZE];
char buf[MAXDATASIZE];

void menuCommand();
void playHangman();
void sig_handler(int signo);

void *sig() {
	if (signal(SIGINT, sig_handler) == SIG_ERR) {
	  printf("\ncan't catch SIGINT\n");
	}
	return 0;
}

void sig_handler(int signo) {
  if (signo == SIGINT) { 
  	printf("\n\nReceived SIGINT - Terminating all threads\n");
	
	for (int i = 0; i < THREADNO; i++) {
		///free(a_request);
		pthread_cancel(p_threads[i]);
		
	}
	exit(0);
  }
    
}
	//these functions receive data from the Client through the socket
 	void receiveInt(struct request* a_request){
        recv(a_request->fd, buf2,MAXDATASIZE,0);
	}

	void receiveChar(struct request* a_request) {
		recv(a_request->fd,buf,MAXDATASIZE,0);
	}

	void sendChar(char buf[MAXDATASIZE],struct request* a_request) {
		send(a_request->fd,buf,MAXDATASIZE,0);
	}
	
	void sendInt(int buf2[MAXDATASIZE],struct request* a_request) {
		send(a_request->fd,buf2,MAXDATASIZE,0);
	}

	bool readAuthFile(char user[MAXDATASIZE], char pass[MAXDATASIZE]) {
		char userArr[11][8];
		char passArr[11][8];
		bool next = false;

		FILE *fp;
		fp = fopen("Authentication.txt", "r");
		
		int cnt = 0;
		while(!feof(fp)) {
			fscanf(fp,"%s", (char *)userArr[cnt]);
			fscanf(fp,"%s", (char *)passArr[cnt]);
			cnt += 1;
		}

		for (int i = 0; i < 12; i++) {
			if (strcmp(user, userArr[i]) == 0 && strcmp(pass, passArr[i]) == 0) {
				return true;
			} 
		}
		return false;
	}

	bool authenticate(struct request* a_request) {
		char *username = (char *)malloc(sizeof(char) * MAXDATASIZE);
		char *password = (char *)malloc(sizeof(char) * MAXDATASIZE);

			receiveChar(a_request);
			strncpy(username, buf,MAXDATASIZE);

			receiveChar(a_request);
			strncpy(password, buf,MAXDATASIZE);

			if (readAuthFile(username, password) == true) {
				a_request->username = username;
				return true;
			} else {
				return false;
			}

			free(username);
			free(password);

		return true;
	}
	
	//this function handles anything to do with running the hangman game
	void playHangman(struct request* a_request) {
		char tempArr[1000][15];
		char objectArr[288][15];
		char typeArr[288][15];
		char guess[MAXDATASIZE];
		char *token;
		int tmp = 0;
		char guessNumberObject[MAXDATASIZE];
		char guessNumberType[MAXDATASIZE];
		char tempbuf[MAXDATASIZE];
		

		 FILE *fp;
		 fp = fopen("hangman_text.txt", "r");
		
		 //put all the objects and types read from file into arrays
		 int cnt = 0;
		 while(!feof(fp)) {
			 fscanf(fp,"%s", (char *)tempArr[cnt]);
	 
			 token = strtok(tempArr[cnt],",");		 
			 strcpy(objectArr[cnt], token);
	 
			 token = strtok(NULL,",");
			 strcpy(typeArr[cnt], token);
	 
			 cnt += 1;
		 }

		//picks a random word pair
		srand(time(NULL));
		int a = rand() % 288;

		int send1 =(int)strlen(objectArr[a]);
		int send2 = (int)strlen(typeArr[a]);
		sendInt(&send1,a_request);
		sendInt(&send2,a_request);
		 
		sendChar(a_request->username,a_request);
		
		/*receiving guesses and checking all letters that match then returning
		the appropriate positions in arrays*/
		while (1) {
			receiveChar(a_request);
			if (strcmp(buf,"GameWon") == 0) {
				a_request->gamesWon += 1;
				a_request->gamesPlayed += 1;
				break;
			} else if (strcmp(buf,"GameLost") == 0) {
				a_request->gamesPlayed += 1;
				break;
			} 

			receiveChar(a_request);
			strncpy(guess, buf,MAXDATASIZE);

			tempbuf[0] = 0;
			guessNumberObject[0] = 0;
			guessNumberType[0] = 0;
			tmp = 0;

			while (objectArr[a][tmp] != '\0') {
				if (objectArr[a][tmp] == *guess) {		
					sprintf(tempbuf, "%d", tmp);			
					strcat(guessNumberObject, tempbuf);
				}
				tmp += 1;
			}
			sendChar(guessNumberObject,a_request);
			if ((int)strlen(guessNumberObject) > 0) { 
				sendChar("Correct",a_request);
			} else {
				sendChar("Incorrect",a_request);
			}

			tempbuf[0] = 0;
			guessNumberObject[0] = 0;
			guessNumberType[0] = 0;
			tmp = 0;
	
			while (typeArr[a][tmp] != '\0') {
				if (typeArr[a][tmp] == *guess) {		
					sprintf(tempbuf, "%d", tmp);			
					strcat(guessNumberType, tempbuf);
				}
				tmp += 1;
			}
			sendChar(guessNumberType,a_request);
			if ((int)strlen(guessNumberType) > 0) { 
				sendChar("Correct",a_request);
			} else {
				sendChar("Incorrect",a_request);
			}
		}	
		//return to menu once the game is complete
		menuCommand(a_request);
	}

	//sends the leaderboard info for the current user to client
	int leaderBoard(struct request* a_request) {			
		int played = a_request->gamesPlayed;
		int won = a_request->gamesWon;
		sendInt(&played, a_request);
		sendInt(&won, a_request);
		sendChar(a_request->username, a_request);

		menuCommand(a_request);
	}

	//this function receives the input from the client regarding the main menu
	void menuCommand(struct request* a_request) {
		receiveChar(a_request);
		if (strcmp(buf, "playHangman") == 0) {
			playHangman(a_request);
		} else if (strcmp(buf, "leaderBoard") == 0) {
			leaderBoard(a_request);
		} else if (strcmp(buf, "Terminate") == 0) {
		}
	}


	//this function is where the users identity is validated and commences the program
	void processRun(struct request* a_request) { 
		while(1) {
			if (authenticate(a_request) == true) {
				sendChar("Confirmed",a_request);
				break;
			} else {
				sendChar("Denied",a_request);
				break;
			}
		}
		menuCommand(a_request);
	}






 // function add_request(): add a request to the requests list
void add_request(int request_num, int client_fd, pthread_mutex_t* p_mutex, pthread_cond_t*  p_cond_var)
{
    int rc;                         /* return code of pthreads functions.  */
    struct request* a_request;      /* pointer to newly added request.     */

    /* create structure with new request */
    a_request = (struct request*)malloc(sizeof(struct request));
    if (!a_request) { /* malloc failed?? */
        fprintf(stderr, "add_request: out of memory\n");
        exit(1);
    }
    a_request->number = request_num;
	a_request->fd = client_fd;
    a_request->next = NULL;

    /* lock the mutex, to assure exclusive access to the list */
    rc = pthread_mutex_lock(p_mutex);

    /* add new request to the end of the list, updating list */
    /* pointers as required */
    if (num_requests == 0) { /* special case - list is empty */
        requests = a_request;
        last_request = a_request;
    }
    else {
        last_request->next = a_request;
        last_request = a_request;
    }

    /* increase total number of pending requests by one. */
    num_requests++;

    /* unlock mutex */
    rc = pthread_mutex_unlock(p_mutex);

    /* signal the condition variable - there's a new request to handle */
    rc = pthread_cond_signal(p_cond_var);
}


 // function get_request(): gets the first pending request from the requests list
struct request* get_request(pthread_mutex_t* p_mutex)
{
    int rc;                         /* return code of pthreads functions.  */
    struct request* a_request;      /* pointer to request.                 */

    /* lock the mutex, to assure exclusive access to the list */
    rc = pthread_mutex_lock(p_mutex);

    if (num_requests > 0) {
        a_request = requests;
        requests = a_request->next;
        if (requests == NULL) { /* this was the last request on the list */
            last_request = NULL;
        }
        /* decrease the total number of pending requests */
        num_requests--;
    }
    else { /* requests list is empty */
        a_request = NULL;
    }

    /* unlock mutex */
    rc = pthread_mutex_unlock(p_mutex);

    /* return the request to the caller. */
    return a_request;
}


 //function handle_request(): handle a single given request.
void handle_request(struct request* a_request, int thread_id)
{
    if (a_request) {
        processRun(a_request); //this starts my entire game

        printf("Thread '%d' handled request '%d'\n",
               thread_id, a_request->number);
        fflush(stdout);
    }
}


 //function handle_requests_loop(): infinite loop of requests handling
void* handle_requests_loop(void* data)
{
    int rc;                         /* return code of pthreads functions.  */
    struct request* a_request;      /* pointer to a request.               */
	int thread_id = *((int*)data);  /* thread identifying number           */
	//this ensures that the thread will cancel when asked to
	int s = pthread_setcancelstate(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    /* lock the mutex, to access the requests list exclusively. */
    rc = pthread_mutex_lock(&request_mutex);

    /* do forever.... */
    while (1) {

        if (num_requests > 0) { /* a request is pending */
            a_request = get_request(&request_mutex);
            if (a_request) { /* got a request - handle it and free it */
                /* unlock mutex - so other threads would be able to handle */
                /* other reqeusts waiting in the queue paralelly.          */
                rc = pthread_mutex_unlock(&request_mutex);
				
				/* --- Main stuff goes in here --- */
                handle_request(a_request, thread_id); //  code thread should execute
                free(a_request); //  Code Completed so free the thread
				/* ---- --------------------- --- */
				
				
                /* and lock the mutex again. */
                rc = pthread_mutex_lock(&request_mutex);
            }
        }
        else {
            /* wait for a request to arrive. note the mutex will be */
            /* unlocked here, thus allowing other threads access to */
            /* requests list.                                       */

            rc = pthread_cond_wait(&got_request, &request_mutex);
            // the mutex is locked again

        }
    }
}

int main(int argc, char* argv[]) {
 
    
    
	struct timespec delay;     /* used for wasting time */
	
	/* define socket attributes */
	  /* listen on sock_fd, new connection on new_fd */
	uint16_t portNumber;
	struct sockaddr_in my_addr;    /* my address information */
	struct sockaddr_in their_addr; /* connector's address information */
	socklen_t sin_size;

	/* Get port number for server to listen on */
    if (argc > 1){ 
		portNumber = atoi(argv[1]);
	}else{
		portNumber = PORT_NO;
	}
	/* generate the socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	/* generate the end point */
	my_addr.sin_family = AF_INET;         /* host byte order */
	my_addr.sin_port = htons(portNumber);    /* short, network byte order */
	my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */

	/* bind the socket to the end point */
	if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) \
	== -1) {
		perror("bind");
		exit(1);
	}
    
    /*create thread for signal handler*/
    pthread_create(&sighandler, NULL, sig, 0);

	/* create the request-handling threads */
    for (int i=0; i<THREADNO; i++) {
        thr_id[i] = i;
        pthread_create(&p_threads[i], NULL, handle_requests_loop, (void*)&thr_id[i]);
    }

	/* start listening */
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	printf("server starts listening ...\n");

	/* adds a new request for each connection */
	int i = 0;
	while(1) {  /* main accept() loop */
		sin_size = sizeof(struct sockaddr_in);
		if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, \
		&sin_size)) == -1) {
			perror("accept");
			continue;
		}else{
		printf("server: got connection from %s\n", \
			inet_ntoa(their_addr.sin_addr));
			add_request(i, new_fd, &request_mutex, &got_request);
			i++;
		}
	
	}
    /* now wait till there are no more requests to process */
    sleep(5);

    printf("Glory,  we are done.\n");
    
    return 0;
}