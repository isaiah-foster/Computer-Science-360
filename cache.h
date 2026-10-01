#ifndef CACHE_H
#define CACHE_H

/* outcome of a single cache access */
typedef enum {
    ACCESS_HIT,
    ACCESS_MISS,
    ACCESS_MISS_EVICT
} access_result_t;

typedef struct {
    int valid;
    unsigned long long tag;
    unsigned long long last_used;   // LRU timestamp
} cache_line_t;

typedef struct {
    int s;                  // set index bits
    int E;                  // lines per set
    int b;                  // block offset bits
    cache_line_t *lines;    // S * E lines, set i starts at lines[i * E]
    unsigned long long clock;
    int hits;
    int misses;
    int evictions;
} cache_t;

cache_t *cache_create(int s, int E, int b);
void cache_free(cache_t *cache);
access_result_t cache_access(cache_t *cache, unsigned long long addr);

#endif
