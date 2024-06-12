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
 * @brief FCFS scheduler. The first BATCHSIZE tasks will be scheduled to the first free core.
 * 
 * @param c     Target core.
 * @param tasks Mapped tasks to current core.
 * 
 * @returns Number of scheduled tasks,
 */
int scheduler_fcfs_sched(core_tt c, queue_tt tasks)
{
	int n = 0;       /* Number of tasks scheduled.      */
	int wsize = 0;   /* Size of assigned work.          */
	int cr_size = queue_size(tasks);  /* Current number of tasks that have 'arrived'. */
	int cr_cap = core_capacity(c); /* Core's total capacity. */

	/* Either we schedule what is left, or we schedule batchsize tasks. */
	int max = (cr_size > cr_cap) ? cr_cap : cr_size;
	/* Get Tasks. */
	for ( int i = 0; i < max; i++ )
	{
		task_tt curr_task = queue_remove(tasks);
		core_populate(c, curr_task);
		wsize += task_work_left(curr_task);
		n++;
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
