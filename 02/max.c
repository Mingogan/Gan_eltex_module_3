#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Укажите аргументы запуска\n");
        exit(EXIT_FAILURE);
    }

    int max = atoi(argv[1]);
    for (int i = 2; i < argc; i++) {
        if (max < atoi(argv[i]))
            max = atoi(argv[i]);
    }

    printf("Максимальное число: %d\n", max);
    exit(EXIT_SUCCESS);
}
