#include <assert.h>
#include <stdlib.h>

#include <stdio.h>

#include <process.h>

static struct
{
    int initialized;      /**< Strategy already initialized? */
    int *g_iterator;      /**< Global iterator.              */
    int quantum;          /**< QUANTUM.                      */
    workload_tt workload; /**< Workload.                     */
    array_tt cores;       /**< Cores.                        */
    RAM_tt RAM;           /**< Global RAM.                   */
} processdata = { 0, NULL, 0, NULL, NULL, NULL };

/**
 * @brief Initializes the rr_preemptive processing strategy.
 *
 * @param workload Target workload.
 * @param cores    Total cores.
 * @param g_i      Global iterator.
 * @param RAM      Global RAM.
*/
void processer_rr_preemptive_init(workload_tt workload, array_tt cores, int *g_i, RAM_tt RAM)
{
    /* Sanity check. */
	assert(workload != NULL);
    assert(cores != NULL);

    /* Already initialized. */
    if ( processdata.initialized )
        return;

    processdata.g_iterator = g_i;
    processdata.quantum = QUANTUM;
    processdata.workload = workload;
    processdata.cores = cores;
    processdata.RAM = RAM;
    processdata.initialized = 1;
}



/**
 * @brief Selecting tasks to be processed in a core. Each iteration consists of the first free task if a core is empty, it is just ignored.
 *
 *
 * @returns Total number of iterations spent processing.
*/
void processer_rr_preemptive_process(void)
{
    bool finished = false;
    /*
       Stores the current workload processed in each core. It is used to propagate the waiting time to all tasks in core.
       This value is zeroed at the end, and the max(accum_penalties) value is considered.
    */
    // int accum_penalties[array_size(processdata.cores)];

    /* How much penalties ocurred in each iteration (per core). */
    // int penalties[array_size(processdata.cores)];

    /* How much is left to each task to process based in an initial time slice. */
    int time_to_process[workload_ntasks(processdata.workload)];

    /* How much was processed. */
    int time_processed[workload_ntasks(processdata.workload)];
    for ( int i = 0; i < workload_ntasks(processdata.workload); i++ )
    {
        time_to_process[i] = 0;
        time_processed[i] = 0;
    }


    /* How much penalty each task still have. Mapping is = task_penalty_time(task) = task_id */
    int task_penalty_time[workload_ntasks(processdata.workload)];
    // bool task_had_miss[workload_ntasks(processdata.workload)];
    // for ( int i = 0; i < workload_ntasks(processdata.workload); i++ ) { task_penalty_time[i] = 0; task_had_miss[i] = false; }
    for ( int i = 0; i < workload_ntasks(processdata.workload); i++ ) { task_penalty_time[i] = 0; }


    int last_core_task_processed_id[array_size(processdata.cores)];
    for ( unsigned long int i = 0; i < array_size(processdata.cores); i++ ) last_core_task_processed_id[i] = -1;

    /* How much workload was processed at each core. */
    // int accum_total_processed[array_size(processdata.cores)];

    /* Storing the amount scheduled in this iteration. Useful to know how well-balanced the scheduling strategy is. */
    for ( unsigned long int i = 0; i < array_size(processdata.cores); i++ )
    {
        core_tt c = array_get(processdata.cores, i);
        queue_tt tsks = core_get_tsks(c);
        int size = queue_size(tsks);

        unsigned long int acc = 0;
        for ( int j = 0; j < size; j++ )
        {
            int work_left = task_work_left(queue_peek(tsks, j));
            acc += (work_left < QUANTUM) ? work_left : QUANTUM;
            // acc += task_work_left(queue_peek(tsks, j));
        }
        core_set_workloads(c, acc, size);
        // accum_penalties[i] = 0;
        int iterator = 0;
        while ( iterator < queue_size(tsks) )
        {
            task_tt curr_task = queue_peek(tsks, iterator++);
            // penalties[i] = 0;
            // accum_total_processed[i] = 0;
            time_to_process[task_gettsid(curr_task)] = (task_work_left(curr_task) < QUANTUM) ? task_work_left(curr_task) : QUANTUM;
        }

        // Cleaning up. If values are negative, cache_set's map will be reseted.
        core_cache_sets_accesses_update(c, -1);
        core_cache_sets_conflicts_update(c, -1);
    }

    /* Counts number of cycles spent processing current tasks. */
    int iterator[array_size(processdata.cores)];
    for ( unsigned long int i = 0; i < array_size(processdata.cores); i++ ) iterator[i] = 0;
    int total_cache_misses[array_size(processdata.cores)];
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
            int task_id = task_gettsid(curr_task);

            int current_time = (processdata.g_iterator[core_getcid(c)] + iterator[i]) + core_contention(c);
            // Implies that this task is not re-entering. If that is true, we must update it's e_moment
            if ( last_core_task_processed_id[i] != task_id || queue_size(tasks) == 1 )
            {
                last_core_task_processed_id[i] = task_id;

                // Setting the new moment that task arrived.
                task_set_emoment(curr_task, current_time - task_arrivaltime(curr_task) );
            }

            // Has to wait more, since there is penalties. As if task is in an "idle" state.
            if ( task_penalty_time[task_id] > 0 )
            {
                task_penalty_time[task_id] -= 1;

                task_set_waiting_time(curr_task, task_waiting_time(curr_task) + (current_time - task_lmoment(curr_task)));
                task_set_lmoment(curr_task, (current_time - task_arrivaltime(curr_task) ) );
                // Task goes to 'end-of-line'.
                queue_insert(tasks, queue_remove(tasks));
            }
            else
            {

                int c_sets = core_cache_num_sets(c);
                int r_pages = RAM_SIZE / PAGE_SIZE;
                unsigned long int position = task_memptr(curr_task);


                /* Used at optimizing (grouping tasks by their last used cache sets) */
                int *t_lineacc = task_lineacc(curr_task);
                int *t_pageacc = task_pageacc(curr_task);
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
                    /* If page fault, we must add a penalty. */
                    task_penalty_time[task_id] += PAGE_FAULT_PENALTY;
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
                    core_cache_sets_conflicts_update(c, (mem_physical_addr(m)) % c_sets);
                    total_cache_misses[i]++;
                    /* If miss, we must add a penalty. */
                    task_penalty_time[task_id] += MISS_PENALTY;
                    core_cache_replace(c, m);
                }

                // Mapping which line addr was allocated
                core_cache_sets_accesses_update(c, (mem_physical_addr(m)) % c_sets);

                t_pageacc[position] = (mem_physical_addr(m)) % r_pages;
                t_lineacc[position++] = (mem_physical_addr(m)) % c_sets;

                // printf("%d %ld %ld\n", c_sets, (mem_physical_addr(m) % c_sets), (mem_physical_addr(m) * PAGE_SIZE) % c_sets);

                task_set_memptr(curr_task, position);

                time_processed[task_id]++;
                task_set_workprocess(curr_task, task_work_processed(curr_task) + 1);


                // Task finished
                if ( time_processed[task_id] == time_to_process[task_id] )
                {

                    task_set_waiting_time(curr_task, task_waiting_time(curr_task) + (current_time - task_lmoment(curr_task)));
                    task_set_lmoment(curr_task, (current_time - task_arrivaltime(curr_task) ) );


                    /* If a task has finished, we add it to "finished tasks queue", otherwise, we 'recycle' it into workload. */
                    if ( task_work_left(curr_task) == 0 )
                        queue_insert(workload_fintasks(processdata.workload), queue_remove(tasks));
                    else
                        queue_insert((queue_tt) array_get(workload_arrtasks(processdata.workload), array_size(workload_arrtasks(processdata.workload)) - 2), queue_remove(tasks));
                    // accum_total_processed[i] += time_processed[i];

                }
            }
        }
        if (!finished)
        {
            // If there are still tasks to be processed in core, we must increate core's auxiliar iterator.
            for ( unsigned long int i = 0; i < array_size(processdata.cores); i++ )
                if ( queue_size(core_get_tsks(array_get(processdata.cores, i))) > 0 ) iterator[i] ++;
        }

    }

    /* Cleaning up. */
    for ( unsigned long int i = 0; i < array_size(processdata.cores); i++ )
    {
        core_tt c = array_get(processdata.cores, i);
        core_vacate(c);
    }

    /* Max processing time spent (max_penalties) + preparation time (iterator) */

    for ( unsigned long int i = 0; i < array_size(processdata.cores); i++ )
        processdata.g_iterator[i] += iterator[i];
}

/**
 * @brief Finalizes the non rr_preemptive strategy.
*/
void processer_rr_preemptive_end(void)
{
    processdata.initialized = 0;
}

static struct processer _rr_preemptive = {
    processer_rr_preemptive_init,
    processer_rr_preemptive_process,
    processer_rr_preemptive_end
};

const struct processer *rr_preemptive = &_rr_preemptive;
