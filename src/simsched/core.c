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
};

/**
 * @brief Next available core identification number.
*/
static int next_cid = 0;

/**
 * @brief Creates a core.
 * 
 * @param capacity Processing capacity.
 * 
 * @returns A Core.
*/
struct core *core_create(int capacity)
{
    struct core *c;

    c = smalloc(sizeof(struct core));
    c->cid = next_cid++;
    c->wtotal = 0;
    c->capacity = capacity;
    c->contention = 0;
    c->pr_tasks = queue_create();

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
    queue_insert(c->pr_tasks, t);
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
 * @brief Destroys a core.
 *
 * @param c Target core.
 */
void core_destroy(struct core *c)
{
	/* Sanity check. */
	assert(c != NULL);

    queue_destroy(c->pr_tasks);
	free(c);
}