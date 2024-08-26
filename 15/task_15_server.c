#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <fcntl.h>
#include <sys/mman.h>

// функция обслуживания подключившихся пользователей
void dostuff(int, int);

// функция обработки ошибок
void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int nclients = 0; // количество активных пользователей

// печать количества активных пользователей
void printusers() {
    if (nclients) {
        printf("%d user on-line\n", nclients);
    }
    else {
        printf("No User on-line\n");
    }
}

// функция обработки данных
int myfunc(int a, int b, char s) {
    switch(s) {
        case '+':
            return a + b;
        case '-':
            return a - b;
        case '*':
            return a * b;
        case '/':
            return a / b;
        default:
            return 0;
    }
}

int main(int argc, char *argv[]) {
    char buff[1024]; // Буфер для различных нужд
    int sockfd, newsockfd; // дескрипторы сокетов
    int portno; // номер порта
    int pid; // id номер потока
    socklen_t clilen; // размер адреса клиента типа socklen_t
    struct sockaddr_in serv_addr, cli_addr; // структура сокета сервера и клиента
    
    // ошибка в случае если мы не указали порт
    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        exit(EXIT_FAILURE);
    }
    
    // Шаг 1 - создание сокета
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("ERROR opening socket");
    
    // Шаг 2 - связывание сокета с локальным адресом
    bzero((char*) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY; // сервер принимает подключения на все IP-адреса
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR on binding");
    
    // Шаг 3 - ожидание подключений, размер очереди - 5
    listen(sockfd, 5);
    clilen = sizeof(cli_addr);
    
    // Использование разделяемой памяти для хранения количества активных пользователей
    int shared_mem = shm_open("/shared_mem", O_CREAT | O_RDWR, 0660);
    if (shared_mem == -1){
        error("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shared_mem, sizeof(int)) == -1){
        error("ftruncate");
        exit(EXIT_FAILURE);
    }

    char* addr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem, 0);
    memcpy(addr, &nclients, sizeof(nclients));

    // Шаг 4 - извлекаем сообщение из очереди (цикл извлечения запросов на подключение)
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0) error("ERROR on accept");
        memcpy(&nclients, addr, sizeof(nclients));
        nclients++;
        memcpy(addr, &nclients, sizeof(nclients));

        // вывод сведений о клиенте
        struct hostent *hst;
        hst = gethostbyaddr((char *)&cli_addr.sin_addr, 4, AF_INET);
        printf("+%s [%s] new connect!\n", (hst) ? hst->h_name : "Unknown host", 
         (char*)inet_ntoa(cli_addr.sin_addr));
        printusers();

        pid = fork();
        if (pid < 0) error("ERROR on fork");
        if (pid == 0) {
            close(sockfd);
            dostuff(newsockfd, shared_mem);
            exit(EXIT_SUCCESS);
        }
        else close(newsockfd);
    }
    close(sockfd);
    exit(EXIT_SUCCESS);
}

void dostuff (int sock, int shared_mem) {
    int bytes_recv; // размер принятого сообщения
    int a,b; // переменные для myfunc
    char buff[20 * 1024];
    char* addr = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem, 0);
    
    #define str1 "Enter 1 parameter\n"
    #define str2 "Enter 2 parameter\n"
    #define str3 "Enter one of the operations \"+\", \"*\", \"-\", \"/\"\n"

    // отправляем клиенту сообщение
    write(sock, str1, sizeof(str1));
    
    // обработка первого параметра
    bytes_recv = read(sock, &buff[0], sizeof(buff));
    if (bytes_recv < 0) error("ERROR reading from socket");
    if (!strcmp(buff, "quit\n")){
        memcpy(&nclients, addr, sizeof(nclients));
        nclients--; // уменьшаем счетчик активных клиентов
        memcpy(addr, &nclients, sizeof(nclients));
        
        close(sock);
        printf("-disconnect\n");
        printusers();
        return;
    };
    a = atoi(buff); // преобразование первого параметра в int

    // отправляем клиенту сообщение
    write(sock, str2, sizeof(str2));

    // обработка второго параметра
    bytes_recv = read(sock, &buff[0], sizeof(buff));
    if (bytes_recv < 0) error("ERROR reading from socket");
    if (!strcmp(buff, "quit\n")){
        memcpy(&nclients, addr, sizeof(nclients));
        nclients--; // уменьшаем счетчик активных клиентов
        memcpy(addr, &nclients, sizeof(nclients));
        
        close(sock);
        printf("-disconnect\n");
        printusers();
        return;
    };
    b = atoi(buff); // преобразование второго параметра в int

    // отправляем клиенту сообщение
    write(sock, str3, sizeof(str3));

    // обработка знака
    bytes_recv = read(sock, &buff[0], sizeof(buff));
    if (bytes_recv < 0) error("ERROR reading from socket");
    if (!strcmp(buff, "quit\n")){
        memcpy(&nclients, addr, sizeof(nclients));
        nclients--; // уменьшаем счетчик активных клиентов
        memcpy(addr, &nclients, sizeof(nclients));
        
        close(sock);
        printf("-disconnect\n");
        printusers();
        return;
    };

    a = myfunc(a, b, buff[0]); // вызов пользовательской функции
    sprintf(buff, "%d", a); // преобразование результата в строку
    buff[strlen(buff)] = '\n'; // добавление к сообщению символа конца строки

    // отправляем клиенту результат
    write(sock, &buff[0], strlen(buff) + 1);
    
    memcpy(&nclients, addr, sizeof(nclients));
    nclients--; // уменьшаем счетчик активных клиентов
    memcpy(addr, &nclients, sizeof(nclients));
    
    close(sock);
    printf("-disconnect\n");
    printusers();
    return;
}