#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include "LineParser.h"

#define BUFFER_SIZE 2048

int debug = 0; // Global variable to enable/disable debug mode

void displayPrompt() {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s> ", cwd);
    } else {
        perror("getcwd() error");
    }
}

char* readInput() {
    static char buffer[BUFFER_SIZE];
    if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
        return NULL;
    }
    return buffer;
}

void handleCdCommand(cmdLine *pCmdLine) {
    if (pCmdLine->argCount < 2) {
        fprintf(stderr, "cd: missing argument\n");
    } else {
        if (chdir(pCmdLine->arguments[1]) == -1) {
            perror("cd failed");
        }
    }
}

void handleAlarmCommand(cmdLine *pCmdLine) {
    if (pCmdLine->argCount < 2) {
        fprintf(stderr, "alarm: missing process id\n");
    } else {
        int pid = atoi(pCmdLine->arguments[1]);
        if (kill(pid, SIGCONT) == -1) {
            perror("alarm failed");
        } else {
            printf("Process %d continued\n", pid);
        }
    }
}

void handleBlastCommand(cmdLine *pCmdLine) {
    if (pCmdLine->argCount < 2) {
        fprintf(stderr, "blast: missing process id\n");
    } else {
        int pid = atoi(pCmdLine->arguments[1]);
        if (kill(pid, SIGKILL) == -1) {
            perror("blast failed");
        } else {
            printf("Process %d killed\n", pid);
        }
    }
}

void execute(cmdLine *pCmdLine) {
    if (strcmp(pCmdLine->arguments[0], "cd") == 0) {
        handleCdCommand(pCmdLine);
        return;
    } else if (strcmp(pCmdLine->arguments[0], "alarm") == 0) {
        handleAlarmCommand(pCmdLine);
        return;
    } else if (strcmp(pCmdLine->arguments[0], "blast") == 0) {
        handleBlastCommand(pCmdLine);
        return;
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        // Child process

        // Handle input redirection
        if (pCmdLine->inputRedirect) {
            int fd = open(pCmdLine->inputRedirect, O_RDONLY);
            if (fd == -1) {
                perror("open input file failed");
                _exit(1);
            }
            if (dup2(fd, STDIN_FILENO) == -1) {
                perror("dup2 input redirection failed");
                _exit(1);
            }
            close(fd);
        }

        // Handle output redirection
        if (pCmdLine->outputRedirect) {
            int fd = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd == -1) {
                perror("open output file failed");
                _exit(1);
            }
            if (dup2(fd, STDOUT_FILENO) == -1) {
                perror("dup2 output redirection failed");
                _exit(1);
            }
            close(fd);
        }

        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        
        // If execvp returns, it must have failed
        perror("execvp failed");
        _exit(1); // Exit abnormally if execvp fails
    } else {
        // Parent process
        if (debug) {
            fprintf(stderr, "PID: %d\n", pid);
            fprintf(stderr, "Executing command: %s\n", pCmdLine->arguments[0]);
            fprintf(stderr, "Blocking: %d\n", pCmdLine->blocking);
        }
        if (pCmdLine->blocking) {
            waitpid(pid, NULL, 0); // Wait for the child process to terminate if blocking
        }
    }
}

int main(int argc, char **argv) {
    // Check for debug flag
    if (argc > 1 && strcmp(argv[1], "-d") == 0) {
        debug = 1;
    }

    while (1) {
        displayPrompt();

        char *input = readInput();
        if (input == NULL) {
            break;
        }

        cmdLine *cmd = parseCmdLines(input);
        if (cmd == NULL) {
            continue;
        }

        if (strcmp(cmd->arguments[0], "quit") == 0) {
            freeCmdLines(cmd);
            break;
        }

        execute(cmd);
        freeCmdLines(cmd);
    }
    return 0;
}
