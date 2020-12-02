#include "client.h"

int main(int argc, char **argv){
	if(argc != 2){
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int port = atoi(argv[1]);

	signal(SIGINT, catch_ctrl_c_and_exit);

	printf("USERNAME: ");
	fgets(name, 32, stdin);
	str_trim_lf(name, strlen(name));


	if (strlen(name) > 31 || strlen(name) < 2){
		printf("Username must be less than 32 and more than 2 characters.\n");
		return EXIT_FAILURE;
	}

	// Establish connection with Server
	connect_to_Server(port);

	// calling send message handler
	pthread_t send_msg_thread;
	if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}

	// calling recieve message handler
	pthread_t recv_msg_thread;
    	if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0){
		printf("ERROR: pthread\n");
		return EXIT_FAILURE;
	}
	
	// Closing the Connection
	while (1){
		if(flag){
			printf("----------------------------- GOOD BYE: %s -----------------------------\n", name);
			break;
		}
	}

	close(sockfd);

	return EXIT_SUCCESS;
}
