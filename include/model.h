#ifndef MODEL_H_
#define MODEL_H_

    #include <mylib/array.h>
    #include <mylib/queue.h>

    /**
	 * @brief Model definitions.
	 */
	/**@{*/
    #define Q_TABLE_FILE "q_table.dat" /**< Q-Table file. */ 
	/**@}*/

    /**
     * @brief Opaque pointer to the Reinforcement Learning Model
     */
    typedef struct model * model_tt;

    /**
     * @brief Constant opaque pointer to the Reinforcement Learning Model
     */
    typedef const struct model * const_model_tt;

    /**
     * @name Operations on Model
     */
    /**@{*/
    extern model_tt model_create(int, int, int);
    extern void model_train(model_tt, array_tt, array_tt, queue_tt);
    extern void model_sched(model_tt, array_tt, queue_tt);
    extern void model_update_num_tasks(model_tt, int);
    extern void model_destroy(model_tt);
    /**@}*/


#endif /* MODEL_H_ */