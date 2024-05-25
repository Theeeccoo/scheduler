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

#ifndef TASK_H_
#define TASK_H_

	#include "mylib/array.h"

	/**
	 * @brief Opaque pointer to a task.
	 */
	typedef struct task * task_tt;

	/**
	 * @brief Constant opaque pointer to a task.
	 */
	typedef const struct task * const_task_tt;

	/**
	 * @name Operations on Task
	 */
	/**@{*/
<<<<<<< HEAD
	extern task_tt task_create(int);
	extern void task_destroy(task_tt);
	extern void task_set_workload(task_tt, int);
	extern int task_get_workload(task_tt);
	extern void task_set_waiting_time(task_tt, int);
	extern int task_get_waiting_time(task_tt);
	extern int task_gettsid(task_tt);
=======
	extern task_tt task_create(int, int);
	extern void task_destroy(task_tt);

	extern int task_arrivaltime(const_task_tt);
	extern int task_waiting_time(const_task_tt);
	extern int task_workload(const_task_tt);
	extern int task_work_processed(const_task_tt);
	extern int task_work_left(const_task_tt);
	extern int task_emoment(const_task_tt);
	extern int task_lmoment(const_task_tt);

	extern void task_set_waiting_time(task_tt, int);
	extern void task_set_arrivaltime(task_tt, int);
	extern void task_set_emoment(task_tt, int);
	extern void task_set_lmoment(task_tt, int);
	extern void task_set_workprocess(task_tt, int);
	extern void task_set_workload(task_tt, int);

	extern array_tt task_memacc(const_task_tt);
	extern void task_create_memacc(task_tt, int);
	extern void task_set_memacc(task_tt, array_tt);
	extern void task_set_memptr(task_tt, int);
	extern int  task_memptr(const_task_tt);

	extern void task_set_hit(task_tt, int);
	extern void task_set_miss(task_tt, int);
	extern int task_hit(const_task_tt);
	extern int task_miss(const_task_tt);

	extern void task_core_assign(task_tt, int);
	extern int task_core_assigned(const_task_tt);

	extern int task_gettsid(const_task_tt);
>>>>>>> 9aff985 (Initial commit.)
	/**@}*/

#endif /* TASK_H_ */
