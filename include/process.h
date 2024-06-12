#ifndef PROCESS_H_
#define PROCESS_H_

    #include <stdbool.h>
    #include <mylib/array.h>

    #include "core.h"
    #include "ram.h"
    #include "workload.h"

    /**
	 * @brief Scheduler definitions.
	 */
	/**@{*/
    #define QUANTUM              10000 /**< Round-Robin Quantum (cycles). */
	#define MISS_PENALTY            50 /**< Cache miss penalty (cycles).  */
    #define PAGE_FAULT_PENALTY  500000 /**< Page fault penalty (cycles).  */
	/**@}*/

    /**
     * @brief Task process strategy.
    */
    struct processer
    {
        void (*init)(workload_tt, array_tt, int*, RAM_tt); /**< Initialize processer. */
        void (*process)(void);                             /**< Process.              */
        void (*end)(void);                                 /**< End processer.        */
    };


    /**
     * @brief Supported Object Processing strategies.
    */
    /**@{*/
    extern const struct processer *non_preemptive;
    extern const struct processer *random_preemptive;
    extern const struct processer *rr_preemptive;
    /**@}*/

#endif /* PROCESS_H_ */