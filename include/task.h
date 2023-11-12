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
	extern task_tt task_create(int);
	extern void task_destroy(task_tt);
	extern void task_set_workload(task_tt, int);
	extern int task_get_workload(task_tt);
	extern void task_set_waiting_time(task_tt, int);
	extern int task_get_waiting_time(task_tt);
	extern int task_gettsid(task_tt);
	/**@}*/

#endif /* TASK_H_ */
