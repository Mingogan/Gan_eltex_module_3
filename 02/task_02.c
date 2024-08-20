#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {

    setenv("PATH", ".", 1);

    while (1) {
        printf("\nДоступные команды:\n ");
        printf("\tСуммирование - sum:\n");
        printf("\tПоиск максимального числа - max:\n");
        printf("\tСклеивание строк- concat:\n");
        printf("\tДля выхода - exit:\n");
        printf("Введите команду c аргументами: ");

        int wt;
        char str_input[1000];
        char *args[10];
        char *token;
        int arg_count = 0;

        if (fgets(str_input, sizeof(str_input), stdin) == NULL) {
            printf("Ошибка ввода\n");
            continue;
        }
        
        str_input[strcspn(str_input, "\n")] = 0;

        token = strtok(str_input, " ");
        while (token != NULL && arg_count < 10) {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;  

        if (arg_count == 0) {
            continue;  
        }

        if (strcmp(args[0], "exit") == 0) {
            break;  
        }

        pid_t pid = fork();
        switch(pid){
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
            case 0:
                if (execvp(args[0], args) < 0) {
                    perror("Ошибка при выполнении команды");
                    exit(EXIT_FAILURE);
                }
                exit(EXIT_SUCCESS);
            default:
                wait(&wt);
        }
    }
}