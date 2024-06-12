#include <assert.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <math.h>

#include <model.h>
#include <core.h>


/*====================================================================*
 * BUCKET                                                             *
 *====================================================================*/
struct bucket
{
    int num_tasks;
    unsigned long int current_tasks_load;
    unsigned long int current_tasks_waiting;
};

static inline struct bucket* initialize_bucket()
{
    struct bucket *b = (struct bucket*) malloc(sizeof(struct bucket));
    b->num_tasks = 0;
    b->current_tasks_load = 0;
    b->current_tasks_waiting = 0;
    return b;
}

static inline void destroy_bucket(struct bucket* b)
{
    assert(b != NULL);
    free(b);
}

/*====================================================================*
 * MODEL                                                              *
 *====================================================================*/
struct model
{
    double **q_table;               /**< Model's q-table.                                                                                     */

    int num_cores;                  /**< Total number of cores in our simulation.                                                             */
    int core_capacity;              /**< Cores' capacity. Homogeneous simulation will be the same to all.                                     */
    int num_tasks;                  /**< Current total number of tasks.                                                                       */
    int num_intervals;              /**< How many intervals are we dividing our data.                                                         */
    int winsize;                    /**< Tasks' last WINSIZE cache sets acesses.                                                              */

    int num_states;                 /**< Total number of states in our model.                                                                 */
    int num_actions;                /**< Total number of actions. Actions = Which core to sched.                                              */

    double alpha;                   /**< Train rate.                                                                                          */
    double gamma;                   /**< Discout rate.                                                                                        */
    
    double epsilon;                 /**< Epsilon (E-greedy).                                                                                  */
    double eps_decay;               /**< Epsilon decrement per episode.                                                                       */
    double min_eps;                 /**< Minimal Epsilon value (preventing errors).                                                           */
    double waiting_coefficient;     /**< Adjustable variable. Related to the penalty of high waiting times in a bucket.                       */
    double placement_coefficient;   /**< Adjustable variable. Related to the penalty of assigning a task to a bucket at 3rd interval of load. */
    double balancement_coefficient; /**< Adjustable variable. Related to the penalty of high waiting times in a bucket.                       */


    struct bucket** buckets;        /**< Buckets in our simulation. Number of buckets = num_cores.                                            */

    bool trained;                   /**< If the model was already trained.                                                                    */
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
    m->epsilon = 0.5;
    m->eps_decay = 0.995;
    m->min_eps = 0.0;
    m->winsize = winsize;

    // Variable paramethers. Adjust them as best suit.
    m->waiting_coefficient = 2.0;
    m->placement_coefficient = 2.0;
    m->balancement_coefficient = 2.0;

    m->num_actions = num_cores;
    m->num_cores = num_cores;
    // At the moment, 3 intervals only. Preventing problems with state/action dimentions.
    m->num_intervals = 3;
    
    m->num_states = pow(m->num_intervals * m->num_intervals, num_cores);
    
    m->core_capacity = core_capacity;
    m->num_tasks = 0;

    m->buckets = (struct bucket**) malloc(sizeof(struct bucket*) * m->num_cores);
    for ( int i = 0; i < m->num_cores; i++ ) m->buckets[i] = initialize_bucket();

    m->q_table = (double**) malloc(sizeof(double*) * m->num_states);
    for ( int i = 0; i < m->num_states; i++ )
        m->q_table[i] = (double*) malloc(sizeof(double) * m->num_actions); 

    FILE *f = fopen(Q_TABLE_FILE, "rb");
    // If file doesn't exists, it implies that the model wasn't trained before. So we must train it.
    if ( f != NULL )
    {
        fclose(f);
        load_q_table(m, Q_TABLE_FILE);
        m->trained = true;
    }
    else 
    {
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
 * @brief Calculates how much the buckets are deviating from the mean (Normalized).
 * 
 * @param m Target model.
 * 
 * @returns Unbalancement penalty.
 */
static inline double calc_balance_penalty(struct model *m)
{
    /* Sanity check. */
    assert(m != NULL);
    
    double total_load = 0.0;
    for ( int i = 0; i < m->num_cores; i++ )
    {
        total_load += m->buckets[i]->current_tasks_load;
    }

    double avg_load = total_load / m->num_cores;
    double balance_penalty = 0.0;
    if ( total_load > 0 )
    {
        for ( int i = 0; i < m->num_cores; i++ )
        {
            balance_penalty += fabs(m->buckets[i]->current_tasks_load - avg_load) / avg_load;
        }
    }
    return balance_penalty;
}

/**
 * @brief Calculates how much conflicts happened in "core"'s cache's set.
 * 
 * @param m      Target model.
 * @param core   Target core (choosen at action).
 * @param bucket Target core's bucket.
 * @param ts     Task.
 * 
 * @returns Related penalty.
 */
static inline double calc_cache_penalty(struct model *m, struct core *core, struct queue *bucket, struct task *ts)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(core != NULL);
    assert(bucket != NULL);
    assert(ts != NULL);

    double cache_penalty = 0.0;
    // If task has processed atleast once.
    if ( task_work_processed(ts) > 0 )
    {
        int num_sets = core_cache_num_sets(core);
        for ( int i = 0; i < num_sets; i++ )
        {
            int total_accesses = core_cache_sets_accesses(core)[i];
            int set_conflicts = 0;
            for ( int j = 0; j < queue_size(bucket); j++ )
            {
                task_tt ts = queue_peek(bucket, j);
                bool has = task_accessed_set(ts, i, m->winsize);
                if ( has ) set_conflicts++;
            }
            if ( total_accesses > 0 )
            {
                cache_penalty += (double) set_conflicts / total_accesses;
            }
        }
    }
    return -cache_penalty;
}

/**
 * @brief Calculates action's reward. It's the sum of all hits (currently in our simulation) with the balancement penalty and the conflict penalty.
 * 
 * @param m       Target model.
 * @param tks     All tasks being scheduled.
 * @param cores   All cores.
 * @param buckets Core's buckets.
 * @param ts      Task that is being scheduled at current iteration.
 * @param index   Index to which bucket was choosen.
 * 
 * @returns Total reward.
 */
static inline double calc_reward(struct model *m, struct queue *tks, struct array *cores, struct array *buckets, struct task *ts, int index)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(tks != NULL);
    assert(cores != NULL);
    assert(buckets != NULL);
    assert(ts != NULL);

    double balance_penalty = calc_balance_penalty(m);
    double cache_penalty = calc_cache_penalty(m, array_get(cores, index), array_get(buckets, index), ts);
    double hit_reward = 0.0;

    for ( int i = 0; i < m->num_tasks; i++ )
    {
        hit_reward += task_hit(queue_peek(tks, i));
    }

    double total_reward = (m->balancement_coefficient * balance_penalty) + (m->placement_coefficient * cache_penalty) + hit_reward;
    return total_reward;
}

/**
 * @brief Gets the state index.
 * 
 * @param m     Target model. 
 * @param tasks All tasks to be scheduled.
 * 
 * @returns Current State.
 */
static inline int get_state_index(struct model *m, struct queue *tasks)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(tasks != NULL);

    int max_waiting = 0;
    for ( int i = 0; i < queue_size(tasks); i++ )
    {
        if ( task_waiting_time(queue_peek(tasks, i)) > max_waiting ) max_waiting = task_waiting_time(queue_peek(tasks, i));
    }

    int index = 0;
    int factor = 1;
    for ( int i = 0; i < m->num_cores; i++ )
    {
        int b_waiting_interval = (int) floor( (m->buckets[i]->current_tasks_waiting * m->num_intervals) / (max_waiting + 1) );
        int b_num_tasks_interval = (int) floor( (m->buckets[i]->num_tasks * m->num_intervals) / (m->core_capacity + 1) );
        
        if (b_waiting_interval >= m->num_intervals) b_waiting_interval = m->num_intervals - 1;
        if (b_num_tasks_interval >= m->num_intervals) b_num_tasks_interval = m->num_intervals - 1;

        index += (b_num_tasks_interval + b_waiting_interval * m->num_intervals) * factor;
        factor *= (m->num_intervals * m->num_intervals);
    }
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
 * @param m        Target model.
 * @param buckets  All buckets.
 * @param t        Task.
 * @param position Which bucket will be choosen.
 */
static inline void populate_bucket(struct model *m, struct array *buckets, struct task *t, int position)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(buckets != NULL);
    assert(t != NULL);
    assert(position >= 0);

    // Populating the model's buckets.
    m->buckets[position]->current_tasks_load += task_work_left(t);
    m->buckets[position]->current_tasks_waiting += task_waiting_time(t);
    m->buckets[position]->num_tasks++;

    // Populating the simulation's buckets.
    queue_insert(array_get(buckets, position), t);
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
        int state = get_state_index(m, tasks);
        int action = choose_action(m, state);
        task_tt task = queue_remove(tasks);
        populate_bucket(m, buckets, task, action);
        model_update_num_tasks(m, queue_size(tasks));

        int reward = calc_reward(m, tasks, cores, buckets, task, action);
        int next_state = get_state_index(m, tasks);

        update_q_table(m, state, action, reward, next_state);
    }
    if (m->epsilon > m->min_eps)
    {
        m->epsilon *= m->eps_decay;
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

    for ( int i = 0; i < m->num_states; i++ ) free(m->q_table[i]);
    for ( int i = 0; i < m->num_cores; i++ ) destroy_bucket(m->buckets[i]);
    free(m->buckets);
    free(m->q_table);
    free(m);
}
