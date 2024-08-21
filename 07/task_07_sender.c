#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define QUEUE_NAME1 "/queue11"  
#define QUEUE_NAME2 "/queue21"  
#define MAX_SIZE 128


void error_and_exit(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main() {
    mqd_t mq1, mq2;
    struct mq_attr attr;
    char buf[MAX_SIZE];

    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MAX_SIZE;
    attr.mq_curmsgs = 0;

    mq1 = mq_open(QUEUE_NAME1, O_CREAT | O_WRONLY, 0660, &attr); // отправка
    if (mq1 == (mqd_t) -1) {
        perror("Ошибка создания/открытия  очереди 1");
        exit(EXIT_FAILURE);
    }

    mq2 = mq_open(QUEUE_NAME2, O_CREAT | O_RDONLY, 0660, &attr); // получение
    if (mq2 == (mqd_t) -1) {
        perror("Ошибка создания/открытия  очереди 2");
        exit(EXIT_FAILURE);
    }

    int chat_activity = 1;
    while (chat_activity) {
        printf("\n1 - Отправить сообщение\n");
        printf("2 - Проверить новые сообщения\n");
        printf("3 - Выход\n\n");

        int action;
        printf("Выберите номер действия: ");
        scanf("%d", &action);
        getchar(); 

        switch (action) {
            case 1: {
                printf("Введите сообщение: ");
                fgets(buf, MAX_SIZE, stdin);
                buf[strcspn(buf, "\n")] = '\0';  

                if (mq_send(mq1, buf, sizeof(buf), 0) == -1) {
                    perror("Ошибка отправки сообщения");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 2: {
                ssize_t bytes_read;
                mq_getattr(mq2, &attr);
                if (attr.mq_curmsgs == 0){
                    printf("Нет новых сообщений\n\n");
                    break;
                }

                printf("\nНовые сообщения:\n");
                while (attr.mq_curmsgs > 0) {
                    bytes_read = mq_receive(mq2, buf, sizeof(buf), NULL);
                    if (bytes_read >= 0) {
                        buf[bytes_read] = '\0';  
                        if (strcmp(buf, "exit") == 0) {
                            printf("Получен сигнал о завершении обмена, обмен завершен.\n");
                            chat_activity = 0;
                            break;
                        } else {
                            printf("Сообщение: %s\n", buf);
                        }
                    }
                    mq_getattr(mq2, &attr);
                }
                break;
            }
            case 3: {
                if (mq_send(mq1, "exit", MAX_SIZE, 0) == -1) {
                    perror("Ошибка отправки сообщения");
                    exit(EXIT_FAILURE);
                }
                chat_activity = 0;
                break;
            }
            default:
                printf("Неверный выбор, попробуйте снова.\n");
                break;
        }
    }

    if (mq_close(mq1) == -1) {
        perror("Ошибка закрытия очереди 1");
        exit(EXIT_FAILURE);
    }

    if (mq_close(mq2) == -1) {
        perror("Ошибка закрытия очереди 2");
        exit(EXIT_FAILURE);
    }

    if (mq_unlink(QUEUE_NAME2) == -1) {
        perror("Ошибка при удалении очереди 2");
        exit(EXIT_FAILURE);
    }

    return 0;
}
