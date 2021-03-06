/*
 * Copyright (c) 2011-2013  University of Texas at Austin. All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * This file is part of PerfExpert.
 *
 * PerfExpert is free software: you can redistribute it and/or modify it under
 * the terms of the The University of Texas at Austin Research License
 *
 * PerfExpert is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.
 *
 * Authors: Leonardo Fialho and Ashay Rane
 *
 * $HEADER$
 */

/* System standard headers */
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

/* Cache simulator headers */
#include "cache_sim.h"
#include "cache_sim_types.h"
#include "cache_sim_reuse.h"
#include "cache_sim_symbol.h"
#include "cache_policy_lru.h"
#include "cache_policy_plru.h"

/* List of policies */
static policy_t policies[] = {
    { "lru",  &policy_lru_init,  &policy_lru_access  },
    { "plru", &policy_plru_init, &policy_plru_access },
    { NULL,   NULL,              NULL,               }
};

/* cache_sim_init */
cache_handle_t* cache_sim_init(const unsigned int total_size,
    const unsigned int line_size, const unsigned int associativity,
    const char *policy) {
    /* safety checks */
    if ((0 >= total_size) || (0 >= line_size) || (0 >= associativity)) {
        return NULL;
    }

    /* variables declaration and initialization */
    cache_handle_t *cache = NULL;

    printf("--------------------------------\n");

    /* create the cache */
    if (NULL == (cache = cache_create(total_size, line_size, associativity))) {
        return NULL;
    }

    /* set the cache replacement policy (or algorithm) */
    if (CACHE_SIM_SUCCESS != (set_policy(cache, policy))) {
        cache_destroy(cache);
        return NULL;
    }

    printf(" Cache initialized successfully \n");
    printf("--------------------------------\n");

    return cache;
}

/* cache_sim_fini */
int cache_sim_fini(cache_handle_t *cache) {
    /* is reuse distance calculation enabled? */
    if (NULL != cache->reuse_data) {
        cache_sim_reuse_disable(cache);
    }

    /* is symbols tracking enabled? */
    if (NULL != cache->symbol_data) {
        cache_sim_symbol_disable(cache);
    }

    printf("--------------------------------\n");
    printf("Total accesses: %16"PRIu64"\n", cache->access);
    printf("Cache hits:     %16"PRIu64"\n", cache->hit);
    printf(" -> rate        %15.2f%%\n",
        (((double)cache->hit / (double)cache->access) * 100));
    printf(" -> prefetched  %16"PRIu64"\n", cache->prefetcher_hit);
    printf("Cache misses:   %16"PRIu64"\n", cache->miss);
    printf(" -> rate        %15.2f%%\n",
        (((double)cache->miss / (double)cache->access) * 100));
    printf("Set conflicts:  %16"PRIu64"\n", cache->conflict);
    printf(" -> rate        %15.2f%%\n",
        (((double)cache->conflict / (double)cache->miss) * 100));
    printf("Prefetcher:                     \n");
    printf(" -> next line   %16"PRIu64"\n", cache->prefetcher_next_line);
    printf(" -> pollution   %16"PRIu64"\n", cache->prefetcher_evict);
    printf("  Cache finalized successfully  \n");
    printf("--------------------------------\n");

    /* destroy the cache and free memory */
    cache_destroy(cache);

    return CACHE_SIM_SUCCESS;
}

/* create_cache */
static cache_handle_t* cache_create(const unsigned int total_size,
    const unsigned int line_size, const unsigned int associativity) {
    /* variables declaration and initialization */
    cache_handle_t *cache = NULL;

    /* allocate memory for this cache */
    cache = (cache_handle_t *)malloc(sizeof(cache_handle_t));
    if (NULL == cache) {
        printf("unable to allocate memory for cache structure\n");
        return NULL;
    }

    /* initialize the cache parameters */
    cache->total_size    = total_size;
    cache->line_size     = line_size;
    cache->associativity = associativity;
    cache->total_lines   = total_size / line_size;
    cache->total_sets    = cache->total_lines / associativity;
    cache->offset_length = (int)log2(line_size);
    cache->set_length    = (int)log2(cache->total_sets);

    /* replacement policy */
    cache->access_fn = NULL;
    cache->data      = NULL;

    /* reused distance */
    cache->reuse_data  = NULL;
    cache->reuse_limit = 0;
    cache->reuse_fn    = NULL;

    /* symbols tracking */
    cache->symbol_data = NULL;

    /* prefetchers */
    cache->next_line = PREFETCHER_INVALID;

    /* initialize performance counters */
    cache->hit                  = 0;
    cache->miss                 = 0;
    cache->access               = 0;
    cache->conflict             = 0;
    cache->prefetcher_next_line = 0;
    cache->prefetcher_hit       = 0;
    cache->prefetcher_evict     = 0;

    printf("   Cache created successfully   \n");
    printf("Cache size:      %9d bytes\n", cache->total_size);
    printf("Line length:     %9d bytes\n", cache->line_size);
    printf("Number of lines: %15d\n", cache->total_lines);
    printf("Associativity:   %15d\n", cache->associativity);
    printf("Number of sets:  %15d\n", cache->total_sets);
    printf("Offset length:   %10d bits\n", cache->offset_length);
    printf("Set length:      %10d bits\n", cache->set_length);

    return cache;
}

/* cache_destroy */
static void cache_destroy(cache_handle_t *cache) {
    if (NULL != cache) {
        if (NULL != cache->data) {
            free(cache->data);
        }
        free(cache);
    }
}

/* set_policy */
static int set_policy(cache_handle_t *cache, const char *policy) {
    int i = 0;

    /* find the requested policy and set the function */
    while (NULL != policies[i].name) {
        if (0 == strcmp(policy, policies[i].name)) {

            /* set the policy function */
            cache->access_fn = policies[i].access_fn;

            printf("Replacem policy: %15s\n", policy);

            /* initialize the data area */
            if (CACHE_SIM_SUCCESS != policies[i].init_fn(cache)) {
                printf("error initializing the cache data area\n");
            }

            return CACHE_SIM_SUCCESS;
        }
        i++;
    }

    printf("replacement policy not found\n");
    return CACHE_SIM_ERROR;
}

// EOF
