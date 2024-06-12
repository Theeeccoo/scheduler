#ifndef SCHED_ITR
#define SCHED_ITR

    /**
	 * @brief Opaque pointer to a scheduler iteration.
	 */
	typedef struct sched_itr * sched_itr_tt;

	/**
	 * @brief Constant opaque pointer to a scheduler iteration.
	 */
	typedef const struct sched_itr * const_sched_itr_tt;

	/**
	 * @name Operations on sched_itr
	 */
	/**@{*/
	extern sched_itr_tt scheditr_create(unsigned long int, int);
	extern void scheditr_destroy(sched_itr_tt);

    extern unsigned long int scheditr_twork(const_sched_itr_tt);
    extern int scheditr_ntasks(const_sched_itr_tt);

    /**@}*/

#endif /* SCHED_ITR */