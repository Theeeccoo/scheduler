#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include <mylib/util.h>
#include <mylib/queue.h>

#include <scheduler.h>

/**
 * @brief SCA scheduler data.
*/
static struct
{
    workload_tt workload; /**< Workload.                     */
    int batchsize;        /**< Batchsize.                    */
    int initialized;      /**< Strategy already initialized? */
} scheddata = { NULL, 1, 0 };


/**
 * @brief Initializes the SCA scheduler.
 * 
 * @param workload  Target workload.
 * @param batchsize Batch size.
*/
void scheduler_sca_init(workload_tt workload, int batchsize)
{
    /* Sanity check. */
    assert(workload != NULL);
    assert(batchsize > 0);

    /* Already initialized. */
    if (scheddata.initialized)
        return;

    /* Initialize scheduler data. */
    scheddata.workload = workload;
    scheddata.batchsize = batchsize;
    scheddata.initialized = 1;
}

/** 
 * @brief Finalizes the SCA scheduler.
*/
void scheduler_sca_end(void)
{
    scheddata.initialized = 0;
}

/**
 * @brief SCA scheduler. A Task will be scheduled to the same core (always). If a task hasn't been scheduled yet, it goes to the first free core.
 * 
 * @param running Target queue of running cores.
 * @param c       Target core.
 * 
 * @returns Number of scheduled tasks.
*/
int scheduler_sca_sched(core_tt c)
{
    int n = 0;       /* Number of tasks scheduled.      */
	int wsize = 0;   /* Size of assigned work.          */
	int wk_size = workload_totaltasks(scheddata.workload); /* Total number of left tasks in workload. */
	int cr_size = workload_currtasks(scheddata.workload);  /* Current number of tasks that have 'arrived'. */

	
	/* We should schedule when there are, atleast, batchsize tasks free OR whenever the total left has arrived. */
	if ( cr_size >= scheddata.batchsize || wk_size == cr_size )
	{
		/* Get Tasks. */
		for ( int i = 0; i < cr_size; i++ )
		{
            /* Scheduled enough tasks. */
            if ( n == scheddata.batchsize) break;

            task_tt curr_task = queue_remove(workload_arrtasks(scheddata.workload));
            
            /* If task shouldn't be scheduled to "c", recycle it. */
            if ( task_core_assigned(curr_task) != -1 && task_core_assigned(curr_task) != core_getcid(c) )
            {
                queue_insert(workload_arrtasks(scheddata.workload), curr_task);
                continue;
            }

            core_populate(c, curr_task);
            task_core_assign(curr_task, core_getcid(c));
            wsize += task_work_left(curr_task);
            n++;
		}
	}
	
	/* If any task was scheduled, global 'time' must increase based on number of scheduled tasks. */
    g_iterator += ( n > 0 ) ? n : 1;
	

	return (n);
}

/**
 * @brief SRTF scheduler.
*/
static struct scheduler _sched_sca = {
	false,
	scheduler_sca_init,
	scheduler_sca_sched,
	scheduler_sca_end
};

const struct scheduler *sched_sca = &_sched_sca;