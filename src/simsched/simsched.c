#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include <mylib/util.h>
#include <mylib/array.h>
#include <mylib/dqueue.h>
#include <mylib/queue.h>

#include <core.h>
#include <scheduler.h>
#include <workload.h>

/**
 * @brief Global iterator.
*/
int g_iterator = 0;

/**
 * @brief Ready cores.
 */
static queue_tt ready;

/**
 * @brief Processing cores.
*/
static queue_tt processing;

/**
 * @brief Running cores.
 */
static queue_tt running;

void sort_ascending(int *a, int nelements, int *m, int *h, int *mi, float *sl)
{
	/* Sanity check. */
	assert(a != NULL);
	assert(m != NULL);
	assert(h != NULL);
	assert(mi != NULL);
	assert(sl != NULL);
	assert(nelements > 0);

	/* Short array. */
	for ( int i = 0; i < nelements; i++ )
	{
		for ( int j = 0; j < nelements; j++ )
		{
			if ( a[j] > a[i] )
			{
				int tmp; /* Temporary data. */
				float tmpf;

				tmp = a[i];
				a[i] = a[j];
				a[j] = tmp;

				tmp = m[i];
				m[i] = m[j];
				m[j] = tmp;

				tmp = h[i];
				h[i] = h[j];
				h[j] = tmp;

				tmp = mi[i];
				mi[i] = mi[j];
				mi[j] = tmp;

				tmpf = sl[i];
				sl[i] = sl[j];
				sl[j] = tmpf;
			}
		}
	}
	
}

/**
 * @brief Dumps simulation statistics.
 *
 * @param cores    Working cores.
 * @param workload Workload.
 */
static void simsched_dump(array_tt cores, workload_tt w)
{
	double min, max, total;
	double mean, stddev;
	int rounded_index;
	int ntasks = workload_ntasks(w);
	int ncores = array_size(cores);

	min = INT_MAX; max = INT_MIN;
	total = 0; mean = 0.0; stddev = 0.0;

	/* Compute min, max, total. */
	for (int i = 0; i < ncores; i++)
	{
		core_tt c;     /* Working core.         */
		double wtotal; /* Workload assigned to c. */

		c = array_get(cores, i);
		wtotal = core_wtotal(c);
		if ( wtotal == 0 ) continue;

		if (min > wtotal)
			min = wtotal;
		if (max < wtotal)
			max = wtotal;

		total += wtotal;
	}

	/* Compute mean. */
	mean = ((double) total)/ncores;

	/* Compute stddev. */
	for (int i = 0; i < ncores; i++)
	{
		core_tt c;

		c = array_get(cores, i);

		stddev += pow(core_wtotal(c) - mean, 2);
	} 
	stddev = sqrt(stddev/(ncores));

	/** Calculating 99th percentile */
	int *map           = smalloc(sizeof(int) * ntasks), // 
		*waiting_times = smalloc(sizeof(int) * ntasks), // Used only for debug purposes            
		*task_hits     = smalloc(sizeof(int) * ntasks), //
		*task_misses   = smalloc(sizeof(int) * ntasks), //
		 percentile = 0,
		 k          = 0;
	float *task_slowdown = smalloc(sizeof(float) * ntasks);

	double percentile_index = 0.0f;
	for ( k = 0; k < ntasks; k++ )
	{
		task_tt curr_task = queue_peek(workload_fintasks(w), k);
		map[k] = task_gettsid(curr_task);
		waiting_times[k] = task_waiting_time(curr_task);
		task_hits[k] = task_hit(curr_task);
		task_misses[k] = task_miss(curr_task);
		task_slowdown[k] = (((float) task_waiting_time(curr_task) + (float) task_workload(curr_task)) / ((float) task_workload(curr_task)));
	}

	sort_ascending(waiting_times, ntasks, map, task_hits, task_misses, task_slowdown);

	// Mapping Task id with its corresponding accumulative waiting_time, cache hits and cache misses.
	for ( int i = 0; i < k; i++)
	{
		printf("%d %d %d %d %lf\n", map[i], waiting_times[i], task_hits[i], task_misses[i], task_slowdown[i]);
	}
	percentile_index = (0.99 * ntasks) - 1;
		
	rounded_index = round(percentile_index);
	percentile = ( percentile_index == rounded_index ) ? (waiting_times[rounded_index] + waiting_times[rounded_index + 1]) / 2 : waiting_times[rounded_index];
	int sum = 0;
	for ( int i = 0; i < queue_size(workload_fintasks(w)); i++ )
	{
		task_tt ts = queue_peek(workload_fintasks(w), i);
		sum += task_waiting_time(ts);
	}

	/** Print statistics. */
	printf("waiting time sum: %d\n", sum);
	printf("99th Percentile: %d\n", percentile);
	printf("time: %lf\n", max);
	printf("cost: %lf\n", max*ncores);
	printf("performance: %lf\n", total/max);
	printf("total: %lf\n", total);
	printf("cov: %lf\n", stddev/mean);
	printf("slowdown: %lf\n", max/((double) min));

	free(map);
	free(waiting_times);
	free(task_hits);
	free(task_misses);
}

/**
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
		queue_insert(ready, c);
	}
}

/**
 * @brief Cleaning queues.
 */
static void threads_join(void)
{
	queue_destroy(running);
	queue_destroy(ready);
	queue_destroy(processing);
}

/**
 * @brief Chooses a core to run next.
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
 * @brief Simulates a parallel execution.
 *
 * @param w         Workload.
 * @param strategy  Scheduling strategy.
 * @param processer Processing strategy.
 * @param cores     Working cores.
 * @param batchsize Batchsize;
 */
void simsched(workload_tt w, array_tt cores, const struct scheduler *strategy, const struct processer *processer, int batchsize)
{
	/* Sanity check. */
	assert(w != NULL);
    assert(cores != NULL);
	assert(strategy != NULL);
	assert(processer != NULL);

	cores_spawn(cores, strategy->pincores);
	strategy->init(w, batchsize);
	processer->init(w, cores, &g_iterator);

    processing = queue_create();

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
		workload_checktasks(w, g_iterator);

		/* Idle. */
		if ( workload_currtasks(w) == 0 )
		{
			/* If no task arrived, we shouldn't propagate any contention value. */
			for ( int i = 0; i < array_size(cores); i++ ) core_set_contention(array_get(cores, i), 0);
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

				queue_insert(processing, c);

			/* Otherwise, keep waiting until enough tasks arrive. */
			} else queue_insert(ready, c);

			// printf("G_iterator %d - Workload_curr %d - Core ID %d \n", g_iterator, workload_currtasks(w), core_getcid(c));
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
	
		processer->process();


		while(!queue_empty(processing))
		{
			queue_insert(ready, queue_remove(processing));
		}

		// /* Reschedule running threads. */
		// while (!queue_empty(running))
		// {
		// 	queue_insert(ready, queue_remove(running));
		// 	// if (dqueue_next_counter(running) != 0)
		// 	// 	break;
		// }
	}

	strategy->end();
	processer->end();

	simsched_dump(cores, w);

	threads_join();
}