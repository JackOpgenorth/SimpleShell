#ifndef SHELL_H
#define SHELL_H



#define LINE_LENGTH 100
#define MAX_ARGS 7
#define MAX_LENGTH 20
#define MAX_PT_ENTRIES 32


typedef struct table_entry{
    int num;
    int pid;
    int time;
    char command[LINE_LENGTH];
    char status;

} table_entry;

typedef struct lookup_table{
    table_entry table[MAX_PT_ENTRIES];
    int active;
    int completed;

}table;



table pcb;// process table needs to be global for the signal handler

void print_entry(table_entry t);

void print_table();


#endif