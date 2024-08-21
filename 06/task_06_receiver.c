#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>

#define MY_TYPE 2  
#define OTHER_TYPE 1  
#define EXIT_TYPE 255 

typedef struct msgbuf {
    long mtype;  
    char mtext[100];  
} msgbuf;

int main() {
    key_t key = ftok(".", '#');
    if (key == -1) {
        perror("Ошибка при генерации ключа");
        exit(EXIT_FAILURE);
    }

    int id = msgget(key, IPC_CREAT | 0660);
    if (id == -1) {
        perror("Ошибка при создании или открытии очереди сообщений");
        exit(EXIT_FAILURE);
    }

    int chat_activity = 1;  
    while (chat_activity) {
        printf("1 - Отправить сообщение пользователю с типом %d\n", OTHER_TYPE);
        printf("2 - Проверить новые сообщения от пользователя с типом %d\n", OTHER_TYPE);
        printf("3 - Выход\n\n");

        int action;
        printf("Выберите номер действия: ");
        scanf("%d", &action);
        getchar();  

        switch (action) {
            case 1:{
                msgbuf buf;
                buf.mtype = MY_TYPE;  
                printf("\nВведите сообщение: ");
                fgets(buf.mtext, sizeof(buf.mtext), stdin);
                buf.mtext[strcspn(buf.mtext, "\n")] = '\0'; 
  
                if (msgsnd(id, &buf, sizeof(buf.mtext), 0) == -1) {
                    perror("Ошибка отправки сообщения");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case 2:{
                msgbuf buf;
                printf("\nНовые сообщения:\n");
                while (msgrcv(id, &buf, sizeof(buf.mtext), 0, IPC_NOWAIT) != -1) {
                    if (buf.mtype == EXIT_TYPE) { 
                        printf("Получен сигнал о завершении обмена, обмен завершен.");
                        chat_activity = 0;  
                    } else if (buf.mtype == OTHER_TYPE) {
                        printf("Сообщение от пользователя %ld: %s\n", buf.mtype, buf.mtext);
                    }
                }
                puts("\n");

                break;
            }
            case 3:{
                msgbuf buf;
                buf.mtype = EXIT_TYPE; 
            
                if (msgsnd(id, &buf, sizeof(buf.mtext), 0) == -1) {
                    perror("Ошибка отправки сообщения");
                    exit(EXIT_FAILURE);
                }
                
                chat_activity = 0;  
                break;
            }
        }
    }

    return 0;
}