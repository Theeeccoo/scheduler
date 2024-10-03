#include <assert.h>
#include <stdlib.h>
#include "mem.h"

#include <mylib/util.h>
#include <mylib/map.h>

/*====================================================================*
 * MAP NODE                                                           *
 *====================================================================*/
/**
 * @brief Map node.
 */
struct mnode
{
    void *obj;          /**< Underlying object.          */
    int num_obj;        /**< Current number of this obj. */
    struct mnode *next; /**< Next object in the map.     */
};

/**
 * @brief Creates a map node.
 * 
 * @param obj Object to store in the node.
 * 
 * @returns A map node.
 */
static inline struct mnode *mnode_create(void *obj)
{
    struct mnode *node;
    node = smalloc(sizeof(struct mnode));

    /* Initialize map node. */
    node->obj = obj;
    node->num_obj = 1;
    node->next = NULL;

    return(node);
}

/**
 * @brief Destroys a map node.
 * 
 * @param node Target map node.
 */
static inline void mnode_destroy(struct mnode *node)
{
    /* Sanity check. */
    assert(node != NULL);

    free(node);
}

/*====================================================================*
 * MAP RETURN                                                         *
 *====================================================================*/
static inline struct map_return *map_return_create(struct mnode *node)
{
    /* Sanity check. */
    assert(node != NULL);
    struct map_return *m_r = smalloc(sizeof(struct map_return));
    m_r->obj = node->obj;
    m_r->num_obj = node->num_obj;

    return (m_r);
}

/*====================================================================*
 * MAP                                                                *
 *====================================================================*/

/**
 * @brief Map.
 */
struct map
{
    int size;                     /**< Current map size.          */
    struct mnode head;            /**< Dummy head node.           */
    struct mnode *tail;           /**< Tail node.                 */
    int (*compare)(void*, void*); /**< Function to compare nodes. */
};

/**
 * @brief Creates a map.
 * 
 * @param compare Function to compare nodes.
 * 
 * @returns A map struct.
 */
struct map *map_create(int (*compare)(void*, void*))
{
    struct map *m;

    m = smalloc(sizeof(struct map));
    m->size = 0;
    m->head.next = NULL;
    m->tail = &m->head;
    m->compare = compare;

    return (m);
}

/**
 * @brief Returns the size of a map.
 *
 * @param m Target map.
 *
 * @returns The current size of the target map,
 */
int map_size(const struct map *m)
{
    /* Sanity check. */
    assert(m != NULL);

    return (m->size);
}

/**
 * @brief Asserts if a map is empty.
 * 
 * @param m Target map.
 * 
 * @returns True if the target map is empty and false otherwise.
 */
bool map_empty(const struct map *m)
{
    /* Sanity check. */
    assert(m != NULL);

    return (map_size(m) == 0);
}

static inline struct mnode *map_search_node(struct map *m, int index)
{
    /* Sanity Check. */
    assert(m != NULL);
    assert(index >= 0);
    assert(index <= m->size);

    /* Get object. */
    struct mnode *node = m->head.next;
    for ( int i = 0; i < index; i++ )
        node = node->next;

    return (node);
}

/**
 * @brief Inserts a new object in map's queue only if it already doens't exists. If it exists, increase obj's number of appearances.
 * 
 * @param m   Target map.
 * @param obj Target object.
 */
void map_insert(struct map *m, void *obj)
{
    /* Sanity check. */
    assert(m != NULL);

    int obj_index = -1;
    for ( int i = 0; i < map_size(m); i++ )
    {
        struct mnode *node = map_search_node(m, i);
        if ( m->compare(obj, node->obj) ) 
        {
            node->num_obj++;
            obj_index = i;
            i = map_size(m);
        }
    }


    if ( obj_index == -1 )
    {
        struct mnode *node = mnode_create(obj);
        m->tail->next = node;
        m->tail = node;
        m->size++;
    }
}

/**
 * @brief Removes an object from map.
 * 
 * @param m Target map.
 * 
 * @returns The object in the front of the map's queue.
 */
struct map_return *map_remove(struct map *m)
{
    /* Sanity check. */
    assert(m != NULL);
    assert(m->size != 0);
    struct mnode *node;

    node = m->head.next;
    m->head.next = node->next;
    m->size--;

    if ( m->size == 0 )
        m->tail = &m->head;
    
    struct map_return *obj = map_return_create(node);
    mnode_destroy(node);

    return (obj);
}

/**
 * @brief Returns the nth object from a map.
 * 
 * @param m     Target map.
 * @param index Object index.
 * 
 * @returns The nth object in map.
 */
struct map_return *map_peek(struct map *m, int index)
{
    /* Sanity Check. */
    assert(m != NULL);
    assert(index >= 0);
    assert(index <= m->size);

    /* Get object. */
    struct mnode *node = m->head.next;
    for ( int i = 0; i < index; i++ )
        node = node->next;
    
    struct map_return *obj = map_return_create(node);
    node = NULL;
    free(node);

    return (obj);
}

/**
 * @brief Destroys a map.
 * 
 * @param m Target map.
 */
void map_destroy(struct map *m)
{
    /* Sanity check. */
    assert(m != NULL);

    while (!map_empty(m))
        map_remove(m);
    free(m);
}

/**
 * @brief Converts two objects into ulong int and check if they are equals
 * 
 * @param obj1 First object.
 * @param obj2 Second object.
 * 
 * @returns 1 if values are equals. 0 otherwise.
 */
int map_compare_ulong_int(void *obj1, void *obj2)
{
    assert(obj1 != NULL);
	assert(obj2 != NULL);

	unsigned long int *a = (unsigned long int*) obj1;
	unsigned long int *b = (unsigned long int*) obj2;
    
	return (a == b);
}

/**
 * @brief Converts two objects into integers and check if they are equals
 * 
 * @param obj1 First object.
 * @param obj2 Second object.
 * 
 * @returns 1 if values are equals. 0 otherwise.
 */
int map_compare_int(void *obj1, void *obj2)
{
	assert(obj1 != NULL);
	assert(obj2 != NULL);

	int *a = (int*) obj1;
	int *b = (int*) obj2;

	return (a == b);
}