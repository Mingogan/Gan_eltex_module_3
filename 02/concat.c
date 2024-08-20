#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Укажите аргументы запуска\n");
        exit(EXIT_FAILURE);
    }

    char result[1024] = "";
    for (int i = 1; i < argc; i++) {
        strcat(result, argv[i]);
    }

    printf("Итоговая строка: %s\n", result);
    exit(EXIT_SUCCESS);
}
