#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <limits.h>
#include <string.h>

#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <semaphore.h>

#define LEN 10

int sets_count = 0;

void sigIntParent(int sig) {
    printf("\nКоличество наборов = %d\n", sets_count);
    exit(EXIT_SUCCESS);
}

void sigIntSub(int sig) {
    exit(EXIT_SUCCESS);
}

int main() {
    sem_unlink("/posix_sem1");
    sem_t* sem_num = sem_open("/posix_sem1", O_CREAT, 0666, 0);
    if (sem_num == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    sem_unlink("/posix_sem2");
    sem_t* sem_find = sem_open("/posix_sem2", O_CREAT, 0666, 0);
    if (sem_find == SEM_FAILED) {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }

    int shared_mem = shm_open("/shared_mem", O_CREAT | O_RDWR, 0666);
    if (shared_mem == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shared_mem, sizeof(int) * (LEN + 3)) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE); 
    }

    srand(time(NULL));

    pid_t pid = fork();
    switch (pid) {
        case -1:
            perror("Fork");
            exit(EXIT_FAILURE);
        case 0: {
            signal(SIGINT, sigIntSub);
            while(1) {
                char* addr = mmap(0, sizeof(int) * (LEN + 3), PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem, 0);
                if (addr == MAP_FAILED) {
                    perror("mmap");
                    exit(EXIT_FAILURE);
                }

                if (sem_wait(sem_num) == -1) {
                    perror("sem_wait");
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

                if (sem_post(sem_find) == -1) {
                    perror("sem_post");
                    exit(EXIT_FAILURE);
                }

                sets_count++;
            }
            exit(EXIT_SUCCESS);
        }
        default: {
            signal(SIGINT, sigIntParent);
            while(1) {
                char* addr = mmap(0, sizeof(int) * (LEN + 3), PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem, 0);
                if (addr == MAP_FAILED) {
                    perror("mmap");
                    exit(EXIT_FAILURE);
                }
                
                int len = rand() % LEN + 2;
                memcpy(addr, &len, sizeof(len));
                for (int i = 0; i < len; i++) {
                    int num = rand() % 10;
                    memcpy(addr + sizeof(len) + i * sizeof(num), &num, sizeof(num));
                }
                puts("");
    
                if (sem_post(sem_num) == -1) {
                    perror("sem_post");
                    exit(EXIT_FAILURE);
                }

                if (sem_wait(sem_find) == -1) {
                    perror("sem_wait");
                    exit(EXIT_FAILURE);
                }

                int min, max;
                memcpy(&min, addr + sizeof(len) + len * sizeof(int), sizeof(min));
                memcpy(&max, addr + sizeof(len) + (len + 1) * sizeof(int), sizeof(max));
                printf("Pодителсикй процесс: min = %d, max = %d\n", min, max);

                sets_count++;
                sleep(1);
            }
            exit(EXIT_SUCCESS);
        }
    }
}