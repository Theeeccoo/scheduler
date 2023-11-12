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

#ifndef THREAD_H_
#define THREAD_H_

	#include "task.h"

	/**
	 * @brief Opaque pointer to a thread.
	 */
	typedef struct thread * thread_tt;

	/**
	 * @brief Constant opaque pointer to a thread.
	 */
	typedef const struct thread * const_thread_tt;

	/**
	 * @name Operations on Thread
	 */
	/**@{*/
	extern thread_tt thread_create(int, int);
	extern void thread_destroy(thread_tt);
	extern int thread_gettid(const_thread_tt);
	extern double thread_wtotal(const_thread_tt);
	extern int thread_assign(thread_tt, int);
	extern int thread_capacity(const_thread_tt);
	extern int thread_required_process_time(thread_tt, int);
	extern task_tt thread_get_task(thread_tt, int);
	extern int thread_task(thread_tt, int);
	extern int thread_num_assigned_tasks(thread_tt);
	extern int thread_num_processed_tasks(thread_tt);
	extern int thread_increase_processed_tasks(thread_tt);
	/**@}*/

#endif /* THREAD_H_ */
