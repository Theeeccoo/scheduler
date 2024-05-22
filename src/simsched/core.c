#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <core.h>
#include <workload.h>


/**
 * @brief Core.
 */
struct core
{
	int cid;           /**< Identification number.                            */
	int wtotal;        /**< Total assigned workload.                          */
	int capacity;      /**< Processing capacity.                              */
    int contention;    /**< Core contention value.                            */
    queue_tt pr_tasks; /**< Tasks that are currently being processed on Core. */

    array_tt cache;    /**< Core's cache.                                     */
};

/**
 * @brief Next available core identification number.
*/
static int next_cid = 0;

/**
 * @brief Creates a core.
 * 
 * @param capacity   Processing capacity.
 * @param cache_size Cache size.
 * 
 * @returns A Core.
*/
struct core *core_create(int capacity, int cache_size)
{
    struct core *c;

    c = smalloc(sizeof(struct core));
    c->cid = next_cid++;
    c->wtotal = 0;
    c->capacity = capacity;
    c->contention = 0;
    c->pr_tasks = queue_create();

    c->cache = array_create(cache_size);   
    /* Initializing cache. */
    for ( int i = 0; i < cache_size; i++ )
        array_set(c->cache, i, mem_create(-1));
    
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

int core_wtotal(const struct core *c)
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
 * @param ts Target core.
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
 * @brief Gets the cache of a core.
 *
 * @param ts Target core.
 *
 * @returns The cache of the target core.
 */
array_tt core_cache(const struct core *c)
{
    /* Sanity check. */
	assert(c != NULL);

	return (c->cache);
}


/**
 * @brief Checks if a specified address is already in core's cache.
 * 
 * @param c    Target core.
 * @param addr Specified addr.
 * 
 * @return True if hit. False if miss.
*/
bool core_cache_checkaddr(const struct core *c, struct mem *addr)
{   
    /* Sanity check. */
	assert(c != NULL);
    assert(addr != NULL);

    int cache_way = mem_addr(addr) % array_size(c->cache);
    struct mem* c_addr = array_get(c->cache, cache_way);
    return (mem_addr(c_addr) == mem_addr(addr));
}   

/**
 * @brief Replaces a cache way with a new address.
 * 
 * @param c    Target core.
 * @param addr New address.
*/
void core_cache_replace(struct core *c, struct mem *addr)
{
    /* Sanity check. */
	assert(c != NULL);
    assert(addr != NULL);

    int cache_way = mem_addr(addr) % array_size(c->cache);
    array_set(c->cache, cache_way, addr);
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

    array_destroy(c->cache);
    queue_destroy(c->pr_tasks);
	free(c);
}