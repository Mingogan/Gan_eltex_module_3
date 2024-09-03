#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int nclients = 0; // количество активных пользователей

void printusers() {
    if (nclients) {
        printf("%d user(s) on-line\n", nclients);
    } else {
        printf("No User on-line\n");
    }
}

int myfunc(int a, int b, char s) {
    switch (s) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return a / b;
        default: return 0;
    }
}

struct client_state {
    int sock;
    int param1;
    int param2;
    char operation;
    enum { WAITING_FOR_COMMAND, WAITING_FOR_FIRST_PARAM, WAITING_FOR_SECOND_PARAM, WAITING_FOR_OPERATION } state;
};

void set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    struct epoll_event ev, events[MAX_EVENTS];

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(EXIT_FAILURE);
    }

    // Шаг 1 - создание сокета
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    // Шаг 2 - связывание сокета с локальным адресом
    bzero((char *)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    // Шаг 3 - ожидание подключений, размер очереди - 5
    listen(sockfd, 5);

    // Создаем объект epoll
    int epollfd = epoll_create1(0);
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = sockfd;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);

    // Использование разделяемой памяти для хранения количества активных пользователей
    int shared_mem = shm_open("/shared_mem", O_CREAT | O_RDWR, 0660);
    if (shared_mem == -1) {
        error("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shared_mem, sizeof(int)) == -1) {
        error("ftruncate");
        exit(EXIT_FAILURE);
    }

    char *addr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem, 0);
    memcpy(addr, &nclients, sizeof(nclients));

    #define instructions "Commands:\n 1. Perform a mathematical operation with command 'operation'\n 2. Exit with command 'quit'\n"

    printf("Server started on port %d\n", portno);

    // Шаг 4 - главный цикл сервера
    while (1) {
        int nfds, n;
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);

        for (n = 0; n < nfds; n++) {
            if (events[n].data.fd == sockfd) {
                // Новое входящее соединение
                clilen = sizeof(cli_addr);
                newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                if (newsockfd < 0) error("ERROR on accept");
                set_nonblock(newsockfd);

                memcpy(&nclients, addr, sizeof(nclients));
                nclients++;
                memcpy(addr, &nclients, sizeof(nclients));

                struct client_state *new_client = malloc(sizeof(struct client_state));
                new_client->sock = newsockfd;
                new_client->state = WAITING_FOR_COMMAND;

                ev.events = EPOLLIN | EPOLLET;
                ev.data.ptr = new_client;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, newsockfd, &ev);

                printf("New connection from %s on socket %d\n", inet_ntoa(cli_addr.sin_addr), newsockfd);
                printusers();

                // Отправляем начальное сообщение новому клиенту
                write(newsockfd, instructions, strlen(instructions));
            } else {
                dostuff((struct client_state *)events[n].data.ptr, shared_mem);
                // Удаляем сокет из epoll, если клиент отключился
                epoll_ctl(epollfd, EPOLL_CTL_DEL, events[n].data.fd, NULL);
            }
        }
    }

    close(sockfd);
    return 0;
}

void dostuff(struct client_state *client, int shared_mem) {
    int bytes_recv;
    char buff[20 * 1024];
    char *addr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem, 0);

    #define str1 "Enter 1 parameter\n"
    #define str2 "Enter 2 parameter\n"
    #define str3 "Enter one of the operations \"+\", \"*\", \"-\", \"/\"\n"

    memset(buff, 0, sizeof(buff));
    bytes_recv = read(client->sock, buff, sizeof(buff) - 1);
    buff[bytes_recv] = '\0'; 

    if (strcmp(buff, "quit") == 0) {
        client->state = WAITING_FOR_COMMAND;
        memcpy(&nclients, addr, sizeof(nclients));
        nclients--;
        memcpy(addr, &nclients, sizeof(nclients));
        close(client->sock);
        printf("-disconnect\n");
        printusers();
        return;
    }

    switch (client->state) {
        case WAITING_FOR_COMMAND:
            if (strcmp(buff, "operation") == 0) {
                write(client->sock, str1, strlen(str1));
                client->state = WAITING_FOR_FIRST_PARAM;
            } else {
                const char *error_msg = "Unknown command, use 'operation', or 'quit'.\n";
                write(client->sock, error_msg, strlen(error_msg));
                printf("Unknown command received\n");
            }
            break;
        case WAITING_FOR_FIRST_PARAM:
            client->param1 = atoi(buff);
            write(client->sock, str2, strlen(str2));
            client->state = WAITING_FOR_SECOND_PARAM;
            break;
        case WAITING_FOR_SECOND_PARAM:
            client->param2 = atoi(buff);
            write(client->sock, str3, strlen(str3));
            client->state = WAITING_FOR_OPERATION;
            break;
        case WAITING_FOR_OPERATION:
            client->operation = buff[0];
            int result = myfunc(client->param1, client->param2, client->operation);
            snprintf(buff, sizeof(buff), "%d\n", result);
            write(client->sock, buff, strlen(buff));
            client->state = WAITING_FOR_COMMAND;
            break;
    }
    return;
}