#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Укажите аргументы запуска\n");
        exit(EXIT_FAILURE);
    }

    int sum = 0;
    for (int i = 1; i < argc; i++) {
        sum += atoi(argv[i]);
    }

    printf("Сумма: %d\n", sum);
    exit(EXIT_SUCCESS);
}
