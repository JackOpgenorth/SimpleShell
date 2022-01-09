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
#include <string.h> //just for parsing
#include "command.h"
#include "table.h"



table pcb; // Since we need to update the Proccess control block (pcb) whenever we get a signal it (sadly) must be gobal.


//Executes any generic unix command. Also accounts for the <, >, and &, operators

void execute_command(char command[]){

    int f;
    int write_to_file = 0; 
    int read_from_file = 0; 
    table_entry new; // our new process, soon to go into the table
    strcpy(new.command, command); // need to clone this for parseing
    int do_not_add = 0;
    char * args[50];
    int shouldwait = 1;

    const char sep1[2] = " ";
    const char sep2[2] = "<";
    const char sep3[2] = ">";

    char *token;// for parsing

    char new_comm[LINE_LENGTH];
    char write_to[LINE_LENGTH];
    char read_from[LINE_LENGTH];

    
    if (strchr(command, '>') != NULL){
        // parse to get our command we will be running and the file we write it to
        write_to_file = 1;
        token = strtok(command, sep3);
        strcpy(new_comm, token);
        token = strtok(NULL, sep3);
        strcpy(write_to, token);
        command = new_comm;
    }
    
    if (strchr(command, '<') != NULL){
        // parse to get our command we will be running and the file we read from
        read_from_file = 1;
        token = strtok(command, sep2);
        strcpy(new_comm, token);
        token = strtok(NULL, sep2);
        strcpy(read_from, token);
        command = new_comm;
    }
    

    // now we parse the command into arguments
    token = strtok(command, sep1);
    int i = 0;
    while (token != NULL){
        
        if (!strcmp("&", token)){
            shouldwait = 0;
        }

        args[i] = token;
        token = strtok(NULL, sep1);
        i++;
    }
    args[i] = 0x0; // just so if no args were entered we don't try and reference somwhere we shouldn't and segfault


    if (!shouldwait){
        args[i - 1] = NULL; // Since fork is already running in the background the & is irrelevent now and seems to be causing problems
    }


    int cid = fork();
    if (cid < 0){
        perror(cid);
    }else if (cid == 0){

        //new child process

        // if we write to a new file just change stdout to the file
        if (write_to_file){
            int fwrite = open(write_to, O_CREAT | O_WRONLY, 0666);
            dup2(fwrite, STDOUT_FILENO);
            close(fwrite);
        }
        // if we read from a file, just change stdin to the file
        if (read_from_file){
            int fread = open(read_from, O_CREAT | O_RDONLY, 0644);
            dup2(fread, STDIN_FILENO);
        }

        if (execvp(args[0], args) < 0){
            perror("exec failed:\n");
            do_not_add = 1;
            exit(0);
            
        }   
    }else if (!do_not_add){
        

        // we want to make sure the child executed correctly before we add

        FILE * pcheck;
        char check[LINE_LENGTH];
        char buff2[LINE_LENGTH];
        check[0] = '\0';
        buff2[0] = '\0';
        
        snprintf(check, LINE_LENGTH, "ps -p %d | grep defunct", cid);
        pcheck = popen(check, "r");

        fgets(buff2, LINE_LENGTH, pcheck);

        if (buff2[0] != '\0'){
            return; // lets not bother waiting for or adding somthing which did not execute
        }

        new.num = pcb.active + 2;
        new.status = 'R';
        new.time = 0;
        new.pid = cid;

        int size = pcb.active;
        pcb.table[size + 1] = new;
        pcb.active += 1;


        if (shouldwait){
            while(wait(NULL) != cid){
                sleep(1);
            }
        }
        
    }
}



//This is a signal handler for deleting child processes from the Process table when they terminate
// It will be called whenever we receive the SIGCHLD signal
void delete_process_async(int signum, siginfo_t *siginf, void *ucontext){
    
    pid_t chld_pid = siginf->si_pid;
    
    int status = siginf->si_status;

    // We only care if the child was killed, terminated, or failed to execute its command
    if (status != 9 && status != 0 && status != 2){
        return;
    }

    //Simply copy all but the deleted process to a new table
    table new_t;
    int i = 0;
    int is_leading = 0; // a boolean
    int removed_process = 0; // a boolean

    int current_num = 0;
    while(i < pcb.active + 1){
        
        if (pcb.table[i].pid == (int)chld_pid){ // found the deleted process
            new_t.table[i] = pcb.table[i + 1];
            i += 2;
            is_leading = 1;
            removed_process = 1;
        }

        else{

            if (is_leading){// if we found the deleted process before, our indexing will be a bit different
                new_t.table[i - 1] = pcb.table[i];
               
            }else{
                new_t.table[i] = pcb.table[i];
            
            }
            i++;
            new_t.table[i].num = current_num;
            current_num++;
        }
        

    }

    if (removed_process){
        new_t.active = pcb.active - 1;
        pcb = new_t;
    }
    
    return;

}



int main(){

    // get our signal handler ready
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = delete_process_async; 
    signal(SIGCHLD, delete_process_async);
    sigaction(SIGCHLD, &sa, NULL);
    
    pcb.active = -1; // -1 means no active processes
    char input[LINE_LENGTH];
    char parse[LINE_LENGTH];

    char * comm; // command
    char * args;
    
    const char* welcome = "SHELL379: ";
    int dont_welcome = 0;
    while(1){

        //printf("Active processes: %d\n", pcb.active + 1);
        

        if (dont_welcome){
            dont_welcome = 0;
        }
        else{
            printf("%s", welcome);
        }
        
        char * err = fgets(input, LINE_LENGTH, stdin); // I realize gets() is unsecure but fgets was constantly segfaulting for some reason.

        if (err == NULL){ // somthing went wrong during input, just return to FGETS like nothing happened
            dont_welcome = 1;
            continue;
        }
        
        // having newlines in the input messes everything up, so just getting rid of them here
        for (int i = 0; i < LINE_LENGTH; i++){
            if ((char)input[i] ==  '\n'){
                input[i] = '\0';
                break;
            }
        }


        strcpy(parse, input);
        comm = strtok(parse, " ");
        args = strtok(NULL, " ");
        int id;

        if (parse[0] == '\0'){
            continue;
        }


        if (!strcmp(comm, "jobs")){
            print_table();
        }else if (!strcmp(comm, "kill"))
        {   
            sscanf(args, "%d", &id);
            kill(id, SIGKILL);
        }else if (!strcmp(comm, "resume"))
        {
            sscanf(args, "%d", &id);
            resume_pid(id);
        }else if (!strcmp(comm, "suspend"))
        {
            sscanf(args, "%d", &id);
            suspend_pid(id);
        }else if (!strcmp(comm, "wait"))
        {
            sscanf(args, "%d", &id);
            wait_for_pid(id);
        }else if (!strcmp(comm, "sleep")){
            sscanf(args, "%d", &id);
            sleep(id);
        }
        else if (!strcmp(comm, "exit")){
            exit_shell();
            break;
        }
        else
        {
            execute_command(input);
        }
        
        
        
        
        


    }

}