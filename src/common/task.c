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
#include <stdbool.h>

#include <task.h>
#include <mylib/util.h>



/*============================================================================*
 * MEMORY                                                                     *
 *============================================================================*/

struct mem
{
	int memaddr;  /**< Memory address. */
};

/**
 * @brief Instantiate a new memory address.
 * 
 * @param addr Target address.
*/
void* mem_create(int addr)
{
	struct mem *mem;

	assert(addr >= 0);

	mem = smalloc(sizeof(struct mem));

	mem->memaddr = addr;
	return (mem);
}




/*============================================================================*
 * TASK                                                                       *
 *============================================================================*/

/**
 * @brief Task.
 */
struct task
{
	int tsid;           /**< Task identification number.             */
	int arrival_time;   /**< Which iteration current task arrived.   */
	int waiting_time;   /**< Waiting time for completion of a task.  */
	int work;           /**< Total workload of a task.               */
	int work_processed; /**< Total workload processed.               */ 

	array_tt memacc;    /**< Tasks memory accesses.                  */
	int memptr;         /**< Points to the last memory accessed.     */

	int e_moment;       /**< Moment when task (re)entered in a core. */
	int l_moment;       /**< Moment when task left from a core.      */
};

/**
 * @brief Next available task identification number.
 */
static int next_tsid = 0;

/**
 * @brief Creates a task.
 *
 * @param work    Workload of a task.
 * @param arrival Arrival moment of a task.
 *
 * @returns A task.
 */
struct task *task_create(int work, int arrival)
{
	struct task *task;

	/* Sanity check. */
	assert(work >= 0);

	task = smalloc(sizeof(struct task));

	task->tsid = next_tsid++;
	task->arrival_time = arrival;
	task->waiting_time = 0;
	task->work = work;
	task->work_processed = 0;
	task->e_moment = 0;
	task->l_moment = 0;

	task->memacc = array_create(work);
	task->memptr = 0;

	return (task);
}

/**
 * @brief Sets the arrival time of given task.
 * 
 * @param ts   Target task.
 * @param time Arrival time.
 */
void task_set_arrivaltime(struct task *ts, int time)
{
	/* Sanity check. */
    assert(ts != NULL);
	assert(time >= 0);

    ts->arrival_time = time;
}

/**
 * @brief Returns the arrival time of given task.
 * 
 * @param ts Target task.
 * 
 * @returns Arrival time of specified task.
 */
int task_arrivaltime(const struct task *ts)
{
	/* Sanity check. */
    assert(ts != NULL);

    return(ts->arrival_time);
}

/**
 * @brief Sets waiting time to a task. 
 * 
 * @param ts           Target task.
 * @param waiting_time How much task has waited.
 */
void task_set_waiting_time(struct task *ts, int waiting_time)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(waiting_time >= 0);

	ts->waiting_time = waiting_time;
}

/**
 * @brief Sets the amount of processed work of a task. 
 * 
 * @param ts   Target task.
 * @param work How much task has processed.
 */
void task_set_workprocess(struct task *ts, int work)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(work >= 0);

	ts->work_processed = work;
}

/**
 * @brief Sets the moment when task entered a core to be processed. 
 * If processing algorithm is based in a preemptive approach, this value will vary. 
 * 
 * @param ts     Target task.
 * @param moment Moment when task entered.
*/
void task_set_emoment(struct task *ts, int moment)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(moment >= 0);

	ts->e_moment = moment;
}

/**
 * @brief Gets the last moment that a task entered in a core. 
 * 
 * @param ts Target task.
 * 
 * @returns Returns the last moment that a task entered in a core.
*/
int task_emoment(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return ts->e_moment;
}

/**
 * @brief Sets the moment when task left from a core. 
 * If processing algorithm is based in a preemptive approach, this value vary. 
 * 
 * @param ts     Target task.
 * @param moment Moment when task entered.
*/
void task_set_lmoment(struct task *ts, int moment)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(moment >= 0);

	ts->l_moment = moment;
}

/**
 * @brief Creates task's memory address that will be accessed.
 * 
 * @param ts Target task. 
*/
void task_create_memacc(struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	/* Storing randomly-generated task's memory acesses. Will contain task_workload random "memory addresses" */
	for ( int i = 0; i < ts->work; i++ )
	{
		struct mem *m = mem_create((rand() % ts->work)); /* Generating the memory accesses. Possible addresses: [0 - task_workload) */
		array_set(ts->memacc, i, m);
	}
}

/**
 * @brief Sets task's memory address that will be accessed.
 * 
 * @param ts Target task. 
 * @param a  Array of memory addresses.
*/
void task_set_memacc(struct task *ts, array_tt a)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(a != NULL);

	ts->memacc = a;
}

/**
 * @brief Sets task's memory pointer.
 * 
 * @param t   Target task.
 * @param pos Position of last memory address accessed.
*/
void task_set_memptr(struct task *ts, int pos)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(pos >= 0);
	assert(pos < task_workload(ts));

	ts->memptr = pos;
}

/**
 * @brief Gets the last moment that a task left in a core. 
 * 
 * @param ts Target task.
 * 
 * @returns Returns the last moment that a task left in a core.
*/
int task_lmoment(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return ts->l_moment;
}


/**
 * @brief Returns waiting time of a task. 
 * 
 * @param ts Target task.
 * 
 * @returns Waiting time of a task.
 */
int task_waiting_time(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return(ts->waiting_time);
}

/**
 * @brief Returns the workload of given task.
 * 
 * @param ts Target task.
 * 
 * @returns Workload of specified task.
 */
int task_workload(const struct task *ts)
{
	/* Sanity check. */
    assert(ts != NULL);

    return(ts->work);
}


/**
 * @brief Assigns a new workload to given task.
 * 
 * @param ts Target task.
 */
void task_set_workload(struct task *t, int w)
{
	/* Sanity check. */
    assert(t != NULL);
	assert(w >= 0);

	t->work = w;
}


/**
 * @brief Returns the total processed workload of given task.
 * 
 * @param ts Target task.
 * 
 * @returns Total processed workload of specified task.
 */
int task_work_processed(const struct task *ts)
{
	/* Sanity check. */
    assert(ts != NULL);

    return(ts->work_processed);
}

/**
 * @brief Returns how much work is left to be processed on given task.
 * 
 * @param ts Target task.
 * 
 * @returns How much work is left to be processed on specified task.
 */
int task_work_left(const struct task *ts)
{
	/* Sanity check. */
    assert(ts != NULL);

    return(ts->work - ts->work_processed);
}

/**
 * @brief Returns task's memory addresses.
 * 
 * @param ts Target task. 
 * 
 * @return Task's memory addresses.
*/
array_tt task_memacc(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return (ts->memacc);
}

/**
 * @brief Returns task's memory pointer.
 * 
 * @param Target task.
 * 
 * @returns Task's memory pointer.
*/
int task_memptr(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return (ts->memptr);
}

/**
 * @brief Gets the ID of a task.
 *
 * @param ts Target task.
 *
 * @returns The ID of the target task.
 */
int task_gettsid(const struct task *ts)
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

	array_destroy(ts->memacc);
	free(ts);
}