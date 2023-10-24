#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define PHILOSOPHERS 5
#define EATING_TIME 10


typedef struct {
    int thinkTimeTotal;
    int eatTimeTotal;
    int cycles;
} PhilosopherData;

typedef struct {
    PhilosopherData *data;
    int num;
    pthread_mutex_t *mutexes;
    pthread_cond_t *cond_variables;
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
//function for picking up chopsticks
void pickup_chopsticks(ThreadArgs *threadArgs, int num){ 
    pthread_mutex_t *mutex = threadArgs->mutexes;   //get needed vars out of threadargs
    pthread_cond_t *cond_vars = threadArgs->cond_variables;
    int right = num;
    int left = (num+1)%PHILOSOPHERS;
    
    //attempt to lock right and left, 
    int result_right = pthread_mutex_trylock(&threadArgs->mutexes[right]);
    if (result_right != 0 && result_right != EBUSY) fprintf(stderr, "ERROR: Failed to trylock right mutex. STRERR: %s, ERRNO: %d \n", strerror(errno), errno);
    int result_left = pthread_mutex_trylock(&threadArgs->mutexes[left]);
    if (result_left != 0 && result_left != EBUSY) fprintf(stderr, "ERROR: Failed to trylock left mutex. STRERR: %s, ERRNO: %d \n", strerror(errno), errno);

    if (!(result_right == 0 && result_left == 0)){ //if cannot lock both print that we are waiting
        if (!result_right && pthread_mutex_unlock(&threadArgs->mutexes[right]) != 0) fprintf(stderr, "ERROR: Failed to unlock right mutex. STRERR: %s, ERRNO: %d \n", strerror(errno), errno);
        if (!result_left && pthread_mutex_unlock(&threadArgs->mutexes[left]) != 0) fprintf(stderr, "ERROR: Failed to unlock left mutex. STRERR: %s, ERRNO: %d \n", strerror(errno), errno);
        printf("Philosopher %d waiting for sticks %d and %d\n", num, right, left);
        
        //do a conditional wait loop which ensures that both chopsticks can be picked up by current philosopher
        if (pthread_mutex_lock(&mutex[num]) != 0) fprintf(stderr, "ERROR: Failed to lock mutex. STRERR: %s, ERRNO: %d \n", strerror(errno), errno); 
        while (pthread_mutex_trylock(&mutex[left]) != 0) {
            if (pthread_cond_wait(&cond_vars[right], &mutex[right]) != 0) fprintf(stderr, "ERROR: Failed to wait on conditional variable. STRERR: %s, ERRNO: %d \n", strerror(errno), errno);
        }
    }
}
//function for putting down chopsticks, unlocking mutexes, signaling neibors.
void putdown_chopsticks(pthread_mutex_t *mutex, pthread_cond_t *cond_vars, int num, int left, int right){
    printf("Philosopher %d putting down sticks %d and %d\n", num, num, (num+1) % PHILOSOPHERS);
    if (pthread_mutex_unlock(&mutex[right]) != 0)fprintf(stderr, "ERROR: Failed to unlock right mutex. STRERR: %s, ERRNO: %d \n", strerror(errno), errno);
    if (pthread_mutex_unlock(&mutex[left]) != 0) fprintf(stderr, "ERROR: Failed to unlock left mutex. STRERR: %s, ERRNO: %d \n", strerror(errno), errno);
    if (pthread_cond_signal(&cond_vars[(num+1)%PHILOSOPHERS]) != 0) fprintf(stderr, "ERROR: Failed to signal right conditional variable. STRERR: %s, ERRNO: %d \n", strerror(errno), errno);
    if (pthread_cond_signal(&cond_vars[(num-1+PHILOSOPHERS)%PHILOSOPHERS]) != 0) fprintf(stderr, "ERROR: Failed to signal left conditional variable. STRERR: %s, ERRNO: %d \n", strerror(errno), errno);
}

void *philosopher_thread(void *args){
    //take apart the args
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    pthread_mutex_t *mutex = threadArgs->mutexes;
    pthread_cond_t *cond_vars = threadArgs->cond_variables;
    PhilosopherData *data = threadArgs->data;
    int num = threadArgs->num;
    int right = num, left = (num+1)%PHILOSOPHERS;
    
	//seed for the rng
	srand(time(NULL) * num);
    int eatTimeTotal = 0, thinkTimeTotal = 0, cycles = 0;

	while (eatTimeTotal < EATING_TIME) {
                // thinking phase
                int thinkTime =  randomGaussian(11, 7);
                if (thinkTime < 0) thinkTime = 0;
                printf("Philosopher %d thinking for %d seconds (total = %d)\n", num, thinkTime, thinkTimeTotal);
				thinkTimeTotal += thinkTime;
                sleep(thinkTime);

                pickup_chopsticks(threadArgs, num); //pickup chopsticks

                // eating phase
                int eatTime = randomGaussian(9, 3);
                if (eatTime < 0) eatTime = 0;
                printf("Philosopher %d eating for %d seconds (total = %d)\n", num, eatTime, eatTimeTotal);
                eatTimeTotal += eatTime;
                sleep(eatTime);

                putdown_chopsticks(mutex, cond_vars, num, left, right); //puts chopsticks down
                cycles++;
            }
            printf("Philosopher %d done with meal\n", num);  //save details about process. print that meal was finished.
            data->thinkTimeTotal = thinkTimeTotal;
            data->eatTimeTotal = eatTimeTotal;
            data->cycles = cycles;
}

pthread_mutex_t* init_mutexes(){ //function to initialize mutexes array.
    pthread_mutex_t *mutexes = malloc(PHILOSOPHERS * sizeof(pthread_mutex_t));
    if (mutexes == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory for mutexes. STRERR: %s, ERRNO: %d \n", strerror(errno), errno);
        exit(1);
    }
    for(int i = 0; i < PHILOSOPHERS; i++){
        int result = pthread_mutex_init(&mutexes[i], NULL);
        if (result != 0) {
            fprintf(stderr, "ERROR: Failed to initialize mutex %d. STRERR: %s, ERRNO: %d \n", i, strerror(result), result);
            exit(1);
        }
    }
    return mutexes;
}

pthread_cond_t* init_conds(){ //function to initialize conditionals
    pthread_cond_t *cond_vars = malloc(PHILOSOPHERS * sizeof(pthread_cond_t));
    if (cond_vars == NULL) {
        fprintf(stderr, "ERROR: Failed to allocate memory for conditional variables. STRERR: %s, ERRNO: %d \n", strerror(errno), errno);
        exit(1);
    }

    for(int i = 0; i < PHILOSOPHERS; i++){
        int result = pthread_cond_init(&cond_vars[i], NULL);
        if (result != 0) {
            fprintf(stderr, "ERROR: Failed to initialize conditional variable %d. STRERR: %s, ERRNO: %d \n", i, strerror(result), result);
            exit(1);
        }
    }
    return cond_vars;
}

void end_mutexes_conds(pthread_mutex_t *mutexes, pthread_cond_t *cond_vars){ //function to end mutexes and destroy conditionals
    for(int i = 0; i < PHILOSOPHERS; i++){
        if (pthread_mutex_destroy(&mutexes[i]) != 0) {
            fprintf(stderr, "ERROR: Failed to destroy mutex %d. STRERR: %s, ERRNO: %d \n", i, strerror(errno), errno);
        }
        if (pthread_cond_destroy(&cond_vars[i]) != 0) {
            fprintf(stderr, "ERROR: Failed to destroy conditional variable %d. STRERR: %s, ERRNO: %d \n", i, strerror(errno), errno);
        }
    }
    free(mutexes);
    free(cond_vars);
}

PhilosopherData* run_philosophers(pthread_t *philos_thread, pthread_mutex_t *mutexes, pthread_cond_t *cond_vars){

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
        } //store the args into their struct
        threadArgs[i]->mutexes = mutexes;
        threadArgs[i]->cond_variables = cond_vars;
        threadArgs[i]->data = &data[i];
        threadArgs[i]->num =i; //philosopher number
        pthread_create(&philos_thread[i], NULL, philosopher_thread, (void*)threadArgs[i]);
    }

    for(int i = 0; i < PHILOSOPHERS; i++){ //wait for philosophers to finish
        if (pthread_join(philos_thread[i], NULL) != 0){
            fprintf(stderr, "ERROR: pthread_join failed '%s' ERRNO: %d", strerror(errno), errno );
            exit(1);
        }
        free(threadArgs[i]); //free the args.
    }
    return data;
}

int main(){

    struct timeval start, end;	
    gettimeofday(&start, NULL);	//get time to display total time ran at end.
    //initialze the pthreads mutexes and coditionals
    pthread_t philos_thread[PHILOSOPHERS];
    pthread_mutex_t *mutexes = init_mutexes();
    pthread_cond_t *cond_vars = init_conds();
    PhilosopherData *data = run_philosophers(philos_thread, mutexes, cond_vars);  //run philosophers
    
    //print results
    printf("-\n");
    for(int i = 0; i < PHILOSOPHERS; i++){
        printf("Philosopher %d thought for %d seconds, ate for %d seconds over %d cycles.\n", 
               i, data[i].thinkTimeTotal, data[i].eatTimeTotal, data[i].cycles);
    }
    gettimeofday(&end, NULL); //get end time
    long int timePassed = end.tv_sec - start.tv_sec;
    printf("Program took %ld seconds Total to execute \n", timePassed);
    //clean up
    end_mutexes_conds(mutexes, cond_vars); 
    free(data);
    return 0;
}