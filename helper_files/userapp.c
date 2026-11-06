#include "userapp.h"
#include <stdio.h>
#include <stdlib.h>
// Additional headers as needed


// Register process with kernel module
void register_process(unsigned int pid)
{
    FILE *status_file = fopen(PROC_STATUS_PATH, "w");

    if (status_file == NULL) {
        perror(PROC_STATUS_PATH);
        exit(EXIT_FAILURE);
    }

    fprintf(status_file, "%u\n", pid);

    // The write reaches the module when the stream is flushed here
    if (fclose(status_file) != 0) {
        perror(PROC_STATUS_PATH);
        exit(EXIT_FAILURE);
    }
}

// Print the registered processes and their CPU times to the screen
void print_status(void)
{
    FILE *status_file = fopen(PROC_STATUS_PATH, "r");
    int c;

    if (status_file == NULL) {
        perror(PROC_STATUS_PATH);
        exit(EXIT_FAILURE);
    }

    while ((c = fgetc(status_file)) != EOF) {
        putchar(c);
    }

    fclose(status_file);
}

// Compute a sum of squares so the process accumulates userspace CPU time
void compute(void)
{
    volatile unsigned long sum = 0;
    unsigned long i;

    for (i = 0; i < 100000; ++i) {
        sum += i * i;
    }
}

int main(int argc, char* argv[])
{
    int __expire = 10;
    time_t start_time = time(NULL);

    if (argc == 2) {
        __expire = atoi(argv[1]);
    }

    register_process(getpid());

    // Terminate user application if the time has expired
    while (1) {
        if ((int)(time(NULL) - start_time) > __expire) {
            break;
        }
        compute();
    }

    print_status();

	return 0;
}
