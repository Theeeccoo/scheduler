#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include <mylib/util.h>
#include <mylib/queue.h>

#include <scheduler.h>

/**
 * @brief SRJF scheduler data.
*/
static struct
{
    workload_tt workload; /**< Workload.                     */
    int batchsize;        /**< Batchsize.                    */
    int initialized;      /**< Strategy already initialized? */
} scheddata = { NULL, 1, 0 };


/**
 * @brief Initializes the SRJF scheduler.
 * 
 * @param workload  Target workload.
 * @param batchsize Batch size.
*/
void scheduler_srjf_init(workload_tt workload, int batchsize)
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
 * @brief Finalizes the SRJF scheduler.
*/
void scheduler_sjrf_end(void)
{
    scheddata.initialized = 0;
}

/**
 * @brief SRJF scheduler. The first BATCHSIZE tasks will be scheduled to the first free core (tasks are sorted based on their remaining work)
 * 
 * @param running Target queue of running cores.
 * @param c       Target core.
 * 
 * @returns Number of scheduled tasks.
*/
int scheduler_srjf_sched(core_tt c)
{
    int n = 0;       /* Number of tasks scheduled.      */
	int wsize = 0;   /* Size of assigned work.          */
	int wk_size = workload_totaltasks(scheddata.workload); /* Total number of left tasks in workload. */
	int cr_size = workload_currtasks(scheddata.workload);  /* Current number of tasks that have 'arrived'. */

	
	/* We should schedule when there are, atleast, batchsize tasks free OR whenever the total left has arrived. */
	if ( cr_size >= scheddata.batchsize || wk_size == cr_size )
	{
		/* Sorting */
		workload_sort(scheddata.workload, WORKLOAD_REMAINING_WORK);
		/* Either we schedule what is left, or we schedule batchsize tasks. */
		int max = (cr_size > scheddata.batchsize) ? scheddata.batchsize : cr_size;
		/* Get Tasks. */
		for ( int i = 0; i < max; i++ )
		{
			task_tt curr_task = queue_remove(workload_arrtasks(scheddata.workload));
			core_populate(c, curr_task);
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
static struct scheduler _sched_srtf = {
	false,
	scheduler_srjf_init,
	scheduler_srjf_sched,
	scheduler_sjrf_end
};

const struct scheduler *sched_srtf = &_sched_srtf;