#ifndef MEM_H_
#define MEM_H_

    #include <stdbool.h>


    /**
	 * @brief Opaque pointer to a memory address.
	 */
	typedef struct mem * mem_tt;

	/**
	 * @brief Constant opaque pointer to a memory address.
	 */
	typedef const struct mem * const_mem_tt;

	/**
	 * @name Operations on mem
	 */
	/**@{*/
	extern mem_tt mem_create(int);
	extern void   mem_destroy(mem_tt);

    extern int    mem_addr(const_mem_tt);
    /**@}*/

#endif /* MEM_H_ */