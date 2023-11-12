#include <assert.h>
#include <stdlib.h>

#include <process.h>

static struct 
{
    int initialized;      /**< Strategy already initialized? */
    int *g_iterator;      /**< Global iterator.              */
    workload_tt workload; /**< Workload.                     */
    array_tt threads;     /**< Threads.                      */
    array_tt cores;       /**< Cores.                        */
    queue_tt finished;    /**< Queue of finished tasks.      */
} processdata = { 0, NULL, NULL, NULL, NULL, NULL };

/**
 * @brief Initializes the random_preemptive processing strategy.
 * 
 * @param workload Target workload.
 * @param cores    Total cores. 
 * @param threads  Total threads.
 * @param g_i      Global iterator.
*/
void processer_random_preemptive_init(workload_tt workload, array_tt cores, array_tt threads, int *g_i)
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
    processdata.threads = threads;
    processdata.finished = queue_create();
    processdata.initialized = 1;
}

/**
 * @brief Selecting tasks to be processed in a core. Each iteration consists of the first free task in a Thread, if a Thread is empty, it is just ignored.
 * 
 * 
 * @returns Total number of iterations spent processing. 
*/
void processer_random_preemptive_process(void)
{
    /* Total number of iterations spent processing in all cores. An iteration is considered after processing each group of tasks. */
    int it = 0;
    bool finished = false;

    for ( int i = 0; i < array_size(processdata.threads); i++ ) printf("TID %d - CONTENTION %d\n", thread_gettid(array_get(processdata.threads, i)), thread_contention(array_get(processdata.threads, i)));

    /* 
       Stores the current workload processed in each thread. It is used to propagate the waiting time to all tasks in thread.
       This value is zeroed at the end, and the max(aux) value is considered. 
    */
    int *aux = smalloc(array_size(processdata.threads));
    for ( int i = 0; i < array_size(processdata.threads); i++ ) aux[i] = 0;

    /* 
        Number of iterations that will be spent processing threads. 
        It will be the thread will higher number of tasks, since we are simulating a parallel processing.
    */
    int max_it = 0;

    while ( !finished )
    {
        finished = true;
        /* We are simulating a parallel processing here, all cores will process "at the same time".  */
        for ( int i = 0; i < array_size(processdata.cores); i++ )
        {
            core_tt c = array_get(processdata.cores, i);
            /* If all cores are done finishing, then we're done :) */
            if ( queue_size(core_get_ths(c)) == 0 )
                continue;
            
            /* If atleast one core is not done, we're not done yet ;[ */
            finished = false;

            queue_tt c_threads = core_get_ths(c);
            /* Current number of threads in core. */
            int c_numthreads = queue_size(c_threads);

            /* Populate core's queue of tasks. We must have only one task (per thread) per iteration. */
            for ( /*noop*/; c_numthreads > 0; /*noop*/ )
            {
                thread_tt t = queue_remove(c_threads);

                if ( queue_size(thread_tasks(t)) != 0 ) 
                {
                    int t_it = queue_size(thread_tasks(t));
                    /* Adding into pr_tasks the first task in Thread */
                    queue_insert(core_get_tsks(c), queue_remove(thread_tasks(t)));


                    // if ( t_it > max_it ) max_it = t_it;
                    /* Recycling a Thread only if it still got any task in it */
                    if ( t_it > 0 ) queue_insert(c_threads, t);
                }

                c_numthreads = queue_size(c_threads);
            }

            random_preemptive->execute(c, aux);
        }
    }

    /* Finding the MAX waiting time. We must keep in mind that our theads waits until all threads are free to get the next batch of tasks. */
    int max = 0;
    for ( int i = 0; i < array_size(processdata.threads); i++ ) { if (max < aux[i]) max = aux[i]; }
    for ( int i = 0; i < array_size(processdata.threads); i++ ) thread_set_lastwork(array_get(processdata.threads,i), max);

    free(aux);

    it = max_it;
    /* Cleaning up. */
    for ( int i = 0; i < array_size(processdata.cores); i++ )
        core_vacate(array_get(processdata.cores, i));

    /* Max processing time spent (max) + preparation time (it) */
    *(processdata.g_iterator) += (max + it);
}


void processer_random_preemptive_execute(struct core *c, int *aux)
{
    /* Sanity check. */
    assert(c != NULL);


    while ( !queue_empty(core_get_tsks(c)) )
    {
        task_tt ts = queue_remove(core_get_tsks(c));
        int contention = 0, 
            thread_pos = 0;

        /* 
           Getting the contention value from thread that received current task. 
           This search is necessary because threads might not be pinned (they will be sorted), 
           so we can't map aux[thread_id] = thread_contention
        */
        for ( int i = 0; i < array_size(processdata.threads); i++ ) 
        { 
            if ( thread_gettid(array_get(processdata.threads, i)) == task_assigthread(ts) ) 
            {
                contention = thread_contention(array_get(processdata.threads, i));
                thread_pos = i;
                break;
            }
        }
        
        thread_tt t = array_get(processdata, thread_pos);

        /* 
            Aux is used because our threads must wait until them all are fully processed to schedule again.
            This implies that a non-balanced strategies will suffer more, since a thread that finished earlier
            will have to wait for all the others. 

            Contention is added into consideration. If a thread didn't suffer contention, this value is, actualy, subtracted from 'accum_waiting'.
        */
        int accum_waiting = *(processdata.g_iterator) + aux[task_assigthread(ts)] + contention;

        task_set_emoment(ts, accum_waiting - task_arrivaltime(ts) );

        int e_moment = task_emoment(ts),
            l_moment = task_lmoment(ts);
        printf("accum_waiting %d - contention %d - assign %d - tid %d\n", accum_waiting, contention, task_assigthread(ts), thread_gettid(array_get(processdata.threads, task_assigthread(ts))));
        /*  
            Waiting time will be either: 
            - Tasks' response time: How much task waited for the first processing moment;
            - Tasks' 'idle' time: The current time (e_moment) - the last moment that task was processed (l_moment).
        */
        int waiting_time = ( e_moment == 0 ) ? accum_waiting - task_arrivaltime(ts) : e_moment - l_moment;
        /* Saving the moment that task started to be processed. This value is adjusted with task arrival time */

        task_set_waiting_time(ts, task_waiting_time(ts) + waiting_time);
        int amount_processed = 0,
            amount_to_process = (rand() % (task_work_left(ts))) + 1; /* Range: [1 - task_work_left) */

        /* For convenience only. If you want to, just remove this 'for' here and use amount_to_process as your processed amount. */
        for ( /*noop*/; amount_processed < amount_to_process; amount_processed++ );    
        task_set_workprocess(ts, task_work_processed(ts) + amount_processed);
        aux[task_assigthread(ts)] += amount_processed;

        /* If a task has finished, we add into a different queue, otherwise, we 'recycle' it into workload. */
        if ( task_work_left(ts) == 0 ) queue_insert(processdata.finished, ts);
        else queue_insert_at(thread_tasks(t), ts, 0);
        
        printf("Task %d - Waiting time calc. %d - Arrival Time %d - Global Iterator %d - E_Moment %d - L_moment %d\n", task_workload(ts), waiting_time, task_arrivaltime(ts), *(processdata.g_iterator), task_emoment(ts), task_lmoment(ts));
        /* Saving the moment that task left. This value is also adjusted with task arrival time. This value is the same as saying: e_moment + amount_processed. */
        task_set_lmoment(ts, (accum_waiting + amount_processed) - task_arrivaltime(ts));
        printf("Task %d - Processed: %d - Left %d - Thread %d\n\n", task_workload(ts), amount_to_process, task_work_left(ts), task_assigthread(ts));
    }

}


/**
 * @brief Finalizes the non random_preemptive strategy.
 * 
 * @returns The queue of tasks that have finished its processing.
*/
queue_tt processer_random_preemptive_end(void)
{
    processdata.initialized = 0;
    return processdata.finished;
}

static struct processer _random_preemptive = {
    processer_random_preemptive_init,
    processer_random_preemptive_process,
    processer_random_preemptive_execute,
    processer_random_preemptive_end
};

const struct processer *random_preemptive = &_random_preemptive;