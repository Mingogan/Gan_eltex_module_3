#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

int wait_sig = 0;

void fileForbidden(int sig) {
    wait_sig = 1;
}

void fileАllowed(int sig) {
    wait_sig = 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Укажите количество чисел\n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));
    signal(SIGUSR1, fileForbidden);
    signal(SIGUSR2, fileАllowed);

    int file_create = creat("result.txt", 0777);
    close(file_create);

    int pipefd[2];
    int amount = atoi(argv[1]);

    if (pipe(pipefd) == -1) {
        perror("Pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    switch (pid) {
        case -1:
            perror("Fork");
            exit(EXIT_FAILURE);
            break;
        case 0:
            close(pipefd[0]);
            for (int i = 0; i < amount+1; i++) {
                while (wait_sig) {}

                int file = open("result.txt", O_RDONLY);
                if (file == -1) {
                    perror("Ошибка открытия файла");
                    exit(EXIT_FAILURE);
                }
                

                printf("\nСодержимое файла:\n");
                int code;
                do {
                char c;
                code = read(file, &c, sizeof(c));
                if (code == -1) {
                    perror("Ошибка чтения файла");
                    exit(EXIT_FAILURE);
                }
                printf("%c", c);
                } while (code);

                close(file);

                int num = rand() % 10;
                write(pipefd[1], &num, sizeof(int));
                kill(getpid(), SIGUSR1);
                pause();
            }
            close(pipefd[1]);
            exit(EXIT_SUCCESS);
        default:
            close(pipefd[1]);
            for (int i = 0; i < amount; i++) {

                int num;
                read(pipefd[0], &num, sizeof(int));
                printf("Число от родителя: %d\n", num);

                kill(0, SIGUSR1);

                int file = open("result.txt", O_WRONLY | O_APPEND);
                if (file == -1) {
                    perror("Ошибка открытия файла");
                    exit(EXIT_FAILURE);
                }

                char c = num + '0';
                write(file, &c, 1);
                write(file, "\n", 1);
                close(file);
                kill(0, SIGUSR2);
                
            }
            close(pipefd[0]);
            exit(EXIT_SUCCESS);
    }
}