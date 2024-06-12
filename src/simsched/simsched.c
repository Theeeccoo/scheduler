#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include <mylib/util.h>
#include <mylib/array.h>
#include <mylib/dqueue.h>
#include <mylib/queue.h>
#include <kmeans.h>

#include <core.h>
#include <model.h>
#include <ram.h>
#include <sched_itr.h>
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

void sort_ascending(unsigned long int *a, int nelements, unsigned long int *m, unsigned long int *ph, unsigned long int *pf, unsigned long int *h, unsigned long int *mi, float *sl, unsigned long int *ids)
{
	/* Sanity check. */
	assert(a != NULL);
	assert(m != NULL);
	assert(ph != NULL);
	assert(pf != NULL);
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
				unsigned long int tmp; /* Temporary data. */
				float tmpf;

				tmp = a[i];
				a[i] = a[j];
				a[j] = tmp;

				tmp = m[i];
				m[i] = m[j];
				m[j] = tmp;

				tmp = ph[i];
				ph[i] = ph[j];
				ph[j] = tmp;

				tmp = pf[i];
				pf[i] = pf[j];
				pf[j] = tmp;

				tmp = h[i];
				h[i] = h[j];
				h[j] = tmp;

				tmp = mi[i];
				mi[i] = mi[j];
				mi[j] = tmp;

				tmpf = sl[i];
				sl[i] = sl[j];
				sl[j] = tmpf;

				tmp = ids[i];
				ids[i] = ids[j];
				ids[j] = tmp;
			}
		}
	}
	
}

void one_sort_asceding(float *a, int nelements)
{
	assert(a != NULL);
	

	for ( int i = 0; i < nelements; i++ )
	{
		for ( int j = 0; j < nelements; j++ )
		{
			if ( a[j] > a[i] )
			{
				float tmp = a[i];
				a[i] = a[j];
				a[j] = tmp;
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
	unsigned long int min, max, total;
	double mean, stddev;
	int rounded_index;
	int ntasks = workload_ntasks(w);
	int ncores = array_size(cores);

	min = INT_MAX; max = 0;
	total = 0; mean = 0.0; stddev = 0.0;

	/* Compute min, max, total. */
	for (int i = 0; i < ncores; i++)
	{
		core_tt c;                /* Working core.         */
		unsigned long int wtotal; /* Workload assigned to c. */

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

	/** Calculating Metrics */
	unsigned long int ids[ntasks],                 //
		              map[ntasks],                 // 
		              waiting_times[ntasks],       //    
					  task_page_hits[ntasks],	   // Tasks' metrics
					  task_page_faults[ntasks],    //   
		              task_hits[ntasks],           //
		              task_misses[ntasks] ,        //
		              percentile_waitingtime = 0;
	int k = 0;
	float percentile_slowdown   = 0;
	float task_slowdown[ntasks];

	double percentile_index = 0.0f;
	for ( k = 0; k < ntasks; k++ )
	{
		task_tt curr_task = queue_peek(workload_fintasks(w), k);
		ids[k] = task_realid(curr_task);
		map[k] = task_gettsid(curr_task);
		waiting_times[k] = task_waiting_time(curr_task);
		task_page_hits[k] = task_page_hit(curr_task);
		task_page_faults[k] = task_page_fault(curr_task); 
		task_hits[k] = task_hit(curr_task);
		task_misses[k] = task_miss(curr_task);
		task_slowdown[k] = (((float) task_waiting_time(curr_task) + (float) task_workload(curr_task)) / ((float) task_workload(curr_task)));
	}

	sort_ascending(waiting_times, ntasks, map, task_page_hits, task_page_faults, task_hits, task_misses, task_slowdown, ids);

	
	/** Print statistics. */
	// Mapping Task id with its corresponding accumulative waiting_time, cache hits and cache misses.
	for ( int i = 0; i < k; i++)
	{
		printf("%3lu | %3lu | %10lu | %5lu %5lu | %5lu %5lu | %lf\n", ids[i], map[i], waiting_times[i], task_page_hits[i], task_page_faults[i], task_hits[i], task_misses[i], task_slowdown[i]);
	}
	one_sort_asceding(task_slowdown, ntasks);
	
	percentile_index = (0.99 * ntasks) - 1;
		
	rounded_index = round(percentile_index);
	percentile_waitingtime = ( percentile_index == rounded_index ) ? (waiting_times[rounded_index] + waiting_times[rounded_index + 1]) / 2 : waiting_times[rounded_index];
	percentile_slowdown = ( percentile_index == rounded_index ) ? ( task_slowdown[rounded_index] + task_slowdown[rounded_index + 1] ) / 2 : task_slowdown[rounded_index];
	unsigned long int sum = 0;
	for ( int i = 0; i < queue_size(workload_fintasks(w)); i++ )
	{
		task_tt ts = queue_peek(workload_fintasks(w), i);
		sum += task_waiting_time(ts);
	}

	// Analysing how well balanced was the scheduling.
	int num_itr = queue_size(core_workloads(array_get(cores, 0)));
	unsigned long int all_wrk[ncores];

	unsigned long int total_unbalancement = 0;
	for ( int i = 1; i < num_itr; i++ )
	{
		unsigned long int diff = 0;
		for( unsigned long int j = 0; j < array_size(cores); j++ )
		{
			core_tt c = array_get(cores, j);
			all_wrk[j] = scheditr_ntasks(queue_peek(core_workloads(c), i));
		}

		for (int j = 0; j < ncores; j++ )
		{
			for (int k = j + 1; k < ncores; k++ )
			{
				if ( all_wrk[j] > all_wrk[k] )
					diff += all_wrk[j] - all_wrk[k];
				else 
					diff += all_wrk[k] - all_wrk[j];

			}
		}
		total_unbalancement += diff;
	}

	unsigned long int page_hit = 0,
					  page_fault = 0,
					  cache_hit = 0,
	    			  cache_miss = 0;

	for ( unsigned long int i = 0; i < array_size(cores); i++ )
	{
		page_hit += core_page_hit(array_get(cores, i));
		page_fault += core_page_fault(array_get(cores, i));
		cache_hit += core_hit(array_get(cores, i));
		cache_miss += core_miss(array_get(cores, i));
	}

	printf("waiting time sum: %lu\n", sum);
	printf("99th Percentile Waiting Time: %ld\n", percentile_waitingtime);
	printf("99th Percentile Tasks' Slowdown: %f\n", percentile_slowdown);
	printf("Total page hits: %lu - Total page faults: %lu\n", page_hit, page_fault);
	printf("Total cache hits: %lu - Total cache misses: %lu\n", cache_hit, cache_miss);
	printf("Total Unbalancement: %lu\n", total_unbalancement);
	printf("time: %lu\n", max);
	printf("cost: %lu\n", max*ncores);
	printf("performance: %ld\n", total/max);
	printf("total: %lu\n", total);
	printf("cov: %lf\n", stddev/mean);
	printf("slowdown: %lf\n", max/((double) min));

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

	for ( unsigned long int i = 0; i < array_size(cores); i++ )
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

static void populate_queues_not_opt(struct workload *w, array_tt all_cores)
{
	/* Sanity Check. */
	assert(w != NULL);
	assert(all_cores != NULL);

	/* Tasks that are waiting for their first piece of processing time. */
	queue_tt waiting_tasks = (queue_tt) array_get(workload_arrtasks(w), array_size(workload_arrtasks(w)) - 1);
	/* Orphan tasks. Tasks that were processed, atleast once, and are waiting for a free core to be processed again. */
	queue_tt orphan_tasks = (queue_tt) array_get(workload_arrtasks(w), array_size(workload_arrtasks(w)) - 2);

	while ( queue_size(waiting_tasks) != 0 )
		queue_insert(orphan_tasks, queue_remove(waiting_tasks));
}

static void populate_queues_opt(struct workload *w, array_tt all_cores, int ncores)
{
	/* Sanity Check. */
	assert(w != NULL);
	assert(all_cores != NULL);

	/* Tasks that are waiting for their first piece of processing time. */
	queue_tt waiting_tasks = (queue_tt) array_get(workload_arrtasks(w), array_size(workload_arrtasks(w)) - 1);
	/* Orphan tasks. Tasks that were processed, atleast once, and are waiting for a free core to be processed again. */
	queue_tt orphan_tasks = (queue_tt) array_get(workload_arrtasks(w), array_size(workload_arrtasks(w)) - 2);

	/* How many tasks, that haven't received their initial piece of processing time, aren't scheduled at the moment. */
	int waiting_tasks_size = queue_size(waiting_tasks);
	/* How many tasks, that were scheduled once, aren't scheduled at the moment.*/
	int orphan_tasks_size = queue_size(orphan_tasks);
	

	bool keep_going = true;
	
	// Nothing to see here
	if ( waiting_tasks_size == 0 && orphan_tasks_size == 0 ) keep_going = false;

	while( keep_going )
	{
		int min_tasks_assigned = INT_MAX,
	    	min_core_pos = 0;
		keep_going = false;
		waiting_tasks_size = queue_size(waiting_tasks);
		orphan_tasks_size = queue_size(orphan_tasks);

		// If there are still any task left
		if ( waiting_tasks_size != 0 || orphan_tasks_size != 0 )
		{	
			// Iterate through all cores in order to find which is the one that has less tasks assigned to (but making sure that it don't surpass core's maximum capacity).
			for ( int i = 0; i < ncores; i++ )
			{
				core_tt core = (core_tt) array_get(all_cores, i);
				queue_tt core_tasks = (queue_tt) array_get(workload_arrtasks(w), core_getcid(core)); 
				int num_tasks_queue = queue_size(core_tasks);
				if ( num_tasks_queue < core_capacity(core) )
				{
					if ( num_tasks_queue < min_tasks_assigned )
					{
						min_tasks_assigned = num_tasks_queue;
						min_core_pos = i;
						keep_going = true;
					}
				}
			}
			
			if ( keep_going )
			{
				queue_tt core_tasks = (queue_tt) array_get(workload_arrtasks(w), core_getcid(array_get(all_cores, min_core_pos)));
				if ( orphan_tasks_size > 0 )
					queue_insert(core_tasks, queue_remove(orphan_tasks));
				else
					queue_insert(core_tasks, queue_remove(waiting_tasks));

				
			}
		}
	}

}

/**
 * @brief Clustering tasks by their last (winsize) memory acesses using KMeans.
 * 
 * @param w       Target workload
 * @param winsize Window size of memory accesses
 * @param k       Target KMeans model.
 */
static void group(struct workload *w, int winsize, struct kmeans *k)
{
	/* Sanity check. */
	assert(w != NULL);
	assert(k != NULL);
	assert(winsize >= 0);


	array_tt all_tasks = workload_arrtasks(w);
	// Getting the second-from-last queue of tasks (where all tasks that were processed are)
	queue_tt tasks = (queue_tt) array_get(all_tasks, array_size(all_tasks) - 2);
	int tasks_size = queue_size(tasks);


	int **values = (int**) malloc(sizeof(int*) * tasks_size);
	for ( int i = 0; i < tasks_size; i++ )
	{
		task_tt curr_task = queue_peek(tasks, i);
		values[i] = (int*) malloc(sizeof(int) * winsize);
		int mem_ptr = task_memptr(curr_task);
		unsigned long int* lines = task_lineacc(curr_task);

		// Getting the last WINSIZE accesses
		for ( int j = 0; j < winsize; j++ )
		{
			values[i][j] = lines[(mem_ptr - winsize) + j];
		}
	}

	kmeans_start(k, all_tasks, tasks, values, tasks_size);
	for ( int i = 0; i < tasks_size; i++ ) 
	{
		free(values[i]);
	}
	free(values);
}

/**
 * @brief Reinforcement Learning strategy to map tasks.
 * 
 * @param w     Target workload.
 * @param m     Target model.
 * @param cores All cores in our simulation.
 */
static void model_optimization(struct workload *w, struct model *m, array_tt cores)
{
	/* Sanity check. */
	assert(w != NULL);
	assert(m != NULL);
	assert(cores != NULL);

	array_tt all_tasks = (array_tt) workload_arrtasks(w);
	/* Orphan tasks. Tasks that were processed, atleast once, and are waiting for a free core to be processed again. */
	queue_tt orphan_tasks = (queue_tt) array_get(all_tasks, array_size(all_tasks) - 2);

	model_train(m, cores, all_tasks, orphan_tasks);

	int total_capacity = 0;
	for ( unsigned long int i = 0; i < array_size(cores); i++ )
	{
		queue_tt bucket = array_get(all_tasks, i);
		core_tt core = array_get(cores, i);
		// How much is left.
		total_capacity += (core_capacity(core) - queue_size(bucket));
	}

}

/**
 * @brief Simulates a parallel execution.
 *
 * @param w         Workload.
 * @param strategy  Scheduling strategy.
 * @param processer Processing strategy.
 * @param cores     Working cores.
 * @param batchsize Batchsize;
 * @param winsize   Memory accesses window size.
 * @param optimize  Optimize schedulers? 
 */
void simsched(workload_tt w, array_tt cores, const struct scheduler *strategy, const struct processer *processer, int batchsize, int winsize, int optimize)
{
	/* Sanity check. */
	assert(w != NULL);
    assert(cores != NULL);
	assert(strategy != NULL);
	assert(processer != NULL);

	RAM_tt RAM = RAM_init(w);
	cores_spawn(cores, strategy->pincores);
	strategy->init(w, batchsize);
	processer->init(w, cores, &g_iterator, RAM);

    processing = queue_create();
	/* 
	 * 'workload' is a critical region, so we must add a queue contention delay to proper analyse.
	 * This value is accumulative and calculated based on how many iterations was spent scheduling tasks.
	 */
	int queue_contention = 0;	
	int controller = 0;
	/* Simulate. */

	/**
	 * If decide to optimizate, tasks are grouped (KMeans using DTW distance) 
	 * based in their last memory accesses and assigned to cores based in selected scheduling strategy. 
	 * 
	 * If decided to don't optimize, tasks won't be grouped, and assignment to cores will follow the selected scheduling heuristic.
	 * 
	 * In theory, it's pretty much the same idea, the difference will be only the grouping idea. But, to prevent unnecessary verification
	 * each iteration, decided to create two different "branches".
	 */
	if ( optimize == 1)
	{
		kmeans_tt k = kmeans_create(100, array_size(cores), winsize);
		for ( /* noop */; workload_totaltasks(w) > 0; /* noop */)
		{    
			controller = 0;
			workload_checktasks(w, g_iterator);

			while ( workload_currtasks(w) < batchsize && workload_currtasks(w) != workload_totaltasks(w) )
			{
				workload_checktasks(w, g_iterator);
				g_iterator++;
			}
			
			/* Number of already processed tasks, i.e., Number of tasks in our second-from-last queue. */

			int num_tasks = queue_size( (queue_tt) array_get(workload_arrtasks(w), array_size(workload_arrtasks(w)) - 2) );

			if ( num_tasks >= batchsize )
				group(w, winsize, k);
			else 
				populate_queues_opt(w, cores, array_size(cores));

			/* Scheduling tasks to ready cores. */

			while ( !queue_empty(ready) )
			{
				core_tt c = choose_core(ready);

				/* Scheduling with core's "piece" of workload. */
				queue_contention = strategy->sched(c, (queue_tt) array_get(workload_arrtasks(w), core_getcid(c)));
				controller += queue_contention;

				if ( controller != 0 )
				{
					if ( queue_contention == 0 ) g_iterator--;
					queue_insert(processing, c);
				} else queue_insert(ready, c);

				core_set_contention(c, -(queue_contention));
			}
			
			processer->process();
	
			/* Cleaning up. */
			while(!queue_empty(processing))
				queue_insert(ready, queue_remove(processing));
		}
		kmeans_destroy(k);	
	} else if ( optimize == 2 ){
		for ( /* noop */; workload_totaltasks(w) > 0; /* noop */)
		{    
			controller = 0;
			workload_checktasks(w, g_iterator);

			while ( workload_currtasks(w) < batchsize && workload_currtasks(w) != workload_totaltasks(w) )
			{
				workload_checktasks(w, g_iterator);
				g_iterator++;
			}
			
			populate_queues_opt(w, cores, array_size(cores));

			/* Scheduling tasks to ready cores. */

			while ( !queue_empty(ready) )
			{
				core_tt c = choose_core(ready);

				/* Scheduling with core's "piece" of workload. */
				queue_contention = strategy->sched(c, (queue_tt) array_get(workload_arrtasks(w), core_getcid(c)));
				controller += queue_contention;

				if ( controller != 0 )
				{
					if ( queue_contention == 0 ) g_iterator--;
					queue_insert(processing, c);
				} else queue_insert(ready, c);

				core_set_contention(c, -(queue_contention));
			}
			
			processer->process();
	
			/* Cleaning up. */
			while(!queue_empty(processing))
				queue_insert(ready, queue_remove(processing));
		}
	} else if ( optimize == 3 )
	{
		model_tt model = model_create(array_size(cores), core_capacity(array_get(cores, 0)), winsize);
		for ( /* noop */; workload_totaltasks(w) > 0; /* noop */)
		{    
			controller = 0;
			workload_checktasks(w, g_iterator);

			while ( workload_currtasks(w) < batchsize && workload_currtasks(w) != workload_totaltasks(w) )
			{
				workload_checktasks(w, g_iterator);
				g_iterator++;
			}
			
			/* Number of already processed tasks, i.e., Number of tasks in our second-from-last queue. */

			int num_tasks = queue_size( (queue_tt) array_get(workload_arrtasks(w), array_size(workload_arrtasks(w)) - 2) );

			if ( num_tasks >= batchsize )
				model_optimization(w, model, cores);
			else
				populate_queues_opt(w, cores, array_size(cores));

			/* Scheduling tasks to ready cores. */

			while ( !queue_empty(ready) )
			{
				core_tt c = choose_core(ready);

				/* Scheduling with core's "piece" of workload. */
				queue_contention = strategy->sched(c, (queue_tt) array_get(workload_arrtasks(w), core_getcid(c)));
				controller += queue_contention;

				if ( controller != 0 )
				{
					if ( queue_contention == 0 ) g_iterator--;
					queue_insert(processing, c);
				} else queue_insert(ready, c);

				core_set_contention(c, -(queue_contention));
			}
			
			processer->process();
	
			/* Cleaning up. */
			while(!queue_empty(processing))
				queue_insert(ready, queue_remove(processing));
		}
		model_destroy(model);
	}
	else if (optimize == 0) {

		for ( /* noop */ ; workload_totaltasks(w) > 0 ; /* noop */ )
		{	
			controller = 0;
			workload_checktasks(w, g_iterator);

			/* 
			   Idle. 
			   While no task arrived. 
			*/
			while ( workload_currtasks(w) < batchsize && workload_currtasks(w) != workload_totaltasks(w) ) 
			{ 
				g_iterator++;
				workload_checktasks(w, g_iterator);
			}

			populate_queues_not_opt(w, cores);

			/* Scheduling tasks to ready cores. */
			while(!queue_empty(ready))
			{
				core_tt c = choose_core(ready);

				/* Scheduling with entire workload. */
				queue_contention = strategy->sched(c, (queue_tt) array_get(workload_arrtasks(w), array_size(workload_arrtasks(w)) - 2));

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
			processer->process();

			/* Cleaning up. */
			while(!queue_empty(processing))
				queue_insert(ready, queue_remove(processing));
		}
	}

	strategy->end();
	processer->end();
	simsched_dump(cores, w);

	threads_join();
	RAM_destroy(RAM);
}