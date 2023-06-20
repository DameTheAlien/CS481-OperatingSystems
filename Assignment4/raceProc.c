/*=========================================================*/
/* race.c --- for playing with CS481/ECE437                */
/* Run code with: gcc -o raceProc raceProc.c               */
/*                then run ./raceProc                      */
/*=========================================================*/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define SHMSZ 27

struct Bank {
    int balance[2];
};

void* MakeTransactions(struct Bank* bank) { //routine for thread execution
    int i, j, tmp1, tmp2, rint;
    double dummy;
    for (i = 0; i < 100; i++) {
        rint = (rand() % 30) - 15;
        if (((tmp1 = bank->balance[0]) + rint) >= 0 && ((tmp2 = bank->balance[1]) - rint) >= 0) {
            //printf("tmp1: %d, rint: %d\n", tmp1, rint);
            bank->balance[0] = tmp1 + rint;
            for (j = 0; j < rint * 1000; j++) {
                dummy = 2.345 * 8.765 / 1.234; // spend time on purpose
            }
            //printf("tmp2: %d, rint: %d\n", tmp2, rint);
            bank->balance[1] = tmp2 - rint;
        }
    }
    return NULL;
}


int main(int argc, char **argv) {
    pid_t pid = fork();
    int shmid;
    char* shmaddr;
    struct Bank* bank;
    struct shmid_ds shm_desc;
    key_t key = ftok("shmfile", 65);

    shmid = shmget(key, SHMSZ, IPC_CREAT | 0666);
     if (shmid == -1) {
        perror("GET ERROR");
        exit(1);
    }

    shmaddr = shmat(shmid, NULL, 0);
    if (!shmaddr) {
        perror("ATTACH ERROR");
        exit(1);
    }

    bank = (struct Bank*) ((void*)shmaddr + sizeof(int));
    bank->balance[0] = 100;
    bank->balance[1] = 100;

    if(pid > 0) {
        printf("In the parent process\n");
        MakeTransactions(bank);
    }
    else if(pid == 0) {
        printf("In the child process\n");
        MakeTransactions(bank);
    }
    printf("Let's check the balances A:%d + B:%d ==> %d ?= 200\n",
            bank->balance[0], bank->balance[1], bank->balance[0] + bank->balance[1]);

    shmdt(shmaddr);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
