all: 
	gcc -pthread -o server server.c
	gcc -pthread -o client client.c
