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
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include <mylib/util.h>
#include <mylib/array.h>
#include <mylib/dqueue.h>
#include <mylib/queue.h>

#include <scheduler.h>
#include <workload.h>
#include <thread.h>

/**
 * @brief Number of chunks.
 */
int nchunks = 0;

/**
<<<<<<< HEAD
 * @brief Ready threads.
=======
 * @brief Global iterator.
*/
int g_iterator = 0;

/**
 * @brief Ready cores.
>>>>>>> 7f1db94 (Removing thread structure from simulation)
 */
static queue_tt ready;

/**
<<<<<<< HEAD
 * @brief Running threads.
=======
 * @brief Processing cores.
*/
static queue_tt processing;

/**
 * @brief Running cores.
>>>>>>> 7f1db94 (Removing thread structure from simulation)
 */
static dqueue_tt running;

/**
 * @brief Spawns threads.
 *
 * @param threads Working threads.
 * @param pin     Pin threads?
 */
static void threads_spawn(array_tt threads, bool pinthreads)
{
	ready = queue_create();	
	running = dqueue_create();

	if (!pinthreads)
		array_shuffle(threads);

	for (int i = 0; i < array_size(threads); i++)
	{
		thread_tt t = array_get(threads, i);
		queue_insert(ready, t);
	}
}

/**
 * @brief Joins threads.
 */
static void threads_join(void)
{
	dqueue_destroy(running);
	queue_destroy(ready);
}

/**
 * @brief Dumps simulation statistics.
 *
 * @param cores    Working cores.
 * @param ntasks   Total number of tasks
 * @param finished Array of finished tasks
 */
static void simsched_dump(array_tt threads)
{
	double min, max, total;
	double mean, stddev;
	int nthreads;

	nthreads = array_size(threads);

	min = INT_MAX; max = INT_MIN;
	total = 0; mean = 0.0; stddev = 0.0;

	/* Compute min, max, total. */
	for (int i = 0; i < nthreads; i++)
	{
		thread_tt t;   /* Working thread.         */
		double wtotal; /* Workload assigned to t. */

		t = array_get(threads, i);
		wtotal = thread_wtotal(t);
		if ( wtotal == 0 ) continue;

		if (min > wtotal)
			min = wtotal;
		if (max < wtotal)
			max = wtotal;

		total += wtotal;
	}

	/* Compute mean. */
	mean = ((double) total)/nthreads;

	/* Compute stddev. */
	for (int i = 0; i < nthreads; i++)
	{
		thread_tt t;

		t = array_get(threads, i);

		stddev += pow(thread_wtotal(t) - mean, 2);
	}
	stddev = sqrt(stddev/(nthreads));

	/* Print statistics. */
	printf("nchunks: %d\n", nchunks);
	thread_tt aux = array_get(threads, 0);	
	for ( int i = 0; i < thread_num_processed_tasks(aux); i++ ){
		printf("Task workload: %d // Task waiting time: %d - (tsid: %d)\n", task_get_workload(thread_get_task(aux, i)),
																	        task_get_waiting_time(thread_get_task(aux, i)),
																	        task_gettsid(thread_get_task(aux, i)));
	}

	printf("time: %lf\n", max);
	printf("cost: %lf\n", max*nthreads);
	printf("performance: %lf\n", total/max);
	printf("total: %lf\n", total);
	printf("cov: %lf\n", stddev/mean);
	printf("slowdown: %lf\n", max/((double) min));
}

/**
<<<<<<< HEAD
 * @brief Chooses a thread to run next.
=======
 * @brief Spawns cores.
 *
 * @param cores Working cores.
 * @param pin   Pin cores?
 */
static void cores_spawn(array_tt cores, bool pincores)
{
	ready = queue_create();	
	running = queue_create();

	if (!pincores)
		array_shuffle(cores);

	for (int i = 0; i < array_size(cores); i++)
	{
		core_tt c = array_get(cores, i);
		queue_insert(ready, t);
	}
}

/**
 * @brief Joins threads.
 */
static void threads_join(void)
{
	queue_destroy(running);
	queue_destroy(ready);
	queue_destroy(processing);
}

/**
 * @brief Chooses a core to run next.
>>>>>>> 7f1db94 (Removing thread structure from simulation)
 *
 * @param Target core queue.
 *
 * @returns The next core to run.
 */
static core_tt choose_core(queue_tt q)
{
	core_tt t;

	/* Sanity check. */
	assert(q != NULL);
	assert(!queue_empty(q));

	do
	{
		t = queue_remove(q);

		if (rand()%2)
			break;

		queue_insert(q, t);
	} while (!queue_empty(q));

	return (t);
}

/**
<<<<<<< HEAD
 * @brief Process the workload tasks assigned to a specified thread.
 *
 * @param t Target thread.
 */
static void process_thread(struct thread *t)
{
	int num_processed_tasks = thread_num_processed_tasks(t),
		num_assigned_tasks  = thread_num_assigned_tasks(t);

	if ( num_assigned_tasks == 0 )
		return;

	// First task (batch of workload) waits for nothing
	struct task *foo = thread_get_task(t, num_processed_tasks);
	task_set_waiting_time(foo, 0);

	for ( int i = num_processed_tasks + 1; i < num_assigned_tasks; i++ )
	{
		struct task *barz = thread_get_task(t, i);
		foo = thread_get_task(t, i - 1);

		// Setting the waiting time of current task to be
		// the last task's workload + its waiting time
		task_set_waiting_time(barz, task_get_workload(foo) + task_get_waiting_time(foo));
		thread_increase_processed_tasks(t);
	}
}

/**
 * @brief Simulates a parallel loop.
=======
 * @brief Simulates a parallel execution.
>>>>>>> 7f1db94 (Removing thread structure from simulation)
 *
 * @param w         Workload.
 * @param strategy  Scheduling strategy.
 * @param chunksize Chunksize;
 */
<<<<<<< HEAD
void simshed(workload_tt w, array_tt threads, const struct scheduler *strategy, int chunksize)
{
	/* Sanity check. */
	assert(w != NULL);
	assert(threads != NULL);
=======
void simsched(workload_tt w, array_tt cores, const struct scheduler *strategy, const struct processer *processer, int batchsize)
{
	/* Sanity check. */
	assert(w != NULL);
    assert(cores != NULL);
>>>>>>> 7f1db94 (Removing thread structure from simulation)
	assert(strategy != NULL);

<<<<<<< HEAD
	threads_spawn(threads, strategy->pinthreads);

	strategy->init(w, threads, chunksize);

	queue_tt aux = queue_create();

	/* Simulate. */
	for (int i = 0; i < workload_ntasks(w); /* noop */)
	{
		/* Schedule ready threads. */
		while (!queue_empty(ready))
		{
			thread_tt t;
=======
	cores_spawn(cores, strategy->pincores);
	strategy->init(w batchsize);
	processer->init(w, cores, &g_iterator);

    processing = queue_create();

	/* TODO Tirar o fato de que a simulação toda ta acontecendo em apenas um nucleo (feito no choose_core). Também, lembrar que as métricas deverão ser calculadas entre os núcleos. */
	/* TODO ver o que fazer com o dqueue running ai. Caso realmente pra voltar ele, olhar no código original onde ele tava. */
	
	/* 
	 * 'workload' is a critical region, so we must add a queue contention delay to proper analyse.
	 * This value is accumulative and calculated based on how many iterations was spent scheduling tasks.
	 */
	int queue_contention = 0;	
	int controller = 0;
	/* Simulate. */
	for ( /* noop */ ; workload_totaltasks(w) > 0 ; /* noop */ )
	{	
		controller = 0;

		/* Idle. */
		if ( workload_currtasks(w) == 0 )
		{
			/* If no task arrived, we shouldn't propagate any contention value. */
			for ( int i = 0; i < array_size(cores); i++ ) core_contention(array_get(cores, i), 0);
			/* While no task arrived. */
			while ( workload_currtasks(w) == 0 ) 
			{ 
				workload_checktasks(w, g_iterator);
				g_iterator++;
			}
		}

		// printf("1 - Global iterator: %d\n", g_iterator);

		/* Scheduling tasks to ready cores. */
        while(!queue_empty(ready))
        {
			/* Checking if there are tasks that have arrived while scheduling. */
			workload_checktasks(w, g_iterator);

			core_tt c = choose_core(ready);
			
            queue_contention = strategy->sched(c);
			controller += queue_contention;

			/* If any task was scheduled, send all cores to processing. */
			if ( controller != 0 )
			{
				/* 
				  But if none were scheduled to current core, we must desconsider that we 'looked' for more tasks.
				  As if we knew, beforehand, that there weren't enough tasks yet.
				*/
				if ( queue_contention == 0 ) g_iterator--;

				queue_insert(processing, t);

			/* Otherwise, keep waiting until enough tasks arrive. */
			} else queue_insert(ready, t);

			/*
				We are basing ourselves in g_iterator to indicate the waiting time of tasks (Check processing strategies).
				Since we can't prevent a core from trying to schedule (increasing the g_iterator), 
				we force those cores that shouldn't be affected by queue contention to have a negative
				value (by counting how many tasks were scheduled and removing it from how many tasks given core have)
				in order to remove this contention value from them.

				So, instead of adding contention values we are removing, from total time spent by scheduling between all cores, 
				the time that core 'c' spent scheduling.
			*/
			core_set_contention(c, -(queue_contention));
        }
		// printf("2- Global iterator: %d\n", g_iterator);

		/* We must have, atleast, one task scheduled to be able to process. */
		if ( controller != 0 )
		{
			processer->process();
			/* Scheduling ready threads to cores. */
			while(!queue_empty(processing))
			{
				thread_tt t;					//
				t = choose_thread(processing);  //
				core_tt c;              		//  Here we can add the Thread scheduling strategy.
				c = choose_core(cores); 		//
				core_populate(c, t);    		//
>>>>>>> 7f1db94 (Removing thread structure from simulation)

			t = choose_thread(ready);
			i += strategy->sched(running, t);
			queue_insert(aux, t);
		}
		/* Processing threads. */
		while(!queue_empty(aux)){
			thread_tt t;

			t = choose_thread(aux);
			process_thread(t);
		}
		/* Reschedule running threads. */
		while (!dqueue_empty(running))
		{
			queue_insert(ready, dqueue_remove(running));
			if (dqueue_next_counter(running) != 0)
				break;
		}
	}
	queue_destroy(aux);

	strategy->end();

	simsched_dump(threads);

	threads_join();
}

