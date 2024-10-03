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
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <task.h>
#include <math.h>
#include <mem.h>
#include <mylib/util.h>
#include <mylib/map.h>

/*====================================================================*
 * PAGE TABLE LINE                                                    *
 *====================================================================*/

/**
 * @brief Page Table's line.
 */
struct page_table_line
{
	bool valid;    /**< Valid bit.                            */
	int  frame_id; /**< Which frame this line is assigned to. */
};

/**
 * @brief Creates a new instance of a page_table_line.
 *  
 * @returns New instance of page_table_line.
 */
static inline struct page_table_line* page_table_line_create()
{
	struct page_table_line *pl = smalloc(sizeof(struct page_table_line));
	pl->valid = false;
	pl->frame_id = -1;
	return (pl);
}

/**
 * @brief Checks if a page_table_line is valid.
 * 
 * @param pl Target page_table_line.
 * 
 * @returns True if valid. False otherwise.
 */
static inline bool page_check_table_line_valid(const struct page_table_line *pl)
{
	/* Sanity check. */
	assert(pl != NULL);
	return (pl->valid);
}

/**
 * @brief Sets page_table_line's valid bit to true.
 * 
 * @param pl Target page_table_line.
 */
static inline void page_valid_table_line(struct page_table_line *pl)
{
	/* Sanity check. */
	assert(pl != NULL);
	pl->valid = true;
}

/**
 * @brief Sets page_table_line's valid bit to false.
 * 
 * @param pl Target page_table_line.
 */
static inline void page_invalid_table_line(struct page_table_line *pl)
{
	/* Sanity check. */
	assert(pl != NULL);
	pl->valid = false;
}


/**
 * @brief Returns the frame id of a page_table_line. If page is invalid, assert will break execution.
 * 
 * @param pl Target page_table_line.
 * 
 * @returns Frame id stored at page_table_line. 
 */
static inline int page_table_line_frameid(const struct page_table_line *pl)
{
	/* Sanity check. */
	assert(pl != NULL);
	assert(pl->valid);
	return (pl->frame_id);
}

/**
 * @brief Sets the frame id of a page_table_line.
 * 
 * @param pl       Target page_table_line.
 * @param frame_id Desired frame_id.
 */
static inline void page_set_table_line_frameid(struct page_table_line *pl, int frame_id)
{
	/* Sanity check. */
	assert(pl != NULL);
	assert(frame_id >= 0);
	pl->frame_id = frame_id;
}

/**
 * @brief Destroys a page_table_line instance
 * 
 * @param pl Target page_table_line.
 */
static inline void page_table_line_destroy(struct page_table_line *pl)
{
	/* Sanity check. */
	assert(pl != NULL);
	free(pl);
}

/*====================================================================*
 * PAGE TABLE                                                         *
 *====================================================================*/

/**
 * @brief Page Table.
 */
struct page_table
{
	int task_id;                         /**< Which task is this page table. */
	int num_lines;                       /**< Number of lines.               */
	struct page_table_line** page_lines; /**< Page lines.                    */
};

/**
 * @brief Creates a new instance of a page_table.
 * 
 * @param task_id  Task's id of target task.
 * @param mem_size Number of task's memory accesses.
 * 
 * @returns New instance of page_table.
 */
static inline struct page_table *page_table_create(int task_id, unsigned long int mem_size)
{
	struct page_table *pt;
	/* Sanity check. */
	assert(task_id >= 0);
	int num_lines = (int) (ceil(mem_size / PAGE_SIZE) + 1);
	pt = smalloc(sizeof(struct page_table));
	pt->task_id = task_id;
	pt->num_lines = num_lines;

	pt->page_lines = smalloc(sizeof(struct page_table_line*) * num_lines);
	for ( int i = 0; i < num_lines; i++ )
		pt->page_lines[i] = page_table_line_create();
	
	return (pt);
}

/**
 * @brief Returns page_table_line at specified line.
 * 
 * @param pt  Target page table
 * @param idx Desired line.
 * 
 * @returns Page_table line.
 */
static inline struct page_table_line* page_table_line_at(const struct page_table *pt, int idx)
{
	/* Sanity check. */
	assert(pt != NULL);
	assert(idx <= pt->num_lines);
	return (pt->page_lines[idx]);
}

/**
 * @brief Returns page_table's number of lines.
 * 
 * @param ts Target task.
 * 
 * @returns Page_table's number of lines.
 */
static inline int page_table_num_lines(const struct page_table *pt)
{
	/* Sanity check. */
	assert(pt != NULL);
	return (pt->num_lines);
}

/**
 * @brief Destroys a page_table instance
 * 
 * @param pt Target page_table.
 */
static inline void page_table_destroy(struct page_table *pt)
{
	/* Sanity check. */
	assert(pt != NULL);
	for ( int i = 0; i < pt->num_lines; i++ ) page_table_line_destroy(pt->page_lines[i]);
	free(pt->page_lines);
	free(pt);
}

/*====================================================================*
 * TASK                                                               *
 *====================================================================*/

/**
 * @brief Task.
 */
struct task
{
	int tsid;                          /**< Task identification number.             */
	int real_id;                       /**< Real id assigned when workload created. */
	int arrival_time;                  /**< Which iteration current task arrived.   */
	int waiting_time;                  /**< Waiting time for completion of a task.  */
	unsigned long int work;            /**< Total workload of a task.               */
	unsigned long int work_processed;  /**< Total workload processed.               */ 
 
	int core_assigned;                 /**< CORE ID where task was assigned (sca).  */

	unsigned long int page_hits;       /**< Number of page hits.                    */
	unsigned long int page_faults;     /**< Number of page faults.                  */
	unsigned long int hits;			   /**< Number of hits.                         */
	unsigned long int misses;          /**< Number of misses.                       */

	int* all_sets_accessed;            /**< All cache sets accessed p/ task.        */
	int* all_pages_accessed;           /**< All page lines accessed p/ task.        */

	map_tt mem_accessed;               /**< All memory addresses accessed p/ task.  */
	map_tt sets_accessed;              /**< Which cache sets were accessed p/ task. */
	map_tt pages_accessed;             /**< Which page lines were accessed p/ task. */

	int lineptr;                       /**< Points to the last line accessed.       */

	struct page_table* p_table;        /**< Task's page table.                      */
	array_tt memacc;                   /**< Tasks' memory accesses.                 */
	unsigned long int memptr;          /**< Points to the last memory accessed.     */
 
	int e_moment;                      /**< Moment when task (r)entered in a core.  */
	int l_moment;                      /**< Moment when task left from a core.      */
};

/**
 * @brief Next available task identification number.
 */
static int next_tsid = 0;

/**
 * @brief Creates a task.
 *
 * @param real_id Initial ID assigned when workload created.
 * @param work    Workload of a task.
 * @param arrival Arrival moment of a task.
 *
 * @returns A task.
 */
struct task *task_create(int real_id, unsigned long int work, int arrival)
{
	struct task *task;

	/* Sanity check. */
	assert(arrival >= 0);

	task = smalloc(sizeof(struct task));

	task->real_id = real_id;
	task->tsid = next_tsid++;
	task->arrival_time = arrival;
	task->waiting_time = 0;
	task->work = work;
	task->work_processed = 0;
	task->e_moment = 0;
	task->l_moment = 0;
	task->core_assigned = -1;
	task->page_hits = 0;
	task->page_faults = 0;
	task->hits = 0;
	task->misses = 0;

	task->all_sets_accessed = smalloc(sizeof(int) * task->work);
	task->all_pages_accessed = smalloc(sizeof(int) * task->work);
	task->sets_accessed = map_create(map_compare_int);
	task->pages_accessed = map_create(map_compare_int);
	task->mem_accessed = map_create(map_compare_ulong_int);

	task->p_table = page_table_create(task->tsid, work);
	// Initializing. 
	for ( unsigned long int i = 0; i < work; i++ ) task->all_sets_accessed[i] = -1;

	task->memacc = array_create(work);
	task->memptr = 0;

	return (task);
}

/**
 * @brief Sets the id assigned when workload created.
 * 
 * @param ts      Target task.
 * @param real_id Real id.
 */
void task_set_realid(struct task *ts, int real_id)
{
	/* Sanity check. */
    assert(ts != NULL);
	assert(real_id >= 0);

	ts->real_id = real_id;
}

/**
 * @brief Sets the arrival time of given task.
 * 
 * @param ts   Target task.
 * @param time Arrival time.
 */
int  task_realid(const struct task *ts)
{
	/* Sanity check. */
    assert(ts != NULL);
	return (ts->real_id);
}

/**
 * @brief Sets the arrival time of given task.
 * 
 * @param ts   Target task.
 * @param time Arrival time.
 */
void task_set_arrivaltime(struct task *ts, int time)
{
	/* Sanity check. */
    assert(ts != NULL);
	assert(time >= 0);

    ts->arrival_time = time;
}

/**
 * @brief Returns the arrival time of given task.
 * 
 * @param ts Target task.
 * 
 * @returns Arrival time of specified task.
 */
int task_arrivaltime(const struct task *ts)
{
	/* Sanity check. */
    assert(ts != NULL);

    return(ts->arrival_time);
}

/**
 * @brief Sets waiting time to a task. 
 * 
 * @param ts           Target task.
 * @param waiting_time How much task has waited.
 */
void task_set_waiting_time(struct task *ts, int waiting_time)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(waiting_time >= 0);

	ts->waiting_time = waiting_time;
}

/**
 * @brief Sets the amount of processed work of a task. 
 * 
 * @param ts   Target task.
 * @param work How much task has processed.
 */
void task_set_workprocess(struct task *ts, unsigned long int work)
{
	/* Sanity check. */
	assert(ts != NULL);

	ts->work_processed = work;
}

/**
 * @brief Sets the moment when task entered a core to be processed. 
 * If processing algorithm is based in a preemptive approach, this value will vary. 
 * 
 * @param ts     Target task.
 * @param moment Moment when task entered.
*/
void task_set_emoment(struct task *ts, int moment)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(moment >= 0);

	ts->e_moment = moment;
}

/**
 * @brief Gets the last moment that a task entered in a core. 
 * 
 * @param ts Target task.
 * 
 * @returns Returns the last moment that a task entered in a core.
*/
int task_emoment(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return ts->e_moment;
}

/**
 * @brief Sets the moment when task left from a core. 
 * If processing algorithm is based in a preemptive approach, this value vary. 
 * 
 * @param ts     Target task.
 * @param moment Moment when task entered.
*/
void task_set_lmoment(struct task *ts, int moment)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(moment >= 0);

	ts->l_moment = moment;
}

/**
 * @brief Creates task's memory address that will be accessed.
 * 
 * @param ts   Target task. 
 * @param lim  Upper limit of memory addresses. 
 * @param hist Memory Acesses histogram.
*/
void task_create_memacc(struct task *ts, histogram_tt hist)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(hist != NULL);
	int l = 0,
		j = 0;
	unsigned long int i = 0,
        			  k = 0;


	/* Storing randomly-generated task's memory acesses. Will contain size(task_workload) following gaussian distribuition */
	for ( l = 0; l < histogram_nclasses(hist); l++ )
	{
		int n = floor(histogram_class(hist, l) * ts->work);
		for ( j = 0; j < n; j++ )
		{
			/* Generating the memory accesses. */
			struct mem *m = mem_create(l);
			array_set(ts->memacc, k++, m);
		}
	}
	i = k;
	for ( /* */; i < ts->work; i++ )
	{
		j = rand()%histogram_nclasses(hist);
		struct mem *m = mem_create((i));
		array_set(ts->memacc, k++, m);
	}
}

/**
 * @brief Sets task's memory address that will be accessed.
 * 
 * @param ts Target task. 
 * @param a  Array of memory addresses.
*/
void task_set_memacc(struct task *ts, array_tt a)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(a != NULL);

	ts->memacc = a;
}

/**
 * @brief Sets task's memory pointer.
 * 
 * @param t   Target task.
 * @param pos Position of last memory address accessed.
*/
void task_set_memptr(struct task *ts, unsigned long int pos)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(pos <= task_workload(ts));

	ts->memptr = pos;
}

/**
 * @brief Gets the last moment that a task left in a core. 
 * 
 * @param ts Target task.
 * 
 * @returns Returns the last moment that a task left in a core.
*/
int task_lmoment(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return ts->l_moment;
}


/**
 * @brief Returns waiting time of a task. 
 * 
 * @param ts Target task.
 * 
 * @returns Waiting time of a task.
 */
int task_waiting_time(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return(ts->waiting_time);
}

/**
 * @brief Returns the workload of given task.
 * 
 * @param ts Target task.
 * 
 * @returns Workload of specified task.
 */
unsigned long int task_workload(const struct task *ts)
{
	/* Sanity check. */
    assert(ts != NULL);

    return(ts->work);
}

/**
 * @brief Assigns a new workload to given task.
 * 
 * @param ts Target task.
 * @param w  Workload.
 */
void task_set_workload(struct task *ts, unsigned long int w)
{
	/* Sanity check. */
    assert(ts != NULL);

	ts->work = w;
}

/**
 * @brief Returns the total processed workload of given task.
 * 
 * @param ts Target task.
 * 
 * @returns Total processed workload of specified task.
 */
unsigned long int task_work_processed(const struct task *ts)
{
	/* Sanity check. */
    assert(ts != NULL);

    return(ts->work_processed);
}

/**
 * @brief Returns how much work is left to be processed on given task.
 * 
 * @param ts Target task.
 * 
 * @returns How much work is left to be processed on specified task.
 */
unsigned long int task_work_left(const struct task *ts)
{
	/* Sanity check. */
    assert(ts != NULL);

    return(ts->work - ts->work_processed);
}

/**
 * @brief Checks specified page_table_line's valid bit.
 * 
 * @param ts  Target task.
 * @param idx Line's index.
 * 
 * @returns True if page_table_line is valid. False, otherwise.
 */
bool task_check_pt_line_valid(const struct task *ts, int idx)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(idx >= 0);
	return page_check_table_line_valid(page_table_line_at(ts->p_table, idx));
}

/**
 * @brief Sets page_table_lines's valid bit to false.
 * 
 * @param ts  Target task.
 * @param idx Line's index.
 */
void task_invalid_pt_line(struct task *ts, int idx)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(idx >= 0);
	page_invalid_table_line(page_table_line_at(ts->p_table, idx));
}

/**
 * @brief Sets page_table_lines's valid bit to true.
 * 
 * @param ts  Target task.
 * @param idx Line's index.
 */
void task_valid_pt_line(struct task *ts, int idx)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(idx >= 0);
	page_valid_table_line(page_table_line_at(ts->p_table, idx));
}

/**
 * @brief Searchs for a page_table_line that is valid and is assigned with frame_idx.
 * Returns page_table_line's id if found.
 * 
 * @param ts        Target task.
 * @param frame_idx Desired frame id.
 * 
 * @returns Page_table_line's id if found. -1, otherwise.
 */
int task_find_pt_line_frame_id(const struct task *ts, int frame_idx)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(frame_idx >= 0);

	int num_lines = page_table_num_lines(ts->p_table);
	for ( int i = 0; i < num_lines; i++ )
	{
		struct page_table_line *pl = page_table_line_at(ts->p_table, i);
		if ( page_check_table_line_valid(pl) )
		{
			if ( page_table_line_frameid(pl) == frame_idx ) return i;
		}

	}
	return -1;
}

/**
 * @brief Returns the page_table_line of current task's mem_addr.
 * 
 * @param ts Target task.
 * 
 * @returns Page_table_line of current task's mem_addr.
 */
int task_find_pt_line_memptr(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	struct mem *mem = array_get(ts->memacc, ts->memptr);
	return (mem_virtual_addr(mem));
}

/**
 * @brief Returns the frame_id of specified page_table_line.
 * 
 * @param ts Target task.
 * @param id Desired line.
 * 
 * @returns Frame_id of specified page_table_line.
 */
int task_get_pt_line_frameid(const struct task *ts, int id)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(id >= 0);
	struct page_table_line *pl = page_table_line_at(ts->p_table, id);
	return page_table_line_frameid(pl);		
}

/**
 * @brief Sets the frame_id of specified page_table_line.
 * 
 * @param ts       Target task.
 * @param id       Desired line.
 * @param frame_id Desired frame_id.
 */
void task_set_pt_line_frameid(struct task *ts, int id, int frame_id)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(id >= 0);
	struct page_table_line *pl = page_table_line_at(ts->p_table, id);
	page_set_table_line_frameid(pl, frame_id);		
}


/**
 * @brief Returns Task's page_table's number of lines.
 * 
 * @param ts Target task.
 * 
 * @returns Page_table's number of lines.
 */
int task_pt_num_lines(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return page_table_num_lines(ts->p_table);
}

/**
 * @brief Returns task's memory addresses.
 * 
 * @param ts Target task. 
 * 
 * @return Task's memory addresses.
*/
array_tt task_memacc(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return (ts->memacc);
}

/**
 * @brief Returns task's current memory_addr->cache_line mapping. i.e., Which cache sets tasks' addresses were assigned to.
 * 
 * @param ts Target task. 
 * 
 * @return Task's current memory_addr->cache_line mapping.
*/
int* task_lineacc(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return (ts->all_sets_accessed);
}

/**
 * @brief Returns task's current memory_addr->page_line mapping. i.e., Which page lines tasks' addresses were assigned to.
 * 
 * @param ts Target task. 
 * 
 * @return Task's current memory_addr->page_line mapping.
*/
int* task_pageacc(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return (ts->all_pages_accessed);
}

int map_compare_mem(void* addr1, void* addr2)
{
	/* Sanity check. */
	assert(addr1 != NULL);
	assert(addr2 != NULL);
    
	unsigned long int addr1_phy = mem_physical_addr((mem_tt) addr1) * PAGE_SIZE + mem_addr_offset((mem_tt) addr1);
	unsigned long int addr2_phy = mem_physical_addr((mem_tt) addr2) * PAGE_SIZE + mem_addr_offset((mem_tt) addr2);

	return (addr1_phy == addr2_phy);
}

/**
 * @brief Returns the total percentage of repeated sets in last WINSIZE.
 * 
 * @param ts      Target task.
 * @param winsize Winsize.
 * 
 * @returns Total percentage of repeated sets.
 */
double task_hotness(struct task *ts, int winsize)
{
	/* Sanity check. */
	assert(ts != NULL);
	int total_sum = 0;
	if ( task_work_processed(ts) != 0 ) 
	{

		/* Cleaning old values. */

		map_destroy(ts->sets_accessed);
		ts->sets_accessed = map_create(map_compare_int);

		// Saving last WINSIZE accesses into map
		for ( int i = 0; i < winsize; i++ )
		{
			int curr_set = ts->all_sets_accessed[ts->memptr - winsize + i];
			map_insert(ts->mem_accessed, &curr_set);
		}
		
		// Getting the number of appearences that each address had.
		for ( int i = 0; i < map_size(ts->mem_accessed); i++ )
		{
			struct map_return *m_r = map_peek(ts->mem_accessed, i);
			int num_obj = (int) m_r->num_obj;
			total_sum += num_obj;
		}
	}
	return (double) total_sum / winsize;
}

/**
 * @brief Returns task's memory pointer.
 * 
 * @param ts Target task.
 * 
 * @returns Task's memory pointer.
*/
unsigned long int task_memptr(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return (ts->memptr);
}

/**
 * @brief Checks if task has accessed specified cache set in last WINSIZE accesses.
 * 
 * @param ts      Target task.
 * @param set     Target set.
 * @param winsize Winsize.
 * 
 * @returns True if has accessed. False, otherwise.
 */
bool task_accessed_set(const struct task *ts, int set, int winsize)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(winsize >= 0);

	// Not processed yet.
	if ( ts->memptr == 0 ) return false;

	for ( int i = 0; i < winsize; i++ )
	{
		if ( ts->all_sets_accessed[(ts->lineptr - winsize + i)] == set ) return true;
	}
	return false;
}

/**
 * @brief Gets the ID of a task.
 *
 * @param ts Target task.
 *
 * @returns The ID of the target task.
 */
int task_gettsid(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return (ts->tsid);
}

/**
 * @brief Sets the number of page hits that happened while processing.
 * 
 * @param ts    Target task.
 * @param p_hit Number of hits.
*/
void task_set_page_hit(struct task *ts, unsigned long int p_hit)
{
	/* Sanity check. */
	assert(ts != NULL);
	ts->page_hits = p_hit;
}

/**
 * @brief Sets the number of page faults that happened while processing.
 * 
 * @param ts      Target task.
 * @param p_fault Number of faults.
*/
void task_set_page_fault(struct task *ts, unsigned long int p_fault)
{
	/* Sanity check. */
	assert(ts != NULL);
	ts->page_faults = p_fault;
}

/**
 * @brief Gets the number of page hits that happened while processing.
 * 
 * @param ts Target task.
*/
unsigned long int task_page_hit(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);
	return (ts->page_hits);
}

/**
 * @brief Gets the number of page faults that happened while processing.
 * 
 * @param ts Target task.
*/
unsigned long int task_page_fault(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);
	return (ts->page_faults);
}


/**
 * @brief Sets the number of cache hits that task had while processing.
 * 
 * @param ts  Target task.
 * @param hit Number of hits.
*/
void task_set_hit(struct task *ts, unsigned long int hit)
{
	/* Sanity check. */
	assert(ts != NULL);

	ts->hits = hit;
}

/**
 * @brief Sets the number of cache misses that task had while processing.
 * 
 * @param ts   Target task.
 * @param miss Number of misses.
*/
void task_set_miss(struct task *ts, unsigned long int miss)
{
	/* Sanity check. */
	assert(ts != NULL);

	ts->misses = miss;
}

/**
 * @brief Gets the number of cache hits that task had while processing.
 * 
 * @param ts Target task.
*/
unsigned long int task_hit(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);
	return (ts->hits);
}

/**
 * @brief Gets the number of cache hits that task had while processing.
 * 
 * @param ts Target task.
*/
unsigned long int task_miss(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);
	return (ts->misses);
}

/**
 * @brief Sets the core id where task is assigned to. Used at SCA scheduling strategy.
 * 
 * @param ts  Target task.
 * @param cid Core id.
*/
void task_core_assign(struct task *ts, int cid)
{
	/* Sanity check. */
	assert(ts != NULL);
	assert(cid >= 0);

	ts->core_assigned = cid;
}

/**
 * @brief Returns the core id where task is assigned to. Used at SCA scheduling strategy.
 * 
 * @param ts  Target task.
 * @param cid Core id.
 * 
 * @returns The core id where task is assigned to.
*/
int task_core_assigned(const struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);

	return (ts->core_assigned);
}

/**
 * @brief Destroys a task.
 *
 * @param ts Target task.
 */
void task_destroy(struct task *ts)
{
	/* Sanity check. */
	assert(ts != NULL);


	page_table_destroy(ts->p_table);
	for ( unsigned long int i = 0; i < ts->work; i++ )
		mem_destroy(array_get(ts->memacc, i));

	map_destroy(ts->mem_accessed);
	map_destroy(ts->sets_accessed);
	map_destroy(ts->pages_accessed);

	array_destroy(ts->memacc);
	free(ts->all_sets_accessed);
	free(ts->all_pages_accessed);
	free(ts);
}