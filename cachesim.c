#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>
#include "cache.h"

#define ADDRESS_LENGTH 64  // 64-bit memory addressing
#define LINE_BUF_SIZE 256


// Provide a standard way for the cache
// simulator to display its final statistics
void print_summary(int hits, int misses, int evictions)
{
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
}

// Print usage info
void print_usage(char* argv[])
{
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of set index bits.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of block offset bits.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/trace01.dat\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/trace01.dat\n", argv[0]);
}

// Parse a non-negative integer argument. Returns -1 if invalid
static int parse_int_arg(const char *str)
{
    char *end;
    errno = 0;
    long val = strtol(str, &end, 10);
    if (errno != 0 || end == str || *end != '\0' || val < 0 || val > INT_MAX)
        return -1;
    return (int)val;
}

static void simulate_access(cache_t *cache, unsigned long long addr, int verbose)
{
    access_result_t result = cache_access(cache, addr);
    if (!verbose)
        return;

    switch (result) {
    case ACCESS_HIT:
        printf("hit ");
        break;
    case ACCESS_MISS:
        printf("miss ");
        break;
    case ACCESS_MISS_EVICT:
        printf("miss eviction ");
        break;
    }
}


//replay every data access in the trace file against the cache
static int replay_trace(cache_t *cache, const char *trace_file, int verbose)
{
    FILE *fp = fopen(trace_file, "r");
    if (fp == NULL) {
        perror(trace_file);
        return -1;
    }

    char line[LINE_BUF_SIZE];
    char op;
    unsigned long long addr;
    int size;

    while (fgets(line, sizeof(line), fp) != NULL) {
        // instruction loads have no leading space, so they are skipped here
        if (line[0] != ' ')
            continue;
        if (sscanf(line, " %c %llx,%d", &op, &addr, &size) != 3)
            continue;
        if (op != 'L' && op != 'S' && op != 'M')
            continue;

        if (verbose)
            printf("%c %llx,%d ", op, addr, size);

        simulate_access(cache, addr, verbose);
        // a modify is a load followed by a store to the same address
        if (op == 'M')
            simulate_access(cache, addr, verbose);

        if (verbose)
            printf("\n");
    }

    fclose(fp);
    return 0;
}

int main(int argc, char* argv[])
{
    int s = -1, E = -1, b = -1;
    int verbose = 0;
    char *trace_file = NULL;
    int c;

    while ((c = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
        switch (c) {
        case 's':
            s = parse_int_arg(optarg);
            break;
        case 'E':
            E = parse_int_arg(optarg);
            break;
        case 'b':
            b = parse_int_arg(optarg);
            break;
        case 't':
            trace_file = optarg;
            break;
        case 'v':
            verbose = 1;
            break;
        case 'h':
            print_usage(argv);
            exit(0);
        default:
            print_usage(argv);
            exit(1);
        }
    }

    // all of s, E, b, t are required and must describe a valid cache
    if (s < 0 || b < 0 || E < 1 || trace_file == NULL ||
        s >= ADDRESS_LENGTH || s + b > ADDRESS_LENGTH) {
        print_usage(argv);
        exit(1);
    }

    cache_t *cache = cache_create(s, E, b);
    if (cache == NULL) {
        fprintf(stderr, "Failed to allocate cache\n");
        exit(1);
    }

    if (replay_trace(cache, trace_file, verbose) != 0) {
        cache_free(cache);
        exit(1);
    }

    int hit_count = cache->hits;
    int miss_count = cache->misses;
    int eviction_count = cache->evictions;
    cache_free(cache);

    // output cache hit and miss statistics
    print_summary(hit_count, miss_count, eviction_count);

    return 0;
}
