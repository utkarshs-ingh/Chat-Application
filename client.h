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

#define LENGTH 2048

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];
char *ip = "127.0.0.1";


void catch_ctrl_c_and_exit(int sig) {

	flag = 1;

}

void str_overwrite_stdout() {

	printf("%s", ">>");
	fflush(stdout);

}

void str_trim_lf(char* arr, int length) {

	int i;
	for (i = 0; i < length; i++) { 
		if (arr[i] == '\n') {
			arr[i] = '\0';
			break;
		}
	}
}

void printWelcome(){

	printf("----------------------------- INSTRUCTIONS -----------------------------\n");
	printf("1. PRIVATE MESSAGE CAN BE SENT USING '@user message' FORMAT. \n");
	printf("2. BY DEFAULT ALL THE MESSAGES ARE SENT TO THE COMMON GROUP CHAT. \n");
	printf("3. ALL PRESENT MEMEBERS OF THE COMMON ROOM CAN BE FOUND USING '#who_all' message. \n");
	printf("\n----------------------------- WELCOME: %s -----------------------------\n", name);

}

int connect_to_Server(int port){

	struct sockaddr_in server_addr;

	/* Socket settings */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);


  // Connect to Server
	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (err == -1) {
			printf("CONNECTION ERROR!!!\n");
			return EXIT_FAILURE;
	}
	else {
		// Printing User manual and Welcome message
		printWelcome();
	}

	// Send name to the server
	send(sockfd, name, 32, 0);

}


void send_msg_handler() {

	char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};

	while(1) {
		str_overwrite_stdout();
		fgets(message, LENGTH, stdin);
		str_trim_lf(message, LENGTH);

		if (strcmp(message, "exit") == 0) {
				break;
		} 
		else if (strcmp(message, "#who_all") == 0) {
			send(sockfd, message, strlen(message), 0);
		} 
		else {
			sprintf(buffer, "%s: %s\n", name, message);
			send(sockfd, buffer, strlen(buffer), 0);
		}

		bzero(message, LENGTH);
		bzero(buffer, LENGTH + 32);
	}
	catch_ctrl_c_and_exit(2);

}

void recv_msg_handler() {

	char message[LENGTH] = {};
  	while (1) {
		int receive = recv(sockfd, message, LENGTH, 0);
		if (receive > 0) {
			printf("%s", message);
			str_overwrite_stdout();
		} 
		else if (receive == 0) {
				break;
		} 
		
		memset(message, 0, sizeof(message));
  	}

}
