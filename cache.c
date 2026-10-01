#include <stdint.h>
#include <stdlib.h>
#include "cache.h"

cache_t *cache_create(int s, int E, int b)
{
    // reject sizes whose line count would overflow size_t
    if (s < 0 || s >= 64 || E < 1 || (size_t)E > (SIZE_MAX >> s))
        return NULL;

    cache_t *cache = malloc(sizeof(cache_t));
    if (cache == NULL)
        return NULL;

    size_t num_lines = ((size_t)1 << s) * E;
    cache->lines = calloc(num_lines, sizeof(cache_line_t));
    if (cache->lines == NULL) {
        free(cache);
        return NULL;
    }

    cache->s = s;
    cache->E = E;
    cache->b = b;
    cache->clock = 0;
    cache->hits = 0;
    cache->misses = 0;
    cache->evictions = 0;
    return cache;
}

void cache_free(cache_t *cache)
{
    if (cache == NULL)
        return;
    free(cache->lines);
    free(cache);
}

// Simulate one access to addr, updating the cache state and counters.
access_result_t cache_access(cache_t *cache, unsigned long long addr)
{
    int tag_shift = cache->s + cache->b;
    unsigned long long set_mask = ((unsigned long long)1 << cache->s) - 1;
    // shifting a 64-bit value by 64 is undefined, so handle it explicitly
    unsigned long long set_index = cache->b >= 64 ? 0 : (addr >> cache->b) & set_mask;
    unsigned long long tag = tag_shift >= 64 ? 0 : addr >> tag_shift;

    cache_line_t *set = &cache->lines[set_index * cache->E];
    cache_line_t *victim = &set[0];
    cache->clock++;

    for (int i = 0; i < cache->E; i++) {
        cache_line_t *line = &set[i];
        if (line->valid && line->tag == tag) {
            line->last_used = cache->clock;
            cache->hits++;
            return ACCESS_HIT;
        }
        // prefer an empty line; otherwise track the least recently used
        if (victim->valid && (!line->valid || line->last_used < victim->last_used))
            victim = line;
    }

    cache->misses++;
    access_result_t result = ACCESS_MISS;
    if (victim->valid) {
        cache->evictions++;
        result = ACCESS_MISS_EVICT;
    }

    victim->valid = 1;
    victim->tag = tag;
    victim->last_used = cache->clock;
    return result;
}
