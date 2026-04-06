#include <stdio.h>
#include <stdlib.h>

#define CACHE_SIZE 32
#define BLOCK_SIZE 4
#define ASSOCIATIVITY 2
#define WINDOW 20
#define TRACE_SIZE 1000

#define NUM_BLOCKS (CACHE_SIZE / BLOCK_SIZE)
#define NUM_SETS (NUM_BLOCKS / ASSOCIATIVITY)

typedef struct {
    int tag;
    int valid;
    int age;
    int freq;
} Line;

typedef struct {
    Line lines[ASSOCIATIVITY];
    int fifo_ptr;
} Set;

typedef struct {
    Set sets[NUM_SETS];
} Cache;

void init_cache(Cache *c) {
    for (int i = 0; i < NUM_SETS; i++) {
        c->sets[i].fifo_ptr = 0;
        for (int j = 0; j < ASSOCIATIVITY; j++) {
            c->sets[i].lines[j].valid = 0;
            c->sets[i].lines[j].tag = -1;
            c->sets[i].lines[j].age = 0;
            c->sets[i].lines[j].freq = 0;
        }
    }
}

void copy_cache(Cache *dest, Cache *src) {
    for (int i = 0; i < NUM_SETS; i++) {
        dest->sets[i].fifo_ptr = src->sets[i].fifo_ptr;
        for (int j = 0; j < ASSOCIATIVITY; j++) {
            dest->sets[i].lines[j] = src->sets[i].lines[j];
        }
    }
}

void decode_address(int addr, int *tag, int *index) {
    int block_addr = addr / BLOCK_SIZE;
    *index = block_addr % NUM_SETS;
    *tag = block_addr / NUM_SETS;
}

int access_LRU(Cache *c, int addr, int time) {
    int tag, index;
    decode_address(addr, &tag, &index);

    Set *set = &c->sets[index];

    /* Hit: update age (recency) and frequency */
    for (int i = 0; i < ASSOCIATIVITY; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            set->lines[i].age = time;
            set->lines[i].freq++;
            return 1;
        }
    }

    /* Miss: find LRU victim — prefer invalid lines first */
    int lru = -1;
    for (int i = 0; i < ASSOCIATIVITY; i++) {
        if (!set->lines[i].valid) {
            lru = i;
            break;
        }
    }
    if (lru == -1) {
        lru = 0;
        for (int i = 1; i < ASSOCIATIVITY; i++) {
            if (set->lines[i].age < set->lines[lru].age)
                lru = i;
        }
    }

    set->lines[lru].valid = 1;
    set->lines[lru].tag   = tag;
    set->lines[lru].age   = time;
    set->lines[lru].freq  = 1;   /* reset frequency for new occupant */

    return 0;
}

int access_FIFO(Cache *c, int addr) {
    int tag, index;
    decode_address(addr, &tag, &index);

    Set *set = &c->sets[index];

    /* Hit: update frequency only (FIFO doesn't change insertion order on hit) */
    for (int i = 0; i < ASSOCIATIVITY; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            set->lines[i].freq++;
            return 1;
        }
    }

    /* Miss: evict at fifo_ptr and install new line */
    int pos = set->fifo_ptr;

    set->lines[pos].valid = 1;
    set->lines[pos].tag   = tag;
    set->lines[pos].age   = 0;   /* FIFO doesn't use age; reset to avoid stale data */
    set->lines[pos].freq  = 1;   /* reset frequency for new occupant */

    set->fifo_ptr = (set->fifo_ptr + 1) % ASSOCIATIVITY;

    return 0;
}

int access_LFU(Cache *c, int addr) {
    int tag, index;
    decode_address(addr, &tag, &index);

    Set *set = &c->sets[index];

    /* Hit: update frequency */
    for (int i = 0; i < ASSOCIATIVITY; i++) {
        if (set->lines[i].valid && set->lines[i].tag == tag) {
            set->lines[i].freq++;
            return 1;
        }
    }

    /* Miss: find LFU victim — prefer invalid lines first, then lowest freq */
    int lfu = -1;
    for (int i = 0; i < ASSOCIATIVITY; i++) {
        if (!set->lines[i].valid) {
            lfu = i;
            break;
        }
    }
    if (lfu == -1) {
        lfu = 0;
        for (int i = 1; i < ASSOCIATIVITY; i++) {
            if (set->lines[i].freq < set->lines[lfu].freq)
                lfu = i;
        }
    }

    set->lines[lfu].valid = 1;
    set->lines[lfu].tag   = tag;
    set->lines[lfu].age   = 0;   /* LFU doesn't use age; reset to avoid stale data */
    set->lines[lfu].freq  = 1;   /* new occupant starts at 1 */

    return 0;
}

int main() {
    Cache main_cache;
    init_cache(&main_cache);

    int trace[TRACE_SIZE];
    int idx = 0;

    FILE *fp = fopen("trace.txt", "r");

    if (fp != NULL) {
        printf("Reading trace from file\n");

        char op;
        unsigned int addr;

        while (fscanf(fp, " %c %x", &op, &addr) == 2 && idx < TRACE_SIZE) {
            trace[idx++] = (int)addr;
        }

        fclose(fp);
    } else {
        printf("File not found! Using synthetic trace...\n");

        for (int i = 0; i < 100; i++)
            trace[idx++] = i * BLOCK_SIZE;         /* sequential, stride 1 block */

        for (int i = 0; i < 100; i++)
            trace[idx++] = (i % 16) * BLOCK_SIZE;  /* repeating hot set */
    }

    int n = idx;
    printf("Total accesses read: %d\n", n);

    int main_hits = 0;
    /* Policy encoding: 0 = FIFO, 1 = LRU, 2 = LFU */
    int policy = 1;  /* start with LRU */
    int global_time = 0;

    for (int i = 0; i < n; i += WINDOW) {

        /* Snapshot the current main cache state into three trial caches */
        Cache temp_lru, temp_fifo, temp_lfu;
        copy_cache(&temp_lru,  &main_cache);
        copy_cache(&temp_fifo, &main_cache);
        copy_cache(&temp_lfu,  &main_cache);

        int lru_hits = 0, fifo_hits = 0, lfu_hits = 0;
        int temp_time = global_time;

        /* Trial run: count hits for each policy on this window */
        for (int j = i; j < i + WINDOW && j < n; j++) {
            int addr = trace[j];
            temp_time++;

            if (access_LRU (&temp_lru,  addr, temp_time)) lru_hits++;
            if (access_FIFO(&temp_fifo, addr))             fifo_hits++;
            if (access_LFU (&temp_lfu,  addr))             lfu_hits++;
        }

        printf("\nWindow %d:\n", (i / WINDOW) + 1);
        printf("  LRU: %d  FIFO: %d  LFU: %d\n", lru_hits, fifo_hits, lfu_hits);

        /*
         * Policy selection: pick the winner; break ties in favour of LRU
         * (generally better due to temporal locality).  LFU wins only when
         * strictly greater than both others; FIFO wins only when strictly
         * greater than LRU.
         */
        if (lfu_hits > lru_hits && lfu_hits > fifo_hits) {
            policy = 2;
            printf("  Switching to LFU\n");
        } else if (fifo_hits > lru_hits) {
            policy = 0;
            printf("  Switching to FIFO\n");
        } else {
            policy = 1;
            printf("  Switching to LRU\n");
        }

        /* Apply chosen policy on the real cache for this window */
        for (int j = i; j < i + WINDOW && j < n; j++) {
            int addr = trace[j];
            global_time++;

            if (policy == 1)
                main_hits += access_LRU (&main_cache, addr, global_time);
            else if (policy == 0)
                main_hits += access_FIFO(&main_cache, addr);
            else
                main_hits += access_LFU (&main_cache, addr);
        }
    }

    printf("\nTotal Hits: %d / %d\n", main_hits, n);
    printf("Hit Rate:   %.2f%%\n", (main_hits * 100.0) / n);

    return 0;
}