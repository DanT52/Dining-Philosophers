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
#include <sys/time.h>

#define PHILOSOPHERS 5
#define EATING_TIME 100

typedef struct {
    int thinkTimeTotal;
    int eatTimeTotal;
    int cycles;
} PhilosopherData;

//rng function
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

//philosopher cycle / meal routuine
void philosopher(int i, int semid, PhilosopherData *data){

	struct sembuf putdownchops[2] = {{i,1,0}, {(i+1)%PHILOSOPHERS,1,0}};
	struct sembuf checkforchops[2] = {{i,-1,IPC_NOWAIT}, {(i+1)%PHILOSOPHERS,-1,IPC_NOWAIT}};
	struct sembuf pickupchops[2] = {{i,-1,0}, {(i+1)%PHILOSOPHERS,-1,0}};

	//seed for the rng
	srand(time(NULL) ^ getpid());
    int eatTimeTotal = 0, thinkTimeTotal = 0, cycles = 0;

	while (eatTimeTotal < EATING_TIME) {
                cycles++;
                
                // thinking phase
                int thinkTime = randomGaussian(11, 7);
                if (thinkTime < 0) thinkTime = 0;
                printf("Philosopher %d thinking for %d seconds (total = %d)\n", i, thinkTime, thinkTimeTotal);
				thinkTimeTotal += thinkTime;
                sleep(thinkTime);

				// trying to pick up chopsticks
				if (semop(semid, checkforchops, 2) == -1) {
					int res = 1;
					if (errno == EAGAIN) {
						printf("Philosopher %d waiting for sticks %d and %d.\n", i, i, (i+1)%PHILOSOPHERS);
						res = semop(semid, pickupchops, 2);
					} 
					else if (errno != EAGAIN || res == -1){
						fprintf(stderr, "ERROR: Philosipher %d error pickup sticks. ERRNO: %d , %s \n", i, errno, strerror(errno));
						exit(1);
					}
        		}
                
                // eating phase
                int eatTime = randomGaussian(9, 3);
                if (eatTime < 0) eatTime = 0;
                printf("Philosopher %d eating for %d seconds (total = %d)\n", i, eatTime, eatTimeTotal);
				eatTimeTotal += eatTime;
                sleep(eatTime);

				//put down chopsticks
				printf("Philosopher %d putting down sticks %d and %d\n", i, i, (i+1)%PHILOSOPHERS);
				if (semop(semid, putdownchops, 2) == -1){
					fprintf(stderr, "ERROR: Philosipher %d error dropping sticks. ERRNO: %d , %s \n", i, errno, strerror(errno));
					exit(1);
				}
            }
            //save details about process. print that meal was finished.
            printf("Philosopher %d done with meal (process %d)\n", i, getpid());
            data[i].thinkTimeTotal = thinkTimeTotal;
            data[i].eatTimeTotal = eatTimeTotal;
            data[i].cycles = cycles;
            exit(EXIT_SUCCESS);
}


int main() {

	struct timeval start, end;	
    gettimeofday(&start, NULL);	//get time to display total time ran at end.

	//sem and shared mem IDs and the array for philosopher data.
    int semid, shmid;
    PhilosopherData *data;

	//make sem and get shared memory
	semid = semget(IPC_PRIVATE, PHILOSOPHERS, IPC_CREAT | IPC_EXCL | 0600);
    shmid = shmget(IPC_PRIVATE, PHILOSOPHERS * sizeof(PhilosopherData), 0600 | IPC_CREAT);
    data = (PhilosopherData *)shmat(shmid, NULL, 0);

	// Child process: philosopher start one for each.
    for (int i = 0; i < PHILOSOPHERS; i++) {
        semctl(semid, i, SETVAL, 1);
        if (fork() == 0) { 
            philosopher(i, semid, data);
        }
    }
    
    // waiting for all children to exit
    for (int i = 0; i < PHILOSOPHERS; i++) {
        wait(NULL);
    }

    // printing recap
	gettimeofday(&end, NULL); // Get the time at the end of execution

	printf("-\n");
    for (int i = 0; i < PHILOSOPHERS; i++) {
        printf("Philosopher %d thought for %d seconds, ate for %d seconds over %d cycles.\n", 
               i, data[i].thinkTimeTotal, data[i].eatTimeTotal, data[i].cycles);
    }
	printf("Program took %ld seconds Total to execute \n", end.tv_sec - start.tv_sec);

    // Cleanup semaphores and shared memory
     if (semctl(semid, 0, IPC_RMID) == -1) {
        fprintf(stderr, "ERROR: cleanup semaphore, ERRNO: %d , %s \n", errno, strerror(errno));
        exit(1);
    }
    if (shmctl(shmid, IPC_RMID, NULL) == -1) {
        fprintf(stderr, "ERROR: cleanup shared mem, ERRNO: %d , %s \n", errno, strerror(errno));
        exit(1);
	}

    return 0;
}
