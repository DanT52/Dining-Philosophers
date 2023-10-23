#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

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



void *printMessage(void *message) {
    printf("%s\n", (char *)message);
    return NULL;
}


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
                int thinkTime = 0; //randomGaussian(11, 7);
                if (thinkTime < 0) thinkTime = 0;
                printf("Philosopher %d thinking for %d seconds (total = %d)\n", num, thinkTime, thinkTimeTotal);
				thinkTimeTotal += thinkTime;
                sleep(thinkTime);

				// trying to pick up chopsticks
                int first = 1;
                while(1){
                    if(pthread_mutex_trylock(&threadArgs->mutexes[num]) == 0){
                        if (pthread_mutex_trylock(&threadArgs->mutexes[(num+1)%PHILOSOPHERS]) == 0){
                            break;
                        }
                        pthread_mutex_unlock(&threadArgs->mutexes[num]);
                    }
                    if (first){
                        printf("Philosopher %d waiting for sticks %d and %d\n", num, num, (num+1) % PHILOSOPHERS);
                        first = 0;
                    }
                    
                }
                
                // eating phase
                int eatTime = 0;//randomGaussian(9, 3);
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

void init_mutexes(pthread_mutex_t *mutexes){
    for(int i = 0; i < PHILOSOPHERS; i++){
        pthread_mutex_init(&mutexes[i], NULL);
    }
}

void end_mutexes(pthread_mutex_t *mutexes){
    for(int i = 0; i < PHILOSOPHERS; i++){
        pthread_mutex_destroy(&mutexes[i]);

    }
    free(mutexes);
}

int main(){

    struct timeval start, end;	
    gettimeofday(&start, NULL);	//get time to display total time ran at end.

    pthread_t philos_thread[PHILOSOPHERS];

    //initialize mutexes
    pthread_mutex_t *mutexes = malloc(PHILOSOPHERS * sizeof(pthread_mutex_t));
    init_mutexes(mutexes);

    //allocate philosopher data
    PhilosopherData *data = malloc(PHILOSOPHERS * sizeof(PhilosopherData));

    ThreadArgs *threadArgs[PHILOSOPHERS];

    for (int i = 0; i< PHILOSOPHERS; i++){
        threadArgs[i] = malloc(sizeof(ThreadArgs));
        threadArgs[i]->mutexes = mutexes;
        threadArgs[i]->data = &data[i];
        threadArgs[i]->num =i;

        int result = pthread_create(&philos_thread[i], NULL, philosopher_thread, (void*)threadArgs[i]);

    }

    for(int i = 0; i < PHILOSOPHERS; i++){
        pthread_join(philos_thread[i], NULL);
    }


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