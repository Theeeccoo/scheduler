/*
 * Copyright(C) 2016 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * This file is part of Scheduler.
 *
 * Scheduler is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * Scheduler is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Scheduler; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>

#include <mylib/util.h>
#include <mylib/queue.h>

#include <scheduler.h>

/**
 * @brief FCFS scheduler data.
 */
static struct
{
	workload_tt workload; /**< Workload.                     */
	int batchsize;        /**< Batchsize.                    */
    int initialized;      /**< Strategy already initialized? */
} scheddata = { NULL, 1, 0 };

/**
 * @brief Initializes the FCFS scheduler.
 * 
 * @param workload  Target workload.
 * @param batchsize Batch size.
 */
void scheduler_fcfs_init(workload_tt workload, int batchsize)
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
 * @brief Finalizes the FCFS scheduler.
 */
void scheduler_fcfs_end(void)
{
	scheddata.initialized = 0;
}

/**
 * @brief FCFS scheduler.
 * 
 * @param running Target queue of running cores.
 * @param c       Target core.
 * 
 * @returns Number of scheduled tasks,
 */
int scheduler_fcfs_sched(core_tt c)
{
	int n = 0;       /* Number of tasks scheduled.      */
	int wsize = 0;   /* Size of assigned work.          */
	int wk_size = workload_totaltasks(scheddata.workload); /* Total number of left tasks in workload. */
	int cr_size = workload_currtasks(scheddata.workload);  /* Current number of tasks that have 'arrived'. */

	/* We should schedule when there are, atleast, batchsize tasks free OR whenever the total left has arrived. */
	if ( cr_size >= scheddata.batchsize || wk_size == cr_size )
	{
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
 * @brief FCFS scheduler.
 */
static struct scheduler _sched_fcfs = {
	false,
	scheduler_fcfs_init,
	scheduler_fcfs_sched,
	scheduler_fcfs_end
};

const struct scheduler *sched_fcfs = &_sched_fcfs;
