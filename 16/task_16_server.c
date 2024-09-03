#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/mman.h>

// функция обработки ошибок
void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

// функция обработки математических операций
int myfunc(int a, int b, char s) {
    switch (s) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return a / b;
        default: return 0;
    }
}

void recv_file(int sock, char* name) {
    FILE* file = fopen(name, "w");
    if (!file) {
        perror("ERROR opening file");
        return;
    }
    
    char buf[1000];
    int n;

    while ((n = recv(sock, buf, sizeof(buf) - 1, 0)) > 0) {
        // Поиск строки "EOF" в полученных данных
        char *eof = strstr(buf, "EOF");
        if (eof != NULL) {
            *eof = '\0';  // Обрезаем строку на позиции "EOF"
            fprintf(file, "%s", buf);  // Сохраняем данные до "EOF"
            break;
        }
        fprintf(file, "%s", buf);
        bzero(buf, sizeof(buf));
    }
    fclose(file);
}

int nclients = 0; // количество активных пользователей

// печать количества активных пользователей
void printusers() {
    if (nclients) {
        printf("%d user(s) on-line\n", nclients);
    } else {
        printf("No User on-line\n");
    }
}

// Функция обработки данных
void dostuff(int sock, int shared_mem) {
    int bytes_recv;
    int a, b;
    char buff[1024];
    char *addr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem, 0);

    // Отправляем инструкции клиенту
    const char *instructions = "Commands:\n"
                               "1. Send a file with command 'send'\n"
                               "2. Perform a mathematical operation with command 'operation'\n"
                               "3. Exit with command 'quit'\n";
    write(sock, instructions, strlen(instructions));

    while (1) {
    	memset(buff, 0, sizeof(buff));
        bytes_recv = read(sock, &buff[0], sizeof(buff) - 1);
        if (bytes_recv <= 0) break;

        buff[strcspn(buff, "\n")] = '\0';

        if (strcmp(buff, "send") == 0) {
            const char *msg = "Please send filename\n";
            write(sock, msg, strlen(msg));

            // Получаем имя файла
            char filename[256];
            bytes_recv = read(sock, filename, sizeof(filename) - 1);
            if (bytes_recv <= 0) break;
            filename[bytes_recv] = '\0';
            filename[strcspn(filename, "\n")] = '\0'; // Удаление символа новой строки

            printf("Receiving file: %s\n", filename);
            recv_file(sock, filename);
            printf("File reception completed.\n");
            msg = "File send successfully\n";
            write(sock, msg, strlen(msg));
        }else if (strcmp(buff, "quit") == 0) {
            break;
        } else if (strcmp(buff, "operation") == 0) {
            // Отправляем запрос на первый параметр
            write(sock, "Enter 1 parameter\n", 19);
            memset(buff, 0, sizeof(buff));
            bytes_recv = read(sock, &buff[0], sizeof(buff) - 1);
            if (bytes_recv <= 0) break;
            a = atoi(buff); // первый параметр

            // Отправляем запрос на второй параметр
            write(sock, "Enter 2 parameter\n", 19);
            memset(buff, 0, sizeof(buff));
            bytes_recv = read(sock, &buff[0], sizeof(buff) - 1);
            if (bytes_recv <= 0) break;
            b = atoi(buff); // второй параметр

            // Отправляем запрос на операцию
            write(sock, "Enter operation: \"+\", \"-\", \"*\", \"/\"\n", 39);
            memset(buff, 0, sizeof(buff));
            bytes_recv = read(sock, &buff[0], sizeof(buff) - 1);
            if (bytes_recv <= 0) break;
            char operation = buff[0];

            int result = myfunc(a, b, operation);
            sprintf(buff, "%d\n", result);
            write(sock, buff, strlen(buff));
        } else {
            const char *error_msg = "Unknown command, use 'send <filename>', 'operation', or 'quit'.\n";
            write(sock, error_msg, strlen(error_msg));
            memset(buff, 0, sizeof(buff));
        }
    }

    memcpy(&nclients, addr, sizeof(nclients));
    nclients--; // уменьшаем счетчик активных клиентов
    memcpy(addr, &nclients, sizeof(nclients));
    close(sock);
    printf("-disconnect\n");
    printusers();
}

int main(int argc, char *argv[]) {
    char buff[1024];
    int sockfd, newsockfd;
    int portno;
    int pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(EXIT_FAILURE);
    }

    // Шаг 1 - создание сокета
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");

    // Шаг 2 - связывание сокета с локальным адресом
    bzero((char*)&serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");

    // Шаг 3 - ожидание подключений, размер очереди - 5
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

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

    char* addr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem, 0);
    memcpy(addr, &nclients, sizeof(nclients));

    // Шаг 4 - извлечение сообщений из очереди (цикл извлечения запросов на подключение)
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (newsockfd < 0) error("ERROR on accept");
        memcpy(&nclients, addr, sizeof(nclients));
        nclients++;
        memcpy(addr, &nclients, sizeof(nclients));

        // вывод сведений о клиенте
        struct hostent *hst;
        hst = gethostbyaddr((char*)&cli_addr.sin_addr, 4, AF_INET);
        printf("+%s [%s] new connect!\n", (hst) ? hst->h_name : "Unknown host", (char*)inet_ntoa(cli_addr.sin_addr));
        printusers();

        pid = fork();
        if (pid < 0) error("ERROR on fork");
        if (pid == 0) {
            close(sockfd);
            dostuff(newsockfd, shared_mem);
            exit(EXIT_SUCCESS);
        } else {
            close(newsockfd);
        }
    }

    close(sockfd);
    exit(EXIT_SUCCESS);
}
