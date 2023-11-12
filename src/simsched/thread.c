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

#include <mylib/util.h>

#include <thread.h>

/**
 * @brief Thread.
 */
struct thread
{
	int tid;                  /**< Identification number,                  */
	int wtotal;               /**< Total assigned workload.                */
	int capacity;             /**< Processing capacity.                    */
	int num_assigned_tasks;   /**< Number of assigned tasks to a thread.   */
	int num_processed_tasks;  /**< Number of processed tasks.              */
	task_tt *tasks;           /**< Tasks allocated to a thread.            */
};

/**
 * @brief Next available thread identification number.
 */
static int next_tid = 0;

/**
 * @brief Creates a thread.
 *
 * @param capacity Processing capacity.
 * @param ntasks   MaxNumber of tasks that a thread might be assigned with.
 *
 * @returns A thread.
 */
struct thread *thread_create(int capacity, int ntasks)
{
	struct thread *t;

	/* Sanity check. */
	assert((capacity >= 1) && (capacity <= 100));

	t = smalloc(sizeof(struct thread));

	t->tid = next_tid++;
	t->wtotal = 0;
	t->num_assigned_tasks = 0;
	t->num_processed_tasks = 0;
	t->capacity = capacity;

	// An optimization may be handy here
	// For the moment, allocatint ntasks for each thread
	t->tasks = smalloc(sizeof(task_tt) * ntasks);

	return (t);
}

/**
 * @brief Destroys a thread.
 *
 * @param t Target thread.
 */
void thread_destroy(struct thread *t)
{
	/* Sanity check. */
	assert(t != NULL);

	for ( int i = 0; i < t->num_assigned_tasks; i++ )
		task_destroy(t->tasks[i]);
	free(t);
}

/**
 * @brief Gets the ID of a thread.
 *
 * @param t Target thread.
 *
 * @returns The ID of the target thread.
 */
int thread_gettid(const struct thread *t)
{
	/* Sanity check. */
	assert(t != NULL);

	return (t->tid);
}

/**
 * @brief Returns the total workload assigned to a thread.
 *
 * @param t Target thread.
 *
 * @returns The total workload assigned to a thread.
 */
double thread_wtotal(const struct thread *t)
{
	/* Sanity check. */
	assert(t != NULL);

	return (((double)t->wtotal)*t->capacity);
}

/**
 * @brief Assigns a workload to a thread.
 *
 * @param t     Target thread.
 * @param wsize Work size.
 *
 * @returns Required processing time.
 */
int thread_assign(struct thread *t, int wsize)
{
	/* Sanity check. */
	assert(t != NULL);
	int required_procss_time = t->capacity * wsize;
	t->wtotal += wsize;
	t->tasks[t->num_assigned_tasks++] = task_create(required_procss_time);

	return required_procss_time;
}

/**
 * @brief Returns the number of processed tasks by a thread.
 *
 * @param t Target thread.
 *
 * @returns The total number of processed tasks by a thread.
 */
int thread_num_processed_tasks(struct thread *t)
{
	/* Sanity check. */
	assert(t != NULL);

	return (t->num_processed_tasks);
}

/**
 * @brief Returns the number of assigned tasks to a thread.
 *
 * @param t Target thread.
 *
 * @returns The total number of assigned tasks to a thread.
 */
int thread_num_assigned_tasks(struct thread *t)
{
	/* Sanity check. */
	assert(t != NULL);
	assert(t->num_assigned_tasks >= 0);

	return (t->num_assigned_tasks);
}

/**
 * @brief Increase the number of processed tasks by a thread.
 *
 * @param t Target thread.
 *
 * @returns The new total number of processed tasks by a thread.
 */
int thread_increase_processed_tasks(struct thread *t)
{
	/* Sanity check. */
	assert(t != NULL);
	assert(t->num_processed_tasks <= t->num_assigned_tasks);
	t->num_processed_tasks++;
	return (t->num_processed_tasks);
}

/**
 * @brief Returns the required processing time of a given task by a thread.
 * 
 * @param t   Target thread.
 * @param idx Index of a specific task.
 * 
 * @returns Required processing time of given workload at the moment.
 */
int thread_required_process_time(struct thread *t, int idx)
{
	assert(t != NULL);
	assert(idx < t->num_assigned_tasks);
	return (t->capacity * task_get_workload(t->tasks[idx]));
}

/**
 * @brief Returns the workload of a specific task.
 * 
 * @param t   Target thread.
 * @param idx Index of a specific task.
 * 
 * @returns Workload of specified task
 */
int thread_task(struct thread *t, int idx)
{
	assert(t != NULL);
	assert(idx < t->num_assigned_tasks);
	return (task_get_workload(t->tasks[idx]));
}

/**
 * @brief Returns a specific thread's task.
 * 
 * @param t   Target thread.
 * @param idx Index of a specific task.
 * 
 * @returns A specific task
 */
struct task *thread_get_task(struct thread *t, int idx)
{
	assert(t != NULL);
	assert(idx < t->num_assigned_tasks);
	return (t->tasks[idx]);
}

/**
 * @brief Returns the capacity of a thread.
 *
 * @param t target thread.
 *
 * @returns The capacity of the target thread.
 */
int thread_capacity(const struct thread *t)
{
	/* Sanity check. */
	assert(t != NULL);

	return (t->capacity);
}
