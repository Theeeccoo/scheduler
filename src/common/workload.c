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

typedef int (*FUNC_PTR)(task_tt, task_tt);

/**
 * @brief Synthetic workload.
 */
struct workload
{
	int ntasks;                 /**< Total number of tasks.                                                                                                                                                                                                              */
	array_tt all_tasks;         /**< All tasks in our simulation.                                                                                                                                                                                                        */
	queue_tt tasks;             /**< All initial tasks.                                                                                                                                                                                                                  */
	array_tt all_arrived_tasks; /**< All queues of tasks that will be assigned to cores. There are ncores + 2 arrays. 0 to ncores are the queues for each core. Second from last has the tasks (not yet grouped) but processed. Last position has the not grouped cores. */
	queue_tt finished_tasks;    /**< All tasks that have finished.                                                                                                                                                                                                       */
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
struct workload *workload_create(histogram_tt h, histogram_tt a, int skewness, int arrskewness,  int ntasks)
{
	int k;              /* Residual tasks.       */
	struct workload *w; /* Workload.             */

	/* Sanity check. */
	assert(h != NULL);
	assert(ntasks > 0);

	/* Create workload. */
	w = smalloc(sizeof(struct workload));
	w->ntasks = ntasks;   
	w->all_tasks = array_create(0);
	w->tasks = queue_create();
	w->all_arrived_tasks = array_create(0);
	w->finished_tasks = queue_create();

	/* Create workload. */
	k = 0;
	for (int i = 0; i < histogram_nclasses(h); i++)
	{
		int n;

		n = floor(histogram_class(h, i)*ntasks);

		for (int j = 0; j < n; j++){
			queue_insert(w->tasks, task_create(k++, workload_skewness(i, histogram_nclasses(h), skewness), 0));
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
		queue_insert(w->tasks, task_create(k++, workload_skewness(j, histogram_nclasses(h), skewness), 0));
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

	/* Generating Tasks' memory addresses. */
	distribution_tt dist_accesses = dist_beta();
	for ( int j = 0; j < ntasks; j ++)
	{
		task_tt curr_task = queue_peek(workload_tasks(w), j);
		histogram_tt histogram_accesses = distribution_histogram(dist_accesses, task_workload(curr_task));
		task_create_memacc(curr_task, histogram_accesses);
	}
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
	for ( unsigned long int i = 0; i < array_size(w->all_arrived_tasks); i++ )
		queue_destroy(array_get(w->all_arrived_tasks, i));
	array_destroy(w->all_arrived_tasks);
	array_destroy(w->all_tasks);
	free(w);
}

/**
 * @brief Sorts tasks in ascending order.
 *
 * @param elem1 Target task.
 * @param elem2 Target task.
 * 
 * @returns 1 If first task's workload is lesser than second. 0 If both have the same value. -1 If first task's workload is greater than second
 */
static int workload_ascending(task_tt elem1, task_tt elem2)
{
	int sub_result = task_workload(elem1) - task_workload(elem2);
	int return_result = 0;

	if ( sub_result < 0 )
		return_result = 1;
	else if ( sub_result == 0 )
		return_result = 0;
	else
		return_result = -1;
	
	return return_result;
}

/**
 * @brief Sorts tasks in descending order.
 *
 * @param elem1 Target task.
 * @param elem2 Target task.
 * 
 * @returns 1 If first task's workload is greater than second. 0 If both have the same value. -1 If first task's workload is lesser than second
 */
static int workload_descending(task_tt elem1, task_tt elem2)
{
	int sub_result = task_workload(elem1) - task_workload(elem2);
	int return_result = 0;

	if ( sub_result > 0 )
		return_result = 1;
	else if ( sub_result == 0 )
		return_result = 0;
	else
		return_result = -1;
	
	return return_result;
}

/**
 * @brief Sorts tasks in ascending order based in tasks arrival time.
 *
 * @param elem1 Target task.
 * @param elem2 Target task.
 * 
 * @returns 1 If first task's arrival_time is lesser than second. 0 If both have the same value. -1 If first task's arrival_time is greater than second
 */
static int workload_sort_arrival(task_tt elem1, task_tt elem2)
{
	int sub_result = task_arrivaltime(elem1) - task_arrivaltime(elem2);
	int return_result = 0;

	if ( sub_result < 0 )
		return_result = 1;
	else if ( sub_result == 0 )
		return_result = 0;
	else
		return_result = -1;
	
	return return_result;
}

/**
 * @brief Sorts tasks in ascending order based in tasks remaining workload.
 *
 * @param elem1 Target task.
 * @param elem2 Target task.
 * 
 * @returns 1 If first task's remaining_workload is lesser than second. 0 If both have the same value. -1 If first task's remaining_workload is greater than second
 */
static int workload_sort_remainingwork(task_tt elem1, task_tt elem2)
{
	int sub_result = task_work_left(elem1) - task_work_left(elem2);
	int return_result = 0;

	if ( sub_result < 0 )
		return_result = 1;
	else if ( sub_result == 0 )
		return_result = 0;
	else
		return_result = -1;
	
	return return_result;
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

static void swap(struct task **tasks, int i, int j)
{
    struct task *temp = tasks[i];
	tasks[i] = tasks[j];
	tasks[j] = temp;
}

static int partition(struct task **tasks, int low, int high, FUNC_PTR compare)
{
	task_tt pivot = tasks[high];
	int i = low;

	for ( int j = low; j < high; j++ )
	{
		if ( (compare(tasks[j], pivot) == 1) || ( (compare(tasks[j], pivot) == 0) && task_gettsid(tasks[j]) < task_gettsid(pivot)) ) 
		{
			swap(tasks, i, j);
			i++;
		}
	}
	swap(tasks, i, high);
	return i;
}

static void quickSort(struct task **tasks, int low, int high, FUNC_PTR compare)
{
	/* Sanity check. */
	assert(tasks != NULL);
	if ( low < high )
	{
		int pivot = partition(tasks, low, high, compare);

		quickSort(tasks, low, pivot - 1, compare);
		quickSort(tasks, pivot + 1, high, compare);
	}	
}


static void sort_tasks(struct task **tasks, int num_tasks, FUNC_PTR compare)
{
	/* Sanity check. */
	assert(tasks != NULL);
	quickSort(tasks, 0, num_tasks - 1, compare);
}

/**
 * @brief Transforms a queue into an array. Used in quickSorting.
 * 
 * @param to_transform Target queue.
 * 
 * @returns Queue's transformation. 
 */
static struct task** generate_array(struct queue *to_transform)
{
	/* Sanity check. */
	assert(to_transform != NULL);
	int q_size = queue_size(to_transform);
	struct task **tasks = (struct task**) malloc(sizeof(struct task*) * q_size);
	for ( int i = 0; i < q_size; i++ )
	{
		tasks[i] = queue_remove(to_transform);
	}

	return tasks;
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

	FUNC_PTR compare;
	struct task **tasks = NULL;
	struct queue *queue = NULL;
	int num_elems = 0;

	/* Sort workload. */
	switch(sorting)
	{
		/* Sort in ascending order. */
		case WORKLOAD_ASCENDING:
			compare = workload_ascending;
			queue = workload_tasks(w);
			num_elems = queue_size(queue);
			tasks = generate_array(queue);
			break;

		/* Sort in descending order. */
		case WORKLOAD_DESCENDING:
			compare = workload_descending;
			queue = workload_tasks(w);
			num_elems = queue_size(queue);
			tasks = generate_array(queue);
			break;

		/* Shuffle workload. */
		case WORKLOAD_SHUFFLE:
			workload_shuffle(w);
			break;
		
		/* Sort based in workload arrival time. */
		case WORKLOAD_ARRIVAL:
			compare = workload_sort_arrival;
			queue = workload_tasks(w);
			num_elems = queue_size(queue);
			tasks = generate_array(queue);
			break;
		
		/* Sort based in remaining workload. */
		case WORKLOAD_REMAINING_WORK:
			compare = workload_sort_remainingwork;
			for ( unsigned long int i = 0; i < array_size(workload_arrtasks(w)); i++ )
			{
				queue = array_get(workload_arrtasks(w), i);
				int num_elems = queue_size(queue);
				tasks = generate_array(queue);
				sort_tasks(tasks, num_elems, compare);

				for ( int j = 0; j < num_elems; j++ )
					queue_insert(queue, tasks[j]);

				free(tasks);
			}
			tasks = NULL;
			break;

		/* Should not happen. */
		default:
			assert(0);
			break;
	}

	if ( tasks != NULL )
	{
		sort_tasks(tasks, num_elems, compare);
		for ( int i = 0; i < num_elems; i++ )
			queue_insert(queue, tasks[i]);
	}
	free(tasks);
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
		fprintf(outfile, "%d %lu %d ", task_realid(ts), task_workload(ts), task_arrivaltime(ts));
		for ( unsigned long int j = 0; j < task_workload(ts); j++ )
		{
			unsigned long int elem = (mem_virtual_addr(array_get(memacc, j)) * PAGE_SIZE) + mem_addr_offset(array_get(memacc, j));
			fprintf(outfile, "%lu ", elem);
		}
		fprintf(outfile, "\n");
		free(memacc);
	}
}

/**
 * @brief Reads a workload from a file.
 *
 * @param infile Input file.
 * @param ncores Total number of working cores in Simulation. 
 *
 * @returns A workload.
 */
struct workload *workload_read(FILE *infile, int ncores)
{
	int ntasks;         /**< Number of tasks. */
	struct workload *w; /**< Workload.        */

	/* Sanity check. */
	assert(infile != NULL);

	assert(fscanf(infile, "%d\n", &ntasks) == 1);

	w = smalloc(sizeof(struct workload));
	w->all_tasks = array_create(ntasks);
	w->tasks = queue_create();
	w->all_arrived_tasks = array_create(ncores + 2);

	// Adding a queue into all_arrived_tasks
	for ( unsigned long int i = 0; i < array_size(w->all_arrived_tasks); i++ )
		array_set(w->all_arrived_tasks, i, queue_create());
	w->finished_tasks = queue_create();
	w->ntasks = ntasks;

	/* Write workload to file. */
	int real_id   = 0,
		arrivtime = 0;
	unsigned long int addr      = 0,
	                  workload  = 0;
	for (int i = 0; i < ntasks; i++) {
		assert(fscanf(infile, "%d", &real_id) == 1);
		assert(fscanf(infile, "%lu", &workload) == 1);
		assert(fscanf(infile, "%d\n", &arrivtime) == 1);

		array_tt t_addr = array_create(workload);

		for ( unsigned long int j = 0; j < workload; j++ )
		{
			assert(fscanf(infile, "%lu\n", &addr) == 1);
			struct mem *m = mem_create(addr);
			array_set(t_addr, j, m);

		}
		task_tt ts = task_create(real_id, workload, arrivtime);
		task_set_memacc(ts, t_addr);

		workload_set_task(w, i, ts);
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
 * @brief Returns a task at specified position (id).
 * 
 * @param w  Target workload.
 * @param id Desired task.
 * 
 * @returns Corresponding tas.
 */
task_tt workload_find_task(const struct workload *w, int id)
{
	/* Sanity check. */
	assert(w != NULL);
	assert(id >= 0);
	assert(id < workload_ntasks(w));

	return array_get(w->all_tasks, id);
}

/**
 * @brief Returns the array of arrived tasks of a given workload.
 *
 * @param w   Target workload.
 * 
 * @returns The array of arrived tasks of a given workload.
 */
array_tt workload_arrtasks(const struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);   

	return(w->all_arrived_tasks);
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
 * @param idx  Index where task will stay at "all_tasks".
 * @param task New task.
 */
void workload_set_task(struct workload *w, int idx, struct task *t)
{
	/* Sanity check. */
	assert(w != NULL);
	assert(t != NULL);

	array_set(w->all_tasks, idx, t);
	queue_insert(w->tasks, t);
}

/**
 * @brief Adds new task into arrived tasks array.
 *
 * @param w    Target workload.
 * @param task New task.
 * @param pos  Which queue to add. 
 */
void workload_set_arrtask(struct workload *w, struct task *t, int pos)
{
	/* Sanity check. */
	assert(w != NULL);   
	assert(t != NULL);   
	assert(pos >= 0);

	queue_tt queue = (queue_tt) array_get(w->all_arrived_tasks, pos);
	queue_insert(queue, t);
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
 *        If positive, add them into all_arrived_tasks queue.
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
		/* 
			Tasks are sorted in arrival time, so if one haven't arrived, all after it haven't also.
			Newly arrived tasks will be allocated to the last queue in all_arrived_tasks.
		 */
		if ( task_arrivaltime(curr_task) <= g_i )
		{
			workload_set_arrtask(w, queue_remove(w->tasks), array_size(w->all_arrived_tasks) - 1);
			i --;
		} 
		else break;
	}
}

/**
 * @brief Returns the current total number of tasks left in our simulation.
 * 
 * @param w   Target workload.
 * 
 * @returns Current total number of tasks left in our simulation.
*/
int workload_totaltasks(const struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);   

	int sum = 0;
	for ( unsigned long int i = 0; i < array_size(w->all_arrived_tasks); i++ ) 
		sum += queue_size((queue_tt) array_get(w->all_arrived_tasks, i));

	return (queue_size(w->tasks) + sum);
}

/**
 * @brief Returns the number of tasks that are currently (based on arrival time) in workload.
 *
 * @param w Target workload.
 *
 * @returns Number of tasks at workload at given time.
 */
int workload_currtasks(const struct workload *w)
{
	/* Sanity check. */
	assert(w != NULL);   

	int sum = 0;
	for ( unsigned long int i = 0; i < array_size(w->all_arrived_tasks); i++ ) 
		sum += queue_size((queue_tt) array_get(w->all_arrived_tasks, i));

	return (sum);
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