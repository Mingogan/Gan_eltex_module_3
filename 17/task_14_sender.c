#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc != 3){
        printf("Укажите аргументы запуска <IP> <port>\n");
        exit(EXIT_FAILURE);
    }

    int sockfd; 
    int n, len; 
    char sendline[1000]; 
    struct sockaddr_in servaddr, cliaddr; 
    
    /* Создаем UDP сокет */
    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket"); 
        exit(EXIT_FAILURE);
    }

    /* Заполняем структуру для адреса клиента */
    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons(0);
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Настраиваем адрес сокета */
    if (bind(sockfd, (struct sockaddr *) &cliaddr, sizeof(cliaddr)) < 0){
        perror("bind");
        close(sockfd); 
        exit(EXIT_FAILURE);
    }

    /* Заполняем структуру для адреса сервера */
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    int port = atoi(argv[2]);
    if (port == 0) {
        printf("Неверный порт\n");
        close(sockfd); 
        exit(EXIT_FAILURE);
    }
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (inet_aton(argv[1], &servaddr.sin_addr) == 0) {
        printf("Неверный IP\n");
        close(sockfd); /* По окончании работы закрываем дескриптор сокета */
        exit(EXIT_FAILURE);
    }

    while (1) {
        printf("Введи сообщение => ");
        fgets(sendline, 1000, stdin);
        
        /* Отсылаем датаграмму */
        if (sendto(sockfd, sendline, strlen(sendline)+1, 0, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
            perror("sendto");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if (!strcmp(sendline, "exit\n")) break;

        /* Ожидаем ответа и читаем его */
        if ((n = recvfrom(sockfd, sendline, 1000, 0, (struct sockaddr *) NULL, NULL)) < 0){
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if (!strcmp(sendline, "exit\n")) break;
        printf("Получено сообщение: %s", sendline);
    }

    printf("Чат завершен\n");
    close(sockfd);
    exit(EXIT_SUCCESS);
}