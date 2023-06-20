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
#include <getopt.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/*
 * Initilize the shared memory used to keep track of the
 * fib(1) = 1 tracker. This function will handle all the
 * set up of shared memory by calling various shm function.
 */
int shm_init(void **shm, int *shm_fd, char *shm_handle) {
    if (!(*shm_fd = shm_open(shm_handle, (O_RDWR | O_CREAT), (S_IRUSR | S_IWUSR)))) {
        printf("%s\n", strerror(errno));
        abort();
    }

    if (ftruncate(*shm_fd, sizeof(int)) == -1) {
        printf("ftruncate: %s\n", strerror(errno));
        abort();
    }

    if (!(*shm = mmap(NULL, sizeof(int), (PROT_READ | PROT_WRITE), (MAP_SHARED | MAP_LOCKED), *shm_fd, 0))) {
        printf("%s\n", strerror(errno));
        abort();
    }

    if (shm_unlink(shm_handle) == -1) {
        printf("%s\n", strerror(errno));
        abort();
    }
    return 1;
}

/*
 * slow/recursive implementation of Fib given to 
 * everybody in class and used to compare to the
 * fibonacci function that we wrote.
 */
int fib_seq(int x) {
    int i, rint = (rand() % 30);
    double dummy;
    for(i = 0; i< rint * 100; i++) {
        dummy = 2.345 * i * 8.765/1.234;
    }
    if(x == 0) {
        return(0);
    }
    else if (x == 1) {
        return(1);
    }
    else {
        return(fib_seq(x-1) + fib_seq(x-2));
    }
}

/*
 * This fibonacci function will handle compute the
 * fibonacci sequence to the user given number n.
 * The difference is that this function computes it
 * by using the fork() system call and creating new 
 * child processes. The function will recurse and 
 * call the fork() system call if the input number x
 * is greater than 1. This is because fib(0) = 0 and
 * fib(1) = 1. If the input number x is equal to 1, then 
 * the shared memory counter will be incremented and if
 * x is equal to 0 then nothing is need to be done.
 * 
 */
void my_fib(int x, void *shm) {
    // printf("intial x: %d\n", x);

    if(x > 1) {
        pid_t pid;
        pid = fork();
        if(pid == 0) {
            // Child code here
            // printf("NEW CHILD CREATED\n");
            // printf("child x: %d \n", x);
            x = x - 1;
            my_fib(x, shm);
        }
        else {
            // Parent code here
            waitpid(pid, NULL, 0);
            // printf("NEW PARENT CREATED\n");
            x = x - 2;
            my_fib(x, shm);
        }
    }
    else {
        int val = *((int *) shm);
        int val_old = val;
        if(x == 1) {
            val++;
            int val_last = __sync_val_compare_and_swap((int *) shm, val_old, val);
            // printf("Shared counter is at: %d", val);
        }
        else if(x == 0) {
          // Do nothing
        }

    }
}

/*
 * The handleInput function will handle all the user input with 
 * the getopt() function call. After processing the user input,
 * the n and m values should be casted from a string to a integer.
 * Next, the while loop for handling the user input will be exited
 * and the handling of the correct case (case 1 or 2) will take place.
 * case 1 will call the my_fib function that will utilize
 * the fork() system call to find the fibonacci number of the
 * given input n. Case 2 will call the fib_seq function call that
 * computes the fibonacci sequence as well but not using processes.
 */
void handleInput(int argc, char **argv, void *shm) {
  int c, n, m;

  char* helpMePlz = "PLEASE ENTER:\n"
                    "-h help\n"
                    "-F nth number of Fibonacci sequence\n"
                    "-S computing threshold\n";

  while((c = getopt(argc, argv, "S:F:")) != -1) {
    switch(c) {
      case 'h':
        printf("%s", helpMePlz);
        exit(0);
      case 'F':
        n = atoi(optarg);
        break;
      case 'S':
        m = atoi(optarg);
        break;
      default:
        printf("Invalid argument type.\n");
        exit(0);
    }
  }

  printf("Entered n: %d\n", n);
  printf("Entered m: %d\n", m);

  int fibNum = 0;
  if((n - 1) > m && (n - 2) > m) {
    // Case 1
    my_fib(n, shm);
    fibNum = *((int *) shm);
    printf("\nMy fib output steps: %d\n", fibNum);
    printf("Above is what my fibonacci found\n");
  }
  else {
    // Case 2
    fibNum = fib_seq(n);
    printf("Fib output: %d\n", fibNum);
  }
}


/*
 * Calls all necassary functions for the program.
 */
int main(int argc, char **argv) {
    // Shared memory set up
    int shm_fd = 0;
    char *shm_handle = "/cas_test_shm";
    void *shm = NULL;
    shm_init(&shm, &shm_fd, shm_handle);

    // Handle program logic
    handleInput(argc, argv, shm);
}
