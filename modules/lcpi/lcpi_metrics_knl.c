/*
 * Copyright (c) 2011-2016  University of Texas at Austin. All rights reserved.
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
 * Authors: Antonio Gomez-Iglesias, Leonardo Fialho and Ashay Rane
 *
 * $HEADER$
 */

/* System standard headers */
#include <string.h>

/* Modules headers */
#include "lcpi.h"

/* PerfExpert common headers */
#include "common/perfexpert_constants.h"

int metrics_knl(void) {
    char s[MAX_LCPI];

    /* Set the events on the measurement module */
//    USE_EVENT("ARITH.FPU_DIV");
    USE_EVENT("BR_INST_RETIRED.ALL_BRANCHES");
    USE_EVENT("BR_MISP_RETIRED.ALL_BRANCHES");
    USE_EVENT("CPU_CLK_UNHALTED.THREAD_P");
    USE_EVENT("DTLB_LOAD_MISSES.WALK_DURATION");
    USE_EVENT("FP_COMP_OPS_EXE.SSE_PACKED_SINGLE");
    USE_EVENT("FP_COMP_OPS_EXE.SSE_FP_PACKED_DOUBLE");
    USE_EVENT("FP_COMP_OPS_EXE.SSE_FP_SCALAR_SINGLE");
    USE_EVENT("FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE");
    USE_EVENT("FP_COMP_OPS_EXE.X87");
    USE_EVENT("ICACHE.MISSES");
    USE_EVENT("ITLB_MISSES.WALK_DURATION");
    USE_EVENT("L2_RQSTS.ALL_DEMAND_RD_HIT");
    USE_EVENT("L2_RQSTS.CODE_RD_HIT");
    USE_EVENT("L2_RQSTS.CODE_RD_MISS");
    USE_EVENT("LAST_LEVEL_CACHE_MISSES");
    USE_EVENT("LAST_LEVEL_CACHE_REFERENCES");
    USE_EVENT("MEM_UOP_RETIRED.ANY_LOADS");
    USE_EVENT("INST_RETIRED.ANY_P");
    USE_EVENT("SIMD_FP_256.PACKED_SINGLE");
    USE_EVENT("SIMD_FP_256.PACKED_DOUBLE");
    USE_EVENT("CYCLE_ACTIVITY.STALLS_L2_PENDING");
    USE_EVENT("MEM_LOAD_UOPS_LLC_HIT_RETIRED");
    USE_EVENT("MEM_LOAD_UOPS_MISC_RETIRED.LLC_MISS");

    /* Set the profile total cycles and total instructions counters */
    my_module_globals.measurement->total_cycles_counter = "CPU_CLK_UNHALTED.THREAD_P";
    my_module_globals.measurement->total_inst_counter = "INST_RETIRED.ANY_P";

    /* ratio stall/cycles */
    bzero(s, MAX_LCPI);
    strcpy(s, ("CYCLE_ACTIVITY.STALLS_L2_PENDING/CPU_CLK_UNHALTED.THREAD_P"));
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("ratio.stall",s)) {
        return PERFEXPERT_ERROR;
    }
    /* ratio.floating_point */
    bzero(s, MAX_LCPI);
    strcpy(s, "(SIMD_FP_256.PACKED_SINGLE+SIMD_FP_256.PACKED_DOUBLE+"
        "FP_COMP_OPS_EXE.SSE_PACKED_SINGLE+FP_COMP_OPS_EXE.SSE_FP_PACKED_DOUBLE+"
        "FP_COMP_OPS_EXE.SSE_FP_SCALAR_SINGLE+FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE+"
        "FP_COMP_OPS_EXE.X87)/INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("ratio.floating_point", s)) {
        return PERFEXPERT_ERROR;
    }

    /* ratio.data_accesses */
    bzero(s, MAX_LCPI);
    strcpy(s, "MEM_UOP_RETIRED.ANY_LOADS / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("ratio.data_accesses", s)) {
        return PERFEXPERT_ERROR;
    }

    /* GFLOPS_(%_max).overall */
    bzero(s, MAX_LCPI);
    strcpy(s, "(((SIMD_FP_256.PACKED_SINGLE*8)+("
        "(SIMD_FP_256.PACKED_DOUBLE+FP_COMP_OPS_EXE.SSE_PACKED_SINGLE)*4)+"
        "(FP_COMP_OPS_EXE.SSE_FP_PACKED_DOUBLE*2)+"
        "FP_COMP_OPS_EXE.SSE_FP_SCALAR_SINGLE+FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE"
        ")/INST_RETIRED.ANY_P)/8");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("GFLOPS_(%_max).overall", s)) {
        return PERFEXPERT_ERROR;
    }

    /* GFLOPS_(%_max).packed */
    bzero(s, MAX_LCPI);
    strcpy(s, "(((SIMD_FP_256.PACKED_SINGLE*8)+"
        "((SIMD_FP_256.PACKED_DOUBLE+FP_COMP_OPS_EXE.SSE_PACKED_SINGLE)*4)+"
        "(FP_COMP_OPS_EXE.SSE_FP_PACKED_DOUBLE * 2))/INST_RETIRED.ANY_P)/8");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("GFLOPS_(%_max).packed", s)) {
        return PERFEXPERT_ERROR;
    }

    /* GFLOPS_(%_max).scalar */
    bzero(s, MAX_LCPI);
    strcpy(s, "((FP_COMP_OPS_EXE.SSE_FP_SCALAR_SINGLE+"
        "FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE)/INST_RETIRED.ANY_P)/8");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("GFLOPS_(%_max).scalar", s)) {
        return PERFEXPERT_ERROR;
    }

    /* overall */
    bzero(s, MAX_LCPI);
    strcpy(s, "CPU_CLK_UNHALTED.THREAD_P / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("overall", s)) {
        return PERFEXPERT_ERROR;
    }

    /* data_accesses.overall */
    bzero(s, MAX_LCPI);
    strcpy(s, "((MEM_UOP_RETIRED.ANY_LOADS * L1_dlat) + "
        "(L2_RQSTS.ALL_DEMAND_RD_HIT * L2_lat) + "
        "(LAST_LEVEL_CACHE_REFERENCES * L3_lat) + "
        "LAST_LEVEL_CACHE_MISSES * mem_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_accesses.overall", s)) {
        return PERFEXPERT_ERROR;
    }

    /* data_accesses.L1_cache_hits */
    bzero(s, MAX_LCPI);
    strcpy(s, "(MEM_UOP_RETIRED.ANY_LOADS * L1_dlat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_accesses.L1_cache_hits", s)) {
        return PERFEXPERT_ERROR;
    }

    /* data_accesses.L2_cache_hits */
    bzero(s, MAX_LCPI);
    strcpy(s, "(L2_RQSTS.ALL_DEMAND_RD_HIT * L2_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_accesses.L2_cache_hits", s)) {
        return PERFEXPERT_ERROR;
    }

    /* data_accesses.L3_cache_hits */
    bzero(s, MAX_LCPI);
    strcpy(s, "(LAST_LEVEL_CACHE_REFERENCES * L3_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_accesses.L3_cache_hits", s)) {
        return PERFEXPERT_ERROR;
    }

    /* data_accesses.LLC_misses */
    bzero(s, MAX_LCPI);
    strcpy(s, "(LAST_LEVEL_CACHE_MISSES * mem_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_accesses.LLC_misses", s)) {
        return PERFEXPERT_ERROR;
    }

    /*  % memory bound */
    /*  %MEM Bound = CYCLE_ACTIVITY.STALLS_L2_PENDING * L3_Miss_fraction / CLOCKS */
    /*  L3_Miss_fraction is:
     *  MEM_L3_WEIGHT * MEM_LOAD_UOPS_RETIRED.LLC_MISS_PS / (MEM_LOAD_UOPS_RETIRED.LLC_HIT_PS + 
     *  MEM_LOAD_UOPS_RETIRED.LLC_MISS_PS * MEM_L3_WEIGHT) */
    /*  MEM_L3_WEIGHT is system dependent, but we can assume 7 */
    bzero(s, MAX_LCPI);
    strcpy(s, "(CYCLE_ACTIVITY.STALLS_L2_PENDING * ( 7 *"
              "MEM_LOAD_UOPS_MISC_RETIRED.LLC_MISS / ( MEM_LOAD_UOPS_LLC_HIT_RETIRED +"
              "7 * MEM_LOAD_UOPS_MISC_RETIRED.LLC_MISS ) ) / CPU_CLK_UNHALTED.THREAD_P )");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_accesses.memory_bound", s)) {
        return PERFEXPERT_ERROR;
    }

    /* instruction_accesses.overall */
    bzero(s, MAX_LCPI);
    strcpy(s, "((ICACHE.MISSES * L1_ilat) + (L2_RQSTS.CODE_RD_HIT * L2_lat)"
            " + (L2_RQSTS.CODE_RD_MISS * mem_lat)) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("instruction_accesses.overall", s)) {
        return PERFEXPERT_ERROR;
    }

    /* instruction_accesses.L1_hits */
    bzero(s, MAX_LCPI);
    strcpy(s, "(ICACHE.MISSES * L1_ilat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("instruction_accesses.L1_hits", s)) {
        return PERFEXPERT_ERROR;
    }

    /* instruction_accesses.L2_hits */
    bzero(s, MAX_LCPI);
    strcpy(s, "(L2_RQSTS.CODE_RD_HIT * L2_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("instruction_accesses.L2_hits", s)) {
        return PERFEXPERT_ERROR;
    }

    /* instruction_accesses.L2_misses */
    bzero(s, MAX_LCPI);
    strcpy(s, "(L2_RQSTS.CODE_RD_MISS * mem_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("instruction_accesses.L2_misses", s)) {
        return PERFEXPERT_ERROR;
    }

    /* data_TLB.overall */
    bzero(s, MAX_LCPI);
    strcpy(s, "DTLB_LOAD_MISSES.WALK_DURATION / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_TLB.overall", s)) {
        return PERFEXPERT_ERROR;
    }

    /* instruction_TLB.overall */
    bzero(s, MAX_LCPI);
    strcpy(s, "ITLB_MISSES.WALK_DURATION / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("instruction_TLB.overall", s)) {
        return PERFEXPERT_ERROR;
    }

    /* branch_instructions.overall */
    bzero(s, MAX_LCPI);
    strcpy(s, "((BR_INST_RETIRED.ALL_BRANCHES * BR_lat) "
        "+ (BR_MISP_RETIRED.ALL_BRANCHES * BR_miss_lat)) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("branch_instructions.overall", s)) {
        return PERFEXPERT_ERROR;
    }

    /* branch_instructions.correctly_predicted */
    bzero(s, MAX_LCPI);
    strcpy(s, "(BR_INST_RETIRED.ALL_BRANCHES * BR_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("branch_instructions.correctly_predicted", s)) {
        return PERFEXPERT_ERROR;
    }

    /* branch_instructions.mispredicted */
    bzero(s, MAX_LCPI);
    strcpy(s, "(BR_MISP_RETIRED.ALL_BRANCHES * BR_miss_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("branch_instructions.mispredicted", s)) {
        return PERFEXPERT_ERROR;
    }

    /* FP_instructions.overall */
    bzero(s, MAX_LCPI);
    strcpy(s, "((SIMD_FP_256.PACKED_SINGLE + SIMD_FP_256.PACKED_DOUBLE + "
        "FP_COMP_OPS_EXE.X87 + FP_COMP_OPS_EXE.SSE_PACKED_SINGLE + "
        "FP_COMP_OPS_EXE.SSE_FP_PACKED_DOUBLE + "
        "FP_COMP_OPS_EXE.SSE_FP_SCALAR_SINGLE + "
        "FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE) + ARITH.FPU_DIV) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("FP_instructions.overall", s)) {
        return PERFEXPERT_ERROR;
    }

    /* FP_instructions.fast */
    bzero(s, MAX_LCPI);
    strcpy(s, "(SIMD_FP_256.PACKED_SINGLE + SIMD_FP_256.PACKED_DOUBLE + "
        "FP_COMP_OPS_EXE.X87 + FP_COMP_OPS_EXE.SSE_PACKED_SINGLE + "
        "FP_COMP_OPS_EXE.SSE_FP_PACKED_DOUBLE + "
        "FP_COMP_OPS_EXE.SSE_FP_SCALAR_SINGLE + "
        "FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("FP_instructions.fast", s)) {
        return PERFEXPERT_ERROR;
    }

    /* FP_instructions.slow */
    bzero(s, MAX_LCPI);
    strcpy(s, "ARITH.FPU_DIV / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("FP_instructions.slow", s)) {
        return PERFEXPERT_ERROR;
    }

    return PERFEXPERT_SUCCESS;
}


int metrics_knl_vtune(void) {
    char s[MAX_LCPI];

    /* Set the events on the measurement module */
    USE_EVENT("INST_RETIRED.ANY_P");
    USE_EVENT("BR_INST_RETIRED.ALL_BRANCHES");
    USE_EVENT("BR_MISP_RETIRED.ALL_BRANCHES");
    USE_EVENT("CPU_CLK_UNHALTED.THREAD_P");
    USE_EVENT("PAGE_WALKS.WALKS"); // This can be used to measure TLB misses
    USE_EVENT("PAGE_WALKS.D_SIDE_CYCLES");
    USE_EVENT("PAGE_WALKS.I_SIDE_CYCLES");
    USE_EVENT("ICACHE.MISSES");
    USE_EVENT("LONGEST_LAT_CACHE.MISS");
    USE_EVENT("LONGEST_LAT_CACHE.REFERENCE");
    USE_EVENT("MEM_UOPS_RETIRED.ALL_LOADS");
    USE_EVENT("MEM_UOPS_RETIRED.L2_MISS_LOADS_PS");
    USE_EVENT("UOPS_RETIRED.ALL"); // -> DO I NEED THIS ONE???
    USE_EVENT("UOPS_RETIRED.SCALAR_SIMD");
    USE_EVENT("UOPS_RETIRED.PACKED_SIMD");

//    USE_EVENT("NO_ALLOC_CYCLES.ALL"); //Is this one OK?
    USE_EVENT("UNC_E_RPQ_INSERTS");
    USE_EVENT("UNC_E_WPQ_INSERTS");
    USE_EVENT("UNC_E_EDC_ACCESS.HIT_CLEAN");
    USE_EVENT("UNC_E_EDC_ACCESS.HIT_DIRTY");
    USE_EVENT("UNC_E_EDC_ACCESS.MISS_CLEAN");
    USE_EVENT("UNC_E_EDC_ACCESS.MISS_DIRTY");

    /* Set the profile total cycles and total instructions counters */
    my_module_globals.measurement->total_cycles_counter = "CPU_CLK_UNHALTED.THREAD_P";
    my_module_globals.measurement->total_inst_counter = "INST_RETIRED.ANY_P";

    /* ratio.floating_point */
/*    bzero(s, MAX_LCPI);
    strcpy(s, "(SIMD_FP_256.PACKED_SINGLE+SIMD_FP_256.PACKED_DOUBLE+"
        "FP_COMP_OPS_EXE.SSE_PACKED_SINGLE+FP_COMP_OPS_EXE.SSE_PACKED_DOUBLE+"
        "FP_COMP_OPS_EXE.SSE_SCALAR_SINGLE+FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE+"
        "FP_COMP_OPS_EXE.X87)/INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("ratio.floating_point", s)) {
        return PERFEXPERT_ERROR;
    }
*/
    /* ratio.data_accesses */
    bzero(s, MAX_LCPI);
    strcpy(s, "MEM_UOPS_RETIRED.ALL_LOADS / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("ratio.data_accesses", s)) {
        return PERFEXPERT_ERROR;
    }

    /* GFLOPS_(%_max).overall */
/*    bzero(s, MAX_LCPI);
    strcpy(s, "(((SIMD_FP_256.PACKED_SINGLE*8)+("
        "(SIMD_FP_256.PACKED_DOUBLE+FP_COMP_OPS_EXE.SSE_PACKED_SINGLE)*4)+"
        "(FP_COMP_OPS_EXE.SSE_PACKED_DOUBLE*2)+"
        "FP_COMP_OPS_EXE.SSE_SCALAR_SINGLE+FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE"
        ")/INST_RETIRED.ANY_P)/8");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("GFLOPS_(%_max).overall", s)) {
        return PERFEXPERT_ERROR;
    }
*/
    /* GFLOPS_(%_max).packed */
/*    bzero(s, MAX_LCPI);
    strcpy(s, "(((SIMD_FP_256.PACKED_SINGLE*8)+"
        "((SIMD_FP_256.PACKED_DOUBLE+FP_COMP_OPS_EXE.SSE_PACKED_SINGLE)*4)+"
        "(FP_COMP_OPS_EXE.SSE_PACKED_DOUBLE * 2))/INST_RETIRED.ANY_P)/8");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("GFLOPS_(%_max).packed", s)) {
        return PERFEXPERT_ERROR;
    }
*/
    /* GFLOPS_(%_max).scalar */
/*    bzero(s, MAX_LCPI);
    strcpy(s, "((FP_COMP_OPS_EXE.SSE_SCALAR_SINGLE+"
        "FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE)/INST_RETIRED.ANY_P)/8");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("GFLOPS_(%_max).scalar", s)) {
        return PERFEXPERT_ERROR;
    }
*/

    bzero(s, MAX_LCPI);
    strcpy(s, "(UOPS_RETIRED.SCALAR_SIMD+UOPS_RETIRED.PACKED_SIMD) / UOPS_RETIRED.ALL");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("SIMD overall",s)) {
        return PERFEXPERT_ERROR;
    }

    /* overall */
    bzero(s, MAX_LCPI);
    strcpy(s, "CPU_CLK_UNHALTED.THREAD_P / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("overall", s)) {
        return PERFEXPERT_ERROR;
    }

    /* data_accesses.overall */
    bzero(s, MAX_LCPI);
    strcpy(s, "((MEM_UOPS_RETIRED.ALL_LOADS * L1_dlat) + "
        "(L2_RQSTS.DEMAND_DATA_RD_HIT * L2_lat) + "
        "(LONGEST_LAT_CACHE.REFERENCE * L3_lat) + "
        "LONGEST_LAT_CACHE.MISS * mem_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_accesses.overall", s)) {
        return PERFEXPERT_ERROR;
    }

    /* data_accesses.L1_cache_hits */
    bzero(s, MAX_LCPI);
    strcpy(s, "(MEM_UOPS_RETIRED.ALL_LOADS * L1_dlat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_accesses.L1_cache_hits", s)) {
        return PERFEXPERT_ERROR;
    }

    /* data_accesses.L2_cache_hits */
/*    bzero(s, MAX_LCPI);
    strcpy(s, "(L2_RQSTS.DEMAND_DATA_RD_HIT * L2_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_accesses.L2_cache_hits", s)) {
        return PERFEXPERT_ERROR;
    }
*/
    /* data_accesses.L3_cache_hits */
    bzero(s, MAX_LCPI);
    strcpy(s, "(LONGEST_LAT_CACHE.REFERENCE * L3_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_accesses.L3_cache_hits", s)) {
        return PERFEXPERT_ERROR;
    }

    /* data_accesses.LLC_misses */
    bzero(s, MAX_LCPI);
    strcpy(s, "(LONGEST_LAT_CACHE.MISS * mem_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_accesses.LLC_misses", s)) {
        return PERFEXPERT_ERROR;
    }


    /* MCDRAM Read BW */
    bzero(s, MAX_LCPI);
    strcpy(s, "(UNC_E_RPQ_INSERTS - UNC_E_EDC_ACCESS.HIT_CLEAN / UNC_E_EDC_ACCESS.MISS_CLEAN - "
              "UNC_E_EDC_ACCESS.HIT_DIRTY / UNC_E_EDC_ACCESS.MISS_DIRTY) * 64 / CPU_CLK_UNHALTED.THREAD_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("mcdram.read_bandwidth", s)) {
        return PERFEXPERT_ERROR;
    }

    /* MCDRAM Write BW */
    bzero(s, MAX_LCPI);
    // I should add this: DCLK_Events_CAS_Reads
    strcpy(s, "UNC_E_WPQ_INSERTS * 64 / CPU_CLK_UNHALTED.THREAD_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("mcdram.write_bandwidth", s)) {
        return PERFEXPERT_ERROR;
    }

    /* instruction_accesses.overall */
  /*  bzero(s, MAX_LCPI);
    strcpy(s, "((ICACHE.MISSES * L1_ilat) + (L2_RQSTS.CODE_RD_HIT * L2_lat)"
            " + (L2_RQSTS.CODE_RD_MISS * mem_lat)) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("instruction_accesses.overall", s)) {
        return PERFEXPERT_ERROR;
    }
*/
    /* instruction_accesses.L1_hits */
    bzero(s, MAX_LCPI);
    strcpy(s, "(ICACHE.MISSES * L1_ilat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("instruction_accesses.L1_hits", s)) {
        return PERFEXPERT_ERROR;
    }

    bzero(s, MAX_LCPI);
    strcpy(s, "FETCH_STALL.ICACHE_FILL_PENDING_CYCLES / CPU_CLK_UNHALTED.THREAD_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("instruction_accesses.L1_misses", s)) {
        return PERFEXPERT_ERROR;
    }

    /* instruction_accesses.L2_hits */
  /*  bzero(s, MAX_LCPI);
    strcpy(s, "(L2_RQSTS.CODE_RD_HIT * L2_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("instruction_accesses.L2_hits", s)) {
        return PERFEXPERT_ERROR;
    }
*/
    /* instruction_accesses.L2_misses */
/*    bzero(s, MAX_LCPI);
    strcpy(s, "(L2_RQSTS.CODE_RD_MISS * mem_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("instruction_accesses.L2_misses", s)) {
        return PERFEXPERT_ERROR;
    }
*/
    /* data_TLB.overall */
/*    bzero(s, MAX_LCPI);
    strcpy(s, "DTLB_LOAD_MISSES.WALK_DURATION / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("data_TLB.overall", s)) {
        return PERFEXPERT_ERROR;
    }
*/
    /* instruction_TLB.overall */
    bzero(s, MAX_LCPI);
    strcpy(s, "PAGE_WALKS.WALKS / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("instruction_TLB.overall", s)) {
        return PERFEXPERT_ERROR;
    }

    bzero(s, MAX_LCPI);
    strcpy(s, "(PAGE_WALKS.D_SIDE_CYCLES+PAGE_WALKS.I_SIDE_CYCLES) / CPU_CLK_UNHALTED.THREAD_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("instruction_TLB.time", s)) {
        return PERFEXPERT_ERROR;
    }

    /* branch_instructions.overall */
    bzero(s, MAX_LCPI);
    strcpy(s, "((BR_INST_RETIRED.ALL_BRANCHES * BR_lat) "
        "+ (BR_MISP_RETIRED.ALL_BRANCHES * BR_miss_lat)) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("branch_instructions.overall", s)) {
        return PERFEXPERT_ERROR;
    }

    /* branch_instructions.correctly_predicted */
    bzero(s, MAX_LCPI);
    strcpy(s, "(BR_INST_RETIRED.ALL_BRANCHES * BR_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("branch_instructions.correctly_predicted", s)) {
        return PERFEXPERT_ERROR;
    }

    /* branch_instructions.mispredicted */
    bzero(s, MAX_LCPI);
    strcpy(s, "(BR_MISP_RETIRED.ALL_BRANCHES * BR_miss_lat) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("branch_instructions.mispredicted", s)) {
        return PERFEXPERT_ERROR;
    }

    /* FP_instructions.overall */
/*    bzero(s, MAX_LCPI);
    strcpy(s, "((SIMD_FP_256.PACKED_SINGLE + SIMD_FP_256.PACKED_DOUBLE + "
        "FP_COMP_OPS_EXE.X87 + FP_COMP_OPS_EXE.SSE_PACKED_SINGLE + "
        "FP_COMP_OPS_EXE.SSE_PACKED_DOUBLE + "
        "FP_COMP_OPS_EXE.SSE_SCALAR_SINGLE + "
        "FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE) + ARITH.FPU_DIV) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("FP_instructions.overall", s)) {
        return PERFEXPERT_ERROR;
    }
*/
    /* FP_instructions.fast */
/*    bzero(s, MAX_LCPI);
    strcpy(s, "(SIMD_FP_256.PACKED_SINGLE + SIMD_FP_256.PACKED_DOUBLE + "
        "FP_COMP_OPS_EXE.X87 + FP_COMP_OPS_EXE.SSE_PACKED_SINGLE + "
        "FP_COMP_OPS_EXE.SSE_PACKED_DOUBLE + "
        "FP_COMP_OPS_EXE.SSE_SCALAR_SINGLE + "
        "FP_COMP_OPS_EXE.SSE_SCALAR_DOUBLE) / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("FP_instructions.fast", s)) {
        return PERFEXPERT_ERROR;
    }
*/
    /* FP_instructions.slow */
/*    bzero(s, MAX_LCPI);
    strcpy(s, "ARITH.FPU_DIV / INST_RETIRED.ANY_P");
    if (PERFEXPERT_SUCCESS != lcpi_add_metric("FP_instructions.slow", s)) {
        return PERFEXPERT_ERROR;
    }
*/
    return PERFEXPERT_SUCCESS;
}

// EOF
