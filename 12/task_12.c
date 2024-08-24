#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define LEN 10

int sets_count = 0;

void sigIntParent(int sig) {
    printf("\nКоличество наборов = %d\n", sets_count);
    exit(EXIT_SUCCESS);
}

void sigIntSub(int sig) {
    exit(EXIT_SUCCESS);
}

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *_buf;
};

int main() {
    //key_t key = ftok(".", '#');
    int semid = semget(IPC_PRIVATE, 2, IPC_CREAT | 0666);
    struct sembuf lock_num = {0, -1, 0}; 
    struct sembuf unlock_num[2] = {{0, 0, 0},{0, 1, 0}};
    struct sembuf lock_find = {1, -1, 0}; 
    struct sembuf unlock_find[2] = {{1, 0, 0},{1, 1, 0}};

    union semun arg;
    arg.val = 0;

    if (semctl(semid, 0, SETVAL, arg) == -1) {
        perror("semctl 1");
        exit(EXIT_FAILURE);
    }

    if (semctl(semid, 1, SETVAL, arg) == -1) {
        perror("semctl 2");
        exit(EXIT_FAILURE);
    }

    int shared_mem = shmget(IPC_PRIVATE, sizeof(int) * (LEN + 3), IPC_CREAT | 0666);
    if (shared_mem == -1) {
        perror("shmget");
        exit(EXIT_FAILURE);
    }

    srand(time(NULL));

    pid_t pid = fork();
    switch (pid) {
        case -1: 
            perror("Fork");
            exit(EXIT_FAILURE);
        case 0:{
            signal(SIGINT, sigIntSub);
            while (1) {
                if (semop(semid, &lock_num, 1) == -1) {
                    perror("semop");
                    exit(EXIT_FAILURE);
                }

                char* addr = shmat(shared_mem, 0, 0);
                if (addr == (char*)-1) {
                    perror("shmat");
                    exit(EXIT_FAILURE);
                }

                int len, min = INT_MAX, max = INT_MIN;
                memcpy(&len, addr, sizeof(len));

                printf("Дочерний процесс: ");
                for (int i = 0; i < len; i++) {
                    int num;
                    memcpy(&num, addr + sizeof(len) + i * sizeof(num), sizeof(num));
                    if (num < min) min = num;
                    if (num > max) max = num;
                    printf("%d ", num);
                }
                puts("");

                memcpy(addr + sizeof(len) + len * sizeof(int), &min, sizeof(min));
                memcpy(addr + sizeof(len) + (len + 1) * sizeof(int), &max, sizeof(max));

                if (shmdt(addr) == -1) {
                    perror("shmdt");
                    exit(EXIT_FAILURE);
                }

                if (semop(semid, unlock_find, 2) == -1) {
                    perror("semop");
                    exit(EXIT_FAILURE);
                }

                sets_count++;
            }
        }
        default:{
            signal(SIGINT, sigIntParent);
            while (1) {
                char* addr = shmat(shared_mem, 0, 0);
                if (addr == (char*)-1) {
                    perror("shmat");
                    exit(EXIT_FAILURE);
                }

                int len = rand() % LEN + 2;
                memcpy(addr, &len, sizeof(len));
                for (int i = 0; i < len; i++) {
                    int num = rand() % 10;
                    memcpy(addr + sizeof(len) + i * sizeof(num), &num, sizeof(num));
                }
                puts("");

                if (semop(semid, unlock_num, 2) == -1) {
                    perror("semop");
                    exit(EXIT_FAILURE);
                }

                if (semop(semid, &lock_find, 1) == -1) {
                    perror("semop");
                    exit(EXIT_FAILURE);
                }

                int min, max;
                memcpy(&min, addr + sizeof(len) + len * sizeof(int), sizeof(min));
                memcpy(&max, addr + sizeof(len) + (len + 1) * sizeof(int), sizeof(max));
                printf("Pодителсикй процесс: min = %d, max = %d\n", min, max);

                if (shmdt(addr) == -1) {
                    perror("shmdt");
                    exit(EXIT_FAILURE);
                }
                sets_count++;
                sleep(1);
            }
        }   
    }
    return 0;
}

