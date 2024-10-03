#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

#include <model.h>
#include <core.h>
#include <mylib/map.h>


struct conflicts_finder
{
    int num_elements;    /**< Total number of unique elements (cache sets)                            */
    queue_tt elements;   /**< Which cache set was read.                                               */
    queue_tt elem_pos;   /**< Combination of which array + (10*iteration) that element was last seen. */
    queue_tt elem_count; /**< Total number of conflicts related to each cache set.                    */
};

static inline struct conflicts_finder* initialize_conflicts_finder()
{
    struct conflicts_finder *c_f = (struct conflicts_finder*) malloc(sizeof(struct conflicts_finder));
    c_f->num_elements = 0;
    c_f->elements = queue_create();
    c_f->elem_pos = queue_create();
    c_f->elem_count = queue_create();

    return (c_f);
}

static inline int conflicts_finder_has(struct conflicts_finder *c_f, int cache_set)
{
    /* Sanity Check. */
    assert(c_f != NULL);
    assert(cache_set >= 0);

    int found = -1;
    for ( int i = 0; i < c_f->num_elements; i++ )
    {
        int *set = (int*) queue_peek(c_f->elements, i);
        // If found, break the loop
        if ( *set == cache_set ) 
        {
            found = i;
            i = c_f->num_elements;
        }
    }
    return found;
}

static inline void destroy_conflicts_finder(struct conflicts_finder *c_f)
{
    /* Sanity check. */
    assert(c_f != NULL);
    queue_destroy(c_f->elements);
    queue_destroy(c_f->elem_pos);
    queue_destroy(c_f->elem_count);

    free(c_f);
}

/**
 * @brief Iterates through tasks' assigned to current bucket and analyze the ammount of conflicts iter-tasks that happens.
 * Since the idea is to minimize the conflicts between tasks, we musn't consider a conflict being from the same task.
 * see
 * @param bucket        Target bucket to be analyzed.
 * @param winsize       Ammount of accesses to be analyzed.
 * @param core_capacity Max number of tasks in a bucket.
 * 
 * @returns The total number of conflictant sets inter-tasks in target bucket.
 */
static inline int conflicts_finder_bucket(struct queue *bucket, int winsize, int core_capacity)
{
    /* Sanity Check. */
    assert(bucket != NULL);
    assert(winsize >= 0);
    assert(core_capacity >= 0);

    struct conflicts_finder *c_f = initialize_conflicts_finder();
    int num_conflicts = 0;
    bool calc = false;
    for ( int i = 0; i < winsize; i++ )
    {
        for ( int j = 0; j < queue_size(bucket); j++ )
        {
            task_tt ts = queue_peek(bucket, j);
            if ( task_work_processed(ts) == 0 ) 
                continue;
            calc = true;
            
            unsigned long int memptr = task_memptr(ts);
            int *task_accesses = task_lineacc(ts);
            int set = task_accesses[(memptr - winsize + i)];
            int position = conflicts_finder_has(c_f, set);
            int incr_value = 1;
            if ( position == -1 )
            {
                int index = j + (core_capacity * i);
                queue_insert(c_f->elements, &set);
                queue_insert(c_f->elem_pos, &index);
                queue_insert(c_f->elem_count, &incr_value);
            }
            else 
            {
                int *index = (int*) queue_peek(c_f->elem_pos, position);
                int which_task = *index % core_capacity;
                // If not from the same task,increase counter
                if ( which_task != j )
                {
                    int *prev_value = (int*) queue_peek(c_f->elem_count, position) + incr_value;
                    queue_change_elem(c_f->elem_count, position, prev_value);
                }
                int new_index = j + (core_capacity * i);
                queue_change_elem(c_f->elem_pos, position, &new_index);
            }

        }   
    }

    if (calc)
    {
        // We musn't consider the first set appearence as a conflict, that's why we remove 1 count.
        for ( int i = 0; i < c_f->num_elements; i++ )
        {
            int *conflicts = (int*) queue_peek(c_f->elem_count, i);
            num_conflicts += *conflicts - 1;
        }
    }   
    destroy_conflicts_finder(c_f);
    return num_conflicts;
}

/*====================================================================*
 * BUCKET                                                             *
 *====================================================================*/
struct bucket
{
    int num_tasks;
    unsigned long int current_tasks_load;    /**< Current (bucket) load.                */
    unsigned long int current_tasks_waiting; /**< Current (bucket) accumulated waiting. */
    double current_conflicts;                /**< Percentage of conflicts in a bucket.  */
};

static inline struct bucket* initialize_bucket()
{
    struct bucket *b = (struct bucket*) malloc(sizeof(struct bucket));
    b->num_tasks = 0;
    b->current_tasks_load = 0;
    b->current_tasks_waiting = 0;
    b->current_conflicts = 0.0;
    return b;
}


static inline void destroy_bucket(struct bucket *b)
{
    /* Sanity check. */
    assert(b != NULL);
    free(b);
}

/*====================================================================*
 * MODEL                                                              *
 *====================================================================*/
struct model
{
    double **q_table;        /**< Model's q-table.                                                                                     */

    int num_cores;           /**< Total number of cores in our simulation.                                                             */
    int core_capacity;       /**< Cores' capacity. Homogeneous simulation will be the same to all.                                     */
    int num_tasks;           /**< Current total number of tasks.                                                                       */
    int num_intervals;       /**< How many intervals are we dividing our data.                                                         */
    int winsize;             /**< Tasks' last WINSIZE cache sets acesses.                                                              */

    int num_states;          /**< Total number of states in our model.                                                                 */
    int num_actions;         /**< Total number of actions. Actions = Which core to sched.                                              */

    double alpha;            /**< Train rate.                                                                                          */
    double gamma;            /**< Discout rate.                                                                                        */
    double reward_penalty;   /**< Penalizes whenever the the model made an action that brought more conflicts than there were before.  */
    
    double epsilon;          /**< Epsilon (E-greedy).                                                                                  */
    double eps_decay;        /**< Epsilon decrement per episode.                                                                       */
    double min_eps;          /**< Minimal Epsilon value (preventing errors).                                                           */
    struct bucket** buckets; /**< Buckets in our simulation. Number of buckets = num_cores.                                            */

    bool trained;            /**< If the model was already trained.                                                                    */
};


static inline void save_q_table(struct model *m, const char* filename)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(m->q_table != NULL);
    assert(filename != NULL);

    FILE *f = fopen(filename, "wb");
    assert(f != NULL);

    for (int i = 0; i < m->num_states; i++)
    {
        fwrite(m->q_table[i], sizeof(double), m->num_actions, f);
    }

    fclose(f);
}

static inline void load_q_table(struct model *m, const char* filename)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(m->q_table != NULL);
    assert(filename != NULL);

    FILE *f = fopen(filename, "rb");
    assert(f != NULL);

    for (int i = 0; i < m->num_states; i++)
    {
        int read_elements = fread(m->q_table[i], sizeof(double), m->num_actions, f);
        assert(read_elements == m->num_actions);
    }

    fclose(f);
}

static inline void save_eps(struct model *m, const char* filename)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(filename != NULL);

    FILE *f = fopen(filename, "wb");
    assert(f != NULL);
    fwrite(&m->epsilon, sizeof(double), 1, f);
    fclose(f);
}

static inline void load_eps(struct model *m, const char* filename)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(filename != NULL);
    FILE *f = fopen(filename, "rb");
    assert(f != NULL);

    assert(fread(&m->epsilon, sizeof(double), 1, f) == 1);
    fclose(f);
}

/** 
 * @brief Initializes a new instance of a Q-Learning based Reinforcement Learning model.
 * 
 * @param num_cores     Total number of cores in our simulation.
 * @param core_capacity Cores' capacity.
 * @param winsize       Current winsize.
 * 
 * @returns New Reinforcement Learning model instance.
 */
model_tt model_create(int num_cores, int core_capacity, int winsize)
{
    struct model *m;
    /* Sanity check. */
    assert(num_cores > 0);
    assert(core_capacity > 0);

    m = (struct model*) malloc(sizeof(struct model));
    m->alpha = 0.5;
    m->gamma = 0.9;
    m->eps_decay = 0.995;
    m->min_eps = 0.0;
    m->reward_penalty = 0.2;
    m->winsize = winsize;

    m->num_actions = num_cores;
    m->num_cores = num_cores;
    // At the moment, 3 intervals only (Low, Medium, High). Preventing problems with state/action dimentions.
    m->num_intervals = 3;
    
    m->num_states = pow((m->num_intervals), num_cores) + m->num_intervals;

    m->core_capacity = core_capacity;
    m->num_tasks = 0;

    m->buckets = (struct bucket**) malloc(sizeof(struct bucket*) * m->num_cores);
    for ( int i = 0; i < m->num_cores; i++ ) m->buckets[i] = initialize_bucket();

    m->q_table = (double**) malloc(sizeof(double*) * m->num_states);
    for ( int i = 0; i < m->num_states; i++ )
        m->q_table[i] = (double*) malloc(sizeof(double) * m->num_actions); 

    // If file doesn't exists, it implies that the model wasn't trained before. So we must train it.
    FILE *f = fopen(Q_TABLE_FILE, "rb");
    if ( f != NULL )
    {
        fclose(f);  
        load_q_table(m, Q_TABLE_FILE);
        load_eps(m, Q_EPS_FILE);
        m->trained = true;
    }
    else 
    {
        m->epsilon = 0.5;
        for ( int i = 0; i < m->num_states; i++ )
        {
            for ( int j = 0; j < m->num_actions; j++ )
            {
                m->q_table[i][j] = 0.0;
            }
        }
        m->trained = false;
    }
    return (m);    
}

/** 
 * @brief Updates the number of current tasks that will be scheduled by our model.
 * 
 * @param m         Target model.
 * @param num_tasks Current number of waiting tasks. Minimum = BATCHSIZE, Maximum = SUM(cores.capacity)
 */
void model_update_num_tasks(struct model *m, int num_tasks)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(num_tasks >= 0);
    m->num_tasks = num_tasks;
}

/**
 * @brief Updates the Q-Table.
 * 
 * @param m          Target model.
 * @param state      Current State.
 * @param action     Action taken.
 * @param reward     Reward obtained.
 * @param next_state Next state.
 */
static inline void update_q_table(struct model *m, int state, int action, int reward, int next_state)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(state >= 0);
    assert(next_state >= 0);
    assert(action >= 0);


    double old_value = m->q_table[state][action];
    double next_max = m->q_table[next_state][0];
    for ( int i = 1; i < m->num_actions; i++ )
    {
        if ( m->q_table[next_state][i] > next_max )
        {
            next_max = m->q_table[next_state][i];
        } 
    }

    m->q_table[state][action] = old_value + m->alpha * (reward + m->gamma * next_max - old_value);
}

/**
 * @brief Calculates the current amount of conflicts in a specified bucket.
 * 
 * @param bucket  Target bucket.
 * @param core    Core related to target bucket.
 * @param winsize Simulation's winsize.
 * 
 * @returns Total conflicts
 */
static inline double calc_perc_conflict(struct queue *bucket, struct core *core, int winsize)
{
    assert(core != NULL);
    assert(winsize >= 0);
    double conflict_perc = 0.0;

    map_tt m_a = core_cache_sets_accesses(core);
    int number_accesses = 0;
    for ( int i = 0; i < map_size(m_a); i++ )
    {
        struct map_return *m_r = (struct map_return*) map_peek(m_a, i);
        number_accesses += m_r->num_obj;
    }

    if ( (queue_size(bucket) > 1) && (number_accesses > 0) )
        conflict_perc = (double) conflicts_finder_bucket(bucket, winsize, core_capacity(core)) / number_accesses;

    return conflict_perc;
}

/**
 * @brief Calculates action's Penalty/Reward. It's measured as the total amount of new conflicts compared to last iteration total number of conflicts.
 * The desirable amount is closer to 0, second best is negative (which implies that, after new task insertion, the amount of conflicts were reduced).
 * The worse case is when the amount is increased, whenever this case happens, the model will receive a negative reward (penalty). 
 * 
 * @param m               Target model.
 * @param conflict_before Total ammount of conflicts before last bucket insertion.
 * @param index           Chosen bucket.
 * 
 * @returns Total reward.
 */
static inline double calc_reward(struct model *m, double conflict_before, int index)
{
    /* Sanity check. */
    assert(m != NULL);
    double reward = 0.0;
    double subtraction = m->buckets[index]->current_conflicts - conflict_before;
    reward = ( (1 / 1 + (fabs(subtraction))) - (m->reward_penalty * fmax(0, subtraction)) );
    return reward;
}

// This is, currently, not being used. Leaving here 'cus might be useful in a near future.
static inline double *calc_variance_lims(int num_acesses, int total_sets)
{
    /* Sanity check. */
    assert(num_acesses > 0);
    double *lims = smalloc(sizeof(double) * 2);
    double mean = (double) num_acesses / total_sets;
    double variance = 0.0;
    // Lower lime =          First NUM_ACCESSES are 1,                 rest is 0.
    variance = ( ( (pow((1 - mean), 2)) * num_acesses ) + ( (pow(mean, 2)) * (total_sets - num_acesses) ) );
    variance /= total_sets;
    lims[0] = variance;

    // Upper lim = First position is NUM_ACCESSES,               rest is 0
    variance = ( ( (pow(num_acesses - mean, 2)) ) + ( (pow(-mean, 2)) * (total_sets - 1) ));
    variance /= total_sets;
    lims[1] = variance;

    return lims;
}

/**
 * @brief Gets the state index.
 * 
 * @param m           Target model. 
 * @param task        Current task to be scheduled.
 * @param cores       All cores in our simulation.
 * 
 * @returns Current State.
 */
static inline int get_state_index(struct model *m, struct task *task, struct array *cores)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(task != NULL);
    assert(cores != NULL);

    double task_tl_perc = task_hotness(task, m->winsize);

    int index = 0;
    for ( int i = 0; i < m->num_cores; i++ )
    {
        int b_conflict_interval = (int) round( (m->buckets[i]->current_conflicts) / (m->num_intervals) );
        index += pow(b_conflict_interval, i);
    }

    if ( task_tl_perc < 0.33 )
        index += 0;
    else if ( task_tl_perc < 0.66 )
        index += 1;
    else 
        index += 2;

    return index;
}

/**
 * @brief Chooses which action is the best. Using E-Greedy (with E decrease per iteration).
 * 
 * @param m     Target model.
 * @param state Current state.
 * 
 * @returns Which action (bucket) task will be mapped to.
 */
static inline int choose_action(struct model *m, int state)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(state >= 0);

    double random_value = (double) rand() / RAND_MAX;
    if ( random_value < m->epsilon )
    {
        return rand () % m->num_actions;
    } else 
    {
        double best_action = m->q_table[state][0];
        int max_index = 0;
        for ( int i = 1; i < m->num_actions; i++ )
        {
            if ( m->q_table[state][i] > best_action )
            {
                best_action = m->q_table[state][i];
                max_index = i;
            }
        }
        return max_index;
    }
}

/**
 * @brief Populates buckets with task's attributes.
 * 
 * @param m       Target model.
 * @param buckets All buckets.
 * @param core    Core related to bucket
 * @param t       Task.
 * @param index   Chosen bucket.
 */
static inline void populate_bucket(struct model *m, struct array *buckets, struct core *core, struct task *t, int index)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(buckets != NULL);
    assert(t != NULL);
    assert(index >= 0);

    // Populating the simulation's buckets.
    queue_insert(array_get(buckets, index), t);

    // Populating the model's buckets.
    m->buckets[index]->current_tasks_load += task_work_left(t);
    m->buckets[index]->current_tasks_waiting += task_waiting_time(t);
    m->buckets[index]->num_tasks++;
    m->buckets[index]->current_conflicts = calc_perc_conflict(array_get(buckets, index), core, m->winsize);
}

/**
 * @brief Cleans buckets.
 * 
 * @param m Target model.
 */
static inline void clean_buckets(struct model *m)
{
    assert(m != NULL);

    for ( int i = 0; i < m->num_cores; i++ )
    {
        m->buckets[i]->num_tasks = 0;
        m->buckets[i]->current_tasks_load = 0;
        m->buckets[i]->current_tasks_waiting = 0;
        m->buckets[i]->current_conflicts = 0.0;
    }
}

/**
 * @brief Trains one episode of our RL Model. The idea is to find the best set for each bucket (and, consequently, for each core).
 *        The filled buckets will be sent to be the input of the desired SCHEDULER.
 * 
 * @param m       Target Model.
 * @param cores   Simulation's cores.
 * @param buckets Target array of buckets.
 * @param tasks   Target tasks waiting to be scheduled.
*/
void model_train(struct model *m, struct array *cores, struct array *buckets, struct queue *tasks)   
{
    /* Sanity check. */
    assert(m != NULL);
    assert(cores != NULL);
    assert(buckets != NULL);
    assert(tasks != NULL);

    for ( int t = 0; queue_size(tasks) > 0; t++ )
    {
        task_tt task = queue_remove(tasks); 
        int state = get_state_index(m, task, cores);
        int action = choose_action(m, state);
        
        double conflicts_before = m->buckets[action]->current_conflicts;
        populate_bucket(m, buckets, array_get(cores, action), task, action);
        model_update_num_tasks(m, queue_size(tasks));

        int reward = calc_reward(m, conflicts_before, action);
        int next_state = 0;
        if ( queue_size(tasks) != 0)
            next_state = get_state_index(m, queue_peek(tasks, 0), cores);
        else 
            next_state = get_state_index(m, task, cores);

        update_q_table(m, state, action, reward, next_state);
    }
    clean_buckets(m);
}

/**
 * @brief Destroys an existing Reinforcement Learning model.
 * 
 * @param m Desired instance.
 */
void model_destroy(struct model *m)
{ 
    /* Sanity check. */
    assert(m != NULL);
    save_q_table(m, Q_TABLE_FILE);
    if (m->epsilon > m->min_eps)
    {
        m->epsilon *= m->eps_decay;
    }
    save_eps(m, Q_EPS_FILE);

    for ( int i = 0; i < m->num_states; i++ ) free(m->q_table[i]);
    for ( int i = 0; i < m->num_cores; i++ ) destroy_bucket(m->buckets[i]);
    free(m->buckets);
    free(m->q_table);
    free(m);
}
