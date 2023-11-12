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
#include <mylib/dqueue.h>
#include <mylib/queue.h>

#include <scheduler.h>

/**
 * @brief Fcfs scheduler data.
 */
static struct
{
<<<<<<< HEAD:src/simsched/static.c
	workload_tt workload; /**< Workload.   */
	array_tt threads;           /**< Threads.    */
	thread_tt *taskmap;         /**< Scheduling. */
	int chunksize;              /**< Chunksize.  */
} scheddata = { NULL, NULL, NULL, 1 };
=======
	workload_tt workload; /**< Workload.                     */
	int batchsize;        /**< Batchsize.                    */
    int initialized;      /**< Strategy already initialized? */
} scheddata = { NULL, 1, 0 };
>>>>>>> 7f1db94 (Removing thread structure from simulation):src/simsched/fcfs.c

/**
 * @brief Initializes the fcfs scheduler.
 * 
 * @param workload  Target workload.
<<<<<<< HEAD:src/simsched/static.c
 * @param threads   Target threads.
 * @param chunksize Chunk size.
 */
void scheduler_static_init(workload_tt workload, array_tt threads, int chunksize)
{
	int tidx;      /* Index of working thread. */
	int ntasks;    /* Workload size.           */
	
	/* Sanity check. */
	assert(workload != NULL);
	assert(threads != NULL);
	assert(chunksize > 0);
=======
 * @param batchsize Batch size.
 */
void scheduler_fcfs_init(workload_tt workload, int batchsize)
{	
	/* Sanity check. */
	assert(workload != NULL);
	assert(batchsize > 0);
>>>>>>> 7f1db94 (Removing thread structure from simulation):src/simsched/fcfs.c

	/* Already initialized. */
	if (scheddata.taskmap != NULL)
		return;
	
	ntasks = workload_ntasks(workload);

	/* Initialize scheduler data. */
	scheddata.workload = workload;
<<<<<<< HEAD:src/simsched/static.c
	scheddata.threads = threads;
	scheddata.taskmap = smalloc(ntasks*sizeof(thread_tt));
		
	/* Assign tasks to threads. */
	tidx = 0;
	for (int i = 0; i < ntasks; i += chunksize)
	{
		for (int j = 0; j < chunksize; j++)
		{
			if (i + j >= ntasks)
				break;

			scheddata.taskmap[i + j] = array_get(threads, tidx);
		}

		nchunks++;
		tidx = (tidx + 1)%array_size(threads);
	}
=======
    scheddata.batchsize = batchsize;
    scheddata.initialized = 1;
>>>>>>> 7f1db94 (Removing thread structure from simulation):src/simsched/fcfs.c
}

/**
 * @brief Finalizes the fcfs scheduler.
 */
void scheduler_fcfs_end(void)
{
	free(scheddata.taskmap);
	scheddata.taskmap = NULL;
}

/**
 * @brief Fcfs scheduler.
 * 
 * @param running Target queue of running cores.
 * @param c       Target core.
 * 
 * @returns Number scheduled tasks,
 */
<<<<<<< HEAD:src/simsched/static.c
int scheduler_static_sched(dqueue_tt running, thread_tt t)
=======
int scheduler_fcfs_sched(core_tt c)
>>>>>>> 7f1db94 (Removing thread structure from simulation):src/simsched/fcfs.c
{
	int n = 0;     /* Number of tasks scheduled. */
	int wsize = 0; /* Size of assigned work.     */

	/* Get next tasks. */
	for (int i = 0; i < workload_ntasks(scheddata.workload); i++)
	{
<<<<<<< HEAD:src/simsched/static.c
		/* Skip tasks from other threads. */
		if (scheddata.taskmap[i] != t)
			continue;

		n++;
		wsize += workload_task(scheddata.workload, i);
		thread_assign(t, workload_task(scheddata.workload, i));
=======
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
		nchunks++;
>>>>>>> 7f1db94 (Removing thread structure from simulation):src/simsched/fcfs.c
	}
	
	dqueue_insert(running, t, wsize);

	return (n);
}

/**
 * @brief Fcfs scheduler.
 */
static struct scheduler _sched_fcfs = {
	false,
	scheduler_fcfs_init,
	scheduler_fcfs_sched,
	scheduler_fcfs_end
};

const struct scheduler *sched_fcfs = &_sched_fcfs;
