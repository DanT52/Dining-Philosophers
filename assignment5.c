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

void philosopher(int i, int semid, PhilosopherData *data){
	struct sembuf putdownforks[2] = {{i,1,0}, {(i+1)%PHILOSOPHERS,1,0}};
	struct sembuf pickupforks_nowait[2] = {{i,-1,IPC_NOWAIT}, {(i+1)%PHILOSOPHERS,-1,IPC_NOWAIT}};
	struct sembuf pickupforks[2] = {{i,-1,0}, {(i+1)%PHILOSOPHERS,-1,0}};


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
				if (semop(semid, pickupforks_nowait, 2) == -1) {
					if (errno == EAGAIN) {

						printf("Philosopher %d waiting for sticks %d and %d.\n", i, i, (i+1)%PHILOSOPHERS);
						semop(semid, pickupforks, 2);


						// continue; // Go back to thinking if both forks are not available

					} else {
						perror("semop");
						exit(EXIT_FAILURE);
					}
        		}
                
                // Eating
                int eatTime = randomGaussian(9, 3);
                if (eatTime < 0) eatTime = 0;

                printf("Philosopher %d eating for %d seconds (total = %d)\n", i, eatTime, eatTimeTotal);
				eatTimeTotal += eatTime;
                sleep(eatTime);
				printf("Philosopher %d putting down sticks %d and %d\n", i, i, (i+1)%PHILOSOPHERS);
				semop(semid, putdownforks, 2);
            }
            
            printf("Philosopher %d done with meal (process %d)\n", i, getpid());
            data[i].thinkTimeTotal = thinkTimeTotal;
            data[i].eatTimeTotal = eatTimeTotal;
            data[i].cycles = cycles;
            exit(EXIT_SUCCESS);

}


int main() {

	struct timeval start, end;
    gettimeofday(&start, NULL);
    int semid, shmid;
    PhilosopherData *data;
	semid = semget(IPC_PRIVATE, PHILOSOPHERS, IPC_CREAT | IPC_EXCL | 0666);
    shmid = shmget(IPC_PRIVATE, PHILOSOPHERS * sizeof(PhilosopherData), 0666 | IPC_CREAT);
    data = (PhilosopherData *)shmat(shmid, NULL, 0);

    for (int i = 0; i < PHILOSOPHERS; i++) {
        semctl(semid, i, SETVAL, 1);
        if (fork() == 0) { // Child process: philosopher

            philosopher(i, semid, data);
        }
    }
    
    // Waiting for all children to exit
    for (int i = 0; i < PHILOSOPHERS; i++) {
        wait(NULL);
    }

    // Printing recap

	gettimeofday(&end, NULL); // Get the time at the end of execution

    long seconds = (end.tv_sec - start.tv_sec);
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

	printf("-\n");
    for (int i = 0; i < PHILOSOPHERS; i++) {
        printf("Philosopher %d thought for %d seconds, ate for %d seconds over %d cycles.\n", 
               i, data[i].thinkTimeTotal, data[i].eatTimeTotal, data[i].cycles);
    }

	printf("Program took %ld seconds Total to execute \n", seconds);

    // Cleanup semaphores and shared memory
    semctl(semid, 0, IPC_RMID);
    shmctl(shmid, IPC_RMID, NULL);
    
    return 0;
}
