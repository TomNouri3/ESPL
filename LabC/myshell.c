#include <linux/limits.h>
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
#include <ctype.h> 

#ifndef WCONTINUED
#define WCONTINUED 8
#endif

#define BUFFER_SIZE 2048
#define TERMINATED -1
#define RUNNING 1
#define SUSPENDED 0
#define HISTLEN 20
#define MAX_BUF 200

char history[HISTLEN][MAX_BUF];
int history_count = 0;
int history_start = 0;
int history_end = 0;
int debug = 0; // Global variable to enable/disable debug mode

typedef struct process
{
    cmdLine *cmd;         /* the parsed command line*/
    pid_t pid;            /* the process id that is running the command*/
    int status;           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next; /* next process in chain */
} process;

process *process_list = NULL; // Global process list

void addToHistory(const char *cmd) {
    strncpy(history[history_end], cmd, MAX_BUF - 1); // copies the command line cmd to the position history[history_end] in the history buffer (MAX_BUF - 1 ensures that at most MAX_BUF - 1 characters are copied, leaving room for the null terminator)
    history[history_end][MAX_BUF - 1] = '\0'; // explicitly adds a null terminator to ensure the string is properly terminated
    history_end = (history_end + 1) % HISTLEN;
    if (history_count < HISTLEN) {
        history_count++;
    } else {
        history_start = (history_start + 1) % HISTLEN;
    }
}

void printHistory() {
    for (int i = 0; i < history_count; i++) {
        int index = (history_start + i) % HISTLEN;
        printf("%d %s", i + 1, history[index]);
    }
}

char* getHistoryCommand(int index) {
    if (index < 1 || index > history_count) {
        printf("No such command in history.\n");
        return NULL;
    }
    int hist_index = (history_start + index - 1) % HISTLEN;
    return history[hist_index];
}

void freeProcessList(process* process_list) {
    process* curr = process_list;
    while (curr != NULL) {
        process* next = curr->next;
        freeCmdLines(curr->cmd);
        free(curr);
        curr = next;
    }
}

void updateProcessStatus(process* process_list, int pid, int status) {
    while (process_list != NULL) {
        if (process_list->pid == pid) {
            process_list->status = status;
            if (debug) {
                fprintf(stderr, "updateProcessStatus: Updated PID %d to status %d\n", pid, status);
            }
            break;
        }
        process_list = process_list->next;
    }
}

void updateProcessList(process **process_list) {
    int status = 0;
    process *curr;
    for (curr = *process_list; curr != NULL; curr = curr->next) {
        int res = waitpid(curr->pid, &status, WCONTINUED | WNOHANG | WUNTRACED );
        // Call waitpid for Each Process
        // waitpid is called with the following flags:
        // WCONTINUED: Report if a stopped process has been continued
        // WNOHANG: Return immediately if no child has exited
        // WUNTRACED: Report the status of stopped children
        // waitpid checks the status of the process with the process ID curr->pid and stores the status in the status variable
        // res is set to the PID of the child whose status is reported, 0 if no status is available, or -1 on error
        if (res == 0) {
            updateProcessStatus(curr, curr->pid, RUNNING);
        } else {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                // WIFEXITED(status): Returns true if the child terminated normally
                // WIFSIGNALED(status): Returns true if the child process was terminated by a signal.
                updateProcessStatus(curr, curr->pid, TERMINATED);
            } 
            else if (WIFCONTINUED(status)) {
                // WIFCONTINUED(status): Returns true if the child process has continued from a job control stop.
                updateProcessStatus(curr, curr->pid, RUNNING);
            }
            else if (WIFSTOPPED(status)) {
                // WIFSTOPPED(status): Returns true if the child process was stopped by delivery of a signal.
                updateProcessStatus(curr, curr->pid, SUSPENDED);
            } 
        }
    }
}

void addProcess(process** process_list, cmdLine* cmd, pid_t pid){
    process *newProcess = (process *)malloc(sizeof(process));
    if(newProcess == NULL){
        fprintf(stderr, "Failed to allocate memory for new process.\n");
    }
    newProcess->cmd = cmd;
    newProcess->pid = pid;
    newProcess->status = RUNNING;
    newProcess->next = *process_list;
    *process_list = newProcess;   
}

void deleteProcess(process** process_list, process* proc) {
    if (proc == NULL) return;

    if (*process_list == proc) {
        // If the process to be deleted (proc) is the head of the list (*process_list), the head of the list is updated to the next process in the list (proc->next).
        *process_list = proc->next; 
    } else {
        // A pointer prev is initialized to point to the head of the list.
        // A loop is used to traverse the list until prev->next is equal to proc
        // Once the process to be deleted is found, prev->next is updated to proc->next, effectively removing proc from the list.
        process* prev = *process_list;
        while (prev->next != proc) {
            prev = prev->next;
        }
        if (prev->next == proc) {
            prev->next = proc->next;
        }
    }

    if (proc->cmd) {
        proc->cmd->next = NULL;
        freeCmdLines(proc->cmd);
    }
    if (debug) {
        fprintf(stderr, "deleteProcess: Deleting process with PID %d\n", proc->pid);
    }
    free(proc);
}

void printProcessList(process** process_list) {
    updateProcessList(process_list);

    printf("PID          Command      STATUS\n");
    process *curr = *process_list;
    process *next = NULL;

    while (curr) {
        next = curr->next; // Save the next pointer before potentially deleting the current process
        printf("%d        %s        %s\n", curr->pid, curr->cmd->arguments[0],
               curr->status == RUNNING ? "Running" : 
               (curr->status == SUSPENDED ? "Suspended" : "Terminated"));

        curr = next; // Move to the next process
    }

    // Now delete the freshly terminated processes
    curr = *process_list;
    while (curr != NULL) {
        next = curr->next;
        if (curr->status == TERMINATED) {
            deleteProcess(process_list, curr);
        }
        curr = next;
    }
}

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

void handleSleepCommand(cmdLine *pCmdLine) {
    if (pCmdLine->argCount < 2) {
        fprintf(stderr, "sleep: missing process id\n");
    } else {
        int pid = atoi(pCmdLine->arguments[1]);    
        if (kill(pid, SIGTSTP) == -1) {
            fprintf(stderr, "sleep failed\n");
            perror("sleep failed");
        } else {
            printf("Process %d suspended\n", pid);
        }
    }
}

void executePipeCommands(cmdLine *pCmdLine) {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(1);
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork failed");
        exit(1);
    } else if (pid1 == 0) {
        // First child process
        close(STDOUT_FILENO);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        close(pipefd[0]);

        if (pCmdLine->outputRedirect) {
            fprintf(stderr, "Output redirection on the left-hand side of the pipe is not allowed\n");
            _exit(1);
        }

        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("execvp failed");
        _exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork failed");
        exit(1);
    } else if (pid2 == 0) {
        // Second child process
        close(STDIN_FILENO);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        if (pCmdLine->next->inputRedirect) {
            fprintf(stderr, "Input redirection on the right-hand side of the pipe is not allowed\n");
            _exit(1);
        }

        execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments);
        perror("execvp failed");
        _exit(1);
    }

    close(pipefd[0]);
    close(pipefd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void executeSingleCommand(cmdLine *pCmdLine) {
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
        addProcess(&process_list, pCmdLine, pid);
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
    } else if (strcmp(pCmdLine->arguments[0], "procs") == 0) {
        printProcessList(&process_list);
        return;
    } else if (strcmp(pCmdLine->arguments[0], "sleep") == 0) {
        handleSleepCommand(pCmdLine);
        return;
    } else if (strcmp(pCmdLine->arguments[0], "history") == 0) {
        printHistory();
        return;
    } 

    if (pCmdLine->next) {
        executePipeCommands(pCmdLine);
    } else {
        executeSingleCommand(pCmdLine);
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

        // Handle "!!" command
        if (strcmp(input, "!!\n") == 0) {
            const char *last_cmd = getHistoryCommand(history_count);
            if (last_cmd != NULL) {
                strcpy(input, last_cmd);
                printf("Executing: %s", input);
            } else {
                printf("No commands in history.\n");
                continue;
            }
        } 
        // Handle "!n" command
        else if (input[0] == '!' && isdigit(input[1])) {
            int index = atoi(&input[1]);
            const char *cmd = getHistoryCommand(index);
            if (cmd != NULL) {
                strcpy(input, cmd);
                printf("Executing: %s", input);
            } else {
                printf("No such command in history.\n");
                continue;
            }
        }
         addToHistory(input);

        cmdLine *cmd = parseCmdLines(input);
        if (cmd == NULL) {
            continue;
        }

        if (strcmp(cmd->arguments[0], "quit") == 0) {
            freeCmdLines(cmd);
            break;
        }

        execute(cmd);
    }
    freeProcessList(process_list);
    return 0;
}