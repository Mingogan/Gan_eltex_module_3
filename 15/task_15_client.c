#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int my_sock, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    printf("TCP DEMO CLIENT\n");
    printf("To exit enter the commands \"quit\"\n");

    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // извлечение порта
    portno = atoi(argv[2]);

    // Шаг 1 - создание сокета
    my_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (my_sock < 0) error("ERROR opening socket");

    // извлечение хоста
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    // заполнение структуры serv_addr
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char *)&serv_addr.sin_addr.s_addr,server->h_length);

    // установка порта
    serv_addr.sin_port = htons(portno);

    // Шаг 2 - установка соединения
    if (connect(my_sock, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    // Шаг 3 - чтение и передача сообщений
    while (1) {
        char buff[1024];
        // получаем сообщение от сервера
        n = recv(my_sock, &buff[0], sizeof(buff) - 1, 0);
        if (n == 0) break;
    
        // выводим на экран
        printf("Received message from server: %s", buff);

        // читаем пользовательский ввод с клавиатуры
        printf("Send messages to the server:"); 
        fgets(&buff[0], sizeof(buff) - 1, stdin);

        // передаем строку клиента серверу
        send(my_sock, &buff[0], strlen(&buff[0]), 0);
        if (!strcmp(&buff[0], "quit\n")) {
            printf("Exit...\n");
            close(my_sock);
            exit(EXIT_SUCCESS);
        }
    }
    printf("Recv error\n");
    close(my_sock);
    exit(EXIT_FAILURE);
}