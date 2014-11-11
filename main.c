#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Constants */
#define READY 0
#define RUNNING 1
#define WAITING 2
#define FINISHED 4

#define NEW 10
#define INP 11
#define CPU 12
#define SSD 13

#define NULL_VALUE 256

/** Global variables */
int process_ctr = 0;
int pid_ctr = 0;
int global_time = 0;
int finished_processes = 0;
int next_new_process = 0;

/** Command struct */
struct command {
    int name;
    int time;
};

/** Process struct */
struct process {
    int pid;
    int state;
    int command_index;
    int total_commands;
    int cpu_usage_time;
    int ssd_access_counter;
    int ssd_usage_time;
    int ssd_wait_time;
    int ssd_entry_time;
    int io_entry_time;
    struct command commands[150];
};

/** CPU and SSD struct */
struct hardware {
    int pid;
    int busy; // If the hardware is being used or not.
    int finish_time; // The global time when the process will be done using that hardware. If start time of process is 120 and it uses the CPU for 15, the finish_time should be 135.
};

/** Queue */
struct queue {
    int pid[26];
    int nextEmpty;
    int busy;
};


// Enqueue a process to the queue.
struct queue enqueue(struct queue q, int pid)
{
    struct queue new_queue = q;

    // Add process
    new_queue.pid[new_queue.nextEmpty] = pid;

    // Increment
    new_queue.nextEmpty = new_queue.nextEmpty + 1;

    // Make it busy
    new_queue.busy = 1;

    return new_queue;
};

// Dequeue and returns the process at position 0 from the queue.
int dequeue(struct queue *q)
{
    // Decrement
    q->nextEmpty = q->nextEmpty - 1;

    // Store the process in a variable
    int pid = q->pid[0];

    // Shifting processes to the beginning of the queue.
    int i;

    for(i = 0; i < 24; i++) {
        q->pid[i] = q->pid[i+1];
    }

    if(q->pid[0] == NULL_VALUE) {
        q->busy = 0;
    }

    return pid;
}

// Puts the process in the IO priority queue
struct queue move_to_WAITING_INPUT(struct queue io, int pid, struct process process_table[])
{
    process_table[pid].io_entry_time = global_time;

    struct queue new_queue = io;
    // Set state to waiting
    int i = 0;

    while(process_table[i].pid != pid) {
        i++;
    }

    process_table[i].state = WAITING;

    // Add process
    new_queue.pid[new_queue.nextEmpty] = pid;

    // Increment
    new_queue.nextEmpty = new_queue.nextEmpty + 1;

    // Sort by IO duration
    int j, process_id;
    //struct process *p;

    for (i = 0; i < new_queue.nextEmpty; ++i)
    {
        for (j = i + 1; j < new_queue.nextEmpty; ++j)
        {
            if (process_table[i].commands->time > process_table[j].commands->time)
            {
                process_id =  new_queue.pid[i];
                new_queue.pid[i] = new_queue.pid[j];
                new_queue.pid[j] = process_id;
            }
        }
    }
    new_queue.busy = 1;
    return new_queue;

}

void clear_hardware(struct hardware *hw)
{
    hw->pid = -1;
    hw->busy = 0;
    hw->finish_time = -1;
}


//this removes process from the hardware and returns the pid
int get_process_from_hardware(struct hardware *hw, struct process process_table[])
{
    //have to store in local variable or it will go away
    int hw_pid = hw->pid;

    clear_hardware(hw);

//    if(process_table[hw->pid].command_index == process_table[hw->pid].total_commands) {
//        process_table[hw->pid].state = FINISHED;
//        finished_processes++;
//    }

    return hw_pid;
}

//this increments to the next command and then returns the next command
void execute_next_command(int pid, struct process process_table[], struct queue *io_q, struct queue *ssd_q, struct queue *rdy_q)
{
    process_table[pid].command_index++;

    if(process_table[pid].command_index==process_table[pid].total_commands) {
        process_table[pid].state = FINISHED;
        finished_processes++;

    } else {
        if(process_table[pid].commands[process_table[pid].command_index].name == CPU) {
            *rdy_q = enqueue(*rdy_q, pid);
        } else if(process_table[pid].commands[process_table[pid].command_index].name == SSD) {
            *ssd_q = enqueue(*ssd_q, pid);
            process_table[pid].state = WAITING;
            process_table[pid].ssd_entry_time = global_time;
        } else if(process_table[pid].commands[process_table[pid].command_index].name == INP) {
            *io_q = move_to_WAITING_INPUT(*io_q, pid, process_table);

        }
    }
}

// Method that creates and returns a process struct
struct process create_new_process()
{
    // Create a process struct
    struct process pcs;

    // Initialize the variables in that process
    pcs.pid = pid_ctr;
    pcs.state = READY;
    pcs.command_index = 0;
    pcs.total_commands = 0;
    pcs.ssd_access_counter = 0;
    pcs.cpu_usage_time = 0;
    pcs.ssd_usage_time = 0;
    pcs.ssd_wait_time = 0;
    pcs.ssd_entry_time = 0;
    pcs.io_entry_time = 0;
    // Return the struct
    return pcs;
}

// Method that creates a command
struct command create_command(int name, int time)
{
    // Create command struct
    struct command cmd;

    // Initialize variables
    cmd.name = name;
    cmd.time = time;

    // Return struct
    return cmd;
}

// Reads the input file and creates a process.
void read_commands(char *commands, struct process process_table[])
{
    FILE * filePointer; // File pointer
    filePointer = fopen(commands, "r"); // Opens the filename pointed to by commands using the given mode.
    char line[128]; // Variable that holds each line that is read
    char command[10]; // Command
    int time; // Time or start time of command

    // Run while fscanf successfully reads in 2 variables and store the first part as
    // a string and the second as an int
    while(fscanf(filePointer, "%s %d", command, &time) == 2) {
        int cmd_name;

        // Parsing the commands
        if (strcmp(command,"NEW")==0) {
            // Create process
            process_table[process_ctr] = create_new_process();
            cmd_name = NEW;

            // Increment process id
            pid_ctr++;

            // Increment process counter
            process_ctr++;
        } else if(strcmp(command,"CPU")==0) {
            // Access CPU
            cmd_name = CPU;

        } else if(strcmp(command,"INP")==0) {
            // Access INP
            cmd_name = INP;

        } else if(strcmp(command,"SSD")==0) {
            // Access SSD
            cmd_name = SSD;

        }

        // Access the process table and get the created process. Access the commands array of that
        // process at position of the command index. Create a command in that position.
        process_table[process_ctr-1].commands[process_table[process_ctr-1].command_index] = create_command(cmd_name, time);

        // Increment the command index
        process_table[process_ctr-1].command_index++;
        process_table[process_ctr-1].total_commands++;
    }

    // Close the file
    fclose(filePointer);
}


// Main function
int main (int argc, char *argv[])
{
    char *input = "input.txt"; // File containing the commands

    // Initialize Process Table
    struct process process_table[25];

    // Initialize CPU 1
    struct hardware cpu1;
    cpu1.busy = 0;

    // Initialize CPU 2
    struct hardware cpu2;
    cpu2.busy = 0;

    // Initialize SSD
    struct hardware ssd;
    ssd.busy = 0;

    // Initialize IO priority queue
    struct queue io_q;
    io_q.nextEmpty = 0;
    io_q.busy = 0;

    // Setting NULL values
    int x;

    for(x = 0; x < 26; x++) {
        io_q.pid[x] = NULL_VALUE;
    }

    // Initialize Ready queue.
    struct queue ready_q;
    ready_q.nextEmpty = 0;
    ready_q.busy = 0;

    // Setting NULL values
    for(x = 0; x < 26; x++) {
        ready_q.pid[x] = NULL_VALUE;
    }

    // Initialize SSD queue.
    struct queue ssd_q;
    ssd_q.nextEmpty = 0;
    ssd_q.busy = 0;

    // Setting NULL values
    for(x = 0; x < 26; x++) {
        ssd_q.pid[x] = NULL_VALUE;
    }


    // Read the file with the commands.
    read_commands(input, process_table);

    // Testing code to check if the process were created correctly
    int i, j;

    for(i=0;i<process_ctr;i++) {

        printf("PID %d\n", process_table[i].pid);
        printf("STATE %d\n", process_table[i].state);
        printf("COMMAND_INDEX %d\n", process_table[i].command_index);
        printf("TOTAL_COMMANDS %d\n", process_table[i].total_commands);
        printf("SSD_ACCESS_COUNTER %d\n", process_table[i].ssd_access_counter);
        printf("SSD_USAGE_TIME %d\n", process_table[i].ssd_usage_time);
        printf("SSD_WAIT_TIME %d\n", process_table[i].ssd_wait_time);
        printf("SSD_ENTRY_TIME %d\n", process_table[i].ssd_entry_time);

        for(j=0;j<process_table[i].command_index;j++) {
            printf("\tNAME %d", process_table[i].commands[j].name);
            printf("\tTIME %d\n", process_table[i].commands[j].time);
        }

        printf("\n");
    }

    // Setting the total commands and reseting the command_index of each process to 0;
    for(i=0;i<process_ctr;i++) {
        process_table[i].total_commands = process_table[i].command_index;
        process_table[i].command_index = 0;
    }


    printf("Current Time: %d, Total Processes in Table: %d\n", global_time,process_ctr);
    for(i = 0; i < process_ctr; i++) {
        printf("PROCESS %d: ", process_table[i].pid);
        if (process_table[i].state == READY)
        {
           printf("Current State = READY");
        }
        else if (process_table[i].state == RUNNING)
        {
           printf("Current State = RUNNING");
        }
        else if (process_table[i].state == WAITING)
        {
           printf("Current State = WAITING");
        }
        else if (process_table[i].state == FINISHED)
        {
           printf("Current State = FINISHED");
        }
        printf(" Next Index = %d\n", process_table[i].command_index);
    }

    printf("\n\n\n");

    while(finished_processes < process_ctr){
        /////////////// Check minimum end time ///////////////
        int minFinishTime = 9999999;
        // Check cpu1
        if(cpu1.busy) {
            if(cpu1.finish_time < minFinishTime) {
                minFinishTime = cpu1.finish_time;
            }
        }

        // Check cpu2
        if(cpu2.busy) {
            if(cpu2.finish_time < minFinishTime) {
                minFinishTime = cpu2.finish_time;
            }
        }

        // Check ssd
        if(ssd.busy) {
            if(ssd.finish_time < minFinishTime) {
                minFinishTime = ssd.finish_time;
            }
        }

        // Check INP queue
        if(io_q.busy) {
            if(process_table[io_q.pid[0]].commands[process_table[io_q.pid[0]].command_index].time + process_table[io_q.pid[0]].io_entry_time < minFinishTime) {
                minFinishTime = process_table[io_q.pid[0]].commands[process_table[io_q.pid[0]].command_index].time + process_table[io_q.pid[0]].io_entry_time;
            }
        }

        // Check if create a new process
        if(next_new_process < process_ctr) {
            if(process_table[next_new_process].commands[0].time < minFinishTime) {
                minFinishTime = process_table[next_new_process].commands[0].time;
            }
        }

        // Updating global time
        global_time = minFinishTime;



        /////////////// Free up hardware, io queue, and new process ///////////////

        // Free cpu1
        if(cpu1.busy) {
            if(cpu1.finish_time == minFinishTime) {
                execute_next_command(get_process_from_hardware(&cpu1, process_table), process_table, &io_q, &ssd_q, &ready_q);
            }
        }

        // Free cpu2
        if(cpu2.busy) {
            if(cpu2.finish_time == minFinishTime) {
                execute_next_command(get_process_from_hardware(&cpu2, process_table), process_table, &io_q, &ssd_q, &ready_q);
            }
        }

        // Free ssd
        if(ssd.busy) {
            if(ssd.finish_time == minFinishTime) {
                execute_next_command(get_process_from_hardware(&ssd, process_table), process_table, &io_q, &ssd_q, &ready_q);
            }
        }

        // Dequeue from INP queue
        if(io_q.busy) {
            while(io_q.busy && (process_table[io_q.pid[0]].commands[process_table[io_q.pid[0]].command_index].time + process_table[io_q.pid[0]].io_entry_time == minFinishTime) ) {
                execute_next_command(dequeue(&io_q), process_table, &io_q, &ssd_q, &ready_q);
            }
        }

        // Creating a new process
        if(next_new_process < process_ctr) {
            while(process_table[next_new_process].commands[0].time == minFinishTime) {
                execute_next_command(next_new_process, process_table, &io_q, &ssd_q, &ready_q);
                next_new_process = next_new_process + 1;
            }
        }




        /////////////// Put process from queue to hardware ///////////////

        // Check if cpu1 is busy
        if(cpu1.busy == 0) {
            // Check if ready q has something
            if(ready_q.busy) {

                // Add to cpu1
                cpu1.pid = dequeue(&ready_q);
                cpu1.busy = 1;
                cpu1.finish_time = global_time + process_table[cpu1.pid].commands[process_table[cpu1.pid].command_index].time;
                process_table[cpu1.pid].state = RUNNING;
                process_table[cpu1.pid].cpu_usage_time = process_table[cpu1.pid].cpu_usage_time + process_table[cpu1.pid].commands[process_table[cpu1.pid].command_index].time;


                // Increment command index
            }

        }

        // Check if cpu2 is busy
        if(cpu2.busy == 0) {
            // Check if ready q has something
             if(ready_q.busy) {

                // Add to cpu2
                cpu2.pid = dequeue(&ready_q);
                cpu2.busy = 1;
                cpu2.finish_time = global_time + process_table[cpu2.pid].commands[process_table[cpu2.pid].command_index].time;
                process_table[cpu2.pid].state = RUNNING;
                process_table[cpu2.pid].cpu_usage_time = process_table[cpu2.pid].cpu_usage_time + process_table[cpu2.pid].commands[process_table[cpu2.pid].command_index].time;
                // Increment command index
            }

        }

        // Check if ssd is busy
        if(ssd.busy == 0) {
            // Check if ssd q has something
            if(ssd_q.busy) {

                // Add to cpu2
                ssd.pid = dequeue(&ssd_q);
                ssd.busy = 1;
                ssd.finish_time = global_time + process_table[ssd.pid].commands[process_table[ssd.pid].command_index].time;
                process_table[ssd.pid].state = WAITING;
                process_table[ssd.pid].ssd_wait_time = (process_table[ssd.pid].ssd_wait_time + global_time) - process_table[ssd.pid].ssd_entry_time;
                process_table[ssd.pid].ssd_access_counter++;
                process_table[ssd.pid].ssd_usage_time = process_table[ssd.pid].ssd_usage_time + process_table[ssd.pid].commands[process_table[ssd.pid].command_index].time;
                // Increment command index
            }

        }

        printf("Current Time: %d, Total Processes in Table: %d\n", global_time,process_ctr);
        for(i = 0; i < process_ctr; i++) {
                printf("PROCESS %d: ", process_table[i].pid);
        if (process_table[i].state == READY)
        {
           printf("Current State = READY");
        }
        else if (process_table[i].state == RUNNING)
        {
           printf("Current State = RUNNING");
        }
        else if (process_table[i].state == WAITING)
        {
           printf("Current State = WAITING");
        }
        else if (process_table[i].state == FINISHED)
        {
           printf("Current State = FINISHED");
        }

        if (process_table[i].state == FINISHED) {
            printf(" Next Index = %d\n", process_table[i].command_index);
        }
        else {
            printf(" Next Index = %d\n", process_table[i].command_index + 1);
        }
    }

        printf("\n\n\n");

    }
    printf("Summary: \n");
    printf("Total Processes Completed: %d \n", finished_processes);

    int ssdAccesses = 0;
    int ssdWaitTime = 0;
    int ssdUsageTime = 0;
    int cpuUsageTime = 0;

    for(i=0; i < finished_processes; i++)
    {
        ssdAccesses = ssdAccesses + process_table[i].ssd_access_counter;
        ssdWaitTime =  ssdWaitTime + process_table[i].ssd_usage_time + process_table[i].ssd_wait_time;
        ssdUsageTime =  ssdUsageTime + process_table[i].ssd_usage_time;
        cpuUsageTime =  cpuUsageTime + process_table[i].cpu_usage_time;

    }

    printf("Total Number of SSD Accesses: %d\n", ssdAccesses);

    if(ssdAccesses != 0) {
        printf("Average SSD Access Duration: %f ms\n", ((double)ssdWaitTime/(double)ssdAccesses));
    }
    else {
        printf("Average SSD Access Duration: 0 ms\n");
    }
    printf("Total Elapsed Time %d ms\n", global_time-process_table[0].commands[0].time);
    printf("CPU Utilization %f\n", ( (double) cpuUsageTime / ((double) global_time-process_table[0].commands[0].time)));
    printf("SSD Utilization %f\n", ((double)ssdUsageTime/((double)global_time-process_table[0].commands[0].time)));



    // Finish program
    return 0;
};
