#include <assert.h>
#include <stdlib.h>

#include <process.h>

static struct 
{
    int initialized;      /**< Strategy already initialized? */
    int *g_iterator;      /**< Global iterator.              */
    workload_tt workload; /**< Workload.                     */
    array_tt cores;       /**< Cores.                        */
    RAM_tt RAM;           /**< Global RAM.                   */
} processdata = { 0, NULL, NULL, NULL, NULL };

/**
 * @brief Initializes the random_preemptive processing strategy.
 * 
 * @param workload Target workload.
 * @param cores    Total cores. 
 * @param g_i      Global iterator.
 * @param RAM      Global RAM.
*/
void processer_random_preemptive_init(workload_tt workload, array_tt cores, int *g_i, RAM_tt RAM)
{
    /* Sanity check. */
	assert(workload != NULL);
    assert(cores != NULL);
    
    /* Already initialized. */
    if ( processdata.initialized )
        return;

    processdata.g_iterator = g_i;
    processdata.workload = workload;
    processdata.cores = cores;
    processdata.RAM = RAM;
    processdata.initialized = 1;
}

/**
 * @brief Selecting tasks to be processed in a core. Each iteration consists of the first free task in a Thread, if a Thread is empty, it is just ignored.
 * 
 * @returns Total number of iterations spent processing. 
*/
void processer_random_preemptive_process(void)
{
    bool finished = false;
    /* 
       Stores the current workload processed in each core. It is used to propagate the waiting time to all tasks in core.
       This value is zeroed at the end, and the max(accum_penalties) value is considered. 
    */
    int accum_penalties[array_size(processdata.cores)];

    /* How much penalties ocurred in each iteration (per core). */
    int penalties[array_size(processdata.cores)];

    /* How much is left to each task to process based in an initial time slice. */
    int time_to_process[array_size(processdata.cores)];

    /* How much was processed. */
    int time_processed[array_size(processdata.cores)];

    /* How much workload was processed at each core. */
    int accum_total_processed[array_size(processdata.cores)];


    /* Storing the amount scheduled in this iteration. Useful to know how well-balanced the scheduling strategy is. */
    for ( unsigned long int i = 0; i < array_size(processdata.cores); i++ ) 
    {
        core_tt c = array_get(processdata.cores, i);
        queue_tt tsks = core_get_tsks(c);
        int size = queue_size(tsks);
        int acc = 0;
        for ( int j = 0; j < size; j++ ) 
            acc += task_work_left(queue_peek(tsks, j));
        core_set_workloads(c, acc, size);
        accum_penalties[i] = 0;
        if ( queue_size(tsks) > 0 )
        {
            penalties[i] = 0;
            accum_total_processed[i] = 0;
            time_to_process[i] = (rand() % (task_work_left(queue_peek(tsks, 0))))+ 1;
            time_processed[i] = 0;
        }
    }

    /* Counts number of cycles spent processing current tasks. */
    int iterator = 0;
    while ( !finished )
    {
        finished = true;

        for ( unsigned long int i = 0; i < array_size(processdata.cores); i++ )
        {
            core_tt c = array_get(processdata.cores, i);
            queue_tt tasks = core_get_tsks(c);

            // Ignore core if there are no left tasks.
            if ( queue_size(tasks) == 0 )
                continue;


            /* 
                Getting the contention value from core that received current task. 
                This search is necessary because cores might not be pinned (they will be sorted), 
                so we can't map accum_penalties[core_id] = core_contention
            */
            finished = false;
            
            task_tt curr_task = queue_peek(tasks, 0);
            
            // Just arrived
            if ( time_processed[i] == 0 ) 
            {
                /*
                    Contention is added into consideration. If a core didn't suffer contention, this value is negative
                    so it will subtract from 'entry_time'.
                */
                int entry_time = (*(processdata.g_iterator) + accum_total_processed[i] + accum_penalties[i]) + core_contention(c);
                // Setting the new moment that task arrived.
                task_set_emoment(curr_task, entry_time - task_arrivaltime(curr_task) );
            }
            

            int c_sets = core_cache_num_sets(c);
            int r_pages = RAM_SIZE / PAGE_SIZE;
            unsigned long int position = task_memptr(curr_task);


            /* Used at optimizing (grouping tasks by their last used cache sets) */
            unsigned long int *t_lineacc = task_lineacc(curr_task);
            unsigned long int *t_pageacc = task_pageacc(curr_task);
            
            struct mem* m = array_get(task_memacc(curr_task), position);
            bool page_hit = core_mmu_translate(c, curr_task, m, processdata.RAM);
            bool hit = core_cache_checkaddr(c, m);

            if ( page_hit )
            {
                task_set_page_hit(curr_task, task_page_hit(curr_task) + 1);
                core_set_page_hit(c, core_page_hit(c) + 1);
            } 
            else
            {
                task_set_page_fault(curr_task, task_page_fault(curr_task) + 1);
                core_set_page_fault(c, core_page_fault(c) + 1);
                penalties[i] += PAGE_FAULT_PENALTY;
            }

            if ( hit )
            {
                task_set_hit(curr_task, task_hit(curr_task) + 1);
                core_set_hit(c, core_hit(c) + 1);
            }
            else
            {
                task_set_miss(curr_task, task_miss(curr_task) + 1);
                core_set_miss(c, core_miss(c) + 1);
                /* If miss, we must add a penalty. */
                penalties[i] += MISS_PENALTY;
                core_cache_replace(c, m);
            }
            // Mapping which line addr was allocated
           
            t_pageacc[position] = (mem_physical_addr(m) * PAGE_SIZE) % r_pages;
            t_lineacc[position++] = (mem_physical_addr(m) * PAGE_SIZE) % c_sets;

            task_set_memptr(curr_task, position);

            time_processed[i]++;
            task_set_workprocess(curr_task, task_work_processed(curr_task) + 1);

            // Task finished
            if ( time_processed[i] == time_to_process[i] )
            {
                // Set waiting time
                /* Time that task spent "idling". */
                int time_waiting = task_emoment(curr_task) - task_lmoment(curr_task);

                // Setting the new moment that task left. ( How much processed + accumulated waitings )
                int left_time = task_emoment(curr_task) + penalties[i] + time_processed[i];
                task_set_lmoment(curr_task, left_time);
                task_set_waiting_time(curr_task, task_waiting_time(curr_task) + penalties[i] + time_waiting);

                /* If a task has finished, we add it to "finished tasks queue", otherwise, we 'recycle' it into workload. */
                if ( task_work_left(curr_task) == 0 ) queue_insert(workload_fintasks(processdata.workload), queue_remove(tasks));
                else queue_insert((queue_tt) array_get(workload_arrtasks(processdata.workload), array_size(workload_arrtasks(processdata.workload)) - 2), queue_remove(tasks));
                accum_total_processed[i]+= time_processed[i];
                accum_penalties[i] += penalties[i];

                // Refresh values for next task (if any)
                if ( queue_size(tasks) > 0 )
                {
                    penalties[i] = 0;
                    time_to_process[i] = (rand() % (task_work_left(queue_peek(tasks, 0)))) + 1;
                    time_processed[i] = 0;
                }
            }
        }
        if (!finished) iterator ++;
    }

    /* Finding the MAX waiting time. We must keep in mind that our cores waits until all core are free to get the next batch of tasks. */
    int max_penalties = 0;
    for ( unsigned long int i = 0; i < array_size(processdata.cores); i++ ) { if (max_penalties < accum_penalties[i]) max_penalties = accum_penalties[i]; }

    /* Cleaning up. */
    for ( unsigned long int i = 0; i < array_size(processdata.cores); i++ )
        core_vacate(array_get(processdata.cores, i));

    /* Max processing time spent (max_penalties) + preparation time (iterator) */
    *(processdata.g_iterator) += max_penalties + iterator;

}

/**
 * @brief Finalizes the non random_preemptive strategy.
*/
void processer_random_preemptive_end(void)
{
    processdata.initialized = 0;
}

static struct processer _random_preemptive = {
    processer_random_preemptive_init,
    processer_random_preemptive_process,
    processer_random_preemptive_end
};

const struct processer *random_preemptive = &_random_preemptive;