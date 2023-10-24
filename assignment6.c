#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define PHILOSOPHERS 5
#define EATING_TIME 100


typedef struct {
    int thinkTimeTotal;
    int eatTimeTotal;
    int cycles;
} PhilosopherData;

typedef struct {
    PhilosopherData *data;
    int num;
    pthread_mutex_t *mutexes;
} ThreadArgs;

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

//helper function for picking up chopsticks, waits for both to be avalible in a busy loop before picking up.
//takes in the thread args and the philosopher num.
void pickup_chopsticks(ThreadArgs *threadArgs, int num) {
    int first = 1;
    while (1) {
        if (pthread_mutex_trylock(&threadArgs->mutexes[num]) == 0) {
            if (pthread_mutex_trylock(&threadArgs->mutexes[(num+1)%PHILOSOPHERS]) == 0) {
                break;
            }
            pthread_mutex_unlock(&threadArgs->mutexes[num]);
        }
        if (first) {
            printf("Philosopher %d waiting for sticks %d and %d\n", num, num, (num+1) % PHILOSOPHERS);
            first = 0;
        }
    }
}

//philisopher thread function, runs each philosopher through their eating/thinking cycle till they are done.
void *philosopher_thread(void *args){ 
    //take apart the args
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    pthread_mutex_t *mutex = threadArgs->mutexes;
    PhilosopherData *data = threadArgs->data;
    int num = threadArgs->num;

	//seed for the rng
	srand(time(NULL) + num);
    int eatTimeTotal = 0, thinkTimeTotal = 0, cycles = 0;

	while (eatTimeTotal < EATING_TIME) {
                cycles++;
                
                // thinking phase
                int thinkTime = randomGaussian(11, 7);
                if (thinkTime < 0) thinkTime = 0;
                printf("Philosopher %d thinking for %d seconds (total = %d)\n", num, thinkTime, thinkTimeTotal);
				thinkTimeTotal += thinkTime;
                sleep(thinkTime);

				pickup_chopsticks(threadArgs, num); //pickup chops
                
                // eating phase
                int eatTime = randomGaussian(9, 3);
                if (eatTime < 0) eatTime = 0;

                printf("Philosopher %d eating for %d seconds (total = %d)\n", num, eatTime, eatTimeTotal);
                eatTimeTotal += eatTime;
                sleep(eatTime);
                printf("Philosopher %d putting down sticks %d and %d\n", num, num, (num+1) % PHILOSOPHERS);
                pthread_mutex_unlock(&threadArgs->mutexes[num]);
                pthread_mutex_unlock(&threadArgs->mutexes[(num+1)%PHILOSOPHERS]);
                
            }
            //save details about process. print that meal was finished.
            printf("Philosopher %d done with meal\n", num);
            data->thinkTimeTotal = thinkTimeTotal;
            data->eatTimeTotal = eatTimeTotal;
            data->cycles = cycles;
}

//initialize and return array of mutexes with one for each chopstick
pthread_mutex_t* init_mutexes(){
    pthread_mutex_t *mutexes = malloc(PHILOSOPHERS * sizeof(pthread_mutex_t));
    if (!mutexes) {
        fprintf(stderr, "ERROR: Failed to allocate memory for mutexes '%s' ERRNO: %d", strerror(errno), errno );
        exit(1);
    }

    for(int i = 0; i < PHILOSOPHERS; i++){
        if (pthread_mutex_init(&mutexes[i], NULL)){
            fprintf(stderr, "ERROR: Failed init mutexes '%s' ERRNO: %d", strerror(errno), errno );
            exit(1);
        }
    }
    return mutexes;
}

//free mutexes
void end_mutexes(pthread_mutex_t *mutexes){
    for(int i = 0; i < PHILOSOPHERS; i++){
        if (pthread_mutex_destroy(&mutexes[i]))fprintf(stderr, "ERROR: Failed destroy mutexes '%s' ERRNO: %d", strerror(errno), errno );
    }
    free(mutexes);
}

// Function for running philosophers cycle
//creates thread for each philosopher that runs till they finish eating
// args: pthread array (one for each philosopher), mutex array with a mutex for each chopstick
//returns data array with data about the simulation.
PhilosopherData* run_philosophers(pthread_t *philos_thread, pthread_mutex_t *mutexes){

    PhilosopherData *data = malloc(PHILOSOPHERS * sizeof(PhilosopherData));
    if (!data){
            fprintf(stderr, "ERROR: failed to allocate data '%s' ERRNO: %d", strerror(errno), errno );
            exit(1);
    }
    ThreadArgs *threadArgs[PHILOSOPHERS];

    for (int i = 0; i< PHILOSOPHERS; i++){ //create thread for each philosopher
        threadArgs[i] = malloc(sizeof(ThreadArgs));
        if (!threadArgs[i]){
            fprintf(stderr, "ERROR: failed to allocate args '%s' ERRNO: %d", strerror(errno), errno );
            exit(1);
        }
        threadArgs[i]->mutexes = mutexes;
        threadArgs[i]->data = &data[i];
        threadArgs[i]->num =i; //philosopher number
        int result = pthread_create(&philos_thread[i], NULL, philosopher_thread, (void*)threadArgs[i]); 
    }

    for(int i = 0; i < PHILOSOPHERS; i++){ //wait for philosophers to finish
        if (pthread_join(philos_thread[i], NULL) != 0){
            fprintf(stderr, "ERROR: pthread_join failed '%s' ERRNO: %d", strerror(errno), errno );
            exit(1);
        }
        free(threadArgs[i]); 
    }
    return data;
}

int main(){

    struct timeval start, end;	
    gettimeofday(&start, NULL);	//get time to display total time ran at end.

    pthread_t philos_thread[PHILOSOPHERS];
    pthread_mutex_t *mutexes = init_mutexes();
    PhilosopherData *data = run_philosophers(philos_thread, mutexes);

    //print results
    printf("-\n");
    for(int i = 0; i < PHILOSOPHERS; i++){
        printf("Philosopher %d thought for %d seconds, ate for %d seconds over %d cycles.\n", 
               i, data[i].thinkTimeTotal, data[i].eatTimeTotal, data[i].cycles);
    }
    gettimeofday(&end, NULL);
    long int timePassed = end.tv_sec - start.tv_sec;
    printf("Program took %ld seconds Total to execute \n", timePassed);

    end_mutexes(mutexes);
    free(data);

}