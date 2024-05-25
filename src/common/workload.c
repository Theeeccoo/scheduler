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
#include <math.h>
#include <time.h>
#include <string.h>

#include <mylib/util.h>

#include <mem.h>
#include <statistics.h>
#include <workload.h>

/**
 * @brief Synthetic workload.
 */
struct workload
{
	int ntasks;              /**< Total number of tasks.                      */
	queue_tt tasks;          /**< All initial tasks.                          */
	queue_tt arrived_tasks;  /**< All tasks that are current being processed. */
	queue_tt finished_tasks; /**< All tasks that have finished.               */
};

/**
 * @brief Computes the skewness of a task.
 *
 * @param i        Task class.
 * @param nclasses Number of task classes
 * @param skewness Skewness.
 *
 * @returns Task skewness.
 */
static int workload_skewness(int i, int nclasses, int skewness)
{
	switch (skewness)
	{
		case WORKLOAD_SKEWNESS_LEFT:
			return (i + 2);
			break;

		case WORKLOAD_SKEWNESS_RIGHT:
			return (nclasses - i + 1);
			break;

		default:
			error("unknown skewness");
	}
	
	/* Never gets here. */
	return (-1);
}

/**
 * @brief Creates a workload.
 *
 * @param h           Histogram of probability distribution.
 * @param a           Histogram of arrival probability distribution.
 * @param skewness    Workload Skewness.
 * @param arrskewness Arrival time Skewness.
 * @param ntasks      Number of tasks.
 */
struct workload *workload_create(histogram_tt h, histogram_tt a, int skewness, int arrskewness,  int ntasks )
{
	int k;              /* Residual tasks.       */
	int max_work = 0;   /* Max workload created. */
	struct workload *w; /* Workload.             */

	/* Sanity check. */
	assert(h != NULL);
	assert(ntasks > 0);

	/* Create workload. */
	w = smalloc(sizeof(struct workload));
	w->ntasks = ntasks;
	w->tasks = queue_create();
	w->arrived_tasks = queue_create();
	w->finished_tasks = queue_create();

	/* Create workload. */
	k = 0;
	for (int i = 0; i < histogram_nclasses(h); i++)
	{
		int n;

		n = floor(histogram_class(h, i)*ntasks);

		for (int j = 0; j < n; j++){
			queue_insert(w->tasks, task_create(i + j, workload_skewness(i, histogram_nclasses(h), skewness), 0));
			k++;
		}
	}

	/* Check for overflow. */
	if (k > ntasks)
	{
		fprintf(stderr, "ntasks=%d\n", k);
		error("histogram overflow");
	}

	/* Fill up remainder tasks. */
	for (int i = k; i < ntasks; i++)
	{
		int j = rand()%histogram_nclasses(h);
		queue_insert(w->tasks, task_create(i, workload_skewness(j, histogram_nclasses(h), skewness), 0));
		k++;
	}

	/* ARRIVAL TIME. */

	k = 0;
	/* Creating arrival time. */
	for ( int i = 0; i < histogram_nclasses(a); i++ )
	{
		int n = floor(histogram_class(a, i) * ntasks);

		for ( int j = 0; j < n; j++ )
		{
			task_set_arrivaltime(queue_peek(w->tasks, k), workload_skewness(i, histogram_nclasses(a), arrskewness));
			k++;
		}
	}

	/* Check for overflow. */
	if (k > ntasks)
	{
		fprintf(stderr, "ntasks=%d\n", k);
		error("histogram overflow");
	}

	/* Fill up remainder tasks. */
	for (int i = k; i < ntasks; i++)
	{
		int j = rand()%histogram_nclasses(a);
		task_set_arrivaltime(queue_peek(w->tasks, k), workload_skewness(j, histogram_nclasses(a), arrskewness));
		k++;
	}

	for ( int i = 0; i < ntasks; i++ )
	{
		task_tt curr_task = queue_peek(w->tasks, i);
		if ( max_work < task_workload(curr_task) ) max_work = task_workload(curr_task);
	}

	/* Generating Tasks' memory addresses. */
	for ( int j = 0; j < ntasks; j ++)
		task_create_memacc(queue_peek(workload_tasks(w), j), max_work);

	return (w);
}

/**
 * @brief Destroys a workload.
 *
 * @param w Target workload.
 */
void workload_destroy(struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);
	
	queue_destroy(w->tasks);
	queue_destroy(w->arrived_tasks);
	queue_destroy(w->finished_tasks);
	free(w);
}

/**
 * @brief Sorts tasks in ascending order.
 *
 * @param w Target workload.
 */
static void workload_ascending(struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);

	for (int i = 0; i < w->ntasks; i++)
	{
		for (int j = 0; j < w->ntasks; j++)
		{
			task_tt task_i = queue_peek(workload_tasks(w), i);
			task_tt task_j = queue_peek(workload_tasks(w), j);
			if (task_workload(task_j) < task_workload(task_i))
			{
				task_tt task_tmp = NULL;

				task_tmp = task_j;
				queue_change_elem(w->tasks, j, task_i);
				queue_change_elem(w->tasks, i, task_tmp);
			}
		}
	}
}

/**
 * @brief Sorts tasks in descending order.
 *
 * @param w Target workload.
 */
static void workload_descending(struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);

	for (int i = 0; i < w->ntasks; i++)
	{
		for (int j = 0; j < w->ntasks; j++)
		{
			task_tt task_i = queue_peek(workload_tasks(w), i);
			task_tt task_j = queue_peek(workload_tasks(w), j);
			if (task_workload(task_j) > task_workload(task_i))
			{
				task_tt task_tmp = NULL;

				task_tmp = task_j;
				queue_change_elem(w->tasks, j, task_i);
				queue_change_elem(w->tasks, i, task_tmp);
			}
		}
	}
}

/**
 * @brief Shuffle tasks.
 *
 * @param w Target workload.
 */
static void workload_shuffle(struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);

	/* Shuffle array. */
	for (int i = 0; i < w->ntasks - 1; i++)
	{
		int j;                   /* Shuffle index.  */
		task_tt task_tmp = NULL; /* Temporary data. */


		j = rand()%w->ntasks;

		task_tt task_i = queue_peek(workload_tasks(w), i);
		task_tt task_j = queue_peek(workload_tasks(w), j);

		task_tmp = task_i;
		queue_change_elem(w->tasks, i, task_j);
		queue_change_elem(w->tasks, j, task_tmp);
	}
}

/**
 * @brief Sorts tasks in ascending order based in tasks arrival time.
 *
 * @param w Target workload.
 */
static void workload_sort_arrival(struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);

	for (int i = 0; i < w->ntasks; i++)
	{
		for (int j = 0; j < w->ntasks; j++)
		{
			task_tt task_i = queue_peek(workload_tasks(w), i);
			task_tt task_j = queue_peek(workload_tasks(w), j);
			if (task_arrivaltime(task_j) > task_arrivaltime(task_i))
			{
				task_tt task_tmp = NULL;

				task_tmp = task_j;
				queue_change_elem(w->tasks, j, task_i);
				queue_change_elem(w->tasks, i, task_tmp);
			}
		}
	}
}

/**
 * @brief Sorts tasks in ascending order based in tasks remaining workload.
 *
 * @param w Target workload.
 */
static void workload_sort_remainingwork(struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);

	for (int i = 0; i < queue_size(workload_arrtasks(w)); i++)
	{
		for (int j = 0; j < queue_size(workload_arrtasks(w)); j++)
		{
			task_tt task_i = queue_peek(workload_arrtasks(w), i);
			task_tt task_j = queue_peek(workload_arrtasks(w), j);
			if (task_work_left(task_j) > task_work_left(task_i))
			{
				task_tt task_tmp = NULL;

				task_tmp = task_j;
				queue_change_elem(w->arrived_tasks, j, task_i);
				queue_change_elem(w->arrived_tasks, i, task_tmp);
			}
		}
	}
	
}

/**
 * @brief Sorts tasks.
 *
 * @param w       Target workload.
 * @param sorting Sorting type.
 */
void workload_sort(struct workload *w, enum workload_sorting sorting)
{
	/* Sanity check. */
	assert(w != NULL);

	/* Sort workload. */
	switch(sorting)
	{
		/* Sort in ascending order. */
		case WORKLOAD_ASCENDING:
			workload_ascending(w);
			break;

		/* Sort in descending order. */
		case WORKLOAD_DESCENDING:
			workload_descending(w);
			break;

		/* Shuffle workload. */
		case WORKLOAD_SHUFFLE:
			workload_shuffle(w);
			break;
		
		/* Sort based in workload arrival time. */
		case WORKLOAD_ARRIVAL:
			workload_sort_arrival(w);
			break;
		
		/* Sort based in remaining workload. */
		case WORKLOAD_REMAINING_WORK:
			workload_sort_remainingwork(w);
			break;

		/* Should not happen. */
		default:
			assert(0);
			break;
	}
}
/**
 * @brief Computes the task sorting map of a workload.
 * 
 * @param w Target workload
 * 
 * @returns Sorting map.
 */
int *workload_sortmap(struct workload *w)
{
	int *map;

	/* Sanity check. */
	assert(w != NULL);
	
	map = smalloc(w->ntasks*sizeof(int));
	for (int i = 0; i < w->ntasks; i++)
		map[i] = i;

	/* Sort. */
	for (int i = 0; i < w->ntasks - 1; i++)
	{
		for (int j = i + 1; j < w->ntasks; j++)
		{
			task_tt task_map_i = queue_peek(workload_tasks(w), map[i]);
			task_tt task_map_j = queue_peek(workload_tasks(w), map[j]);

			/* Swap. */
			if (task_workload(task_map_j) < task_workload(task_map_i))
			{
				task_tt task_tmp = NULL;

				task_tmp = task_map_j;
				queue_change_elem(w->tasks, j, task_map_i);
				queue_change_elem(w->tasks, i, task_tmp);
			}
		}
	}

	return (map);
} 

/**
 * @brief Writes a workload to a file.
 *
 * @param outfile Output file.
 * @param w       Target workload.
 */
void workload_write(FILE *outfile, struct workload *w)
{
	/* Sanity check. */
	assert(outfile != NULL);
	assert(w != NULL);

	/* Write workload to file. */
	fprintf(outfile, "%d\n", w->ntasks);
	for (int i = 0; i < w->ntasks; i++)
	{
		task_tt ts = queue_peek(workload_tasks(w), i);
		array_tt memacc = task_memacc(ts);
		fprintf(outfile, "%d %d %d ", task_realid(ts), task_workload(ts), task_arrivaltime(ts));
		for ( int j = 0; j < task_workload(ts); j++ )
		{
			int *elem = array_get(memacc, j);
			fprintf(outfile, "%d ", *elem);
		}
		fprintf(outfile, "\n");
		free(memacc);
	}
}

/**
 * @brief Reads a workload from a file.
 *
 * @param infile Input file.
 *
 * @returns A workload.
 */
struct workload *workload_read(FILE *infile)
{
	int ntasks;         /**< Number of tasks. */
	struct workload *w; /**< Workload.        */

	/* Sanity check. */
	assert(infile != NULL);

	assert(fscanf(infile, "%d\n", &ntasks) == 1);

	w = smalloc(sizeof(struct workload));
	w->tasks = queue_create();
	w->arrived_tasks = queue_create();
	w->finished_tasks = queue_create();
	w->ntasks = ntasks;

	/* Write workload to file. */
	int real_id   = 0,
		workload  = 0, 
		arrivtime = 0,
		addr      = 0;

	for (int i = 0; i < ntasks; i++) {
		assert(fscanf(infile, "%d", &real_id) == 1);
		assert(fscanf(infile, "%d", &workload) == 1);
		assert(fscanf(infile, "%d\n", &arrivtime) == 1);
		array_tt t_addr = array_create(workload);

		for ( int j = 0; j < workload; j++ )
		{
			assert(fscanf(infile, "%d\n", &addr) == 1);
			struct mem *m = mem_create(addr);
			array_set(t_addr, j, m);

		}
		task_tt ts = task_create(real_id, workload, arrivtime);
		task_set_memacc(ts, t_addr);

		workload_set_task(w, ts);
	}

	return (w);
}

/**
 * @brief Returns the total number of tasks in a workload.
 *
 * @param w Target workload.
 *
 * @returns The number of tasks in the target workload.
 */
int workload_ntasks(const struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);

	return (w->ntasks);
}

/**
 * @brief Returns the queue of tasks of a given workload.
 *
 * @param w Target workload.
 * 
 * @returns The queue of tasks of a given workload.
 */
queue_tt workload_tasks(const struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);

	return(w->tasks);
}

/**
 * @brief Returns the queue of arrived tasks of a given workload.
 *
 * @param w Target workload.
 * 
 * @returns The queue of arrived tasks of a given workload.
 */
queue_tt workload_arrtasks(const struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);

	return(w->arrived_tasks);
}

/**
 * @brief Returns the queue of finished tasks of a given workload.
 *
 * @param w Target workload.
 * 
 * @returns The queue of finished tasks of a given workload.
 */
queue_tt workload_fintasks(const struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);

	return(w->finished_tasks);
}

/**
 * @brief Adds new task into tasks array.
 *
 * @param w    Target workload.
 * @param task New task.
 */
void workload_set_task(struct workload *w, struct task *t)
{
	/* Sanity check. */
	assert(w != NULL);
	assert(t != NULL);

	queue_insert(w->tasks, t);
}

/**
 * @brief Adds new task into arrived tasks array.
 *
 * @param w    Target workload.
 * @param task New task.
 */
void workload_set_arrtask(struct workload *w, struct task *t)
{
	/* Sanity check. */
	assert(w != NULL);
	assert(t != NULL);

	queue_insert(w->arrived_tasks, t);
}

/**
 * @brief Adds new task into finished tasks array.
 *
 * @param w    Target workload.
 * @param task New task.
 */
void workload_set_fintask(struct workload *w, struct task *t)
{
	/* Sanity check. */
	assert(w != NULL);
	assert(t != NULL);

	queue_insert(w->finished_tasks, t);
}

/**
 * @brief Checks if there are tasks that have arrived at g_i moment. 
 *        If positive, add them into arrived_tasks queue.
 * 
 * @param w   Target workload.
 * @param g_i Global iterator.
*/ 
void workload_checktasks(struct workload *w, int g_i)
{
	/* Sanity check. */
	assert(w != NULL);

	for ( int i = 0; i < queue_size(w->tasks); i++ )
	{
		task_tt curr_task = queue_peek(w->tasks, i);
		/* Tasks are sorted in arrival time, so if one haven't arrived, all after it haven't also. */
		if ( task_arrivaltime(curr_task) <= g_i ) workload_set_arrtask(w, queue_remove(w->tasks));
		else break;
	}
}

/**
 * @brief Returns the current total number of tasks left in our simulation.
 * 
 * @param w Target workload.
 * 
 * @returns Current total number of tasks left in our simulation.
*/
int workload_totaltasks(const struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);

	return (queue_size(w->tasks) + queue_size(w->arrived_tasks));	
}

/**
 * @brief Returns the number of tasks tha are currently (based on arrival time) in workload.
 *
 * @param w Target workload.
 *
 * @returns Number of tasks at workload at given time.
 */
int workload_currtasks(const struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);

	return (queue_size(w->arrived_tasks));
}

/**
 * @brief Computes the cummulative sum of a workload.
 *
 * @param w Target Workload.
 *
 * @returns The cummulative sum array.
 */
int *workload_cummulative_sum(struct workload *w)
{
	int *sum;   /* Cummulative sum.     */
	int ntasks; /* Alias for w->ntasks. */

	/* Sanity check. */
	assert(w != NULL);

	ntasks = w->ntasks;

	sum = malloc((ntasks + 1)*sizeof(int));
	assert(sum != NULL);

	/* Compute cummulative sum. */
	sum[0] = 0;
	for (int i = 1; i <= ntasks; i++)
		sum[i] = sum[i - 1] + task_work_left(queue_peek(workload_tasks(w), i - 1));
	return (sum);

}