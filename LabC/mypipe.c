#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h> // Added
#include <sys/wait.h> // Added

#define MESSAGE "hello"
#define BUFFER_SIZE 128

int main() {
    int pipefd[2];  // file descriptors for the pipe
    pid_t pid;
    char buffer[BUFFER_SIZE];

    // Create the pipe
    if (pipe(pipefd) == -1) {
        perror("pipe failed");
        exit(EXIT_FAILURE); 
    }

    // Fork the process
    pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process
        close(pipefd[0]); // Close the read end of the pipe
        write(pipefd[1], MESSAGE, strlen(MESSAGE)); // Write message to the pipe
        close(pipefd[1]); // Close the write end of the pipe
        exit(EXIT_SUCCESS);
    } else {
        // Parent process
        close(pipefd[1]); // Close the write end of the pipe
        read(pipefd[0], buffer, BUFFER_SIZE); // Read message from the pipe
        close(pipefd[0]); // Close the read end of the pipe
        printf("Received message: %s\n", buffer); // Print the message
        wait(NULL); // Wait for child process to terminate
    }

    return 0;
}
