
#include <assert.h>
#include <mylib/util.h>
#include <stdlib.h>

#include <sched_itr.h>

/**
 * @brief Represents one scheduling iteration.
 */
struct sched_itr
{
    unsigned long int twork; /**< Total workload assigned.                      */
    int pmiss;               /**< Total time spent with misses in an iteration. */
    int ntasks;              /**< Total number of tasks assgnied at that itr.   */
};

/**
 * @brief Instantiate a new scheduling iteration.
 * 
 * @param twork  Total work assigned to a core in a given scheduling iteration.
 * @param ntasks Total number of tasks assigned to a core in the same iteration.
 *
*/
sched_itr_tt scheditr_create(unsigned long int twork, int ntasks)
{
	struct sched_itr *si;

	/* Sanity check. */
	assert(ntasks >= 0);


	si = smalloc(sizeof(struct sched_itr));

    si->twork = twork;
    si->ntasks = ntasks;
    si->pmiss = 0;
	return (si);
}

/**
 * @brief Sets the total time spent with misses in an iteration.
 * 
 * @param si    Target iteration.
 * @param pmiss Time spent.
 */
void scheditr_set_pmiss(struct sched_itr *si, int pmiss)
{
    /* Sanity check. */
    assert(si != NULL);
    assert(pmiss >= 0);

    si->pmiss = pmiss;
}

/**
 * @brief Returns the total time spent with misses in an iteration.
 * 
 * @param si Target iteration.
 * 
 * @returns Total time spent with misses in an iteration.
 */
int scheditr_pmiss(const struct sched_itr *si)
{   
    /* Sanity check. */
    assert(si != NULL);

    return (si->pmiss);
}

/**
 * @brief Returns scheduling iteration's total work.
 * 
 * @param si Target scheduling iteration.
 * 
 * @returns Scheduling iteration's total work.
*/
unsigned long int scheditr_twork(const struct sched_itr *si)
{
    /* Sanity check. */
    assert(si != NULL);

    return (si->twork);
}

/**
 * @brief Returns scheduling iteration's total work.
 * 
 * @param si Target scheduling iteration.
 * 
 * @returns Scheduling iteration's total work.
*/
int scheditr_ntasks(const struct sched_itr *si)
{
    /* Sanity check. */
    assert(si != NULL);

    return (si->ntasks);
}

/**
 * @brief Destroys a scheduling iteration.
 *
 * @param si Target scheduling iteration.
 */
void scheditr_destroy(struct sched_itr *si)
{
	/* Sanity check. */
    assert(si != NULL);

    free(si);
}