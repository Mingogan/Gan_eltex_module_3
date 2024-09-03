all: sender receiver

sender: task_18_server.c
	gcc -Wall -o server task_18_server.c

receiver: task_18_client.c
	gcc -Wall -o client task_18_client.c

clean:
	server client