#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void send_file(int sock, char* name) {
    char buf[1000];
    FILE *file = fopen(name, "r");
    if (!file) error("fopen");

    // Чтение и отправка данных из файла
    while (fgets(buf, sizeof(buf), file)) {
        if (send(sock, buf, strlen(buf), 0) == -1)
            error("send");
        bzero(buf, sizeof(buf));
    }

    // Отправка строки завершения
    if (send(sock, "EOF", 4, 0) == -1)  // Отправляем "EOF" плюс '\0'
        error("send eof");

    fclose(file);
}

int main(int argc, char *argv[]) {
    int my_sock, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    printf("TCP DEMO CLIENT\n");
    printf("To exit enter the commands \"quit\" or \"send <filename>\"\n");

    if (argc < 3) {
        fprintf(stderr, "usage %s hostname port\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Извлечение порта
    portno = atoi(argv[2]);

    // Шаг 1 - создание сокета
    my_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (my_sock < 0) error("ERROR opening socket");

    // Извлечение хоста
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        exit(EXIT_FAILURE);
    }

    // Заполнение структуры serv_addr
    bzero((char*)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char*)server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);

    // Установка порта
    serv_addr.sin_port = htons(portno);

    // Шаг 2 - установка соединения
    if (connect(my_sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    // Шаг 3 - чтение и передача сообщений
    while (1) {
        char buff[1024];
        // Получаем сообщение от сервера
        n = recv(my_sock, buff, sizeof(buff) - 1, 0);
        if (n == 0) break;
    
        buff[n] = '\0'; 
        printf("Received message from server: %s", buff);
        
        // Читаем пользовательский ввод с клавиатуры
        printf("Send messages to the server: ");
        fgets(buff, sizeof(buff) - 1, stdin);
        buff[strcspn(buff, "\n")] = '\0'; 

        // Отправляем сообщение серверу
        send(my_sock, buff, strlen(buff), 0);

        if (strcmp(buff, "send") == 0) {
            // Ожидание подтверждения от сервера
            n = recv(my_sock, buff, sizeof(buff) - 1, 0);
            if (n <= 0) break;
            buff[n] = '\0'; 
            printf("Received message from server: %s", buff);

            // Получаем имя файла для отправки
            char filename[256];
            printf("Enter filename to send: ");
            if (fgets(filename, sizeof(filename) - 1, stdin) == NULL) {
                perror("fgets");
                break;
            }
            filename[strcspn(filename, "\n")] = '\0'; // Удаление символа новой строки
            if (send(my_sock, filename, strlen(filename), 0) == -1) {
                error("ERROR");
            }
            send_file(my_sock, filename);
        }else if (strcmp(buff, "quit") == 0) {
            break;
        }
    }

    printf("Connection closed\n");
    close(my_sock);
    exit(EXIT_SUCCESS);
}
