
#include <assert.h>
#include <mylib/util.h>
#include <stdlib.h>

#include <sched_itr.h>

/**
 * @brief Represents one scheduling iteration.
 */
struct sched_itr
{
    int twork;  /**< Total workload assigned.                    */
    int ntasks; /**< Total number of tasks assgnied at that itr. */
};

/**
 * @brief Instantiate a new scheduling iteration.
 * 
 * @param twork  Total work assigned to a core in a given scheduling iteration.
 * @param ntasks Total number of tasks assigned to a core in the same iteration.
 *
*/
sched_itr_tt scheditr_create(int twork, int ntasks)
{
	struct sched_itr *si;

	/* Sanity check. */
	assert(twork >= 0);
	assert(ntasks >= 0);


	si = smalloc(sizeof(struct sched_itr));

    si->twork = twork;
    si->ntasks = ntasks;
	return (si);
}

/**
 * @brief Returns scheduling iteration's total work.
 * 
 * @param si Target scheduling iteration.
 * 
 * @returns Scheduling iteration's total work.
*/
int scheditr_twork(const struct sched_itr *si)
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