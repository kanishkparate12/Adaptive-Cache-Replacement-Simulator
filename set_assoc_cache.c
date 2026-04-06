#include <stdio.h>
#include <stdlib.h>

#define CACHE_SIZE 8
#define BLOCK_SIZE 4
#define NUM_LINES (CACHE_SIZE / BLOCK_SIZE)

#define ASSOCIATIVITY 2
#define NUM_SETS (NUM_LINES / ASSOCIATIVITY)

typedef struct {
    int valid;
    unsigned int tag;
    unsigned long long last_used;
    unsigned long long inserted_at;
    unsigned int access_count;
} CacheLine;

typedef enum {
    FIFO,
    LRU,
    LFU
} ReplacementPolicy;

typedef struct {
    CacheLine lines[NUM_LINES];
    int hits;
    int misses;
    ReplacementPolicy policy;
    unsigned long long time;
} Cache;

void init_cache(Cache *cache, ReplacementPolicy policy) {
    for (int i = 0; i < NUM_LINES; i++) {
        cache->lines[i].valid        = 0;
        cache->lines[i].tag          = 0;
        cache->lines[i].last_used    = 0;
        cache->lines[i].inserted_at  = 0;
        cache->lines[i].access_count = 0;
    }
    cache->hits   = 0;
    cache->misses = 0;
    cache->time   = 0;
    cache->policy = policy;
}

void access_cache(Cache *cache, unsigned int address) {
    cache->time++;

    unsigned int block_number = address / BLOCK_SIZE;
    unsigned int set_index    = block_number % NUM_SETS;
    unsigned int tag          = block_number / NUM_SETS;

    int set_start = set_index * ASSOCIATIVITY;

    /* Check for a hit */
    for (int i = 0; i < ASSOCIATIVITY; i++) {
        int idx = set_start + i;
        if (cache->lines[idx].valid && cache->lines[idx].tag == tag) {
            cache->hits++;
            cache->lines[idx].last_used    = cache->time;
            cache->lines[idx].access_count++;
            return;
        }
    }

    cache->misses++;

    /* Use an invalid (empty) slot if one exists */
    for (int i = 0; i < ASSOCIATIVITY; i++) {
        int idx = set_start + i;
        if (!cache->lines[idx].valid) {
            cache->lines[idx].valid        = 1;
            cache->lines[idx].tag          = tag;
            cache->lines[idx].inserted_at  = cache->time;
            cache->lines[idx].last_used    = cache->time;
            cache->lines[idx].access_count = 1;
            return;
        }
    }

    /* All lines valid — select a victim based on policy */
    int victim = set_start;

    if (cache->policy == FIFO) {
        unsigned long long oldest = cache->lines[set_start].inserted_at;
        for (int i = 1; i < ASSOCIATIVITY; i++) {
            int idx = set_start + i;
            if (cache->lines[idx].inserted_at < oldest) {
                oldest = cache->lines[idx].inserted_at;
                victim = idx;
            }
        }
    } else if (cache->policy == LRU) {
        unsigned long long lru_time = cache->lines[set_start].last_used;
        for (int i = 1; i < ASSOCIATIVITY; i++) {
            int idx = set_start + i;
            if (cache->lines[idx].last_used < lru_time) {
                lru_time = cache->lines[idx].last_used;
                victim = idx;
            }
        }
    } else if (cache->policy == LFU) {
        /*
         * Evict the least-frequently-used line.
         * Break ties by insertion order (oldest-inserted loses),
         * which is the standard LFU tie-breaking rule.
         */
        unsigned int min_count     = cache->lines[set_start].access_count;
        unsigned long long min_ins = cache->lines[set_start].inserted_at;
        for (int i = 1; i < ASSOCIATIVITY; i++) {
            int idx = set_start + i;
            unsigned int    c = cache->lines[idx].access_count;
            unsigned long long ins = cache->lines[idx].inserted_at;
            if (c < min_count || (c == min_count && ins < min_ins)) {
                min_count = c;
                min_ins   = ins;
                victim    = idx;
            }
        }
    }

    /* BUG FIX: must mark the victim line as valid after overwriting it.
     * The original code updated all other fields but forgot valid = 1,
     * so a previously-invalid victim would stay invisible to future hits. */
    cache->lines[victim].valid        = 1;
    cache->lines[victim].tag          = tag;
    cache->lines[victim].inserted_at  = cache->time;
    cache->lines[victim].last_used    = cache->time;
    cache->lines[victim].access_count = 1;
}

void simulate_trace(Cache *cache, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("Error opening file: %s\n", filename);
        return;
    }

    char op;
    unsigned int address;

    while (fscanf(fp, " %c %x", &op, &address) == 2) {
        /* Only process recognised memory operations; skip malformed lines */
        if (op == 'r' || op == 'R' || op == 'w' || op == 'W' ||
            op == 'l' || op == 'L' || op == 's' || op == 'S') {
            access_cache(cache, address);
        }
    }

    fclose(fp);
}

float get_hit_rate(Cache *cache) {
    int total = cache->hits + cache->misses;
    if (total == 0) return 0.0f;
    return (cache->hits * 100.0f) / total;
}

int main(void) {
    Cache cache;
    const char *trace = "trace.txt";

    init_cache(&cache, FIFO);
    simulate_trace(&cache, trace);
    printf("FIFO:\n");
    printf("  Hits:     %d\n", cache.hits);
    printf("  Misses:   %d\n", cache.misses);
    printf("  Hit Rate: %.2f%%\n\n", get_hit_rate(&cache));

    init_cache(&cache, LRU);
    simulate_trace(&cache, trace);
    printf("LRU:\n");
    printf("  Hits:     %d\n", cache.hits);
    printf("  Misses:   %d\n", cache.misses);
    printf("  Hit Rate: %.2f%%\n\n", get_hit_rate(&cache));

    init_cache(&cache, LFU);
    simulate_trace(&cache, trace);
    printf("LFU:\n");
    printf("  Hits:     %d\n", cache.hits);
    printf("  Misses:   %d\n", cache.misses);
    printf("  Hit Rate: %.2f%%\n", get_hit_rate(&cache));

    return 0;
}