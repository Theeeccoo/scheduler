#ifndef CACHE_H_
#define CACHE_H_

    #include <stdbool.h>
    #include "mem.h"

    /**
	 * @brief Opaque pointer to a cache.
	 */
	typedef struct cache * cache_tt;

	/**
	 * @brief Constant opaque pointer to a cache.
	 */
	typedef const struct cache * const_cache_tt;

	/**
	 * @name Operations on cache
	 */
	/**@{*/
    extern cache_tt cache_create(int, int, int);
    extern void     cache_destroy(cache_tt);
    
    extern bool cache_check_addr(const_cache_tt, mem_tt);
    extern void cache_replace(cache_tt, mem_tt);
    extern int cache_num_sets(const_cache_tt);
	extern int* cache_set_accesses(const_cache_tt);
	extern void cache_set_accesses_update(cache_tt, int, int);
    /**@}*/

#endif /* CACHE_H_ */