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
    if (argc != 2){
        printf("Укажите аргументы запуска <port>\n");
        exit(EXIT_FAILURE);
    }

    int sockfd; 
    int clilen, n; 
    char sendline[1000]; 
    struct sockaddr_in servaddr, cliaddr; 
    
    int port = atoi(argv[1]);
    if (port == 0) {
        printf("Неверный порт\n");
        close(sockfd); 
        exit(EXIT_FAILURE);
    }

    /* Заполняем структуру для адреса сервера */
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Создаем UDP сокет */
    if ((sockfd = socket(PF_INET, SOCK_DGRAM, 0)) < 0){
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Настраиваем адрес сокета */
    if (bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while(1) {
        clilen = sizeof(cliaddr);

        /* Ожидаем прихода запроса от клиента и читаем его */
        if ((n = recvfrom(sockfd, sendline, 999, 0, (struct sockaddr *) &cliaddr, &clilen)) < 0){
            perror("recvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if (!strcmp(sendline, "exit\n")) break;

        printf("Получено сообщение: %s", sendline);
        printf("Введи сообщение => ");
        fgets(sendline, 1000, stdin);

        /* Принятый текст отправляем обратно по адресу отправителя */
        if (sendto(sockfd, sendline, strlen(sendline)+1, 0, (struct sockaddr *) &cliaddr, clilen) < 0){
            perror("sendto");
            close(sockfd);
            exit(EXIT_FAILURE);
        } 

        if (!strcmp(sendline, "exit\n")) break;
    }
    
    printf("Чат завершен\n");
    close(sockfd);
    exit(EXIT_SUCCESS);
}