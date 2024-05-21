#ifndef PROCESS_H_
#define PROCESS_H_

    #include <stdbool.h>
    #include <mylib/array.h>

    #include "workload.h"
    #include "core.h"

    /**
     * @brief Task process strategy.
    */
    struct processer
    {
        void (*init)(workload_tt, array_tt, int*); /**< Initialize processer. */
        void (*process)(void);                     /**< Process.              */
        void (*end)(void);                         /**< End processer.        */
    };


    /**
     * @brief Supported Object Processing strategies.
    */
    /**@{*/
    extern const struct processer *non_preemptive;
    extern const struct processer *random_preemptive;
    extern const struct processer *rr_preemptive;
    /**@}*/

    /* Forward definitions. */
    // void process_tasks(core_tt, int*);
#endif /* PROCESS_H_ */