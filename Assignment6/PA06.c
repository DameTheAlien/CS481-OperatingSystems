#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include "random437.h"
#define MAXWAITPEOPLE 800
       
/* 
 * Programming Assignment 6 
 * CS481-003  
 * Damian Franco & Meiling Traeger
 *
 * This program (PA06.c) is a simulation of the Jurassic park ride
 * at Universial Studios and is used to simulate how many people are
 * in the waiting line and and track who is riding/running away from
 * dinosaurs. This is the design of the waiting and riding simulated.
 *
 * Run with: 
 * gcc -pthread PA06.c -lm
 * ./a.out -N # -M #
 */
pthread_mutex_t shared_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
int peopleArrived;
int peopleInLine;
int peopleRejected;
int totalArrived;
int totalRide;
int totalRejected;
int waitingTime;
int averageWaiting;
int maxLineLength;
int maxTimeLength;
int CARNUM;
int MAXPERCAR;
double currTime;
FILE *fp;


/*
 * Print out state will print to the console and .txt file a line
 * indicating the current state of the simulation. This will include
 * information about the people arrived, rejected, the length of
 * the wait-line, and the time that all of this is happening.
 */
void printOutState() {
    int sec = (int) currTime;
    int h, m;
	h = (sec/60);

	if(sec < 60) {
        m = sec;
	}
	else if(sec >= 60 && sec < 120) {
        m = sec-60;
	}
	h = h + 9;

    fprintf(fp, "%d: Arrive %d Reject %d Wait-Line %d at %d:%d:00 (HH:MM:SS)\n",
            sec, peopleArrived, peopleRejected, peopleInLine, h, m);
	printf("%d: Arrive %d Reject %d Wait-Line %d at %d:%d:00 (HH:MM:SS)\n",
            sec, peopleArrived, peopleRejected, peopleInLine, h, m);
}

/*
 * Much like the function above, this function will print out
 * the state of the simulation but instead, this will print out
 * the final state of simulation upon completion of the day.
 */
void printFinalResults() {
    fprintf(fp, "Total People arrived: %d\n", totalArrived);
    fprintf(fp, "Total People taking the ride: %d\n", totalRide);
    fprintf(fp, "Total People that left: %d\n", totalRejected);
    fprintf(fp, "Average waiting time per person: %d mins\n", averageWaiting);
    fprintf(fp, "Worst case line length: %d at %d sec\n", maxLineLength, maxTimeLength);
    printf("SIMULATION COMPLETED\n");
}

/*
 * Checks to see if the simulation should be completed and stopped,
 * which should be at 600 seconds. This will also stop all
 * the current running threads.
 */
void checkEndTime() {
    if(currTime >= 600.0) {
            printFinalResults();
            pthread_mutex_lock(&shared_mutex);
            pthread_cond_broadcast(&cond1);
            pthread_mutex_unlock(&shared_mutex);
            pthread_exit(NULL);
        }
}

/*
 * Does the same thing as the checkEndTime function, but
 * this one is seperated cause all it does is end the 
 * current thread that is running and does not wake up
 * them again so it is not constantly looping.
 */
void checkEndThreads() {
    if(currTime >= 600.0) {
            pthread_exit(NULL);
        }
}

/*
 * This function is the heart of the program and is 
 * created to simulate the time within the simulation.
 * Every second that this timer runs will wake up the
 * threads in the simulation through the pthread cond
 * broadcast call. 
 */
void* startTimer() {
    pid_t tid = syscall(SYS_gettid);
    printf("Clock Thread %d printed\n", tid);
    clock_t begin = clock();
    unsigned int endTime;
    double tenSec = 1.0;
    for(endTime = 0; 1; endTime++) {
        currTime = (double) (clock() - begin) / CLOCKS_PER_SEC;
        if (currTime == tenSec) {
            tenSec = tenSec + 1.0;
            //wake up all threads
            pthread_mutex_lock(&shared_mutex);
            pthread_cond_broadcast(&cond1);
            pthread_mutex_unlock(&shared_mutex);
        }
        checkEndTime();
    }
    printf("Time spent: %d\n", endTime);
}

/*
 * The car thread function is used to simulate the
 * cars within the ride itself. Each car must pick
 * up people upon arrival every second which is what
 * this function mainly handles.
 */
void* carThread() {
    checkEndThreads();
    pthread_mutex_lock(&shared_mutex);

    pthread_cond_wait(&cond1, &shared_mutex);

    //simulate people getting on the ride
    int currentRiding = MAXPERCAR;
    if (peopleInLine <= 0) {
        peopleInLine = 0;
     }
    else {
        peopleInLine = peopleInLine - currentRiding;
    }
    totalRide = totalRide + currentRiding;

    pthread_mutex_unlock(&shared_mutex);
    sleep(1);
    carThread();
}

/*
 * The line thread is created to to simulate the enless
 * roped off line area. The function will simulate
 * how many guests arrive and if they can fit in the line.
 * Some people will need to be rejected as well, which is
 * also handled here.
 */
void* lineThread() {
     checkEndThreads();
     pthread_mutex_lock(&shared_mutex);
     pthread_cond_wait(&cond1, &shared_mutex);

     //generate based off time
     if(currTime >= 0.0 && currTime <= 119.9) {
        peopleArrived = poissonRandom(25);
     }
     else if(currTime >= 120.0 && currTime <= 299.9) {
        peopleArrived = poissonRandom(45);
     }
     else if(currTime >= 300.0 && currTime <= 419.9) {
        peopleArrived = poissonRandom(35);
     }
     else if(currTime >= 420.0 && currTime <= 600.0) {
        peopleArrived = poissonRandom(25);
     }
     totalArrived = totalArrived + peopleArrived;

     //add people to line if space
     if(peopleInLine <= MAXWAITPEOPLE) {
        peopleInLine = peopleInLine + peopleArrived;
     }
     else if (peopleInLine <= 0) {
        peopleInLine = 0;
     }
     else {
        peopleRejected = peopleArrived;
        totalRejected = totalRejected + peopleArrived;
     }
     printOutState();
     sleep(1);
     pthread_mutex_unlock(&shared_mutex);
     lineThread();
}

/*
 * Threads are created here in this function based off the 
 * user input (N and M). There will be n+2 threads created
 * because there are n cars on the ride, a thread handling
 * the line, and a thread that will handle the virtual timer.
 */
void threadCreation(int n) {
     void* voidptr = NULL;
     srand(getpid());
     pthread_t tid[n+2];

     //creation of all threads
     int i;
     for (i = 0; i < n+2; i++) {
        if(i == 0) {
            //creation of timer thread
            if (pthread_create(&tid[i], NULL, startTimer, NULL)) {
                perror("Error in thread creating\n");
            }
        }
        else if(i == 1) {
            //creation of line thread
            if (pthread_create(&tid[i], NULL, lineThread, NULL)) {
                perror("Error in thread creating\n");
            }
        }
        else {
          //creation of car thread(s)
          if (pthread_create(&tid[i], NULL, carThread, NULL)) {
                perror("Error in thread creating\n");
            }
        }
     }

     for (i = 0; i < n; i++) {
        if (pthread_join(tid[i], (void*)&voidptr)) {
            perror("Error in thread joining\n");
        }
     }
}

/*
 * The handleInput function will handle all the user input with
 * the getopt() function call. After processing the user input,
 * the n and m values should be casted from a string to a integer.
 * Lastly, this will call the threadCreation function.
 */
void handleInput(int argc, char **argv) {
  int c, n, m;

  char* helpMePlz = "PLEASE ENTER:\n"
                    "-h help\n"
                    "-N CARNUM\n"
                    "-M MAXPERCAR\n";

  while((c = getopt(argc, argv, "N:M:")) != -1) {
    switch(c) {
      case 'h':
        printf("%s", helpMePlz);
        exit(0);
      case 'N':
        n = atoi(optarg);
        break;
      case 'M':
        m = atoi(optarg);
        break;
      default:
        printf("Invalid argument type.\n");
        exit(0);
    }
  }

  CARNUM = n;
  MAXPERCAR = m;
  printf("Entered n: %d\n", n);
  printf("Entered m: %d\n", m);
  threadCreation(n);
}

/*
 * Calls all necassary functions for the program.
 */
int main(int argc, char **argv) {
    //initializations
    fp = fopen("test.txt", "w+");
    pthread_mutex_init(&shared_mutex, NULL);
    //handle program logic
    handleInput(argc, argv);
    //tie up all loose ends
    fclose(fp);
    pthread_mutex_destroy(&shared_mutex);
    pthread_cond_destroy(&cond1);
    return(0);
}