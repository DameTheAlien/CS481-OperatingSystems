/*
 * Assignment 2
 * CS 481 - 003
 *
 * Damian Franco
 * Meiling Traeger
 *
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

/* Flag to track if the child is running/stopped */
int runFlag;
/* Global variable for pid of child */
pid_t child;

/*
 * Handles the ctrl-c input. This will catch the signal from
 * the keyboard and print out updates for the user about the
 * current state of the program. This will also kill the child
 * process and exit the main process as well.
 */
void handleSigint(int sig) {
    printf("\nCaught signal %d\n", sig);
    printf("Crtl-C was pressed.\n");
    printf("KILLING CHILD PROCESS AND EXITING PROGRAM\n");
    printf("PID: %d\n", child);
    kill(child, SIGKILL);
    exit(0);
}

/*
 * This function will handle the ctrl-z signal call.
 * There will be updates printed to to console to
 * indicate the state change to the user. This function
 * will also pause and resume the child process by checking
 * the global run flag.
 *
 * This does not work. For some reason, the process will
 * not resume after being paused. There are lines of code
 * in the else if section that allow for a semi-functioning
 * resume call. This code only works for one re-run of the
 * child process then it just runs infinitly until the
 * ctrl-c signal call is called.
 *
 */
void handleSigtstp(int sig) {
    printf("\nCaught signal %d\n", sig);
    printf("Crtl-Z was pressed.\n");
    printf("PID: %d\n", child);
    printf("RUN FLAG: %d\n", runFlag);
    if(runFlag == 1) {
        printf("CHILD PAUSED\n");
        runFlag = 0;
        kill(child, SIGSTOP);
    }
    else if(runFlag == 0) {
        printf("CHILD RUNNING\n");
        // Uncomment this code for semi-working resuming of the process
        // char *args[] = {"/bin/sh", "-c", "yes", 0};
        // char *env[] = {0};
        // execve("/bin/sh", args, env);
        kill(child, SIGCONT);
        runFlag = 1;
    }
}


/*
 * The main function will first set the run flag to
 * 1 because the 'yes' unix function will be running
 * upon startup of this program. Next, the function
 * will fork into a child process and parent process
 * and the child process will call the 'yes' unix
 * function. The parent will set up the signal handling
 * for the ctrl-z and ctrl-c signal calls. Lastly, the
 * main function will loop infinitely.
 */
int main(int argc, char **argv) {
    runFlag = 1;

    pid_t pid;
    pid = fork();

    if(pid == 0) {
        // Child code here
        printf("CHILD CREATED! PID: %d\n", getpid());

        // run 'yes' unix function
        char *args[] = {"/bin/sh", "-c", "yes", 0};
        char *env[] = {0};
        execve("/bin/sh", args, env);
    }
    else {
        // Parent code here
        printf("PARENT CREATED! PID: %d\n", getpid());

        // signal set up
        struct sigaction act;
        act.sa_handler = handleSigint;
        sigemptyset(&act.sa_mask);
        act.sa_flags = 0;

        struct sigaction act2;
        act2.sa_handler = handleSigtstp;
        sigemptyset(&act2.sa_mask);
        act2.sa_flags = 0;

        sigaction(SIGINT, &act, 0);
        sigaction(SIGTSTP, &act2, 0);

        // wait for child and assign child pid
        child = pid;
        waitpid(child, NULL, 0);
    }

    // loop infinitely
    while (1) {}

    return 0;

}
