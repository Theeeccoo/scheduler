#ifndef CORE_H_
#define CORE_H_

    #include <stdbool.h>

	#include "cache.h"
	#include "mylib/util.h"
	#include "mylib/array.h"
    #include "mylib/queue.h"
	#include "task.h"
	#include "mmu.h"
	#include "mem.h"

    /**
	 * @brief Opaque pointer to a core.
	 */
	typedef struct core * core_tt;

	/**
	 * @brief Constant opaque pointer to a core.
	 */
	typedef const struct core * const_core_tt;

	/**
	 * @name Operations on Core
	 */
	/**@{*/
	extern core_tt core_create(int, int, int, int);
	extern void core_populate(core_tt, task_tt);
	extern int core_capacity(const_core_tt);
	extern void core_vacate(core_tt);
	extern void core_destroy(core_tt);
	extern int core_getcid(const_core_tt);
	extern queue_tt core_get_tsks(const_core_tt);
	
	extern void core_set_page_hit(core_tt, unsigned long int);
	extern void core_set_page_fault(core_tt, unsigned long int);
	extern unsigned long int core_page_hit(const_core_tt);
	extern unsigned long int core_page_fault(const_core_tt);
	extern void core_set_hit(core_tt, unsigned long int);
	extern void core_set_miss(core_tt, unsigned long int);
	extern unsigned long int core_hit(const_core_tt);
	extern unsigned long int core_miss(const_core_tt);

	extern bool core_mmu_translate(core_tt, task_tt, mem_tt, RAM_tt);
	extern bool core_cache_checkaddr(const_core_tt, mem_tt);
	extern void core_cache_replace(core_tt, mem_tt);
	extern int core_cache_num_sets(const_core_tt);
	extern map_tt core_cache_sets_accesses(const_core_tt);
	extern void core_cache_sets_accesses_update(core_tt, int);
	extern map_tt core_cache_sets_conflicts(const_core_tt);
	extern void core_cache_sets_conflicts_update(core_tt, int);
	extern double core_cache_sets_variance(const_core_tt);


	extern void core_set_contention(core_tt, int);
	extern int core_contention(const_core_tt);

	extern void core_set_workloads(core_tt, unsigned long int, int);
	extern queue_tt core_workloads(const_core_tt);
	extern unsigned long int core_wtotal(const_core_tt);
	
    /**@}*/

#endif /* CORE_H_ */