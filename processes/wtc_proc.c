#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <strings.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char** argv) {
    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL) {
        perror("Could not open file");
        exit(EXIT_FAILURE);
    }

    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    int p, n;

    read = getline(&line, &len, fp);
    p = atoi(line);
    read = getline(&line, &len, fp);
    n = atoi(line);

    size_t graph_len = n*n;

    key_t shmkey = ftok("/dev/null", 5);
    int shmid = shmget(shmkey, graph_len * sizeof(int), 0644 | IPC_CREAT);
    if (shmid < 0) {
        perror("Could not create shared memory");
        exit(EXIT_FAILURE);
    }

    int* g = (int*)shmat(shmid, NULL, 0);
    if ((long int)g == -1) {
        perror("Could not attach shared memory");
        exit(EXIT_FAILURE);
    }
    bzero(g, graph_len*sizeof(int));

    while ((read = getline(&line, &len, fp)) != -1) {
        int i = line[0] - '0';
        int j = line[2] - '0';
        g[j*n + i] = 1;
    }

    free(line);

    for (int k = 0; k < n; k++) {
        int work_size = n/p;
        int extra = n % p;
        for (int processes = 0; processes < p; processes++) {
            int work_start = processes * work_size;
            if (fork() == 0) {
                for (int i = work_start; i < work_start + work_size; i++) {
                    for (int j = 0; j < n; j++) {
                        g[j*n + i] = g[j*n + i] || (g[k*n + i] && g[j*n + k]);
                    }
                }
                exit(EXIT_SUCCESS);
            }
        }
        for (int processes = 0; processes < p; processes++) {
            wait(0);
        }
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (g[j*n + i] == 1) {
                printf("%d %d\n", i, j);
            }
        }
    }

    return 0;
}
