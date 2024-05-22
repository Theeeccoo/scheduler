#include <assert.h>
#include <mylib/util.h>
#include <stdlib.h>

#include <mem.h>

struct mem
{
	int memaddr;  /**< Memory address. */
};

/**
 * @brief Instantiate a new memory space.
 * 
 * @param addr Target address. If addr is -1, it implies that mem is not being used
*/
mem_tt mem_create(int addr)
{
	struct mem *mem;

	/* Sanity check. */
	assert(addr >= -1);

	mem = smalloc(sizeof(struct mem));

	mem->memaddr = addr;
	return (mem);
}

/**
 * @brief Returns memory space's address.
 * 
 * @param m Target memory space.
 * 
 * @returns Memory space's address.
*/
int mem_addr(const struct mem *m)
{
    /* Sanity check. */
    assert(m != NULL);

    return (m->memaddr);
}


/**
 * @brief Destroys a memory space.
 *
 * @param m Target memory space.
 */
void mem_destroy(struct mem *m)
{
	/* Sanity check. */
    assert(m != NULL);

    free(m);
}

