#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

int main() {
    int pipe_fd[2];
    pid_t pid1, pid2;

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    fprintf(stderr, "(parent_process>forking...)\n");
    pid1 = fork();

    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid1 == 0) { // child1
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe...)\n");
        close(STDOUT_FILENO);
        dup(pipe_fd[1]);
        close(pipe_fd[0]);
        close(pipe_fd[1]);

        char* cmd[] = {"ls", "-l", NULL};
        fprintf(stderr, "(child1>going to execute cmd: %s %s)\n", cmd[0], cmd[1]);
        execvp(cmd[0], cmd);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else { // parent
        fprintf(stderr, "(parent_process>created process with id: %d)\n", pid1);
        close(pipe_fd[1]);
        fprintf(stderr, "(parent_process>closing the write end of the pipe...)\n");

        fprintf(stderr, "(parent_process>forking...)\n");
        pid2 = fork();

        if (pid2 == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if (pid2 == 0) { // child2
            fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe...)\n");
            close(STDIN_FILENO);
            dup(pipe_fd[0]);
            close(pipe_fd[0]);
            close(pipe_fd[1]);

            char* cmd[] = {"tail", "-n", "2", NULL};
            fprintf(stderr, "(child2>going to execute cmd: %s %s %s)\n", cmd[0], cmd[1], cmd[2]);
            execvp(cmd[0], cmd);
            perror("execvp");
            exit(EXIT_FAILURE);
        } else { // parent
            fprintf(stderr, "(parent_process>created process with id: %d)\n", pid2);
            close(pipe_fd[0]);
            fprintf(stderr, "(parent_process>closing the read end of the pipe...)\n");
            fprintf(stderr, "(parent_process>waiting for child processes to terminate...)\n");
            waitpid(pid1, NULL, 0);
            waitpid(pid2, NULL, 0);
            fprintf(stderr, "(parent_process>exiting...)\n");
        }
    }

    return 0;
}