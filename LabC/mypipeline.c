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

#include <unistd.h> // for pipe()
#include <stdio.h>  // for perror()
#include <stdlib.h> // for exit()
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

    // Fork a first child process (child1).
    pid_t child1 = fork(); // Create a new process by duplicating the current process

    if (child1 == -1) { // Error handling: If fork() returns -1, it means the fork failed.
        perror("fork failed");
        exit(EXIT_FAILURE);
    }

    if (child1 == 0) { // If fork() returns 0, we are in the child process.
        // Placeholder for child process code
        // In the child process, we will eventually close the standard output,
        // duplicate the write-end of the pipe using dup(), close the file descriptor that was duplicated,
        // and execute "ls -l".
    } else { // If fork() returns a positive number, we are in the parent process.
        // Placeholder for parent process code
    }

    return 0;
}