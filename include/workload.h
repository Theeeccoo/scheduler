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

#ifndef WORKLOAD_H_
#define WORKLOAD_H_

	#include <stdio.h>

<<<<<<< HEAD
=======
	#include "mylib/queue.h"
>>>>>>> 9aff985 (Initial commit.)
	#include "statistics.h"
	#include "task.h"

	/**
	 * @brief Opaque pointer to a synthetic workload.
	 */
	typedef struct workload * workload_tt;

	/**
	 * @brief Constant opaque pointer to a synthetic workload.
	 */
	typedef const struct workload * const_workload_tt;

	/**
	 * @brief Workload sorting types.
	 */
	enum workload_sorting
	{
<<<<<<< HEAD
		WORKLOAD_ASCENDING,  /**< Ascending order.  */
		WORKLOAD_DESCENDING, /**< Descending order. */
		WORKLOAD_SHUFFLE     /**< Shuffle.          */
=======
		WORKLOAD_ASCENDING,  /**< Ascending order.    */
		WORKLOAD_DESCENDING, /**< Descending order.   */
		WORKLOAD_SHUFFLE,    /**< Shuffle.            */
		WORKLOAD_ARRIVAL     /**< Task arrival order. */
>>>>>>> 9aff985 (Initial commit.)
	};

	/**
	 * @brief Workload skewness types.
	 */
	/**@{*/
	#define WORKLOAD_SKEWNESS_NULL  0 /**< None.  */
	#define WORKLOAD_SKEWNESS_LEFT  1 /**< Left.  */
	#define WORKLOAD_SKEWNESS_RIGHT 2 /**< Right. */
	/**@}*/

	/**
	 * @name Operations on Workload
	 */
	/**@{*/
<<<<<<< HEAD
	extern workload_tt workload_create(histogram_tt, int, int);
	extern void workload_destroy(workload_tt);
	extern int workload_ntasks(workload_tt);
	extern int workload_task(workload_tt, int);
=======
	extern workload_tt workload_create(histogram_tt, histogram_tt, int, int, int);
	extern void workload_destroy(workload_tt);
	extern int workload_ntasks(const_workload_tt);
>>>>>>> 9aff985 (Initial commit.)
	extern void workload_sort(workload_tt, enum workload_sorting);
	extern int *workload_sortmap(workload_tt);
	extern void workload_write(FILE *, workload_tt);
	extern workload_tt workload_read(FILE *);
<<<<<<< HEAD
	extern void workload_set_task(workload_tt, int, int);
	extern void workload_increase_elapsed_time(workload_tt, int, int);
	extern int *workload_cummulative_sum(workload_tt);
	/**@}*/

#endif /* WORKLOAD_H_ */
=======
	
	extern void workload_set_task(workload_tt, task_tt);
	extern queue_tt workload_tasks(const_workload_tt);

	extern void workload_set_arrtask(workload_tt, task_tt);
	extern queue_tt workload_arrtasks(const_workload_tt);
	extern void workload_checktasks(workload_tt, int);


	extern void workload_fixqtasks(workload_tt);
	extern int workload_totaltasks(const_workload_tt);
	extern int workload_currtasks(const_workload_tt);
	extern int *workload_cummulative_sum(workload_tt);
	/**@}*/

#endif /* WORKLOAD_H_1 */
>>>>>>> 9aff985 (Initial commit.)
