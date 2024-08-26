#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

void printUDP(int port, char* sendline) {
    // Получение IP заголовка
    struct iphdr *iph = (struct iphdr *)(sendline);
    uint16_t iphdrlen = iph->ihl * 4;

    // Получение UDP заголовка
    struct udphdr *udph = (struct udphdr *)(sendline + iphdrlen);

    // Определение IP адресса
    struct in_addr sAddr, dAddr;
    sAddr.s_addr = iph->saddr;
    dAddr.s_addr = iph->daddr;

    if (ntohs(udph->dest) == port){
        printf("\n\n***********************UDP Packet start *************************\n");
        printf("Source IP Address      : %s\n", inet_ntoa(sAddr));
        printf("Destination IP Address : %s\n", inet_ntoa(dAddr));
        printf("Source Port            : %u\n", ntohs(udph->source));
        printf("Destination Port       : %u\n", ntohs(udph->dest));
        printf("UDP Length             : %u\n", ntohs(udph->len));
        printf("UDP Checksum           : %u\n", ntohs(udph->check));
        printf("\nData Payload\n");       

        int header_size = iphdrlen + sizeof(struct udphdr);
        int udp_data_length = ntohs(udph->len) - sizeof(struct udphdr); 
        unsigned char *data = (sendline + header_size);

        for(int i = 0; i < udp_data_length; i++) {
            if(i != 0 && i % 16 == 0) printf("\n");
            printf(" %.2X", data[i]);
        }
        printf("\n***********************UDP Packet end *************************");
        saveToFile(sendline);
    }
}


void saveToFile(char* sendline) {
    // Получение IP заголовка
    struct iphdr *iph = (struct iphdr *)(sendline);
    uint16_t iphdrlen = iph->ihl * 4;

    // Получение UDP заголовка
    struct udphdr *udph = (struct udphdr *)(sendline + iphdrlen);

    // Определение IP адресса
    struct in_addr sAddr, dAddr;
    sAddr.s_addr = iph->saddr;
    dAddr.s_addr = iph->daddr;

    FILE *logfile = fopen("udp_log.txt", "a");
    if (logfile == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fprintf(logfile, "\n\n***********************UDP Packet start *************************\n");
    fprintf(logfile, "Source IP Address      : %s\n", inet_ntoa(sAddr));
    fprintf(logfile, "Destination IP Address : %s\n", inet_ntoa(dAddr));
    fprintf(logfile, "Source Port            : %u\n", ntohs(udph->source));
    fprintf(logfile, "Destination Port       : %u\n", ntohs(udph->dest));
    fprintf(logfile, "UDP Length             : %u\n", ntohs(udph->len));
    fprintf(logfile, "UDP Checksum           : %u\n", ntohs(udph->check));

    int header_size = iphdrlen + sizeof(struct udphdr);
    int udp_data_length = ntohs(udph->len) - sizeof(struct udphdr); 
    unsigned char *data = (sendline + header_size);

    fprintf(logfile, "\nData Payload\n");
    for(int i = 0; i < udp_data_length; i++) {
        if (i != 0 && i % 16 == 0) fprintf(logfile, "\n");
        fprintf(logfile, " %.2X", data[i]);
    }
    fprintf(logfile, "\n***********************UDP Packet end *************************\n");

    fclose(logfile);
}

int main(int argc, char **argv) {
    if (argc != 2){
        printf("Укажите аргументы запуска <port>\n");
        exit(EXIT_FAILURE);
    }

    int sockfd; 
    int clilen, n; 
    unsigned char sendline[1000]; 
    struct sockaddr_in servaddr, cliaddr; 
    
    int port = atoi(argv[1]);
    if (port <= 0) {
        printf("Неверный порт\n");
        exit(EXIT_FAILURE);
    }

    /* Заполняем структуру для адреса сервера */
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* Создаем RAW сокет */
    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP)) < 0){
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
        if((n = recvfrom(sockfd, sendline, 999, 0, (struct sockaddr *) &cliaddr, &clilen)) < 0){
            perror("becvfrom");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printUDP(port, sendline);
    }

    close(sockfd);
    return 0;
}
