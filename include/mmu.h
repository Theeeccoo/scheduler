#ifndef MMU_H_
#define MMU_H_

    #include <stdbool.h>
    #include "mylib/util.h"
    #include "task.h"
    #include "mem.h"
    #include "ram.h"

    /**
     * @brief Opaque pointer to a memory management unit (MMU)
     */
    typedef struct mmu * mmu_tt;

    /**
     * @brief Constant opaque pointer to a memory management unit (MMU)
     */
    typedef const struct mmu * const_mmu_tt;

    /**
     * @name Operations on mmu
     */
    /**@{*/
    extern mmu_tt mmu_create(int);
    extern bool   mmu_translate(const_mmu_tt, task_tt, mem_tt, RAM_tt);

    extern void   mmu_destroy(mmu_tt);
    /**@}*/



#endif /* MMU_H_ */