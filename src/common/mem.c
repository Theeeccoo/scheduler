#include <assert.h>
#include <stdlib.h>

#include <mylib/util.h>
#include <mem.h>

struct mem
{
	unsigned long int virtual_address;  /**< Virtual address.  */
	unsigned long int physical_address; /**< Physical address. */
	int offset;                         /**< Memory offset.    */
};

/**
 * @brief Instantiate a new memory space.
 * 
 * @param addr Target virtual address.
 * 
 * @returns New memory space instance.
*/
mem_tt mem_create(unsigned long int addr)
{
	struct mem *mem;

	mem = smalloc(sizeof(struct mem));

	mem->virtual_address = (unsigned long int) (addr / PAGE_SIZE);
	mem->physical_address = -1;
	mem->offset = (addr - (mem->virtual_address * PAGE_SIZE));
	return (mem);
}

/**
 * @brief Returns memory's virtual address. IMPORTANT this value is NOT multiplied by PAGE_SIZE,
 * If you want to use it you MUST multiply it.
 * 
 * @param m Target memory. 
 * 
 * @returns Memory's virtual address.
*/
unsigned long int mem_virtual_addr(const struct mem *m)
{
    /* Sanity check. */
    assert(m != NULL);
    return (m->virtual_address);
}

/**
 * @brief Returns memory's physical address. IMPORTANT this value is NOT multiplied by PAGE_SIZE,
 * If you want to use it you MUST multiply it.
 * 
 * @param m Target memory. 
 * 
 * @returns Memory's physical address.
*/
unsigned long int mem_physical_addr(const struct mem *m)
{
    /* Sanity check. */
    assert(m != NULL);

    return (m->physical_address);
}

/**
 * @brief Sets the physical address (based in a frame).
 * 
 * @param m   Target memory.
 * @param idx Frame's initial address.
 */
void mem_set_physical_addr(struct mem *m, unsigned long int idx)
{
	/* Sanity check. */
    assert(m != NULL);

	m->physical_address = idx;
}

/**
 * @brief Returns memory offset.
 * 
 * @param m Target memory. 
 * 
 * @returns Memory offset.
*/
int mem_addr_offset(const struct mem *m)
{
    /* Sanity check. */
    assert(m != NULL);

    return (m->offset);
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

