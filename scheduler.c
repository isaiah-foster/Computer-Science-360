#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// Headers as needed

#define RR_QUANTUM 2                    // Quantum size used by Round Robin

/* The scheduling algorithms that can be simulated */
typedef enum {FCFS, RR, SJF} _algorithm;

/* The states a process moves through during the simulation */
typedef enum {UNSTARTED, READY, RUNNING, BLOCKED, TERMINATED} _state;

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 202311L
typedef enum {false, true} bool;        // Allows boolean types in C (bool is a keyword from C23 onwards)
#endif

/* Defines a job struct */
typedef struct Process {
    uint32_t A;                         // A: Arrival time of the process
    uint32_t B;                         // B: Upper Bound of CPU burst times of the given random integer list
    uint32_t C;                         // C: Total CPU time required
    uint32_t M;                         // M: Multiplier of CPU burst time
    uint32_t processID;                 // The process ID given upon input read
    uint32_t currentCPUTimeRun;          // The current CPU time run for the process
    uint32_t currentIOBlockedTime;       // The current I/O blocked time for the process
    uint32_t currentWaitingTime;         // The current waiting time for the process
    uint32_t finishingTime;              // The finishing time for the process

    _state state;                        // The current state of the process
    uint32_t remainingCPUBurst;          // CPU cycles left in the current burst
    uint32_t remainingIOBurst;           // I/O cycles left in the current burst
    uint32_t lastCPUBurst;               // Length of the most recent CPU burst (sets the next I/O burst)
    uint32_t remainingQuantum;           // Cycles left in the current RR quantum
} _process;

uint32_t TOTAL_CREATED_PROCESSES = 0;   // The total number of processes constructed
const uint32_t SEED_VALUE = 200;  // Seed value for reading from file
uint32_t TOTAL_FINISHED_PROCESSES = 0;  // The total number of processes that have finished
uint32_t CURRENT_CYCLE = 0;  // The current cycle of the simulation
uint32_t TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0;  // The total number of cycles spent blocked

// Additional variables as needed


/**
 * Reads a random non-negative integer X from a file with a given line named random-numbers (in the current directory)
 */
uint32_t getRandNumFromFile(uint32_t line, FILE* random_num_file_ptr){
    uint32_t end, loop;
    char str[512];

    rewind(random_num_file_ptr); // reset to be beginning
    for(end = loop = 0;loop<line;++loop){
        if(0==fgets(str, sizeof(str), random_num_file_ptr)){ //include '\n'
            end = 1;  //can't input (EOF)
            break;
        }
    }
    if(!end) {
        return (uint32_t) atoi(str);
    }

    // Fail-safe return
    return (uint32_t) 1804289383;
}



/**
 * Reads a random non-negative integer X from a file named random-numbers.
 * Returns the CPU Burst: : 1 + (random-number-from-file % upper_bound)
 */
uint32_t randomOS(uint32_t upper_bound, uint32_t process_indx, FILE* random_num_file_ptr)
{
    char str[20];
    
    uint32_t unsigned_rand_int = (uint32_t) getRandNumFromFile(SEED_VALUE+process_indx, random_num_file_ptr);
    uint32_t returnValue = 1 + (unsigned_rand_int % upper_bound);

    return returnValue;
} 


/********************* SOME PRINTING HELPERS *********************/


/**
 * Prints to standard output the original input
 * process_list is the original processes inputted (in array form)
 */
void printStart(_process process_list[])
{
    printf("The original input was: %i", TOTAL_CREATED_PROCESSES);

    uint32_t i = 0;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
    }
    printf("\n");
} 

/**
 * Prints to standard output the final output
 * finished_process_list is the terminated processes (in array form) in the order they each finished in.
 */
void printFinal(_process finished_process_list[])
{
    printf("The (sorted) input is: %i", TOTAL_CREATED_PROCESSES);

    uint32_t i = 0;
    for (; i < TOTAL_FINISHED_PROCESSES; ++i)
    {
        printf(" ( %i %i %i %i)", finished_process_list[i].A, finished_process_list[i].B,
               finished_process_list[i].C, finished_process_list[i].M);
    }
    printf("\n");
} // End of the print final function

/**
 * Prints out specifics for each process  (helper function, you may need to adjust variables accordingly)
 * @param process_list The original processes inputted, in array form
 */
void printProcessSpecifics(_process process_list[])
{
    uint32_t i = 0;
    printf("\n");
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        printf("Process %i:\n", process_list[i].processID);
        printf("\t(A,B,C,M) = (%i,%i,%i,%i)\n", process_list[i].A, process_list[i].B,
               process_list[i].C, process_list[i].M);
        printf("\tFinishing time: %i\n", process_list[i].finishingTime);
        printf("\tTurnaround time: %i\n", process_list[i].finishingTime - process_list[i].A);
        printf("\tI/O time: %i\n", process_list[i].currentIOBlockedTime);
        printf("\tWaiting time: %i\n", process_list[i].currentWaitingTime);
        printf("\n");
    }
} // End of the print process specifics function

/**
 * Prints out the summary data (helper function, you may need to adjust variables accordingly)
 * process_list The original processes inputted in array form
 */
void printSummaryData(_process process_list[])
{
    uint32_t i = 0;
    double total_amount_of_time_utilizing_cpu = 0.0;
    double total_amount_of_time_io_blocked = 0.0;
    double total_amount_of_time_spent_waiting = 0.0;
    double total_turnaround_time = 0.0;
    uint32_t final_finishing_time = CURRENT_CYCLE - 1;
    for (; i < TOTAL_CREATED_PROCESSES; ++i)
    {
        total_amount_of_time_utilizing_cpu += process_list[i].currentCPUTimeRun;
        total_amount_of_time_io_blocked += process_list[i].currentIOBlockedTime;
        total_amount_of_time_spent_waiting += process_list[i].currentWaitingTime;
        total_turnaround_time += (process_list[i].finishingTime - process_list[i].A);
    }

    // Calculates the CPU utilisation
    double cpu_util = total_amount_of_time_utilizing_cpu / final_finishing_time;

    // Calculates the IO utilisation
    double io_util = (double) TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED / final_finishing_time;

    // Calculates the throughput (Number of processes over the final finishing time times 100)
    double throughput =  100 * ((double) TOTAL_CREATED_PROCESSES/ final_finishing_time);

    // Calculates the average turnaround time
    double avg_turnaround_time = total_turnaround_time / TOTAL_CREATED_PROCESSES;

    // Calculates the average waiting time
    double avg_waiting_time = total_amount_of_time_spent_waiting / TOTAL_CREATED_PROCESSES;

    printf("Summary Data:\n");
    printf("\tFinishing time: %i\n", CURRENT_CYCLE - 1);
    printf("\tCPU Utilisation: %6f\n", cpu_util);
    printf("\tI/O Utilisation: %6f\n", io_util);
    printf("\tThroughput: %6f processes per hundred cycles\n", throughput);
    printf("\tAverage turnaround time: %6f\n", avg_turnaround_time);
    printf("\tAverage waiting time: %6f\n", avg_waiting_time);
} // End of the print summary data function


/*****SCHEDULER SIMULATION *******/

const char *ALGORITHM_HEADERS[] = {"FIRST COME FIRST SERVE", "ROUND ROBIN", "SHORTEST JOB FIRST"};
const char *ALGORITHM_NAMES[]   = {"First Come First Serve", "Round Robin", "Shortest Job First"};

/**
 * The tie-breaking rule: returns true if process a should be favored over process b.
 * The earliest arrival time wins; if the arrival times match, the process listed earliest
 * in the input wins.
 */
bool favoredOver(const _process *a, const _process *b)
{
    if (a->A != b->A)
    {
        return a->A < b->A;
    }
    return a->processID < b->processID;
}

/**
 * Appends a batch of processes that all became ready during the same cycle to the back of the
 * ready queue. The batch is first ordered by the tie-breaking rule so that FCFS and RR insert
 * simultaneous arrivals into the FIFO queue in a deterministic order.
 */
void enqueueReady(_process process_list[], uint32_t ready_queue[], uint32_t *ready_count,
                  uint32_t batch[], uint32_t batch_count)
{
    // Insertion sort
    for (uint32_t i = 1; i < batch_count; ++i)
    {
        uint32_t current = batch[i];
        uint32_t j = i;
        for (; j > 0 && favoredOver(&process_list[current], &process_list[batch[j - 1]]); --j)
        {
            batch[j] = batch[j - 1];
        }
        batch[j] = current;
    }

    for (uint32_t i = 0; i < batch_count; ++i)
    {
        process_list[batch[i]].state = READY;
        ready_queue[(*ready_count)++] = batch[i];
    }
}

/**
 * Removes and returns the index of the next process to run from the ready queue.
 * FCFS and RR take the front of the FIFO queue. SJF takes the process with the least total CPU
 * time remaining, braking ties with favoredOver().
 */
uint32_t dequeueReady(_algorithm algorithm, _process process_list[], uint32_t ready_queue[],
                      uint32_t *ready_count)
{
    uint32_t chosen_pos = 0;

    if (algorithm == SJF)
    {
        for (uint32_t i = 1; i < *ready_count; ++i)
        {
            _process *candidate = &process_list[ready_queue[i]];
            _process *best = &process_list[ready_queue[chosen_pos]];
            uint32_t candidate_remaining = candidate->C - candidate->currentCPUTimeRun;
            uint32_t best_remaining = best->C - best->currentCPUTimeRun;

            if (candidate_remaining < best_remaining ||
                (candidate_remaining == best_remaining && favoredOver(candidate, best)))
            {
                chosen_pos = i;
            }
        }
    }

    uint32_t chosen = ready_queue[chosen_pos];
    memmove(&ready_queue[chosen_pos], &ready_queue[chosen_pos + 1],
            (*ready_count - chosen_pos - 1) * sizeof(uint32_t));
    --(*ready_count);
    return chosen;
}

/**
 * Simulates one scheduling algorithm on a fresh copy of the input processes and prints the results.
 */
void simulate(_algorithm algorithm, _process input_list[], uint32_t num_processes,
              FILE *random_num_file_ptr)
{
    _process process_list[num_processes];           // Working copy of the input processes
    _process finished_process_list[num_processes];  // Processes in the order they terminated
    uint32_t ready_queue[num_processes];            // Indices of ready processes, in FIFO order
    uint32_t newly_ready[num_processes];            // Processes that became ready this cycle
    uint32_t ready_count = 0;
    _process *running = NULL;                       // The process on the CPU

    memcpy(process_list, input_list, num_processes * sizeof(_process));
    for (uint32_t i = 0; i < num_processes; ++i)
    {
        _process *p = &process_list[i];
        p->state = UNSTARTED;
        p->currentCPUTimeRun = p->currentIOBlockedTime = p->currentWaitingTime = 0;
        p->finishingTime = p->remainingCPUBurst = p->remainingIOBurst = 0;
        p->lastCPUBurst = p->remainingQuantum = 0;
    }
    TOTAL_FINISHED_PROCESSES = 0;
    CURRENT_CYCLE = 0;
    TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED = 0;

    printf("######################### START OF %s #########################\n",
           ALGORITHM_HEADERS[algorithm]);
    printStart(process_list);

    while (TOTAL_FINISHED_PROCESSES < num_processes)
    {
        uint32_t newly_ready_count = 0;

        //The running process: terminate, block or be preempted, considered in that order
        if (running != NULL)
        {
            if (running->currentCPUTimeRun >= running->C)
            {
                running->state = TERMINATED;
                running->finishingTime = CURRENT_CYCLE;
                finished_process_list[TOTAL_FINISHED_PROCESSES++] = *running;
                running = NULL;
            }
            else if (running->remainingCPUBurst == 0)
            {
                running->state = BLOCKED;
                running->remainingIOBurst = running->lastCPUBurst * running->M;
                running = NULL;
            }
            else if (algorithm == RR && running->remainingQuantum == 0)
            {
                newly_ready[newly_ready_count++] = running->processID;
                running = NULL;
            }
        }

        for (uint32_t i = 0; i < num_processes; ++i)
        {
            _process *p = &process_list[i];

            //Blocked processes whose I/O burst is complete become ready
            if (p->state == BLOCKED && p->remainingIOBurst == 0)
            {
                newly_ready[newly_ready_count++] = i;
            }
            // Processes arriving this cycle become ready
            else if (p->state == UNSTARTED && p->A == CURRENT_CYCLE)
            {
                newly_ready[newly_ready_count++] = i;
            }
        }

        enqueueReady(process_list, ready_queue, &ready_count, newly_ready, newly_ready_count);

        // give an idle CPU to the next ready process
        if (running == NULL && ready_count > 0)
        {
            running = &process_list[dequeueReady(algorithm, process_list, ready_queue, &ready_count)];
            running->state = RUNNING;

            // Start a new CPU burst unless resuming one cut short by an RR preemption
            if (running->remainingCPUBurst == 0)
            {
                uint32_t burst = randomOS(running->B, running->processID, random_num_file_ptr);
                uint32_t cpu_time_remaining = running->C - running->currentCPUTimeRun;
                if (burst > cpu_time_remaining)
                {
                    burst = cpu_time_remaining;
                }
                running->remainingCPUBurst = running->lastCPUBurst = burst;
            }
            running->remainingQuantum = RR_QUANTUM;
        }

        // Charge this cycle to every process according to its state
        bool any_blocked = false;
        for (uint32_t i = 0; i < num_processes; ++i)
        {
            _process *p = &process_list[i];
            switch (p->state)
            {
                case RUNNING:
                    ++p->currentCPUTimeRun;
                    --p->remainingCPUBurst;
                    --p->remainingQuantum;
                    break;
                case BLOCKED:
                    ++p->currentIOBlockedTime;
                    --p->remainingIOBurst;
                    any_blocked = true;
                    break;
                case READY:
                    ++p->currentWaitingTime;
                    break;
                default:
                    break;
            }
        }
        if (any_blocked)
        {
            ++TOTAL_NUMBER_OF_CYCLES_SPENT_BLOCKED;
        }

        ++CURRENT_CYCLE;
    }

    printFinal(finished_process_list);
    printf("\nThe scheduling algorithm used was %s\n", ALGORITHM_NAMES[algorithm]);
    printProcessSpecifics(process_list);
    printSummaryData(process_list);
    printf("######################### END OF %s #########################\n",
           ALGORITHM_HEADERS[algorithm]);
}


/**
 * The magic starts from here
 */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input-file>\n", argv[0]);
        return 1;
    }

    // Write code for your shiny scheduler
    FILE* input_file  = fopen(argv[1], "r");
    if (input_file == NULL) {
        fprintf(stderr, "Error opening input file: %s\n", argv[1]);
        return 1;
    }

    uint32_t total_num_of_process;               // Read from the file -- number of process to create
    if (fscanf(input_file, "%u", &total_num_of_process) != 1) {
        fprintf(stderr, "Error reading process count from %s\n", argv[1]);
        fclose(input_file);
        return 1;
    }

    _process process_list[total_num_of_process]; // Creates a container for all processes
    // Other variables

    // Each process is given as "(A B C M)"; anything after the last one is ignored
    for (uint32_t i = 0; i < total_num_of_process; ++i) {
        _process *p = &process_list[i];
        if (fscanf(input_file, " ( %u %u %u %u )", &p->A, &p->B, &p->C, &p->M) != 4) {
            fprintf(stderr, "Error reading process %u from %s\n", i, argv[1]);
            fclose(input_file);
            return 1;
        }
        if (p->B == 0) {
            fprintf(stderr, "Process %u in %s has B = 0; B must be positive\n", i, argv[1]);
            fclose(input_file);
            return 1;
        }
        p->processID = i;
        ++TOTAL_CREATED_PROCESSES;
    }
    fclose(input_file);

    FILE* random_num_file_ptr = fopen("random-numbers", "r");
    if (random_num_file_ptr == NULL) {
        fprintf(stderr, "Error opening random-numbers (it must be in the current directory)\n");
        return 1;
    }

    // Run every algorithm on the same input
    simulate(FCFS, process_list, total_num_of_process, random_num_file_ptr);
    simulate(RR, process_list, total_num_of_process, random_num_file_ptr);
    simulate(SJF, process_list, total_num_of_process, random_num_file_ptr);

    fclose(random_num_file_ptr);
    return 0;
}
 