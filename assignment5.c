#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/wait.h>
#include <time.h>

#define PHILOSOPHERS 5
#define EATING_TIME 100

typedef struct {
    int thinkTimeTotal;
    int eatTimeTotal;
    int cycles;
} PhilosopherData;

int randomGaussian(int mean, int stddev) {
    double mu = 0.5 + (double) mean;
    double sigma = fabs((double) stddev);
    double f1 = sqrt(-2.0 * log((double) rand() / (double) RAND_MAX));
    double f2 = 2.0 * 3.14159265359 * (double) rand() / (double) RAND_MAX;
    if (rand() & (1 << 5)) 
        return (int) floor(mu + sigma * cos(f2) * f1);
    else            
        return (int) floor(mu + sigma * sin(f2) * f1);
}

int main() {
    int semid, shmid;
    PhilosopherData *data;
    
    semid = semget(IPC_PRIVATE, PHILOSOPHERS, 0666 | IPC_CREAT);
    shmid = shmget(IPC_PRIVATE, PHILOSOPHERS * sizeof(PhilosopherData), 0666 | IPC_CREAT);
    data = (PhilosopherData *)shmat(shmid, NULL, 0);

    for (int i = 0; i < PHILOSOPHERS; i++) {
        semctl(semid, i, SETVAL, 1);
        if (fork() == 0) { // Child process: philosopher
            srand(time(NULL) ^ getpid());
            int eatTimeTotal = 0, thinkTimeTotal = 0, cycles = 0;
            
            while (eatTimeTotal < EATING_TIME) {
                cycles++;
                
                // Thinking
                int thinkTime = randomGaussian(11, 7);
                if (thinkTime < 0) thinkTime = 0;
                
                printf("Philosopher %d thinking for %d seconds (total = %d)\n", i, thinkTime, thinkTimeTotal);
				thinkTimeTotal += thinkTime;
                sleep(thinkTime);
                
                // Eating
                int eatTime = randomGaussian(9, 3);
                if (eatTime < 0) eatTime = 0;

                printf("Philosopher %d eating for %d seconds (total = %d)\n", i, eatTime, eatTimeTotal);
				eatTimeTotal += eatTime;
                sleep(eatTime);
            }
            
            printf("Philosopher %d done with meal (process %d)\n", i, getpid());
            data[i].thinkTimeTotal = thinkTimeTotal;
            data[i].eatTimeTotal = eatTimeTotal;
            data[i].cycles = cycles;
            exit(EXIT_SUCCESS);
        }
    }
    
    // Waiting for all children to exit
    for (int i = 0; i < PHILOSOPHERS; i++) {
        wait(NULL);
    }

    // Printing recap
    for (int i = 0; i < PHILOSOPHERS; i++) {
        printf("Philosopher %d thought for %d seconds, ate for %d seconds over %d cycles.\n", 
               i, data[i].thinkTimeTotal, data[i].eatTimeTotal, data[i].cycles);
    }

    // Cleanup semaphores and shared memory
    semctl(semid, 0, IPC_RMID);
    shmctl(shmid, IPC_RMID, NULL);
    
    return 0;
}
