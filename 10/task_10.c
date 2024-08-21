#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <semaphore.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Укажите количество чисел\n");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

    int file_create = creat("result.txt", 0777);
    close(file_create);

    int pipefd[2];
    int amount = atoi(argv[1]);

    if (pipe(pipefd) == -1) {
        perror("Pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    //Задание 10
    sem_t* sem = sem_open("/posix_sem", O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("Ошибка создания семафора");
        exit(EXIT_FAILURE);
    }

    switch (pid) {
        case -1:
            perror("Fork");
            exit(EXIT_FAILURE);
            break;
        case 0:
            close(pipefd[0]);
            for (int i = 0; i < amount+1; i++) {
                if (sem_wait(sem) == -1) { 
                    perror("sem_wait");
                    exit(EXIT_FAILURE);
                }

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
                putchar(c);
                } while (code);

                close(file);       
                 
                if (sem_post(sem) == -1) { 
                    perror("sem_post");
                    exit(EXIT_FAILURE);
                }

                int num = rand() % 10;
                write(pipefd[1], &num, sizeof(int));
            }
            close(pipefd[1]);
            exit(EXIT_SUCCESS);
        default:
            close(pipefd[1]);
            for (int i = 0; i < amount; i++) {

                int num;
                read(pipefd[0], &num, sizeof(int));
                printf("Число от родителя: %d\n", num);

                if (sem_wait(sem) == -1) {
                    perror("sem_wait");
                    exit(EXIT_FAILURE);
                }

                int file = open("result.txt", O_WRONLY | O_APPEND);
                if (file == -1) {
                    perror("Ошибка открытия файла");
                    exit(EXIT_FAILURE);
                }

                char c = num + '0';
                write(file, &c, 1);
                write(file, "\n", 1);
                close(file);

                if (sem_post(sem) == -1) { 
                    perror("sem_post");
                    exit(EXIT_FAILURE);
                }
            }
            close(pipefd[0]);
            if (sem_unlink("/posix_sem") == -1) {
                perror("Ошибка удаления семафора");
                exit(EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
    }
}