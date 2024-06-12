#include <assert.h>
#include <stdlib.h>

#include <mmu.h>

/**
 * @brief MMU.
 */
struct mmu
{
    int core_id; /**< Which core this MMU is related to. */
};

/**
 * @brief Creates a new MMU instance.
 * 
 * @param core_id Which core MMU is related to.
 * 
 * @returns A MMU instance.
 */
struct mmu *mmu_create(int core_id)
{
    struct mmu *mmu = smalloc(sizeof(struct mmu));
    mmu->core_id = core_id;
    return (mmu);
}

/**
 * @brief Translates the virtual memory to physical memory using RAM's frames.
 * The physical address translated is inside "mem".
 * If task's page table line is invalid, we must ask for RAM's next frame.
 * 
 * @param mmu Target MMU. 
 * @param ts  Task responsible of specified memory instance.
 * @param mem Desired instace of memory to be translated.
 * @param ram Simulation's RAM.
 * 
 * @returns True if Page Hit. False if Page Fault.
 */
bool mmu_translate(const struct mmu *mmu, struct task *ts, struct mem *mem, RAM_tt ram)
{
    /* Sanity check. */
    assert(mmu != NULL);
    assert(ts != NULL);
    assert(mem != NULL);

    unsigned long int mem_virtual_address  = mem_virtual_addr(mem) * PAGE_SIZE,
                      mem_physical_address = 0;
    int index    = (int) (mem_virtual_address / PAGE_SIZE),
        frame_id = 0;
                 /* Checking if mem addr's line is valid. */
    // If not valid, page fault
    bool page_hit = task_check_pt_line_valid(ts, index);

    if ( !page_hit )
    {
        frame_id = RAM_next_frame(ram, task_gettsid(ts));
        mem_physical_address = frame_id;
        task_valid_pt_line(ts, index);
        task_set_pt_line_frameid(ts, index, frame_id);
    } 
    else 
        mem_physical_address = task_get_pt_line_frameid(ts, index);

    mem_set_physical_addr(mem, mem_physical_address);
    return page_hit;
}   

/**
 * @brief Destroys MMU.
 * 
 * @param mmu Target MMU.
 */
void mmu_destroy(struct mmu *mmu)
{
    /* Sanity check. */
    assert(mmu != NULL);
    free(mmu);
}