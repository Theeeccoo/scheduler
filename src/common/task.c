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
#include <stdio.h>

#include <task.h>
#include <mylib/util.h>

/**
 * @brief Task.
 */
struct task
{
	int tsid;                /**< Task identification number.           */
	int work;                /**< Workload of a task.                   */
	int waiting_time;        /**< Waiting time for completion of a task */
};

/**
 * @brief Next available task identification number.
 */
static int next_tsid = 0;

/**
 * @brief Creates a task.
 *
 * @param capacity Workload of a task.
 *
 * @returns A task.
 */
struct task *task_create(int work)
{
	struct task *task;

	/* Sanity check. */
	assert(work >= 0);

	task = smalloc(sizeof(struct task));

	task->tsid = next_tsid++;
	task->work = work;
	task->waiting_time = 0;

	return (task);
}

/**
 * @brief Returns the workload of given task
 * 
 * @param ts Target task
 * 
 * @returns Workload of specified task
 */
int task_get_workload(struct task *ts)
{
    assert(ts != NULL);

    return(ts->work);
}

/**
 * @brief Sets the workload of given task
 * 
 * @param ts       Target task
 * @param workload Workload
 */
void task_set_workload(struct task *ts, int workload)
{
    /* Sanity check. */
    assert(ts != NULL);
	assert(workload >= 0);

    ts->work = workload;
}

/**
 * @brief Sets the waiting time of given task
 * 
 * @param ts    Target task
 * @param wtime Waiting time
 */
void task_set_waiting_time(struct task *ts, int wtime)
{
	/* Sanity check. */
    assert(ts != NULL);
	assert(wtime >= 0);
    ts->waiting_time = wtime;
}

/**
 * @brief Returns the waiting time of given task
 * 
 * @param ts Target task
 * 
 * @returns Waiting time of specified task
 */
int task_get_waiting_time(struct task *ts)
{
    assert(ts != NULL);

    return(ts->waiting_time);
}

int task_gettsid(struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return (ts->tsid);
}

/**
 * @brief Destroys a task.
 *
 * @param ts Target task.
 */
void task_destroy(struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	free(ts);
}