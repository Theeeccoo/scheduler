#ifndef CORE_H_
#define CORE_H_

    #include <stdbool.h>

	#include "mylib/util.h"
	#include "mylib/array.h"
    #include "mylib/queue.h"
	#include "task.h"

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
	extern core_tt core_create(int);
	extern void core_populate(core_tt, task_tt);
	extern void core_vacate(core_tt);
	extern void core_destroy(core_tt);
	extern int core_getcid(const_core_tt);
	extern queue_tt core_get_tsks(const_core_tt);

	extern void core_set_contention(core_tt, int);
	extern int core_contention(const_core_tt);
	extern int core_wtotal(const_core_tt);
    /**@}*/

#endif /* CORE_H_ */