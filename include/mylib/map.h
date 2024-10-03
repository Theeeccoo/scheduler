#ifndef MAP_H_
#define MAP_H_

    #include <stdbool.h>

    /**
     * @brief Map's return struct.
     */
    struct map_return
    {
        void *obj;   /**< Underlying object. */
        int num_obj; /**< Current number of this obj. in map. */
    };

    /**
     * @brief Opaque pointer to a map.
     */
    typedef struct map * map_tt;

    /**
     * @brief Constant pointer to a map.
     */
    typedef const struct map * const_map_tt;

    /**
     * @name Operations on Map
     */
    extern map_tt map_create(int(*compare)(void*,void*));
    extern void map_destroy(map_tt);
    extern int map_size(const_map_tt);
    extern bool map_empty(const_map_tt);

    extern void map_insert(map_tt, void*);
    extern struct map_return *map_remove(map_tt);
    extern struct map_return *map_peek(map_tt, int);

    extern int map_compare_int(void*, void*);
    extern int map_compare_ulong_int(void*, void*);

#endif /* MAP_H_ */