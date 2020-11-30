#include "server.h"


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
	logs("Server started at => ");
	// root node intialization
	root = newNode(client_info, connectfd, 10);
	now = root;

	// Accept connections
	while(1) {

		socklen_t client_len = sizeof(client_info);
		connectfd = accept(listenfd, (struct sockaddr*)&client_info, &client_len);

		// Add client to List
		client_t *cli = newNode(client_info, connectfd, ++uid);
		cli->prev = now;
		now->link = cli;
		now = cli;

		pthread_create(&tid, NULL, &handle_client, (void*)cli); // handle_client called

		sleep(1);

	}

	return EXIT_SUCCESS;
}
