#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <core.h>
#include <workload.h>
#include <sched_itr.h>

/**
 * @brief Core.
 */
struct core
{
	int cid;                            /**< Identification number.                            */
	unsigned long int wtotal;           /**< Total assigned workload.                          */
	int capacity;                       /**< Max number of tasks.                              */
    int contention;                     /**< Core contention value.                            */
    queue_tt pr_tasks;                  /**< Tasks that are currently being processed on Core. */

    queue_tt total_workload;            /**< Total workload per scheduling iteration.          */

    unsigned long int total_page_hit;   /**< Total page hits while processing.                 */
    unsigned long int total_page_fault; /**< Total page faults while processing.               */

    unsigned long int total_hits;       /**< Total cache hits while processing.                */
    unsigned long int total_misses;     /**< Total cache misses while processing.              */

    cache_tt cache;                     /**< Core's cache.                                     */
    mmu_tt mmu;                         /**< Core's MMU.                                       */
};

/**
 * @brief Next available core identification number.
*/
static int next_cid = 0;

/**
 * @brief Creates a core.
 * 
 * @param capacity   Max number of tasks.
 * @param cache_sets Total number of cache sets.
 * @param cache_ways Total number of cache ways.
 * @param num_blocks Total number of blocks per cache way.
 * 
 * @returns A Core.
*/
struct core *core_create(int capacity, int cache_sets, int cache_ways, int num_blocks)
{
    struct core *c;
    
    /* Sanity Check. */
    assert(capacity > 0);
    assert(cache_sets > 0);
    assert(cache_ways > 0);
    assert(num_blocks > 0);

    c = smalloc(sizeof(struct core));
    c->cid = next_cid++;
    c->wtotal = 0;
    c->capacity = capacity;
    c->contention = 0;
    c->pr_tasks = queue_create();
    c->total_page_hit = 0;
    c->total_page_fault = 0;
    c->total_hits = 0;
    c->total_misses = 0;

    /* Initializing cache. */
    c->cache = cache_create(cache_sets, cache_ways, num_blocks);
    /* Initializing MMU. */
    c->mmu = mmu_create(c->cid);

    c->total_workload = queue_create();
    queue_insert(c->total_workload, scheditr_create(0, 0));
    return (c);
}

/**
 * @brief Returns Core's current processing tasks.
 * 
 * @param c Desired Core.
 * 
 * @returns Core's current processing tasks.
*/
queue_tt core_get_tsks(const struct core *c)
{
    /* Sanity check. */
	assert(c != NULL);
    return (c->pr_tasks);
}

/**
 * @brief Populates a specific Core with a task.
 * 
 * @param c  Desired Core.
 * @param ts Desired task.
*/
void core_populate(struct core *c, struct task *ts)
{
	/* Sanity check. */
	assert(c != NULL);
    assert(ts != NULL);

    c->wtotal += task_work_left(ts);
    queue_insert(c->pr_tasks, ts);
}

/**
 * @brief Vacate a Core, removing all tasks in it.
 * 
 * @param c Desired Core.
*/
void core_vacate(struct core *c)
{
	/* Sanity check. */
	assert(c != NULL);

    /* Usually, this queue is emptied throught the simulation */
    for ( int i = 0; i < queue_size(c->pr_tasks); i++ ) queue_remove(c->pr_tasks);
}

/**
 * @brief Stores the total workload currently scheduled to desired core. 
 * 
 * @param c      Desired Core.
 * @param wtotal Total Workload.
 * @param ntasks Total number of tasks. 
*/
void core_set_workloads(struct core *c, unsigned long int wtotal, int ntasks)
{
    /* Sanity check. */
	assert(c != NULL);

    queue_insert(c->total_workload, scheditr_create(wtotal, ntasks));
}

/**
 * @brief Returns the queue that stores the total workload scheduled per iteration, until given moment, to desired core. 
 * 
 * @param c Desired Core.
 * 
 * @returns The queue that stores the total workload scheduled, until given momento, to desired core. 
*/
queue_tt core_workloads(const struct core *c)
{
    /* Sanity check. */
	assert(c != NULL);   
    return (c->total_workload);
}

/**
 * @brief Returns the total workload that a core has received at given moment.
 * 
 * @param c Target core.
 * 
 * @returns The total workload that a core has received at given moment.
 */
unsigned long int core_wtotal(const struct core *c)
{
    /* Sanity check. */
	assert(c != NULL);

    return (c->wtotal);
}

/**
 * @brief Returns the capacity of a core.
 *
 * @param c Target core.
 *
 * @returns The capacity of the target core.
 */
int core_capacity(const struct core *c)
{
	/* Sanity check. */
	assert(c != NULL);

	return (c->capacity);
}

/**
 * @brief Sets the contention value of a desired core.
 * 
 * @param c       Target core.
 * @param c_value Contention value.
*/
void core_set_contention(struct core *c, int c_value)
{
    /* Sanity check. */
	assert(c != NULL);

    c->contention = c_value;
}

/**
 * @brief Returns the contention value of a core.
 *
 * @param c Target core.
 *
 * @returns The contention value of the target core.
 */
int core_contention(const struct core *c)
{
    /* Sanity check. */
	assert(c != NULL);

	return (c->contention);
}


/**
 * @brief Gets the ID of a core.
 *
 * @param c Target core.
 *
 * @returns The ID of the target core.
 */
int core_getcid(const struct core *c)
{
    /* Sanity check. */
	assert(c != NULL);

	return (c->cid);
}

/**
 * @brief Returns cache's number of acesses in each cache set. 
 * 
 * @param c Target core.
 * 
 * @returns Cache's number of acesses in each cache set.
 */
int* core_cache_sets_accesses(const struct core *c)
{
    /* Sanity check. */
    assert(c != NULL);
    assert(c->cache != NULL);
    return cache_set_accesses(c->cache);
}

/**
 * @brief Updates cache's number of acesses of a specified cache set.
 * 
 * @param c            Target core.
 * @param index        Target set.
 * @param update_value Desired update value.
 */
void core_cache_sets_accesses_update(struct core *c, int index, int update_value)
{
    /* Sanity check. */
    assert(c != NULL);
    cache_set_accesses_update(c->cache, index, update_value);
}

/**
 * @brief Translates the virtual memory to physical memory using RAM's frames.
 * The physical address translated is inside "mem". 
 * Translation is made by core's MMU.
 * 
 * @param c   Target core.
 * @param ts  Task responsible of specified memory instance.
 * @param mem Desired instace of memory to be translated.
 * @param ram Simulation's RAM.
 * 
 * @returns True if Page Hit. False if Page Fault.
 */
bool core_mmu_translate(struct core *c, struct task *ts, struct mem *mem, struct RAM *ram)
{
    /* Sanity check. */
	assert(c != NULL);
	assert(ts != NULL);
    assert(mem != NULL);
	assert(ram != NULL);

    return (mmu_translate(c->mmu, ts, mem, ram));
}

/**
 * @brief Returns the number of cache sets in a core.
 * 
 * @param c Target core.
 * 
 * @returns The number of cache sets in core's cache.
 */
int core_cache_num_sets(const struct core *c)
{
	/* Sanity check. */
	assert(c != NULL);

	return cache_num_sets(c->cache);
}

/**
 * @brief Checks if a specified address is already in core's cache.
 * 
 * @param c    Target core.
 * @param addr Specified addr.
 * 
 * @return True if cache hit. False if cache miss.
*/
bool core_cache_checkaddr(const struct core *c, struct mem *addr)
{   
    /* Sanity check. */
	assert(c != NULL);
    assert(addr != NULL);

    return cache_check_addr(c->cache, addr);
}  

/**
 * @brief Replaces a cache way with a new address. FIFO approach
 * 
 * @param c    Target core.
 * @param addr New address.
*/
void core_cache_replace(struct core *c, struct mem *addr)
{
    /* Sanity check. */
	assert(c != NULL);
    assert(addr != NULL);

    cache_replace(c->cache, addr);
}

/**
 * @brief Sets the number of page hits that happened while processing.
 * 
 * @param c     Target core.
 * @param p_hit Number of hits.
*/
void core_set_page_hit(struct core *c, unsigned long int p_hit)
{
    /* Sanity check. */
    assert(c != NULL);
    c->total_page_hit = p_hit;
}

/**
 * @brief Sets the number of page faults that happened while processing.
 * 
 * @param c       Target core.
 * @param p_fault Number of faults.
*/
void core_set_page_fault(struct core *c, unsigned long int p_fault)
{
    /* Sanity check. */
    assert(c != NULL);
    c->total_page_fault = p_fault;
}

/**
 * @brief Gets the number of page hits that happened while processing.
 * 
 * @param c Target core.
*/
unsigned long int core_page_hit(const struct core *c)
{
    /* Sanity check. */
    assert(c != NULL);
    return (c->total_page_hit);
}

/**
 * @brief Gets the number of page faults that happened while processing.
 * 
 * @param c Target core.
*/
unsigned long int core_page_fault(const struct core *c)
{
    /* Sanity check. */
    assert(c != NULL);
    return (c->total_page_fault);
}

/**
 * @brief Sets the number of cache hits that happened while processing.
 * 
 * @param c   Target core.
 * @param hit Number of hits.
*/
void core_set_hit(struct core *c, unsigned long int hit)
{
    /* Sanity check. */
	assert(c != NULL);

	c->total_hits = hit;
}

/**
 * @brief Sets the number of cache misses that happened while processing.
 * 
 * @param c   Target core.
 * @param miss Number of misses.
*/
void core_set_miss(struct core *c, unsigned long int miss)
{
    /* Sanity check. */
	assert(c != NULL);

	c->total_misses = miss;
}

/**
 * @brief Gets the number of cache hits that happened while processing.
 * 
 * @param c Target core.
*/
unsigned long int core_hit(const struct core *c)
{
    /* Sanity check. */
	assert(c != NULL);

    return(c->total_hits);
}


/**
 * @brief Gets the number of cache misses that happened while processing.
 * 
 * @param c Target core.
*/
unsigned long int core_miss(const struct core *c)
{
    /* Sanity check. */
	assert(c != NULL);

    return(c->total_misses);
}

/**
 * @brief Destroys a core.
 *
 * @param c Target core.
 */
void core_destroy(struct core *c)
{
	/* Sanity check. */
	assert(c != NULL);

    mmu_destroy(c->mmu);
    cache_destroy(c->cache);
    for ( int i = 0; i < queue_size(c->pr_tasks); i++ )
    {
        task_destroy(queue_remove(c->pr_tasks));
    }
    queue_destroy(c->pr_tasks);

    for ( int i = 0; i < queue_size(c->total_workload); i++ )
    {
        scheditr_destroy(queue_remove(c->total_workload));
    }
    queue_destroy(c->total_workload);
	free(c);
}