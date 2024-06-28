#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <signal.h>
#include <string.h>

// Signal handler function
void handler(int sig) {
    printf("\nReceived Signal: %s\n", strsignal(sig));

    // Restore default signal handler and propagate the signal
    signal(sig, SIG_DFL);
    raise(sig);

    // Re-instate the custom handlers
    if (sig == SIGCONT) {
        // After handling SIGCONT, reinstate custom handler for SIGTSTP
        signal(SIGTSTP, handler);
    }
    else if (sig == SIGTSTP) {
        // After handling SIGTSTP, reinstate custom handler for SIGCONT
        signal(SIGCONT, handler);
    } 
}

int main(int argc, char **argv) {
    printf("Starting the program\n");

    // Set the custom signal handlers
    signal(SIGINT, handler);
    signal(SIGTSTP, handler);
    signal(SIGCONT, handler);

    // Infinite loop to keep the program running
    while (1) {
        sleep(1);
    }

    return 0;
}
