#ifndef SCHEDULER_H_
#define SCHEDULER_H_

    #include <stdbool.h>
    #include <mylib/array.h>
    #include <mylib/queue.h>

    #include "workload.h"
    #include "process.h"

    /**
     * @brief Task scheduling strategy.
    */
    struct scheduler
    {
        bool pincores;                  /**< Pin Cores?          */
        void (*init)(workload_tt, int); /**< Initialize scheduler. */
        int  (*sched)(core_tt);         /**< Schedule.             */
        void (*end)(void);              /**< End scheduler.        */
    };


    /**
     * @brief Supported Object Scheduling strategies.
    */
    /**@{*/
    extern const struct scheduler *sched_fcfs;
    extern const struct scheduler *sched_srtf;
    extern const struct scheduler *sched_sca;
    /**@}*/

    /* Forward definitions. */
    extern int g_iterator;

    extern void simsched(workload_tt, array_tt, const struct scheduler*, const struct processer*, int);
#endif /* SCHEDULER_H_ */