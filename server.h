#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<signal.h>
#include<pthread.h>
#include<sys/types.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<time.h>
#include<errno.h>
#include<string.h>
#include "clientStructure.h"

#define MAX_CLIENTS 100
#define BUFFER_SIZE 2048
#define name_length 32


static int uid = 10;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;


void logs(char *buffer) {
	int BUFF_SIZE = strlen(buffer);
  	char log_time[50] = {}, log_buffer[100] = {};
  	
	time_t t = time(NULL);
  	struct tm tm = *localtime(&t);

  	sprintf(log_time," ::: %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  	strcpy(log_buffer, buffer);
  	strcat(log_buffer, log_time);
  	
	FILE *filePointer;
	filePointer = fopen("logs.txt", "a");
	fputs(log_buffer, filePointer);
	fputs("\n", filePointer);
	fclose(filePointer);
}

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


int FindReciever(char *buffer) {  
	int i;
    for(i= 0; i< strlen(buffer); i++){
        if(buffer[i] == '@') break;
    }
    return i;
}

void getUsers(char *s, client_t *cli) {
	pthread_mutex_lock(&clients_mutex);

	client_t *tmp = root->link;	
		
	char all_users[3000] = {}, users[3000] = {}; 
	
	while(tmp != NULL) {
		char names[32] = {};
		sprintf(names, "%s ", tmp->name);
		strcat(users, names);
		tmp = tmp->link;
	}
	sprintf(all_users, "%s\n", users);
	logs(all_users);
	if(write(cli->sockfd, all_users, strlen(all_users)) < 0) {
		logs("ERROR: write to the discriptor failed!!\n");
	}
	
	pthread_mutex_unlock(&clients_mutex);
}

// send messages to clients: 1. send in group; 2. send to specified client

void send_group_message(char *s, client_t *cli) {
	pthread_mutex_lock(&clients_mutex);

	client_t *tmp = root->link;

	while(tmp != NULL) {
		if(cli->uid != tmp->uid) {
			if(write(tmp->sockfd, s, strlen(s)) < 0) {
				logs("ERROR: write to the discriptor failed!!\n");
				break;
			}
		}
		tmp = tmp->link;
	}

	pthread_mutex_unlock(&clients_mutex);

}

void send_private_message(char *s, client_t *cli, char *reciever) {

	pthread_mutex_lock(&clients_mutex);

	str_trim(reciever, strlen(reciever));
	int check = 0;
	client_t *sender;
	client_t *tmp = root->link;

	while(tmp != NULL) {
		if(tmp->uid == cli->uid) sender = tmp;
		if(strcmp(tmp->name, reciever) == 0) { //only to reciever
			check = 1;
			if(write(tmp->sockfd, s, strlen(s)) < 0) {
				logs("ERROR: write to the discriptor failed!!\n");
				break;
			}
		}
		tmp = tmp->link;
	}

	if(check != 1) { //user not found
		char buff[100];
		sprintf(buff, "ERROR: seems like %s is not here!!!\n", reciever);
		logs(buff);
		if(write(sender->sockfd, buff, strlen(buff)) < 0) {
			logs("ERROR: write to the discriptor failed!!\n");
		}
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

// Message Handler Function

void *handle_client(void *arg) {

	char buffer[BUFFER_SIZE]; // store client joining and sent messages temporarily
	char name[name_length];
	int leave_flag = 0;

	
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
		logs(buffer);
		send_group_message(buffer, cli); // sent client joining info
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
			if(strcmp(buffer, "#who_all") == 0) {
				getUsers(buffer, cli);
			}
			else if(strlen(buffer) > 0) {
				int i = FindReciever(buffer); // check for @ in the message
					if(i < strlen(buffer) && buffer[i-1] == ' ') {
						i = i + 1;
						int j = 0;
						char reciever[32];

						while(buffer[i] != ' ' && i < strlen(buffer)) {

							reciever[j] = buffer[i];
							j++;
							i++;
						}
						reciever[j] = '\0';
						send_private_message(buffer, cli, reciever); //send private msg
					}

					else {
						send_group_message(buffer, cli); // send group message
					}

					str_trim(buffer, strlen(buffer));
					printf("%s\n", buffer);
					logs(buffer);
			}
			
		}

		else if(recieve == 0 || strcmp(buffer, "exit") == 0) {
			sprintf(buffer, "%s has left the chat\n", cli->name);
			printf("%s", buffer);
			logs(buffer);
			send_group_message(buffer, cli);
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

	if (cli == now) { // remove an edge node
		now = cli->prev;
		now->link = NULL;
	}

	else { // remove a middle node
		cli->prev->link = cli->link;
		cli->link->prev = cli->prev;
	}

	free(cli);

	pthread_detach(pthread_self());

	return NULL;
}
