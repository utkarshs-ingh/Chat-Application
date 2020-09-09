#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<pthread.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define name_length 32

static _Atomic unsigned int client_count = 0;
static int uid = 10;


// Client strucutre : sepearate file.h
typedef struct {
	struct sockaddr_in address;
	int sockfd; 
	int uid;
	char name[name_length];	
} client_t;

client_t *clients[MAX_CLIENTS]; // contain all clients with their description


pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite_stdout() {
	printf("\r%s", ">> ");
	fflush(stdout);	
}

void str_trim(char* arr, int length) {
	for(int i = 0; i < length; i++) {
		if(arr[i] == '\n') {
			arr[i] = '\0';
			break;
		}
	}
}


int LengthofString(char *buffer) {  
    int i;
    for(i= 0; i< strlen(buffer); i++){
        if(buffer[i] == '@') break;
    }
    return i;
} 

//adding and removing clients in/from the waiting queue
void add_to_queue(client_t *cl) {

	pthread_mutex_lock(&clients_mutex);
	
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(!clients[i]) { // if client is not already present 
			clients[i] = cl;
			break;
		}
	}
	
	pthread_mutex_unlock(&clients_mutex);
}


void remove_from_queue(int uid) { // unique id for each client

	pthread_mutex_lock(&clients_mutex);
	
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(clients[i]) {  // if client present remove it
			if(clients[i]->uid == uid) {
				clients[i] = NULL;
				break;
			}
		}
	}
	
	pthread_mutex_unlock(&clients_mutex);
}

// handle messages from client: 1. send in group; 2. send to specified client

void send_group_message(char *s, int uid) {
	
	pthread_mutex_lock(&clients_mutex);
	
	for(int i = 0; i < MAX_CLIENTS; i++) {
		if(clients[i]) {
			if(clients[i]->uid != uid) { //send message to all the clients except sender
				if(write(clients[i]->sockfd, s, strlen(s)) < 0) {
					printf("ERROR: write to the discriptor failed!!\n");
					break;
				}
			}
		}
	}
	
	pthread_mutex_unlock(&clients_mutex);
}

void send_private_message(char *s, int uid, char *reciever) {
	pthread_mutex_lock(&clients_mutex);
		int check = 0, sender = 0;
		for(int i = 0; i < MAX_CLIENTS; i++) {
			if(clients[i]) {
				if(clients[i]->uid == uid) sender = i;
				if(strcmp(clients[i]->name, reciever) == 0) { //only to reciever
					check = 1;
					if(write(clients[i]->sockfd, s, strlen(s)) < 0) {
						printf("ERROR: write to the discriptor failed!!\n");
						break;
					}
				}
			}
		}

		if(check != 1) { //user not found
			char buff[100];
			sprintf(buff, "ERROR: seems like %s is not here!!!\n", reciever);	
			write(clients[sender]->sockfd, buff, strlen(buff));			    
		}
	
	pthread_mutex_unlock(&clients_mutex);
}
 


void print_ip_addr(struct sockaddr_in addr) {
	printf("%d.%d.%d.%d",
	       (addr.sin_addr.s_addr & 0xff),
	       ((addr.sin_addr.s_addr & 0xff00) >> 8),
	       ((addr.sin_addr.s_addr & 0xff0000) >> 16),
	       ((addr.sin_addr.s_addr & 0xff000000) >> 24));
}

void *handle_client(void *arg) {
	
	char buffer[BUFFER_SIZE]; // store client joining and sent messages temporarily
	char name[name_length];
	int leave_flag = 0;
	
	client_count++;
	
	client_t *cli = (client_t*)arg;
	
	// get name from client
	if(recv(cli->sockfd, name, name_length, 0) <= 0 || strlen(name) < 2 || strlen(name) >= name_length - 1) {
		printf("username size must be between 2 and 32\n");
		leave_flag = 1;
	}
	else {
		strcpy(cli->name, name);
		sprintf(buffer, "%s has joined\n", cli->name);
		printf("%s", buffer);
		send_group_message(buffer, cli->uid); // sent client joining info
	}
	bzero(buffer, BUFFER_SIZE); // clear out the buffer
	
	
	while(1) {
	// check if user left 
		if(leave_flag) {
			break;
		}
		
	// exchange messages
		int recieve = recv(cli->sockfd, buffer, BUFFER_SIZE, 0);
	 	if(recieve > 0) {
	 		if(strlen(buffer) > 0) { 
	 			int i = LengthofString(buffer);
    			if(i < strlen(buffer) && buffer[i-1] == ' ') {
        			i = i + 1;
        			int j = 0;
        			char reciever[32];
        			while(buffer[i] != ' ' && i <= strlen(buffer)) {
            			reciever[j] = buffer[i];
            			j++;
            			i++;
        			}
        			reciever[j] = '\0';
        			//printf("%s", reciever);
        			send_private_message(buffer, cli->uid, reciever); //send private msg
    			}
	 			else {
				send_group_message(buffer, cli->uid); // send group message
	 			}
				str_trim(buffer, strlen(buffer));
				printf("%s\n", buffer);
	 		}
	 	}
	 	else if(recieve == 0 || strcmp(buffer, "exit") == 0) {
	 		sprintf(buffer, "%s has left the chat\n", cli->name);
	 		printf("%s", buffer);
	 		send_group_message(buffer, cli->uid);
	 		leave_flag = 1;
	 	}
	 	else {
	 		leave_flag = 1;
	 		printf("some Error occurred!!!");
	 	}
	 	bzero(buffer, BUFFER_SIZE); // clear out the buffer
	}
	// while ends: client closed connection
	close(cli->sockfd);
	remove_from_queue(cli->uid);
	free(cli); // free space allocated by malloc
	client_count -= 1;
	pthread_detach(pthread_self());
	
	return NULL;
}

int main(int argc, char **argv){
	
	if(argc != 2){
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	char *ip = "127.0.0.1"; // fixed address for server
	int port = atoi(argv[1]); // default port for server
	
	int option = 1;
	int listenfd = 0, connectfd = 0; // socket discriptors
	
	struct sockaddr_in server_info, client_info;
	pthread_t tid;
	
	// socket settings
	listenfd = socket(AF_INET, SOCK_STREAM, 0); // create master socket
	
	if (listenfd == -1) {
		printf("Socket creation failed!!!");
	}
	
	server_info.sin_family = AF_INET; // IPV4 address family
	server_info.sin_addr.s_addr = inet_addr(ip); // resolve localhost ip 
	server_info.sin_port =  htons(port); // bind socket to port mentioned on CLI
	
	//signals
	signal(SIGPIPE, SIG_IGN);
			
	// set socket to allow multiple connections
	if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEPORT | SO_REUSEADDR), (char*)&option, sizeof(option)) < 0) {
		printf("ERROR: setsockopt\n");
		return EXIT_FAILURE;
	}
	
	// BIND
	if (bind(listenfd, (struct sockaddr*)&server_info, sizeof(server_info)) < 0){
		printf("ERROR: Bind\n");
		return EXIT_FAILURE;
	}
	
	// LISTEN
	if (listen(listenfd, 10) < 0) {
		printf("ERROR: Listen\n");
		return EXIT_FAILURE;
	}
	
	printf("--------------- CHAT SERVER is UP & RUNNING ---------------\n");
	
	// Accept connections 
	while(1) {
		socklen_t client_len = sizeof(client_info);
		connectfd = accept(listenfd, (struct sockaddr*)&client_info, &client_len);
		
		// check for Max clients 
		if (client_count + 1 == MAX_CLIENTS) {
			printf("Room Full: Try again later!!\n");
			print_ip_addr(client_info);
			close(connectfd);
			continue;
		}
		
		// client settings
		client_t *cli = (client_t *)malloc(sizeof(client_t)); // allocate new memory to every new client
		cli->address = client_info;                           
		cli->sockfd = connectfd;
		cli->uid = uid++;
		
		// Add client to queue
		add_to_queue(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli); // handle_client called 
		
		sleep(1);
		
	}	
	
	return EXIT_SUCCESS;
}
