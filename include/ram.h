#ifndef RAM_H_
#define RAM_H_

    #include <stdbool.h>
    #include "mem.h"
    #include "task.h"
    #include "workload.h"

    /**
     * @brief Opaque pointer to a Random Access Memory (RAM) instance
     */
    typedef struct RAM * RAM_tt;

    /**
     * @brief Constant opaque pointer to a Random Access Memory (RAM) instance
     */
    typedef const struct RAM * const_RAM_tt;

    /**
     * @name Operations on RAM
     */
    /**@{*/
    extern RAM_tt            RAM_init(workload_tt);
    extern unsigned long int RAM_num_frames(const_RAM_tt);
    extern unsigned long int RAM_next_frame(RAM_tt, int);
    extern void              RAM_destroy(RAM_tt);
    /**@}*/



#endif /* RAM_H_ */