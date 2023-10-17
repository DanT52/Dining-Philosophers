Acquire the semaphore for the fork to their left.
Acquire the semaphore for the fork to their right.
Eat.
Release the semaphore for the fork to their right.
Release the semaphore for the fork to their left.

process P[i]
 while true do
   {  THINK;
      PICKUP(CHOPSTICK[i], CHOPSTICK[i+1 mod 5]);
      EAT;
      PUTDOWN(CHOPSTICK[i], CHOPSTICK[i+1 mod 5])
   }


semID = semget(IPC_PRIVATE, 5, IPC_CREAT | IPC_EXCL | 0600);

struct sembuf youdkwtis[2] = {{0,1,0}, {1,1,0}};
