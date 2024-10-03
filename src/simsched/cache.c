#include <assert.h>
#include <stdlib.h>

#include <cache.h>
#include <mylib/util.h>
#include <mylib/map.h>

/*====================================================================*
 * BLOCK                                                              *
 *====================================================================*/

/**
 * @brief Cache way's block.
 */
struct block
{
    bool populated;   /**< Block populated atleast once? */
    int num_of_words; /**< Number of words in block.     */
    int initial_addr; /**< Block's initial word address. */
    int final_addr;   /**< Block's final word address.   */

};

/**
 * @brief Creates a new instace of cache_ways's block.
 * 
 * @returns Instace of cache_way's block.
 */
static inline struct block* block_create()
{
    struct block *b = smalloc(sizeof(struct block));
    b->num_of_words = BLOCK_SIZE / WORD_SIZE;
    b->initial_addr = -1;
    b->final_addr = -1;
    b->populated = false;

    return b;
}

/**
 * @brief Returns if a cache_way's block was already popoulated, atleast once. 
 * 
 * @param b Target cache_way's block.
 * 
 * @returns True if cache_way's block was already popoulated. False otherwise.
 */
static inline bool block_was_populated(const struct block *b)
{
    /* Sanity check. */
    assert(b != NULL);
    return (b->populated);
}

/**
 * @brief Populates a cache_way's block with an offset. Each cache_way's block has [num_of_words = (BLOCK_SIZE / WORD_SIZE)] words.
 * Initial address is calculated as: (offset/num_of_words) * num_of_words. Final address is: initial + (num_of_words).
 * 
 * @param b      Target cache_way's block.
 * @param offset Memory address's offset.
 */
static inline void block_set_limits(struct block *b, int offset)
{
    /* Sanity check. */
    assert(b != NULL);
    b->populated = true;

    b->initial_addr = ( (int) (offset/b->num_of_words) ) * b->num_of_words;
    b->final_addr = b->initial_addr + b->num_of_words;
}

/**
 * @brief Checks if specified offset is in block's limits.
 * 
 * @param b      Target cache_way's block.
 * @param offset Memory address's offset.
 * 
 * @returns True if offset is in block's limits. False otherwise.
 */
static inline bool block_check_offset(const struct block *b, int offset)
{
    /* Sanity check. */
    assert(b != NULL);
    if ( !b->populated ) return false;
    return (b->initial_addr <= offset && offset < b->final_addr);
}

/**
 * @brief Destroys a instace of cache_ways's block.
 * 
 * @param b Target block.
 */
static inline void block_destroy(struct block *b)
{
    /* Sanity check. */
    assert(b != NULL);
    free(b);
}



/*====================================================================*
 * CACHE WAY                                                          *
 *====================================================================*/

/**
 * @brief Cache way
 */
struct cache_way
{
    bool populated;        /**< Way populated atleas once? */
    unsigned long int tag; /**< Way's tag.                 */
    int num_blocks;        /**< Way's number of blocks.    */
    struct block **blocks; /**< Way's blocks.              */
    int next_block;        /**< Next block to be replaced. */
};

/**
 * @brief Creates a new instance of a cache_way.
 * 
 * @param num_blocks Way's number of blocks.
 * 
 * @returns New instance of cache_way
 */
static inline struct cache_way* cache_way_create(int num_blocks)
{
    /* Sanity check. */
    assert(num_blocks > 0); 
    struct cache_way *cw = smalloc(sizeof(struct cache_way));
    cw->tag = 0;
    cw->num_blocks = num_blocks;
    cw->populated = false;
    cw->next_block = 0;
    cw->blocks = smalloc(sizeof(struct block*) * cw->num_blocks);
    
    for ( int i = 0; i < cw->num_blocks; i++ )
        cw->blocks[i] = block_create();
    
    return cw;
}

/**
 * @brief Returns cache_way's tag.
 * 
 * @param cw Target cache_way.
 * 
 * @returns Cache_way's tag.
 */
static inline unsigned long int cache_way_tag(const struct cache_way *cw)
{
    /* Sanity check. */
    assert(cw != NULL);
    return (cw->tag);
}

/**
 * @brief Returns cache_way's total number of blocks.
 * 
 * @param cw Target cache_way.
 * 
 * @returns Cache_way's total number of blocks.
 */
static inline int cache_way_num_blocks(const struct cache_way *cw)
{
    /* Sanity check. */
    assert(cw != NULL);
    return (cw->num_blocks);
}

/**
 * @brief Sets cache_way's tag.
 * 
 * @param cw  Target cache_way.
 * @param tag Desired tag.
 */
static inline void cache_way_set_tag(struct cache_way *cw, unsigned long int tag)
{
    /* Sanity check. */
    assert(cw != NULL);
    cw->tag = tag;
}

/**
 * @brief Returns if a cache_way was already popoulated, atleast once. 
 * 
 * @param cw Target cache_way.
 * 
 * @returns True if cache_way was already popoulated. False otherwise.
 */
static inline bool cache_way_was_populated(const struct cache_way *cw)
{
    /* Sanity check. */
    assert(cw != NULL);
    return (cw->populated);
}

/**
 * @brief Replace an old block with a new one. The new one is populated with the new offset.
 * 
 * @param cw     Target cache_way.
 * @param index  Block's index to be replaced.
 * @param offset Memory address's offset. 
 */
static inline void cache_way_replace_block(struct cache_way *cw, int index, int offset)
{
    /* Sanity check. */
    assert(cw != NULL);
    assert(index < cw->num_blocks);
    assert(offset >= 0);

    cw->populated = true;
    block_set_limits(cw->blocks[index], offset);
    cw->next_block = (cw->next_block + 1) % cw->num_blocks;
}

/** 
 * @brief Checks if a specified offset access a valid word in any way's block.
 * 
 * @param cw     Target cache_way.
 * @param offset Memory address's offset.
 * 
 * @returns True if offset is valid. False otherwise.
 */
static inline bool cache_way_block_has_word(const struct cache_way *cw, int offset)
{   
    /* Sanity check. */
    assert(cw != NULL);
    assert(offset >= 0);

    if ( !cw->populated ) return false;

    for ( int i = 0; i < cw->num_blocks; i++ )
    {
        if ( !block_was_populated(cw->blocks[i]) ) continue;
        if ( block_check_offset(cw->blocks[i], offset) ) return true;
    }
    return false;
}

/**
 * @brief Destroys a instance of cache_way.
 * 
 * @param cw Target cache_way.
 */
static inline void cache_way_destroy(struct cache_way *cw)
{   
    /* Sanity check. */
    assert(cw != NULL);
    for ( int i = 0; i < cw->num_blocks; i++ )
        block_destroy(cw->blocks[i]);
    free(cw);
}


/*====================================================================*
 * CACHE SET                                                          *
 *====================================================================*/

struct cache_set
{
    int index;               /**< Set's index.              */
    int num_ways;            /**< Set's number of ways.     */
    struct cache_way **ways; /**< Set's ways.               */
    int next_way;            /**< Next wayt to be replaced. */
};

/**
 * @brief Creates a new instance of a cache_set.
 * 
 * @param index      Set index.
 * @param num_ways   Number of ways in a cache set.
 * @param num_blocks Number of blocks in a cache way.
 * 
 * @returns New instance of a cache_set.
 */
static inline struct cache_set* cache_set_create(int index, int num_ways, int num_blocks)
{
    /* Sanity check. */
    assert(index >= 0);
    assert(num_ways > 0);

    struct cache_set *cs = smalloc(sizeof(struct cache_set));
    cs->index = index;
    cs->num_ways = num_ways;
    cs->next_way = 0;
    cs->ways = smalloc(sizeof(struct cache_way*) * num_ways);

    for ( int i = 0; i < cs->num_ways; i++ )
        cs->ways[i] = cache_way_create(num_blocks);

    return cs;
}

/**
 * @brief Searchs for a way that has the same tag as "tag". 
 * 
 * @param cs  Target cache_set.
 * @param tag Desired tag.
 * 
 * @returns If found, returns cache_way index. -1, otherwise.
 */
static inline int cache_set_find_way(const struct cache_set *cs, unsigned long int tag)
{
    /* Sanity check. */
    assert(cs != NULL);

    for ( int i = 0; i < cs->num_ways; i++ )
    {
        if ( !cache_way_was_populated(cs->ways[i]) ) continue;
        if ( tag == cache_way_tag(cs->ways[i]) ) return i;
    }
    return -1;
}

/**
 * @brief Replace an old way with a new one. The new one is populated with
 * the new tag and all blocks are populated with different offsets.
 * 
 * @param cw     Target cache_way.
 * @param index  Way's index to be replaced.
 * @param tag    New tag.
 * @param offset Memory address's offset. 
 */
static inline void cache_set_replace_way(struct cache_set *cs, int index, unsigned long int tag, int offset)
{
    /* Sanity check. */
    assert(cs != NULL);
    assert(index < cs->num_ways);
    assert(offset >= 0);

    int num_blocks = cache_way_num_blocks(cs->ways[index]);
    int num_of_words = BLOCK_SIZE / WORD_SIZE;
    cache_way_set_tag(cs->ways[index], tag);
    for ( int i = 0; i < num_blocks; i++ )
        cache_way_replace_block(cs->ways[index], i, offset + (num_of_words * i) );
    cs->next_way = (cs->next_way + 1) % cs->num_ways;
}

/**
 * @brief Destroys a cache_set instance.
 * 
 * @param ce Target cache_set.
 */
static inline void cache_set_destroy(struct cache_set *cs)
{
    /* Sanity check. */
    assert(cs != NULL);
    for ( int i = 0; i < cs->num_ways; i++ )
        cache_way_destroy(cs->ways[i]);
    free(cs);
}



/*====================================================================*
 * CACHE                                                              *
 *====================================================================*/

/**
 * @brief Core's cache.
 */
struct cache
{
    int  num_sets;           /**< Total number of cache sets.            */
    int  num_ways;           /**< Total number of cache ways.            */
    int  num_blocks;         /**< Total number of blocks in cache_way.   */
    struct cache_set **sets; /**< Cache sets.                            */
    map_tt sets_accesses;    /**< Number of tasks that acessed each set. */ 
    map_tt sets_conflicts;   /**< Total number of conflicts in each set. */
};

/**
 * @brief Creates a new instance of cache.
 * 
 * @param num_sets   Total number of sets in cache   
 * @param num_ways   Total number of ways in cache_set
 * @param num_blocks Total number of blocks in cache_way
 * 
 * @returns Cache instance.
 */
cache_tt cache_create(int num_sets, int num_ways, int num_blocks)
{
    /* Sanity check. */
    assert(num_sets > 0);
    assert(num_ways > 0);
    assert(num_blocks > 0);

    struct cache *ce = smalloc(sizeof(struct cache));
    ce->num_sets = num_sets;
    ce->num_ways = num_ways;
    ce->num_blocks = num_blocks;
    ce->sets = smalloc(sizeof(struct cache_set*) * ce->num_sets);
    ce->sets_accesses = map_create(map_compare_int);
    ce->sets_conflicts = map_create(map_compare_int);

    for ( int i = 0; i < ce->num_sets; i++ )
        ce->sets[i] = cache_set_create(i, ce->num_ways, ce->num_blocks);        
    
    return ce;
}

/**
 * @brief Checks a specified address's index (cache set). 
 * Then, check if any way has the same address' tag.
 * If found, check if any block has the desired word.
 * 
 * @param ce  Target cache instance.
 * @param mem Target memory address.
 * 
 * @returns True if cache hit. False otherwise.
 */
bool cache_check_addr(const struct cache *ce, struct mem *mem)
{
    /* Sanity check. */
    assert(ce != NULL);
    assert(mem != NULL);
    bool found = false;

    /* Which set it was mapped to. */
    unsigned long int tag = mem_physical_addr(mem) * PAGE_SIZE;
    int mem_offset = mem_addr_offset(mem);
    int cache_set = tag % ce->num_sets;
    struct cache_set *cs = ce->sets[cache_set];

    /* Searching */
    for ( int i = 0; i < ce->num_ways; i++ )
    {
        int index = cache_set_find_way(cs, tag);
        if ( index != -1 )
        {
            struct cache_way *cw = cs->ways[index]; 
            // Checking if cache_way's blocks has the desired word. If not, we will consider it as a cache miss aswell.
            if ( cache_way_block_has_word(cw, mem_offset) ) found = true;
        } else found = false;
    }

    return found;
}

/**
 * @brief Replacement is necessary only when a cache miss occurred. 
 * We must identify if it ocurred because of "way" not found or because "block" not found, and replace the "guilty" one.
 * 
 * @param ce  Target cache instance.
 * @param mem Target memory address to replace.
 */
void cache_replace(struct cache *ce, struct mem *mem)
{
    /* Sanity check. */
    assert(ce != NULL);
    assert(mem != NULL);

    /* Which set it was mapped to. */
    unsigned long int tag = mem_physical_addr(mem) * PAGE_SIZE;
    int mem_offset = mem_addr_offset(mem);
    int cache_set = tag % ce->num_sets;
    struct cache_set *cs = ce->sets[cache_set];

    for ( int i = 0; i < ce->num_ways; i++ )
    {
        /* 
            First, check if "ways" were the guilties by comparing way's tags.
            If way found, it means that the problem were the blocks. Otherwise, the problem were the ways.
        */
       int index = cache_set_find_way(cs, tag);
       // Way found.
       if ( index != -1 )
       {
            struct cache_way *cw = cs->ways[index]; 
            cache_way_replace_block(cw, cw->next_block, mem_offset);
       }
       // Way not found.
       else
       {
            cs->next_way = (cs->next_way + 1) % cs->num_ways;
            cache_set_conflicts_update(ce, cache_set);
            cache_set_replace_way(cs, cs->next_way, tag, mem_offset);
       }

    }
}

/**
 * @brief Returns cache's number of sets.
 * 
 * @param ce Target cache.
 * 
 * @returns Cache's number of sets.
 */
int cache_num_sets(const struct cache *ce)
{
    /* Sanity check. */
    assert(ce != NULL);

    return (ce->num_sets);
}

/**
 * @brief Returns the map that controls the number of tasks that has, in last iteration, acessed each cache's set.
 * 
 * @param ce Target cache.
 * 
 * @returns Cache's map of set's accesses.
 */
map_tt cache_set_accesses(const struct cache *ce)
{
    /* Sanity check. */
    assert(ce != NULL);
    return ce->sets_accesses;
}

/**
 * @brief Returns the map that controls the number conflicts, in the last iteration, in each cache's set.
 * 
 * @param ce Target cache.
 * 
 * @returns Cache's map of set's conflicts.
 */
map_tt cache_set_conflicts(const struct cache *ce)
{
    /* Sanity check. */
    assert(ce != NULL);
    return ce->sets_conflicts;
}

/**
 * @brief Updates the cache's sets' acesses with specified value.
 * 
 * @param ce  Target cache.
 * @param set Desired set.
 */
void cache_set_accesses_update(struct cache *ce, int set)
{
    /* Sanity check. */
    assert(ce != NULL);

    // If value is negative, we reset the map
    if ( set < 0 )
    {
        map_destroy(ce->sets_accesses);
        ce->sets_accesses = map_create(map_compare_int);
    }
    else 
        map_insert(ce->sets_accesses, &set);
}

/**
 * @brief Updates the cache's sets' conflicts with specified value.
 * 
 * @param ce  Target cache.
 * @param set Desired set.
 */
void cache_set_conflicts_update(struct cache *ce, int set)
{
    /* Sanity check. */
    assert(ce != NULL);
    
    if ( set < 0 )
    {
        map_destroy(ce->sets_conflicts);
        ce->sets_conflicts = map_create(map_compare_int);
    }
    else 
        map_insert(ce->sets_conflicts, &set);
}

/**
 * @brief Destroys a cache instance.
 * 
 * @param ce Target cache.
 */
void cache_destroy(struct cache *ce)
{
    /* Sanity check. */
    assert(ce != NULL);
    for ( int i = 0; i < ce->num_sets; i++ )
    {
        cache_set_destroy(ce->sets[i]);
    }
    map_destroy(ce->sets_accesses);
    map_destroy(ce->sets_conflicts);
    free(ce);
}