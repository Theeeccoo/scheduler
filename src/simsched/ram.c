#include <assert.h>
#include <stdlib.h>

#include <mylib/util.h>

#include <ram.h>

/**
 * @brief RAM.
 */
struct RAM
{
    workload_tt w;                /**< Simulation's workload. Must be used ONLY when a frame is assigned to other task.    */
    unsigned long int next_frame; /**< Which is the next frame.                                                            */
    unsigned long int num_frames; /**< Total number of frames in Simulation's RAM. A frame has the same size as Task PAGE. */
    int* frame_assignment;        /**< Frame/Task assignment. Array index is the frame index.                              */
};

/**
 * @brief Initiates the Simulation's RAM instance.
 * 
 * @return RAM instance.
 */
RAM_tt RAM_init(struct workload *w)
{
    /* Sanity check. */
    assert(w != NULL);
    struct RAM *ram = smalloc(sizeof(struct RAM));
    ram->w = w;
    ram->num_frames = RAM_SIZE / PAGE_SIZE;
    ram->next_frame = ram->num_frames - 1;


    /**
     * For memory reasons, there is no "frame" struct. 
     * Each frame has its initial address (its index * PAGE_SIZE), and which task it's related to (found at frame_assigment)
     */
    ram->frame_assignment = smalloc(sizeof(int) * ram->num_frames);
    for ( unsigned long int i = 0; i < ram->num_frames; i++ ) ram->frame_assignment[i] = -1;

    return ram;
}

/**
 * @brief Returns the total number of frames in RAM.
 * 
 * @param ram Target RAM.
 * 
 * @returns Total number of frames in RAM.
 */
unsigned long int RAM_num_frames(const struct RAM *ram)
{
    /* Sanity check. */
    assert(ram != NULL);
    return (ram->num_frames);
}

/**
 * @brief Selects a next frame in a FIFO order. 
 * If frame was assigned to a task that is still at workload, we must invalid task's line.
 * Also, assigns frame to the new task (task_id). 
 * 
 * @param ram     Target RAM. 
 * @param task_id Task_id to be assigned to frame.
 * 
 * @returns Frame's id.
 */
unsigned long int RAM_next_frame(struct RAM *ram, int task_id)
{
    /* Sanity check. */
    assert(ram != NULL);
    assert(task_id >= 0);

    ram->next_frame = (ram->next_frame + 1) % ram->num_frames;
    int last_task = ram->frame_assignment[ram->next_frame];
    // Will only happen when a frame was assigned before.
    if ( last_task != -1 )
    {
        task_tt replaced_task = workload_find_task(ram->w, last_task);
        // If any task found.
        if ( replaced_task != NULL )
        {
            int index = task_find_pt_line_frame_id(replaced_task, ram->next_frame); 
            if ( index != -1 ) task_invalid_pt_line(replaced_task, index);
        }

    }

    // Assigning frame to task
    ram->frame_assignment[ram->next_frame] = task_id;
    return (ram->next_frame % ram->num_frames);
}

/**
 * @brief Destroys RAM instance.
 * 
 * @param ram Target RAM.
 */
void RAM_destroy(struct RAM *ram)
{
    /* Sanity check. */
    assert(ram != NULL);
    free(ram->frame_assignment);
    free(ram);
}
