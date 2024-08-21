#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/sem.h>
#include <sys/ipc.h>

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *_buf;
};

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
    
    //Задание 09
    key_t key = ftok(".", '#');
    int semid = semget(key, 2, 0666 | IPC_CREAT);
    struct sembuf read_lock = {0, -1, 0};
    struct sembuf read_unlock = {0, 1, 0};
    struct sembuf write_lock = {1, -1, 0};
    struct sembuf write_unlock[2] = {{1, 0, 0}, {1, 1, 0}};
    union semun ar;

    ar.val = 5;
    if (semctl(semid, 0, SETVAL, ar) == -1) {
        perror("Read semaphore create");
        exit(EXIT_FAILURE);
    }

    ar.val = 1;
    if (semctl(semid, 1, SETVAL, ar) == -1) {
        perror("Write semaphore create");
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

                if (semop(semid, &read_lock, 1) == -1) { 
                    perror("semop");
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
                
                usleep(100000);
                if (semop(semid, &read_unlock, 1) == -1) { 
                    perror("semop");
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

                if (semop(semid, &write_lock, 1) == -1) { 
                    perror("semop");
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
                
                usleep(1000);
                if (semop(semid, write_unlock, 2) == -1) { 
                    perror("semop");
                    exit(EXIT_FAILURE);
                }
            }
            close(pipefd[0]);
            exit(EXIT_SUCCESS);
    }
    semctl(semid, 0, IPC_RMID);
    return 0;
}
