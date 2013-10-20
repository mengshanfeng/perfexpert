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

#ifndef PERFEXPERT_CONSTANTS_H_
#define PERFEXPERT_CONSTANTS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Return codes that should be used... */
enum {
    PERFEXPERT_UNDEFINED = -2, /* -2 ...when the error is unknown             */
    PERFEXPERT_FAILURE,        /* -1 ...when an error is -1 (GNU standard)    */
    PERFEXPERT_SUCCESS,        /* 0  ...in case of success                    */
    PERFEXPERT_ERROR,          /* 1  ...in case of general error              */
    PERFEXPERT_NO_REC,         /* 2  ...when we run out of recommendation     */
    PERFEXPERT_NO_PAT,         /* 3  ...when no pattern matches               */
    PERFEXPERT_NO_TRANS,       /* 4  ...when transformations can't be applied */
    PERFEXPERT_NO_DATA,        /* 5  ...when analyzer return an error         */
    PERFEXPERT_NO_HOTSPOTS,    /* 6  ...when there no hotspots were found     */
    PERFEXPERT_FORK_ERROR,     /* 7  ...in case of fork error :)              */
};

#define PERFEXPERT_TRUE  1 /* used to return boolean values */
#define PERFEXPERT_FALSE 0 /* used to return boolean values */

/* Buffers size, will be used for:
 * - parsing INPUT file
 * - parsing metrics file
 * - storing SQL statements (including the 'functions')
 * - maybe something else
 */
#define BUFFER_SIZE       4096
#define MAX_LOG_ENTRY     1048576
#define MAX_FRAGMENT_DATA 1048576
#define PARAM_SIZE        128

/* Default values for some files and directories */
#define PERFEXPERT_OUTPUT        "perfexpert.output"
#define PERFEXPERT_DB            "perfexpert.db"
#define PERFEXPERT_FRAGMENTS_DIR "database/src"
#define PERFEXPERT_SOURCE_DIR    "source"
#define PERFEXPERT_LOGFILE       ".perfexpert.log"

#define RECOMMENDER_PROGRAM      "perfexpert_recommender"
#define RECOMMENDER_OUTPUT       "recommender.output"
#define RECOMMENDER_METRICS      "recommender_metrics.txt"
#define RECOMMENDER_REPORT       "recommender_report.txt"

#define ANALYZER_PROGRAM         "perfexpert_analyzer"
#define ANALYZER_OUTPUT          "analyzer.output"
#define ANALYZER_METRICS         "analyzer_metrics.txt"
#define ANALYZER_REPORT          "analyzer_report.txt"
#define ANALYZER_VARIANCE_LIMIT  0.2

#define CT_PROGRAM               "perfexpert_ct"
#define CT_OUTPUT                "ct.output"
#define CT_REPORT                "ct_report.txt"

#define METRICS_TABLE            "metric"
#define METRICS_FILE             "recommender_metrics.txt"
#define LCPI_FILE                "lcpi.conf"
#define MACHINE_FILE             "machine.conf"

/* Default compiler and compilation options */
#define DEFAULT_COMPILER "cc"
#define DEFAULT_CFLAGS   "-g -O3 -fno-inline "
#define DEFAULT_LDFLAGS  ""

/* Hotspots I know */
enum hotspot_type_t {
    PERFEXPERT_HOTSPOT_UNKNOWN = 0,
    PERFEXPERT_HOTSPOT_PROGRAM,
    PERFEXPERT_HOTSPOT_FUNCTION,
    PERFEXPERT_HOTSPOT_LOOP,
};

#ifdef __cplusplus
}
#endif

#endif /* PERFEXPERT_CONSTANTS_H */
