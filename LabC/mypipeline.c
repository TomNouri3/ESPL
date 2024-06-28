// ls -l
// Lists files in the current directory in long format, 
// which includes details such as permissions, number of links, owner, group, size, and time of last modification.

// tail -n 2
// Outputs the last 2 lines of its input

// Combination: The command ls -l | tail -n 2 
// lists the files in the current directory and then outputs only the last 2 lines of that list.

// 1. Create a pipe.
// Creating a pipe is the first step in setting up inter-process communication (IPC) between the two child processes we will fork.
// A pipe is a unidirectional communication channel that can be used to pass data from one process to another.
// In this context, we will use it to pass the output of the ls -l command executed by the first child process to the input of the tail -n 2 command executed by the second child process.
// The pipe() system call is used to create a pipe. It creates a pair of file descriptors: one for reading and one for writing.
// The file descriptors are returned in an array.

// 2. Fork a first child process (child1).
// Forking a process in Unix-like operating systems creates a new process by duplicating the current process.
// The new process is called the child process, and the original process is called the parent process.
// The fork() system call is used to create this child process.
// If fork() returns -1, it means the fork failed.
// If fork() returns 0, we are in the child process.
// If fork() returns a positive number, we are in the parent process and the return value is the PID of the child.

// 3. In the child1 process:

// Close the standard output:
// We need to close the standard output file descriptor to redirect it to the pipe.
// close(STDOUT_FILENO); closes the standard output file descriptor which is file descriptor 1. 
// This prepares for redirecting standard output to the pipe.
// In Unix-like operating systems, every process has a set of standard file descriptors that are automatically opened when the process starts:
// Standard Input (STDIN_FILENO): File descriptor 0, used for reading input.
// Standard Output (STDOUT_FILENO): File descriptor 1, used for writing output.
// Standard Error (STDERR_FILENO): File descriptor 2, used for writing error messages.
// By default, the standard output of a process writes to the terminal (or console) where the process was started.
// In the context of our example, we want the output of the ls -l command to be written to a pipe instead of the terminal. To achieve this, we need to redirect the standard output to the pipe.
// The standard output file descriptor is automatically used by the process for any output operations. To redirect this output to a different location (in this case, the write end of the pipe), 
// we need to: close the current standard output file descriptor and duplicate the file descriptor for the write end of the pipe so that it takes the place of the standard output file descriptor.

#include <unistd.h>   // for pipe() and fork()
#include <stdio.h>    // for perror()
#include <stdlib.h>   // for exit()
#include <sys/types.h> // for pid_t
#include <sys/wait.h> // for waitpid()

int main() {
    int pipefd[2]; // declares an array of two integers. This array will hold the file descriptors for the read and write ends of the pipe.
    // Create a pipe
    if (pipe(pipefd) == -1) { // checks if the pipe() system call fails. If it returns -1, an error occurred.
    // pipe(pipefd) - creates the pipe. On success, pipefd[0] is the file descriptor for the read end of the pipe, and pipefd[1] is the file descriptor for the write end of the pipe.
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    // pipefd[0] is the read end of the pipe
    // pipefd[1] is the write end of the pipe

    // In the context of our example, where we want to pass the output of the ls -l command executed by the first child process to the input of the tail -n 2 command executed by the second child process:
    // pipefd[1] will be used by the first child process to write the output of ls -l into the pipe.
    // pipefd[0] will be used by the second child process to read the input for tail -n 2 from the pipe.

    // Debug message: Before forking
    fprintf(stderr, "(parent_process>forking…)\n");

    // Fork a first child process (child1).
    pid_t child1 = fork(); // Create a new process by duplicating the current process

    if (child1 == -1) { // Error handling: If fork() returns -1, it means the fork failed.
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (child1 == 0) { // If fork() returns 0, we are in the child process.
        // Debug message: Redirecting stdout to the write end of the pipe
        fprintf(stderr, "(child1>redirecting stdout to the write end of the pipe…)\n");

        // Close the standard output.
        close(STDOUT_FILENO);

        // Duplicate the write-end of the pipe using dup2.
        dup2(pipefd[1], STDOUT_FILENO);

        // Close the file descriptor that was duplicated.
        // After duplicating it to standard output, we no longer need the original descriptor in the child process.
        close(pipefd[1]);

        // Close the read-end file descriptor (we don't need it in the first child process)
        close(pipefd[0]);

        // Debug message: Going to execute cmd: ls -l
        fprintf(stderr, "(child1>going to execute cmd: ls -l)\n");

        // Execute "ls -l".
        char *args[] = {"ls", "-l", NULL};
        execvp(args[0], args); // replaces the current process image with the ls -l command.

        // If execvp returns, it must have failed
        perror("execvp failed");
        _exit(EXIT_FAILURE); // Exit abnormally if execvp fails
    } else { // If fork() returns a positive number, we are in the parent process.
        // Debug message: Created process with id
        fprintf(stderr, "(parent_process>created process with id: %d)\n", child1);

        // Debug message: Closing the write end of the pipe
        fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");

        // Close the write end of the pipe.
        close(pipefd[1]);

        // Fork a second child process (child2).
        pid_t child2 = fork(); // Create a new process by duplicating the current process

        if (child2 == -1) { // Error handling: If fork() returns -1, it means the fork failed.
            perror("fork failed");
            exit(EXIT_FAILURE);
        }

        if (child2 == 0) { // If fork() returns 0, we are in the second child process.
            // Debug message: Redirecting stdin to the read end of the pipe
            fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n");

            // Close the standard input.
            close(STDIN_FILENO);

            // Duplicate the read-end of the pipe using dup2.
            dup2(pipefd[0], STDIN_FILENO);

            // Close the file descriptor that was duplicated.
            close(pipefd[0]);

            // Debug message: Going to execute cmd: tail -n 2
            fprintf(stderr, "(child2>going to execute cmd: tail -n 2)\n");

            // Execute "tail -n 2".
            char *args[] = {"tail", "-n", "2", NULL};
            execvp(args[0], args); // replaces the current process image with the tail -n 2 command.

            // If execvp returns, it must have failed
            perror("execvp failed");
            _exit(EXIT_FAILURE); // Exit abnormally if execvp fails

        } else { // If fork() returns a positive number, we are in the parent process.
            // Debug message: Created process with id
            fprintf(stderr, "(parent_process>created process with id: %d)\n", child2);

            // Debug message: Closing the read end of the pipe
            fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n");

            // Close the read end of the pipe.
            close(pipefd[0]);

            // Debug message: Waiting for child processes to terminate
            fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");

            // Wait for the first child process to terminate.
            waitpid(child1, NULL, 0);

            // Wait for the second child process to terminate.
            waitpid(child2, NULL, 0);

            // Debug message: Exiting
            fprintf(stderr, "(parent_process>exiting…)\n");
        }
    }

    return 0;
}
