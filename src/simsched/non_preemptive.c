#include <assert.h>
#include <stdlib.h>

#include <process.h>

static struct 
{
    int initialized;      /**< Strategy already initialized? */
    int *g_iterator;      /**< Global iterator.              */
    workload_tt workload; /**< Workload.                     */
    array_tt cores;       /**< Cores.                        */
} processdata = { 0, NULL, NULL, NULL };

/**
 * @brief Initializes the non preemptive processing strategy.
 * 
 * @param workload Target workload.
 * @param cores    Total cores. 
 * @param g_i      Global iterator.
*/
void processer_non_preemptive_init(workload_tt workload, array_tt cores, int *g_i)
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
    processdata.initialized = 1;
}

/**
 * @brief Selecting tasks to be processed in a core. Each iteration consists of the first free task if a core is empty, it is just ignored.
 * 
 * @returns Total number of iterations spent processing. 
*/
void processer_non_preemptive_process(void)
{  
    bool finished = false;
    /* 
       Stores the current workload processed in each core. It is used to propagate the waiting time to all tasks in core.
       This value is zeroed at the end, and the max(aux) value is considered. 
    */
    int *aux = smalloc(array_size(processdata.cores));
    for ( int i = 0; i < array_size(processdata.cores); i++ ) 
    {
        core_tt c = array_get(processdata.cores, i);
        queue_tt tsks = core_get_tsks(c);
        int size = queue_size(tsks);
        int acc = 0;
        for ( int j = 0; j < size; j++ ) 
            acc += task_work_left(queue_peek(tsks, j));
        core_set_workloads(c, acc, size);
        aux[i] = 0;
    }

    /* 
        Number of iterations that will be spent processing tasks. 
        It will be the cores will higher number of tasks, since we are simulating a parallel processing.
    */
    int max_it = 0;

    while ( !finished )
    { 
        /* We are simulating a parallel processing here, all cores will process "at the same time".  */
        for ( int i = 0; i < array_size(processdata.cores); i++ )
        {
            core_tt c = array_get(processdata.cores, i);
            queue_tt c_tasks = core_get_tsks(c);
            int c_numtasks = queue_size(c_tasks);

            /* If all cores are done finishing, then we're done :) */
            if ( c_numtasks == 0 )
                continue;
            
            /* If atleast one core is not done, we're not done yet ;[ */

            if ( c_numtasks > max_it ) max_it = c_numtasks;
            /* 
                Getting the contention value from core that received current task. 
                This search is necessary because cores might not be pinned (they will be sorted), 
                so we can't map aux[core_id] = core_contention
            */
            int c_pos = 0;
            for ( /* noop */ ; c_pos < array_size(processdata.cores); c_pos++ )
                if ( core_getcid(array_get(processdata.cores, c_pos)) == core_getcid(c) ) break;
            int contention = core_contention(c);

            while ( !queue_empty(core_get_tsks(c)) )
            {
                task_tt ts = queue_remove(core_get_tsks(c));
                /* 
                    Aux is used because our cores must wait until them all are fully processed to schedule again.
                    This implies that a non-balanced strategies will suffer more, since a core that finished earlier
                    will have to wait for all the others. 

                    Contention is added into consideration. If a core didn't suffer contention, this value is, actualy, subtracted from 'accum_waiting'.
                */
                int accum_waiting = *(processdata.g_iterator) + aux[c_pos] + contention;

                task_set_emoment(ts, accum_waiting - task_arrivaltime(ts) );

                int e_moment = task_emoment(ts),
                    l_moment = task_lmoment(ts);

                /*  
                    Initial waiting time will be: 
                    - Tasks' 'idle' time: The current time (e_moment) - the last moment that task was processed (l_moment).
                */
                int waiting_time = e_moment - l_moment;

                int amount_processed = 0,
                    amount_to_process = task_work_left(ts);

                int miss_waited = 0, /* How much waiting time propagated by cache misses. */
                    position = task_memptr(ts); /* Last address accessed. */
                /* 
                    If you want to remove the cache hit/miss analysis, just set the cache size (in arch file) to 0.
                    You can remove "amount_processed" from this loop and use amount_to_process as amount_processed. It's here for convenience only.
                */
                if ( array_size(core_cache(c)) > 0 )
                {
                    for ( /* noop */; amount_processed < amount_to_process; position++, amount_processed++ )
                    {
                        struct mem* m = array_get(task_memacc(ts), position);
                        bool hit = core_cache_checkaddr(c, m);
                        // printf("Addr %d - Hit %d - workload %d - wait_calc %d - global_c %d - aux %d\n", mem_addr(m), hit, task_workload(ts), waiting_time, *processdata.g_iterator, aux[c_pos]);

                        if ( hit )
                        {
                            task_set_hit(ts, task_hit(ts) + 1);
                            core_set_hit(c, core_hit(c) + 1);
                        }
                        else
                        {
                            task_set_miss(ts, task_miss(ts) + 1);
                            core_set_miss(c, core_miss(c) + 1);
                        }

                        /* If miss, we must add a penalty. */
                        miss_waited += (hit) ? 0 : MISS_PENALTY;
                        core_cache_replace(c, m);
                    }
                } else
                    for ( /* noop */; amount_processed < amount_to_process; amount_processed++ );

                
                task_set_memptr(ts, position);

                /* Saving the moment that task started to be processed. This value is adjusted with task arrival time */
                task_set_waiting_time(ts, task_waiting_time(ts) + waiting_time + miss_waited);

                task_set_workprocess(ts, task_work_processed(ts) + amount_processed);
                aux[c_pos] += amount_processed + miss_waited;

                /* If a task has finished, we add into a different queue, otherwise, we 'recycle' it into workload. */
                if ( task_work_left(ts) == 0 ) queue_insert(workload_fintasks(processdata.workload), ts);
                else queue_insert(workload_arrtasks(processdata.workload), ts);
                
                /* Saving the moment that task left. This value is also adjusted with task arrival time. This value is the same as saying: e_moment + amount_processed. */
                task_set_lmoment(ts, (accum_waiting + amount_processed) - task_arrivaltime(ts));
            }
        }

        finished = true;
    }

    /* Finding the MAX waiting time. We must keep in mind that our cores waits until all core are free to get the next batch of tasks. */
    int max_workload = 0;
    for ( int i = 0; i < array_size(processdata.cores); i++ ) { if (max_workload < aux[i]) max_workload = aux[i]; }

    free(aux);
    /* Cleaning up. */
    for ( int i = 0; i < array_size(processdata.cores); i++ )
        core_vacate(array_get(processdata.cores, i));

    /* Max processing time spent (max_workload) + preparation time (max_it) */
    *(processdata.g_iterator) += (max_workload + max_it);
}

/**
 * @brief Finalizes the non preemptive strategy.
*/
void processer_non_preemptive_end(void)
{
    processdata.initialized = 0;
}

static struct processer _non_preemptive = {
    processer_non_preemptive_init,
    processer_non_preemptive_process,
    processer_non_preemptive_end
};

const struct processer *non_preemptive = &_non_preemptive;