#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"
int redLightGreenLight(){
    int semID;
    int pid;
    struct sembuf canIGo[1] = {{0, -1, 0}};
    struct sembuf iAmDoneRunning[1] = {{0, 1, 0}};
    struct sembuf redLight[1] = {{0, -1, 0}};
    struct sembuf greenLight[1] = {{0, 1, 0}};
    struct sembuf start[1] = {{0, 5, 0}};
    semID = semget(IPC_PRIVATE, 5, IPC_CREAT | IPC_EXCL | 0600);
    
    pid = fork();
    if(pid){
        //parent is player
        while(1){
        semop(semID, canIGo, 1);
        printf(KNRM "Running...\n");
        semop(semID, iAmDoneRunning, 1);
        sleep(1);
        }
    }
    else{
        //child is controller
        while(1){
        sleep(rand() % 5);
        printf(KGRN "Green Light!!!\n");
        semop(semID, greenLight, 1);
        sleep(rand() % 5);
        semop(semID, redLight, 1);
        printf(KRED "Red Light!!\n");
        }
    }
}

int main(int argc, char const *argv[]){
    redLightGreenLight();
    return 0;
}