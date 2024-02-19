#include "linux/limits.h"
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "LineParser.h"
#define STDIN 0
#define STDOUT 1
#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0
#define HISTLEN 20

char *history[HISTLEN];
int newest = 0;
int oldest = 0;
int count = 0;

void add_history(char *line)
{
    // Allocate space for the command and make a copy
    char *cmd = (char *)malloc(strlen(line) + 1);
    strcpy(cmd, line);

    if (count == HISTLEN)
    {
        free(history[oldest]);
        oldest = (oldest + 1) % HISTLEN;
    }
    else
    {
        count++;
    }
    // Add the command to the history
    history[newest] = cmd;
    newest = (newest + 1) % HISTLEN;
}

void print_history()
{
    int i, j;
    j = oldest;
    for (i = 0; i < count; i++)
    {
        printf("%d: %s\n", i + 1, history[j]);
        j = (j + 1) % HISTLEN;
    }
}
typedef struct process
{
    cmdLine *cmd;         /* the parsed command line*/
    pid_t pid;            /* the process id that is
               running the command*/
    int status;           /* status of the process:
              RUNNING/SUSPENDED/TERMINATED */
    struct process *next; /* next process in chain */
} process;

process *process_list = NULL;

/*assitant functions*/
// deletes all TERMINATED processs from process list
void deleteTerminatedProcess(process **process_list)
{
    if (process_list != NULL)
    {
        process *tempProcess = *process_list;
        process *previous = NULL;
        while (tempProcess != NULL)
        {
            if (tempProcess->status == TERMINATED)
            {
                if (previous == NULL)
                {
                    // deletes the first process in the list
                    *process_list = tempProcess->next;
                    freeCmdLines(tempProcess->cmd);
                    free(tempProcess);
                    tempProcess = *process_list;
                }
                else
                {
                    // deletes a process that isnt the first in the list
                    previous->next = tempProcess->next;
                    freeCmdLines(tempProcess->cmd);
                    free(tempProcess);
                    tempProcess = previous->next;
                }
            }
            else
            {
                previous = tempProcess;
                tempProcess = tempProcess->next;
            }
        }
    }
}

void addProcess(process **process_list, cmdLine *cmd, pid_t pid)
{
    process *newProcess = malloc(sizeof(process));
    newProcess->cmd = cmd;
    newProcess->pid = pid;
    newProcess->next = NULL;
    newProcess->status = RUNNING;
    if (*process_list != NULL)
    {
        // get to the end of the process list
        process *endList = *process_list;
        while (endList->next != NULL)
            endList = endList->next;
        endList->next = newProcess;
    }
    else // create a new list with the process given in the arguments
    {
        *process_list = newProcess;
    }
}

void updateProcessStatus(process *process_list, int pid, int status)
{
    if (process_list != NULL)
    {
        process *tempProcess = process_list;
        while (tempProcess != NULL)
        {
            if (tempProcess->pid == pid)
            {
                tempProcess->status = status;
                break;
            }
            tempProcess = tempProcess->next;
        }
    }
}

void free_history() {
    int i;
    for (i = 0; i < HISTLEN; i++) {
        if (history[i] != NULL) {
            free(history[i]);
            history[i] = NULL;
        }
    }
}


void updateProcessList(process **process_list)
{
    int stat = 0;
    process *temp = *process_list;
    while (temp != NULL)
    {
        int returnID = waitpid(temp->pid, &stat, WNOHANG | WUNTRACED | WCONTINUED);
        if (WIFCONTINUED(stat)) // checks if the process changed its state to RUNNING
        {
            updateProcessStatus(temp, temp->pid, RUNNING);
            break;
        }
        else
        {
            if (WIFSTOPPED(stat)) // checks if the process changed its state to SUSPENDED
            {
                updateProcessStatus(temp, temp->pid, SUSPENDED);
                break;
            }
            else
            {
                if (returnID != 0) // checks if the process changed its state to TERMINATED
                    updateProcessStatus(temp, temp->pid, TERMINATED);
            }
        }
        temp = temp->next;
    }
}

void printProcessList(process **process_list)
{
    updateProcessList(process_list);
    if (process_list != NULL)
    {
        printf("PID\tCommand\t\tSTATUS\n");
        process *processToPrint = *process_list; // get the first process in the list
        while (processToPrint != NULL)
        {
            printf("%d\t", processToPrint->pid);
            printf("%s\t", processToPrint->cmd->arguments[0]);
            if (processToPrint->status == -1)
            {
                printf("Teriminated\n");
            }
            else if (processToPrint->status == 0)
            {
                printf("Suspended\n");
            }
            else
            {
                printf("Running\n");
            }
            processToPrint = processToPrint->next;
        }
        deleteTerminatedProcess(process_list);
    }
}

void freeProcessList(process *process_list)
{
    while (process_list != NULL)
    {
        process *temp = process_list;
        process_list = process_list->next;
        freeCmdLines(temp->cmd);
        free(temp);
    }
}

/*End assitant functions*/

int execute(cmdLine *currLine)
{
    int procId = fork();
    int fileDIn, fileDOut;
    if (procId == 0)
    {

        /* edit redirect */
        if (currLine->inputRedirect != NULL)
        {
            fileDIn = open(currLine->inputRedirect, O_RDONLY);
            close(STDIN);
            dup(fileDIn);
            close(fileDIn);
        }
        if (currLine->outputRedirect != NULL)
        {
            fileDOut = creat(currLine->outputRedirect, 0644);
            close(STDOUT);
            dup(fileDOut);
            close(fileDOut);
        }

        /* execute cmdLine */
        if (execvp(currLine->arguments[0], currLine->arguments) < 0)
        {
            perror("error");
            exit(1);
        }
    }
    else
    {
        if (currLine->blocking != 0)
        {
            waitpid(procId, 0, 0);
        }
    }
    return procId;
}

void pipeCommands(char *command1, char *command2, int isDebug)
{
    int myPipe[2];
    int fileDIn, fileDOut;
    cmdLine *firstCommand = parseCmdLines(command1);
    cmdLine *secCommand = parseCmdLines(command2);
    int pid1, pid2;
    if (pipe(myPipe) < 0)
    {
        perror("not success pipe");
        _exit(1);
    }

    if (isDebug == 1)
    {
        fprintf(stderr, "(parent_process>forking...)\n");
    }
    pid1 = fork();

    /* child 1 */
    if (pid1 == 0)
    {
        close(STDOUT); /* close standart output*/
        if (isDebug == 1)
        {
            fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe...)\n");
        }
        dup(myPipe[1]);
        close(myPipe[1]);

        /*parse first command and redirect it */
        if (firstCommand->inputRedirect != NULL)
        {
            fileDIn = open(firstCommand->inputRedirect, O_RDONLY);
            close(STDIN);
            dup(fileDIn);
            close(fileDIn);
        }
        if (firstCommand->outputRedirect != NULL)
        {
            fileDOut = creat(firstCommand->outputRedirect, 0644);
            close(STDOUT);
            dup(fileDOut);
            close(fileDOut);
        }
        if (isDebug == 1)
        {
            fprintf(stderr, "(child1>going to execute cmd: %s)\n", command1);
        }

        /* execute command 1 */
        if (execvp(firstCommand->arguments[0], firstCommand->arguments) < 0)
        {
            perror("error");
            exit(1);
        }
    }

    /* father and child2 */
    else
    {
        if (isDebug == 1)
        {
            fprintf(stderr, "(parent_process>created process with id: %d)\n", pid1);
            fprintf(stderr, "(parent_process>waiting for child processes to terminate...)\n");
        }

        waitpid(pid1, NULL, 0);
        close(myPipe[1]); /* close write end */
        if (isDebug == 1)
        {
            fprintf(stderr, "(parent_process>closing the write end of the pipe...)\n");
            fprintf(stderr, "(parent_process>forking...)\n");
        }

        pid2 = fork();

        /* child  2*/
        if (pid2 == 0)
        {
            close(STDIN); /* close standart input */
            if (isDebug == 1)
            {
                fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe...)\n");
            }

            dup(myPipe[0]);
            close(myPipe[0]);

            if (isDebug == 1)
            {
                fprintf(stderr, "(child2>going to execute cmd: %s)\n", command2);
            }

            /*parse second command and redirect it */

            if (secCommand->inputRedirect != NULL)
            {
                fileDIn = open(secCommand->inputRedirect, O_RDONLY);
                close(STDIN);
                dup(fileDIn);
                close(fileDIn);
            }
            if (secCommand->outputRedirect != NULL)
            {
                fileDOut = creat(secCommand->outputRedirect, 0644);
                close(STDOUT);
                dup(fileDOut);
                close(fileDOut);
            }

            /* execute second command */
            if (execvp(secCommand->arguments[0], secCommand->arguments) < 0)
            {
                perror("error");
                exit(1);
            }
        }
        /* father */
        else
        {
            if (isDebug == 1)
            {
                fprintf(stderr, "(parent_process>created process with id: %d)\n", pid2);
            }

            close(myPipe[0]); /* close read end of pipe */
            if (isDebug == 1)
            {
                fprintf(stderr, "(parent_process>closing the read end of the pipe...)\n");
                fprintf(stderr, "(parent_process>waiting for child processes to terminate...)\n");
            }
            waitpid(pid2, NULL ,0);
            addProcess(&process_list, firstCommand, pid1);
            addProcess(&process_list, secCommand, pid2);
            if (isDebug == 1)
            {
                fprintf(stderr, "(parent_process>exiting...)\n");
            }
        }
    }
}

int main(int argc, char **argv)
{
    int quitted = 0;
    int procId;
    int addedProcess = 0;
    char buffer[PATH_MAX];
    char userInput[2048];
    cmdLine *currentLine;
    char *firstCommand;
    char *secondCommand;
    int isDebug = 0;
    int i;

    /*checking flags*/
    for (i = 1; i < argc; i++)
    {

        /* -d debug */
        if (strncmp(argv[i], "-d", 2) == 0)
        {
            isDebug = 1;
        }
    }

    while (quitted == 0)
    {
        getcwd(buffer, PATH_MAX);
        printf("%s\n", buffer);
        printf("Enter command:\n");
        fgets(userInput, 2048, stdin);
        if (strcmp(userInput, "quit\n") == 0)
        {
            free_history();
            freeProcessList(process_list);
            exit(0);
        }
        
        if (strcmp(userInput, "!!\n") == 0)
        {
            int i, j;
            j = (newest - 1 + HISTLEN) % HISTLEN;
            for (i = 0; i < count; i++)
            {
                if (!(strcmp(history[j], "history\n") == 0))
                {
                    strcpy(userInput, history[j]);
                    printf("%s", userInput);
                    break;
                }
                j = (j - 1 + HISTLEN) % HISTLEN;
            }
        }
        else if (userInput[0] == '!')
        {
            int index = atoi(&userInput[1]);
            if (index >= 1 && index <= count)
            {
                strcpy(userInput, history[(oldest + index - 1 + HISTLEN) % HISTLEN]);
                printf("%s", userInput);
            }
        }
        if (strcmp(userInput, "history\n") == 0)
        {
            print_history();
        }

        if (strcmp(userInput, "history\n" )!= 0)
        {
            if (strchr(userInput, '|') == NULL)
            { /*There is only one command*/
                currentLine = parseCmdLines(userInput);
                char *command = currentLine->arguments[0];
                if (strcmp(currentLine->arguments[0], "cd") == 0)
                { // Execute cd
                    int chdirSuccess = chdir(currentLine->arguments[1]);
                    if (chdirSuccess == -1)
                    { // Error in cd execution
                        fprintf(stderr, "Error in cd execution\n");
                    }
                }
                else if (strcmp(command, "kill") == 0)
                {
                    /*Execute kill*/
                    int pid = atoi(currentLine->arguments[1]);
                    if (kill(pid, SIGINT) < 0)
                    {
                        perror("kill failed\n");
                        _exit(1);
                    }
                    printf("Handling SIGINT\n");
                }
                else if (strcmp(command, "suspend") == 0)
                {
                    /*Execute suspend*/
                    int pid = atoi(currentLine->arguments[1]);
                    if (kill(pid, SIGTSTP) < 0)
                    {
                        perror("suspend failed\n");
                        _exit(1);
                    }
                    printf("Handling SIGSTP\n");
                }
                else if (strcmp(command, "wake") == 0)
                {
                    /*Execute wake*/
                    int pid = atoi(currentLine->arguments[1]);
                    if (kill(pid, SIGCONT) < 0)
                    {
                        perror("wake failed\n");
                        _exit(1);
                    }
                    printf("Handling SIGTCONT\n");
                }
                else if (strcmp(currentLine->arguments[0], "procs") == 0)
                { // Execute procs
                    printProcessList(&process_list);
                }

                else
                {
                    procId = execute(currentLine);
                    addProcess(&process_list, currentLine, procId);
                    addedProcess = 1;
                }
                if(!addedProcess){
                    freeCmdLines(currentLine);
                }
                addedProcess = 0;
            }
            else
            { /* there are 2 commands*/
                char toSplit[2048];
                strcpy(toSplit, userInput);
                firstCommand = strtok(toSplit, "|");
                secondCommand = strtok(NULL, "\n");
                if (strchr(firstCommand, '>') != NULL)
                { /*There is > redirection in first command*/
                    fprintf(stderr, "Error in first command\n");
                }
                else if (strchr(secondCommand, '<') != NULL)
                { /*There is > redirection in first command*/
                    fprintf(stderr, "Error in second command\n");
                }
                else
                {
                    pipeCommands(firstCommand, secondCommand, isDebug);
                }
            }
        }
        add_history(userInput);
    }
    return 0;
}
