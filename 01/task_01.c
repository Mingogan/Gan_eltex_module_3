#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void areaCalculation(int *sides_arr, int start, int end) {
    for (int i = start; i < end; i++) {
        int area = sides_arr[i] * sides_arr[i];
        printf("\tДля стороны = %d вычисленная площадь = %d\n", sides_arr[i], area);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Укажите в аргументах запуска значение сторон квадратов\n");
        exit(EXIT_FAILURE);
    }

    int wt;
    int sides_number = argc - 1; 
    int sides_arr[sides_number];
    
    for (int i = 0; i < sides_number; i++) {
        sides_arr[i] = atoi(argv[i + 1]);
    }

    int mid = sides_number / 2;

    pid_t pid = fork();
    switch(pid){
        case -1:
            perror("fork");
            exit(EXIT_FAILURE);
        case 0:
            printf("Вычисляет дочерний:\n");
            areaCalculation(sides_arr, 0, mid);
            exit(EXIT_SUCCESS);
        default:
            printf("Вычисляет родительский:\n");
            areaCalculation(sides_arr, mid, sides_number);
            wait(&wt);
            exit(EXIT_SUCCESS);
    }
    
    return 0;
}
