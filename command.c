#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/times.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h> //just for parsing
#include "command.h"
#include "table.h"





// waits for a certain process to finish
void wait_for_pid(int pid){
    int err = waitpid((pid_t)pid, NULL, 0);

    if (err < 0){
        printf("The wait failed");
    }

}

// suspends a certain process
void suspend_pid(int pid){
    kill(pid, SIGTSTP);

    for (int i = 0; i < pcb.active + 1; i++){
        if (pcb.table[i].pid == pid){
            pcb.table[i].status = 'S';
            break;
        }

    }

}

// resumes a certain process
void resume_pid(int pid){
    kill(pid, SIGCONT);

    for (int i = 0; i < pcb.active + 1; i++){
        if (pcb.table[i].pid == pid){
            pcb.table[i].status = 'R';
            break;
        }

    }

}

// Kills any suspended processes and waits for active processes. 
// THIS WILL GET STUCK IF THERE IS A PROCESS IN A INFINITE LOOP!
void exit_shell(){


    for (int i = 0; i < pcb.active + 1; i++){
        if (pcb.table[i].status != 'R'){
            int pid = pcb.table[i].pid;
            kill(pid, SIGKILL);

        }

    }

    // wait for the rest
    while (wait(NULL) != -1){
        continue;
    }

    //get our final time used
    struct rusage useage;
    getrusage(RUSAGE_CHILDREN, &useage);
    int utime  = (int)useage.ru_utime.tv_sec;
    int stime = (int)useage.ru_stime.tv_sec;

    printf("Resources used:\nUser time =     %d seconds\nSys time =     %d seconds\n", utime, stime);
}






