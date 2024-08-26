all: sender receiver

sender: task_15_server.c
	gcc -Wall -o server task_15_server.c

receiver: task_15_client.c
	gcc -Wall -o client task_15_client.c

clean:
	server client