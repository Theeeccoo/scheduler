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
	#include "statistics.h"

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
	extern task_tt task_create(int, unsigned long int, int);
	extern void task_destroy(task_tt);
	extern void task_set_realid(task_tt, int);
	extern int task_gettsid(const_task_tt);
	extern int task_realid(const_task_tt);

	extern int task_arrivaltime(const_task_tt);
	extern int task_waiting_time(const_task_tt);
	extern unsigned long int task_workload(const_task_tt);
	extern unsigned long int task_work_processed(const_task_tt);
	extern unsigned long int task_work_left(const_task_tt);
	extern int task_emoment(const_task_tt);
	extern int task_lmoment(const_task_tt);

	extern void task_set_waiting_time(task_tt, int);
	extern void task_set_arrivaltime(task_tt, int);
	extern void task_set_emoment(task_tt, int);
	extern void task_set_lmoment(task_tt, int);
	extern void task_set_workprocess(task_tt, unsigned long int);
	extern void task_set_workload(task_tt, unsigned long int);

	extern bool task_check_pt_line_valid(const_task_tt, int);
	extern int  task_find_pt_line_frame_id(const_task_tt, int);
	extern int  task_find_pt_line_memptr(const_task_tt);
	extern int  task_get_pt_line_frameid(const_task_tt, int);
	extern void task_set_pt_line_frameid(task_tt, int, int);
	extern void task_invalid_pt_line(task_tt, int);
	extern void task_valid_pt_line(task_tt, int);
	extern int  task_pt_line_frameid(const_task_tt, int);
	extern int  task_pt_num_lines(const_task_tt);

	extern array_tt task_memacc(const_task_tt);
	extern void task_create_memacc(task_tt, histogram_tt);
	extern void task_set_memacc(task_tt, array_tt);
	extern void task_set_memptr(task_tt, unsigned long int);
	extern unsigned long int task_memptr(const_task_tt);
	extern int* task_lineacc(const_task_tt);
	extern int* task_pageacc(const_task_tt);
	extern bool task_accessed_set(const_task_tt, int, int);

	extern double task_hotness(task_tt, int);
	
	extern void task_set_page_hit(task_tt, unsigned long int);
	extern void task_set_page_fault(task_tt, unsigned long int);
	extern unsigned long int task_page_hit(const_task_tt);
	extern unsigned long int task_page_fault(const_task_tt);
	extern void task_set_hit(task_tt, unsigned long int);
	extern void task_set_miss(task_tt, unsigned long int);
	extern unsigned long int task_hit(const_task_tt);
	extern unsigned long int task_miss(const_task_tt);

	extern void task_core_assign(task_tt, int);
	extern int task_core_assigned(const_task_tt);


	extern int map_compare_mem(void*, void*);

	/**@}*/

#endif /* TASK_H_ */
