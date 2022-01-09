#include <stdio.h>
#include "table.h"
#include <sys/resource.h>
#include <fcntl.h>


table pcb; // Since we need to update the Proccess control block (pcb) whenever we get a signal it (sadly) must be gobal.


void print_entry(table_entry t){
    printf(" %d: %d %c  %d %s\n", t.num, t.pid, t.status, t.time, t.command);

}


// Prints the process table in the desired format
void print_table(){
    
    printf("%s", "\n");

    printf("Running Processes:\n #    PID S SEC COMMAND\n");

    FILE * p; //pipe

    // Need to get the time of each process using a pipe
    char time_command[LINE_LENGTH]; // This is the command we will be using the pipe for
    char buff[LINE_LENGTH];
    int pid;
    for (int i = 0; i < pcb.active + 1; i++){
        pid = pcb.table[i].pid;
        time_command[0] = '\0';
        snprintf(time_command, LINE_LENGTH + 1, "ps -p %d -o times", pid);
        p = popen(time_command, "r");
        // Need to run fgets twice to skip over the newline
        fgets(buff, LINE_LENGTH, p);
        fgets(buff, LINE_LENGTH, p);
        pclose(p);
        
        // Need to trim off the leading whitespace before the time.
        int j = 0;
        int k = 0;
        char t_trimmed[10]; // 10 is arbitrary,just want to hold a large enough number

        //Theres probably a more efficient way to trim this string but I really don't want to spend too much time on this part
        while(buff[j] != '\n'){
            
            if (buff[j] != ' '){
                t_trimmed[k] = buff[j];
                k++;
            }
            j++; 
        }
        int time;
        sscanf(t_trimmed, "%d", &time);

        pcb.table[i].time = time;

        print_entry(pcb.table[i]);
    }


    //will be using getrusage() to get the time
    printf("processes =   %d active\n", pcb.active + 1);

    printf("Completed processes:\n");


    struct rusage useage;
    getrusage(RUSAGE_CHILDREN, &useage);
    int utime  = (int)useage.ru_utime.tv_sec;
    int stime = (int)useage.ru_stime.tv_sec;

    printf("User time =      %d seconds\n", utime);
    printf("System time =      %d seconds\n", stime);

    printf("%s", "\n");

}

