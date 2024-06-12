#ifndef MEM_H_
#define MEM_H_

	/**
	 * @brief Memory definitions.
	 */
	/**@{*/
	#define WORD_SIZE          4 /**<  4B */
	#define BLOCK_SIZE        64 /**< 64B */
	#define PAGE_SIZE       4096 /**< 4KB */
	#define RAM_SIZE  4294967296 /**< 4GB */
	/**@}*/

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
	extern mem_tt mem_create(unsigned long int);
	extern void   mem_destroy(mem_tt);

    extern unsigned long int mem_virtual_addr(const_mem_tt);
	extern void              mem_set_physical_addr(mem_tt, unsigned long int);
    extern unsigned long int mem_physical_addr(const_mem_tt);
    extern int               mem_addr_offset(const_mem_tt);
    /**@}*/

#endif /* MEM_H_ */