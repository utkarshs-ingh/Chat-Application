
typedef struct ClientNode{

	struct sockaddr_in address;
	struct ClientNode *prev;
	struct ClientNode *link;
	int sockfd;
	int uid;
	char name[32];

} client_t;

// Linked List created
client_t *newNode(struct sockaddr_in clientinfo, int connect_fd, int u_id) {

	client_t *cli = (client_t *)malloc(sizeof(client_t));
	cli->prev = NULL;
	cli->link = NULL;
	strncpy(cli->name, "NULL", 5);

	cli->address = clientinfo;                          
	cli->sockfd = connect_fd;
	cli->uid = u_id;
	return cli;

}

client_t *root, *now; // root and current node pointers