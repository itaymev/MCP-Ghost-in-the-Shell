#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

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

void process_exec(char** programs, int arrayLength) {
    pid_t pids[arrayLength];
    char** args = NULL;
    int numSpaces = 1;

    printf("Parent process: %d\n",getpid());

    for (int currProc = 0; currProc < arrayLength; currProc++) {

        // Count the number of spaces in a given line
        for (unsigned long int j = 0; j < strlen(programs[currProc]); j++)
        {
            if (programs[currProc][j] == ' ')
                numSpaces++;
        }

        pids[currProc] = fork();

        if (pids[currProc] < 0) {
            fprintf(stderr, "Error. encountered an error when forking process. ID: %d", pids[currProc]);
            exit(EXIT_FAILURE);
        }

        if (pids[currProc] == 0) {
            printf("Child process: %d, process id: %d\n", currProc+1, getpid());
            token_parser(programs[currProc], &args, numSpaces);
            if (execvp(args[0], args) < 0) {
                fprintf(stderr, "Error. an error occured when running program from Child[%d]: (%d)\n\n", currProc+1, getpid());
                free(args);
                exit(-1);
            }
            free(args);
            exit(-1);
        }

        for (int i = 0; i < arrayLength; i++) {
            wait(&pids[i]);
        }

        // reset number of items to tokenize
        numSpaces = 1;
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
            fprintf(stderr, "Error occured due to invalid file");
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