#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#define BUFSIZE 1024

// So gcc doesn't throw a tantrum
void process_exec(char** programs, int arrayLength, pid_t pids[], pid_t parent);
void token_parser(char* buffer, char*** arguments, int arrLength);
void parent_process(pid_t pids[], pid_t parent, int process_num);


static volatile sig_atomic_t reset_alarm_flag = 1;

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

// no need for this garbage
/*
void handler_sigusr1(int sig) { // Handler for SIGUSR_1 (this is straight out of stackoverflow ngl)
    if (sig == SIGUSR1) {
        fprintf(stdout, "Signal SIGUSR_1 recieved by process: %d.", getpid());
    }
    else {
        fprintf(stderr, "Error occured, wrong signal sent to SIGUSR_1 handler.\n");
        exit(EXIT_FAILURE);
    }
}
*/

// Do I need a handler for this?
void handler_alarm()
{
    reset_alarm_flag = 1;
}

void process_exec(char** programs, int arrayLength, pid_t pids[], pid_t parent)
{
    char** args = NULL;
    int numSpaces = 1;

    // Parent id
    printf("Parent Proccess ID: (%d), will have %d children proccesses.\n\n", parent, arrayLength);

    printf("\nSetting up SIGSET for child\n");
    printf("---------------------------\n");
    // Signal setup for SIGUSR1
    int sig;
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    if (sigismember(&sigset, SIGUSR1))
        printf("SIGUSR1 added as member of sigset\n");

    printf("---------------------------------------\n\n");

    for (int program = 0; program < arrayLength; program++)
    {
        // Count the number of spaces in a given line (this denotes number of tokens per line)
        for (unsigned long int j = 0; j < strlen(programs[program]); j++)
        {
            if (programs[program][j] == ' ')
                numSpaces++;
        }

        // create new child process
        pids[program] = fork();
        if (getpid() == parent)
            printf("In Parent process(%d), Child(%d) forked -> pids[%d].\n", getpid(), pids[program], program+1);

        // if an error occured when forking this program
        if (pids[program] < 0)
        {
            fprintf(stderr, "Error occured when forking process. ID: %d", pids[program]);
            exit(EXIT_FAILURE);
        }

        // if program is a child process
        if (pids[program] == 0)
        {
            // printf("\n\tEstablishing sigwait for Child[%d]: %d\n\n", program+1, getpid());

            if (sigwait(&sigset, &sig) < 0)
            {
                fprintf(stderr, "Error occured with sigwait in Child[%d]: (%d)\n", program+1, getpid());
                exit(EXIT_FAILURE);
            }

            if (sig == SIGUSR1)
            {
                printf("Child process[%d]: (%d) recieved SIGUSR1 signal.\n\n", program+1, getpid());
                printf("--Running Child[%d], with id: (%d).\n", program+1, getpid());
                token_parser(programs[program], &args, numSpaces);
                if (execvp(args[0], args) < 0)
                {
                    fprintf(stderr, "Error occured when running program from Child[%d]: (%d)\n\n", program+1, getpid());
                    exit(-1);
                }
                exit(-1);
            }
        }

        // reset number of items to tokenize
        numSpaces = 1;

        // free any malloc memory for next itteration
        free(args);
    }
}

void parent_process(pid_t pids[], pid_t parent, int process_num)
{
    printf("\nSetting up SIGSET for parent\n");
    printf("-------------------------------");
    // Signal setup for SIGUSR1
    int sig;
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    if (sigismember(&sigset, SIGALRM))
        printf("\nSIGALRM added as member of parent sigset.\n");

    // if this is the parent process
    if (getpid() == parent)
    {
        // * Send SIGUSR1 signal to child processes
        for (int i = 0; i < process_num; i++)
        {
            // send SIGUSR1 signal to the child process
            if (kill(pids[i], SIGUSR1) < 0) 
            {
                fprintf(stderr, "Error occured when signaling child process. (SIGUSR1)\n");
                exit(EXIT_FAILURE);
            }
            else
            {
                printf("\nSending SIGUSR1 to Child[%d]: (%d)\n\n", i+1, pids[i]);
                // alarm(1);
            }

            // Signaling child procs to stop...
            if (kill(pids[i], SIGSTOP) < 0)
            {
                fprintf(stderr, "Error occured when signaling child process. (SIGSTOP)\n");
                exit(EXIT_FAILURE);
            }
            else
            {
                printf("\tStopping Child[%d]: (%d).\n\n", i+1, pids[i]);
            }
        }

        kill(pids[0], SIGCONT);

        int childStatus[process_num];
        for (int i = 0; i < process_num; i++)
            childStatus[i] = 0;

        int numDone = 0;
        int status;
        int currentProc = 0;
        pid_t waitPid;
        while (1)
        {
            if (childStatus[currentProc] == 0) {
                alarm(1);

                if (sigwait(&sigset, &sig) < 0) {
                    fprintf(stderr, "Error occured with sigwait in Parent: (%d)\n", getpid());
                    exit(EXIT_FAILURE);
                }
                else {
                    printf("\n\nSignaling parent process to wait. current process: %d\n\n", currentProc+1);
                }
            }
            
            // if (reset_alarm_flag == 1)
            // {
            if (childStatus[currentProc] != 1) {
                if (kill(pids[currentProc], SIGSTOP) < 0) {
                    fprintf(stderr, "Error occured when signaling child process (in Scheduler). (SIGSTOP)\n");
                    exit(EXIT_FAILURE);
                }
                else {
                    printf("--Scheduler Stopping Child[%d]: (%d).\n\n", currentProc+1, pids[currentProc]);
                }
            }
            reset_alarm_flag = 0;
            if (currentProc+1 >= process_num) {
                if (childStatus[0] != 1) {
                    if (kill(pids[0], SIGCONT) < 0) {
                        fprintf(stderr, "Error occured when signaling child process (in Scheduler). (SIGCONT):1\n");
                        exit(EXIT_FAILURE);
                    }
                    else {
                        printf("--Scheduler continuing Child[%d]: (%d).\n\n", 1, pids[0]);
                    }
                }
            }
            else {
                if (childStatus[currentProc+1] != 1) {
                    if (kill(pids[currentProc+1], SIGCONT) < 0) {
                        fprintf(stderr, "Error occured when signaling child process (in Scheduler). (SIGCONT):2\n");
                        exit(EXIT_FAILURE);
                    }
                    else {
                        printf("--Scheduler continuing Child[%d]: (%d).\n\n", currentProc+2, pids[currentProc+1]);
                    }
                }
            }
            // }

            // Check if any procs are done this itteration of while(1)
            for (int i = 0; i < process_num; i++) {
                if (childStatus[i] != 1) {
                    waitPid = waitpid(pids[i], &status, WNOHANG | WUNTRACED | WCONTINUED);
                }
                else {
                    continue;
                }
                if (waitPid == -1) {
                    // printf("Status: %d\n", WIFEXITED(status));
                    fprintf(stderr, "Error occured when using waitpid() on Child[%d]: (%d)\n", i+1, pids[i]);
                    exit(EXIT_FAILURE);
                }
                if (WIFEXITED(status) > 0) {
                    childStatus[i] = WIFEXITED(status);
                }
            }

            // Reset which process we are looking at to the first process
            // if we had just looked at the last process
            if (currentProc+1 >= process_num)
                currentProc = 0;
            else
                currentProc++;

            // Check children
            for (int i = 0; i < process_num; i++)
            {
                printf("Child process[%d]: %d, has status: %d\n", i+1, pids[i], childStatus[i]);
                if (childStatus[i] == 1)
                    numDone++;
            }

            // printf("NumDone: %d\n\n", numDone);

            if (numDone >= process_num)
                break;
            else
                numDone = 0;
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

    pid_t parentID = getpid();

    printf("Registering Signal handler for SIGALRM\n\n");
    signal(SIGALRM, handler_alarm);

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
    pid_t pids[len];

    process_exec(processes, len, pids, parentID);

    parent_process(pids, parentID, len);

    free(processes);

    printf("\n\n");
    printf("Done.\n");

    return 0;
}