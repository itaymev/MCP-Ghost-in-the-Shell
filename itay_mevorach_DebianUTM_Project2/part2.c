#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define _POSIX_C_SOURCE 1 // Enables use of sigset_t data type
#define BUFSIZE 1024

// So gcc doesn't throw a tantrum
void process_exec(char** programs, int arrayLength);
void token_parser(char* buffer, char*** arguments, int arrLength);

void token_parser(char* buffer, char *** arguments, int arrLength) {
    // This is basically project 1 but I made it more specific to our current use case
    char** resultArgs;
    char* save = buffer;
    
    for (unsigned int i = 0; i < strlen(buffer); i++) {
        if (buffer[i] == '\n') {
            buffer[i] = '\0';
        }
    }
    
    resultArgs = (char**)malloc(sizeof(char*) * arrLength + 1);

    char* currToken = strtok_r(save, " ", &save);

    int i = 0;
    while (currToken != NULL) {
        resultArgs[i] = currToken;
        i++;
        currToken = strtok_r(save, " ", &save);
    }

    resultArgs[arrLength] = NULL;

    *arguments = resultArgs;
    free(resultArgs);
}

void handler_sigusr1(int sig) { // Handler for SIGUSR_1 (this is straight out of stackoverflow ngl)
    if (sig == SIGUSR1) {
        fprintf(stdout, "Signal SIGUSR_1 recieved by process: %d.", getpid());
    }
    else {
        fprintf(stderr, "Error occured, wrong signal sent to SIGUSR_1 handler.\n");
        exit(EXIT_FAILURE);
    }
}

void process_exec(char** programs, int arrayLength) {
    pid_t pids[arrayLength];
    char** args = NULL;
    int numSpaces = 1;

    // Parent process ID
    int parentID = getpid();
    printf("Parent Proccess ID: (%d), will have %d children proccesses\n\n", parentID, arrayLength);

    printf("Setting up SIGSET");
    printf("\n-----------------\n");
    // Signal setup for SIGUSR_1
    int sig;
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    if (sigismember(&sigset, SIGUSR1))
        printf("SIGUSR1 added to SIGSET\n");

    printf("\n-----------------\n");

    for (int program = 0; program < arrayLength; program++)
    {
        // Count the number of tokens per line
        for (unsigned long int j = 0; j < strlen(programs[program]); j++)
        {
            if (programs[program][j] == ' ')
                numSpaces++;
        }

        // create new process (child)
        pids[program] = fork();
        if (getpid() == parentID) {
            printf("In parent process(%d)... Child(%d) forked -> pids[%d].\n\n", getpid(), pids[program], program+1);
        }

        // if an error occured when forking this program
        if (pids[program] < 0) {
            fprintf(stderr, "Error occured when forking process. ID: %d", pids[program]);
            exit(EXIT_FAILURE);
        }

        // if process is a child process
        if (pids[program] == 0) {
            printf("Establishing sigwait for Child[%d]: %d\n\n", program+1, getpid());

            if (sigwait(&sigset, &sig) < 0) {
                fprintf(stderr, "Error occured with sigwait\n");
                exit(EXIT_FAILURE);
            }

            if (sig == SIGUSR1) {
                printf("--Running child process[%d], process id: (%d)\n", program+1, getpid());
                token_parser(programs[program], &args, numSpaces);
                if (execvp(args[0], args) < 0) {
                    fprintf(stderr, "Error occured when executing child process[%d]: (%d)\n\n", program+1, getpid());
                    free(args);
                    exit(-1);
                }
                free(args);
                exit(-1);
            }
        }

        // if process isn't child process then it is a parent process and we do nothing

        // reset number of items to tokenize
        numSpaces = 1;
    }

    // if this is the parent process
    if (getpid() == parentID) {
        // * Send SIGUSR1 signal to child processes
        for (int i = 0; i < arrayLength; i++) {
            // send SIGUSR1 signal to the child process
            printf("\nSending SIGUSR1 to Child[%d]: %d\n\n", i+1, pids[i]);
            if (kill(pids[i], SIGUSR1) < 0) {
                fprintf(stderr, "Error occured when signaling child process. (SIGUSR1)\n");
                exit(EXIT_FAILURE);
            }
            else {
                printf("Child[%d]: (%d), should be recieving SIGUSR1 signal.\n\n", i+1, pids[i]);
            }

            // Signaling child procs to stop...
            if (kill(pids[i], SIGSTOP) < 0) {
                fprintf(stderr, "Error occured when signaling child process. (SIGSTOP)\n");
                exit(EXIT_FAILURE);
            }
            else {
                printf("--Stopping Child[%d]: (%d).\n\n", i+1, pids[i]);
            }

            // Signaling child procs to continue...
            if (kill(pids[i], SIGCONT) < 0) {
                fprintf(stderr, "Error occured when signaling child process. (SIGCONT)\n");
                exit(EXIT_FAILURE);
            }
            else {
                printf("--Continuing Child[%d]: (%d).\n\n", i+1, pids[i]);
            }

            // Signaling parent to wait for this child process before continuing
            int status;
            wait(&status);
        }
    }
}


int main (int argc, char** argv) {
    printf("MCP TIME\n\n\n");

    // Catch any program usage errors
    if (2 > argc || argc > 2) {  // If the program does not have an input arguement
        fprintf(stderr, "Error.  Invalid usage of program.\n");
        fprintf(stderr, "--Usage: ./part<#> <filename>\n");
        exit(EXIT_FAILURE);
    }

    else { // check if file exists
        FILE* exists = fopen(argv[1], "r");
        if (exists == NULL)
        {
            fprintf(stderr, "Error occured due to invalid file \n");
            exit(EXIT_FAILURE);
        }
    }

    FILE* input = fopen(argv[1], "r");
    int len = 0;
    unsigned long int size;
    char** processes = (char**)malloc(sizeof(char*) * BUFSIZE);

    for (unsigned long int i = 0; i < BUFSIZE; i++)
        processes[i] = NULL;

    while (getline(&(processes[len]), &size, input) != -1)
    {
        len++;
    }

    fclose(input);
    process_exec(processes, len);
    free(processes);

    printf("\n\n");
    printf("Done.\n");

    return 0;
}