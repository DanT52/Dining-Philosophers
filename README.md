# Dining Philosophers Simulation

This C program simulates the classic dining philosophers problem. It demonstrates the potential issues of concurrent programming, such as deadlocks. The program involves five philosophers sitting around a dining table, thinking and eating sporadically. Each philosopher shares a chopstick with their neighboring philosopher. To eat, a philosopher needs two chopsticks.

## Program Structure

The program consists of multiple parts, explained as follows:

### Dependencies

It includes several libraries such as `stdio.h`, `stdlib.h`, `unistd.h`, etc., required for functionalities like input/output, random numbers, and IPC mechanisms.

### Constants

- `PHILOSOPHERS`: Number of philosophers (set to 5).
- `EATING_TIME`: The eating time limit for the philosophers.

### Structures

- `PhilosopherData`: Structure holding the data for each philosopher, including total thinking time, total eating time, and cycles.

### Functions

- `randomGaussian(int mean, int stddev)`: Generates Gaussian random numbers.
- `pickup(int semid, int i)`: Handles the logic of a philosopher picking up the chopsticks.
- `philosopher(int i, int semid, PhilosopherData *data)`: Main logic of each philosopher’s actions.
- `main()`: Orchestrates the simulation by initializing semaphores, creating child processes for each philosopher, and cleaning up resources at the end.

## Execution Flow

1. Each philosopher, implemented as a child process, alternates between thinking and eating.
2. Philosophers try to pick up the chopsticks. If not available, they wait.
3. Once done eating, philosophers put down the chopsticks.
4. The program continues until each philosopher has eaten for a certain amount of time (`EATING_TIME`).
5. Details about each philosopher, such as total thinking and eating times, are printed out.
6. Total execution time of the program is also displayed.

## Compilation and Execution

Compile the code using a C compiler, like gcc:

```bash
gcc -o philosophers philosophers.c -lm
```