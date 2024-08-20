#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Укажите количество чисел\n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

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
        case 0:
            close(pipefd[0]);
            for (int i = 0; i < amount; i++) {
                int num = rand() % 10;
                write(pipefd[1], &num, sizeof(int));
            }
            close(pipefd[1]);
            exit(EXIT_SUCCESS);
        default:
            close(pipefd[1]);
            int num[amount];
            for (int i = 0; i < amount; i++) {
                read(pipefd[0], &num[i], sizeof(int));
                printf("№%d - %d\n",i, num[i]);
            }
            close(pipefd[0]);
            
            int file = open("result.txt", O_WRONLY | O_CREAT | O_TRUNC, 0777);
            if (file == -1) {
                perror("Ошибка открыти файла");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < amount; i++) {
                char c = num[i] + '0';
                write(file, &c, 1);
                write(file, "\n", 1);
            }

            close(file);
            exit(EXIT_SUCCESS);
    }
}
