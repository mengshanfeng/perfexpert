/*
 * Copyright (c) 2013  University of Texas at Austin. All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * This file is part of OptTran and PerfExpert.
 *
 * OptTran as well PerfExpert are free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * OptTran and PerfExpert are distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with OptTran or PerfExpert. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Leonardo Fialho
 *
 * $HEADER$
 */

#ifdef __cplusplus
extern "C" {
#endif

/* System standard headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <inttypes.h>

#if HAVE_SQLITE3 == 1
/* Utility headers */
#include <sqlite3.h>
#endif
    
/* OptTran headers */
#include "config.h"
#include "pr.h"
#include "opttran_output.h"
#include "opttran_util.h"

/* Global variables, try to not create them! */
globals_t globals; // Variable to hold global options, this one is OK    

/* main, life starts here */
int main(int argc, char** argv) {
    opttran_list_t *fragments;
    fragment_t *fragment;
    recommendation_t *recommendation;
    recognizer_t *recognizer;
    
    /* Set default values for globals */
    globals = (globals_t) {
        .verbose          = 0,      // int
        .verbose_level    = 0,      // int
        .use_stdin        = 0,      // int
        .use_stdout       = 1,      // int
        .inputfile        = NULL,   // char *
        .outputfile       = NULL,   // char *
        .outputfile_FP    = stdout, // FILE *
        .opttrandir       = NULL,   // char *
#if HAVE_SQLITE3 == 1
        .opttran_pid      = (unsigned long long int)getpid(), // int
#endif
        .testall          = 0,      // int
        .colorful         = 0       // int
    };
    globals.dbfile = (char *)malloc(strlen(RECOMMENDATION_DB) +
                                    strlen(OPTTRAN_VARDIR) + 2);
    if (NULL == globals.dbfile) {
        OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
        exit(OPTTRAN_ERROR);
    }
    bzero(globals.dbfile,
          strlen(RECOMMENDATION_DB) + strlen(OPTTRAN_VARDIR) + 2);
    sprintf(globals.dbfile, "%s/%s", OPTTRAN_VARDIR, RECOMMENDATION_DB);

    /* Parse command-line parameters */
    if (OPTTRAN_SUCCESS != parse_cli_params(argc, argv)) {
        OPTTRAN_OUTPUT(("%s", _ERROR("Error: parsing command line arguments")));
        exit(OPTTRAN_ERROR);
    }
    
    /* Create the list of code patterns */
    fragments = (opttran_list_t *)malloc(sizeof(opttran_list_t));
    if (NULL == fragments) {
        OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
        exit(OPTTRAN_ERROR);
    }
    opttran_list_construct(fragments);

    /* Parse input parameters */
    if (1 == globals.use_stdin) {
        if (OPTTRAN_SUCCESS != parse_fragment_params(fragments, stdin)) {
            OPTTRAN_OUTPUT(("%s", _ERROR("Error: parsing input params")));
            exit(OPTTRAN_ERROR);
        }
    } else {
        if (NULL != globals.inputfile) {
            FILE *inputfile_FP = NULL;
            
            /* Open input file */
            if (NULL == (inputfile_FP = fopen(globals.inputfile, "r"))) {
                OPTTRAN_OUTPUT(("%s (%s)",
                                _ERROR("Error: unable to open input file"),
                                globals.inputfile));
                return OPTTRAN_ERROR;
            } else {
                if (OPTTRAN_SUCCESS != parse_fragment_params(fragments,
                                                             inputfile_FP)) {
                    OPTTRAN_OUTPUT(("%s",
                                    _ERROR("Error: parsing input params")));
                    exit(OPTTRAN_ERROR);
                }
                fclose(inputfile_FP);
            }
        } else {
            OPTTRAN_OUTPUT(("%s", _ERROR("Error: undefined input")));
            show_help();
        }
    }

    /* Test the pattern recognizers */
    if (OPTTRAN_SUCCESS != test_recognizers(fragments)) {
        OPTTRAN_OUTPUT(("%s", _ERROR("Error: testing pattern recognizers")));
        exit(OPTTRAN_ERROR);
    }

    /* Output results */
    if (1 == globals.use_opttran) {
        globals.use_stdout = 0;

        if (NULL == globals.opttrandir) {
            globals.opttrandir = (char *)malloc(strlen("./opttran-") + 8);
            if (NULL == globals.opttrandir) {
                OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
                exit(OPTTRAN_ERROR);
            }
            bzero(globals.opttrandir, strlen("./opttran-" + 8));
            sprintf(globals.opttrandir, "./opttran-%d", getpid());
        }
        OPTTRAN_OUTPUT_VERBOSE((7, "using (%s) as output directory",
                                globals.opttrandir));

        if (OPTTRAN_ERROR == opttran_util_make_path(globals.opttrandir, 0755)) {
            OPTTRAN_OUTPUT(("%s",
                            _ERROR("Error: cannot create opttran directory")));
            exit(OPTTRAN_ERROR);
        }

        globals.outputfile = (char *)malloc(strlen(globals.opttrandir) +
                                            strlen(OPTTRAN_PR_FILE) + 1);
        if (NULL == globals.outputfile) {
            OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
            exit(OPTTRAN_ERROR);
        }
        bzero(globals.outputfile, strlen(globals.opttrandir) +
              strlen(OPTTRAN_PR_FILE) + 1);
        strcat(globals.outputfile, globals.opttrandir);
        strcat(globals.outputfile, "/");
        strcat(globals.outputfile, OPTTRAN_PR_FILE);
        OPTTRAN_OUTPUT_VERBOSE((7, "printing OPTTRAN output to (%s)",
                                globals.opttrandir));
    }

    if (0 == globals.use_stdout) {
        OPTTRAN_OUTPUT_VERBOSE((7, "printing test results to file (%s)",
                                globals.outputfile));
        globals.outputfile_FP = fopen(globals.outputfile, "w+");
        if (NULL == globals.outputfile_FP) {
            OPTTRAN_OUTPUT(("%s (%s)",
                            _ERROR("Error: unable to open output file"),
                            globals.outputfile));
            return OPTTRAN_ERROR;
        }
    } else {
        OPTTRAN_OUTPUT_VERBOSE((7, "printing test results to STDOUT"));
    }

    if (OPTTRAN_SUCCESS != output_results(fragments)) {
        OPTTRAN_OUTPUT(("%s", _ERROR("Error: outputting results")));
        exit(OPTTRAN_ERROR);
    }
    if (0 == globals.use_stdout) {
        fclose(globals.outputfile_FP);
    }

    /* Free memory */
    while (OPTTRAN_FALSE == opttran_list_is_empty(fragments)) {
        fragment = (fragment_t *)opttran_list_get_first(fragments);
        opttran_list_remove_item(fragments, (opttran_list_item_t *)fragment);
        while (OPTTRAN_FALSE == opttran_list_is_empty(&(fragment->recommendations))) {
            recommendation = (recommendation_t *)opttran_list_get_first(&(fragment->recommendations));
            opttran_list_remove_item(&(fragment->recommendations),
                                     (opttran_list_item_t *)recommendation);
            while (OPTTRAN_FALSE == opttran_list_is_empty(&(recommendation->recognizers))) {
                recognizer = (recognizer_t *)opttran_list_get_first(&(recommendation->recognizers));
                opttran_list_remove_item(&(recommendation->recognizers),
                                         (opttran_list_item_t *)recognizer);
                free(recognizer->program);
                free(recognizer);
            }
            free(recommendation);
        }
        free(fragment->filename);
        free(fragment->code_type);
        free(fragment->fragment_file);
        free(fragment->outer_loop_fragment_file);
        free(fragment->outer_outer_loop_fragment_file);
        free(fragment);
    }
    opttran_list_destruct(fragments);
    free(fragments);
    free(globals.dbfile);
    if (1 == globals.use_opttran) {
        free(globals.outputfile);
    }

    return OPTTRAN_SUCCESS;
}

/* show_help */
static void show_help(void) {
    OPTTRAN_OUTPUT_VERBOSE((10, "printing help"));
    
    /*      12345678901234567890123456789012345678901234567890123456789012345678901234567890 */
    printf("Usage: opttran_pr -i|-f file [-o file] [-tvch] [-l level] [-a dir]");
#if HAVE_SQLITE3 == 1
    printf(" [-d database] [-p pid]");
#endif
    printf("\n");
    printf("  -i --stdin           Use STDIN as input for patterns\n");
    printf("  -f --inputfile       Use 'file' as input for patterns\n");
    printf("  -o --outputfile      Use 'file' as output (default stdout)\n");
    printf("  -t --testall         Test all the pattern recognizers of each code fragment,\n");
    printf("                       otherwise stop on the first valid one\n");
    printf("  -a --opttran         Create OptTran (automatic performance optimization) files\n");
    printf("                       into 'dir' directory (default: create no OptTran files).\n");
    printf("                       This argument overwrites -o (no output on STDOUT, except\n");
    printf("                       for verbose messages)\n");
#if HAVE_SQLITE3 == 1
    printf("  -d --database        Select the recommendation database file\n");
    printf("                       (default: %s/%s)\n", OPTTRAN_VARDIR, RECOMMENDATION_DB);
    printf("  -p --opttranid       Use 'pid' to log on DB consecutive calls to Recommender\n");
#endif
    printf("  -v --verbose         Enable verbose mode using default verbose level (5)\n");
    printf("  -l --verbose_level   Enable verbose mode using a specific verbose level (1-10)\n");
    printf("  -c --colorful        Enable colors on verbose mode, no weird characters will\n");
    printf("                       appear on output files\n");
    printf("  -h --help            Show this message\n");
    
    /* I suppose that if I've to show the help is because something is wrong,
     * or maybe the user just want to see the options, so it seems to be a
     * good idea to exit here with an error code.
     */
    exit(OPTTRAN_ERROR);
}

/* parse_env_vars */
static int parse_env_vars(void) {
    // TODO: add here all the parameter we have for command line arguments
    char *temp_str;
    
    /* Get the variables */
    temp_str = getenv("OPTTRAN_VERBOSE_LEVEL");
    if (NULL != temp_str) {
        globals.verbose_level = atoi(temp_str);
        if (0 != globals.verbose_level) {
            OPTTRAN_OUTPUT_VERBOSE((5, "ENV: verbose_level=%d",
                                    globals.verbose_level));
        }
    }
    
    OPTTRAN_OUTPUT_VERBOSE((4, "=== %s", _BLUE("Environment variables")));
    
    return OPTTRAN_SUCCESS;
}

/* parse_cli_params */
static int parse_cli_params(int argc, char *argv[]) {
    /** Temporary variable to hold parameter */
    int parameter;
    /** getopt_long() stores the option index here */
    int option_index = 0;
    
    /* If some environment variable is defined, use it! */
    if (OPTTRAN_SUCCESS != parse_env_vars()) {
        OPTTRAN_OUTPUT(("%s", _ERROR("Error: parsing environment variables")));
        exit(OPTTRAN_ERROR);
    }
    
    while (1) {
        /* get parameter */
#if HAVE_SQLITE3 == 1
        parameter = getopt_long(argc, argv, "a:cvhif:l:o:p:t", long_options,
                                &option_index);
#else
        parameter = getopt_long(argc, argv, "a:cvhif:l:o:t", long_options,
                                &option_index);
#endif
        /* Detect the end of the options */
        if (-1 == parameter) {
            break;
        }
        
        switch (parameter) {
            /* Verbose level */
            case 'l':
                globals.verbose = 1;
                globals.verbose_level = atoi(optarg);
                OPTTRAN_OUTPUT_VERBOSE((10, "option 'l' set"));
                if (0 >= atoi(optarg)) {
                    OPTTRAN_OUTPUT(("%s (%d)",
                                    _ERROR("Error: invalid debug level (too low)"),
                                    atoi(optarg)));
                    show_help();
                }
                if (10 < atoi(optarg)) {
                    OPTTRAN_OUTPUT(("%s (%d)",
                                    _ERROR("Error: invalid debug level (too high)"),
                                    atoi(optarg)));
                    show_help();
                }
                break;
                
            /* Activate verbose mode */
            case 'v':
                globals.verbose = 1;
                if (0 == globals.verbose_level) {
                    globals.verbose_level = 5;
                }
                OPTTRAN_OUTPUT_VERBOSE((10, "option 'v' set"));
                break;
#if HAVE_SQLITE3 == 1
            /* Which database file? */
            case 'd':
                globals.dbfile = optarg;
                OPTTRAN_OUTPUT_VERBOSE((10, "option 'd' set [%s]",
                                        globals.dbfile));
                break;

            /* Specify OptTran PID */
            case 'p':
                globals.opttran_pid = strtoull(optarg, (char **)NULL, 10);
                OPTTRAN_OUTPUT_VERBOSE((10, "option 'p' set [%llu]",
                                        globals.opttran_pid));
                break;
#endif
            /* Activate colorful mode */
            case 'c':
                globals.colorful = 1;
                OPTTRAN_OUTPUT_VERBOSE((10, "option 'c' set"));
                break;
                
            /* Show help */
            case 'h':
                OPTTRAN_OUTPUT_VERBOSE((10, "option 'h' set"));
                show_help();
                
            /* Use STDIN? */
            case 'i':
                globals.use_stdin = 1;
                OPTTRAN_OUTPUT_VERBOSE((10, "option 'i' set"));
                break;
                
            /* Use input file? */
            case 'f':
                globals.use_stdin = 0;
                globals.inputfile = optarg;
                OPTTRAN_OUTPUT_VERBOSE((10, "option 'f' set [%s]",
                                        globals.inputfile));
                break;

            /* Use output file? */
            case 'o':
                globals.use_stdout = 0;
                globals.outputfile = optarg;
                OPTTRAN_OUTPUT_VERBOSE((10, "option 'o' set [%s]",
                                        globals.outputfile));
                break;

            /* Use opttran? */
            case 'a':
                globals.use_opttran = 1;
                globals.use_stdout = 0;
                globals.opttrandir = optarg;
                OPTTRAN_OUTPUT_VERBOSE((10, "option 'a' set [%s]",
                                        globals.opttrandir));
                break;
                
            /* Test all or stop on the first valid? */
            case 't':
                globals.testall = 1;
                OPTTRAN_OUTPUT_VERBOSE((10, "option 't' set"));
                break;

            /* Unknown option */
            case '?':
                show_help();
                
            default:
                exit(OPTTRAN_ERROR);
        }
    }
    OPTTRAN_OUTPUT_VERBOSE((4, "=== %s", _BLUE("CLI params")));
    OPTTRAN_OUTPUT_VERBOSE((10, "Summary of selected options:"));
    OPTTRAN_OUTPUT_VERBOSE((10, "   Verbose:          %s",
                            globals.verbose ? "yes" : "no"));
    OPTTRAN_OUTPUT_VERBOSE((10, "   Verbose level:    %d",
                            globals.verbose_level));
    OPTTRAN_OUTPUT_VERBOSE((10, "   Colorful verbose? %s",
                            globals.colorful ? "yes" : "no"));
    OPTTRAN_OUTPUT_VERBOSE((10, "   Use STDOUT?       %s",
                            globals.use_stdout ? "yes" : "no"));
    OPTTRAN_OUTPUT_VERBOSE((10, "   Use STDIN?        %s",
                            globals.use_stdin ? "yes" : "no"));
    OPTTRAN_OUTPUT_VERBOSE((10, "   Input file:       %s",
                            globals.inputfile ? globals.inputfile : "(null)"));
    OPTTRAN_OUTPUT_VERBOSE((10, "   Output file:      %s",
                            globals.outputfile ? globals.outputfile : "(null)"));
    OPTTRAN_OUTPUT_VERBOSE((10, "   Test all?         %s",
                            globals.testall ? "yes" : "no"));
    OPTTRAN_OUTPUT_VERBOSE((10, "   Use OPTTRAN?      %s",
                            globals.use_opttran ? "yes" : "no"));
    OPTTRAN_OUTPUT_VERBOSE((10, "   OPTTRAN PID:      %llu",
                            globals.opttran_pid));
    OPTTRAN_OUTPUT_VERBOSE((10, "   Database file:    %s",
                            globals.dbfile ? globals.dbfile : "(null)"));

    /* Not using OPTTRAN_OUTPUT_VERBOSE because I want only one line */
    if (8 <= globals.verbose_level) {
        int i;
        printf("%s complete command line:", PROGRAM_PREFIX);
        for (i = 0; i < argc; i++) {
            printf(" %s", argv[i]);
        }
        printf("\n");
    }
    
    return OPTTRAN_SUCCESS;
}

/* parse_fragment_params */
static int parse_fragment_params(opttran_list_t *fragments_p, FILE *inputfile_p) {
    fragment_t *fragment;
    recommendation_t *recommendation;
    recognizer_t *recognizer;
    char buffer[BUFFER_SIZE];
    int  input_line = 0;
    
    OPTTRAN_OUTPUT_VERBOSE((4, "=== %s", _BLUE("Parsing fragments file")));
    
    /* Which INPUT we are using? (just a double check) */
    if ((NULL == inputfile_p) && (globals.use_stdin)) {
        inputfile_p = stdin;
    }
    if (globals.use_stdin) {
        OPTTRAN_OUTPUT_VERBOSE((3, "using STDIN as input for fragments"));
    } else {
        OPTTRAN_OUTPUT_VERBOSE((3, "using (%s) as input for fragments",
                                globals.inputfile));
    }
    
    /* For each line in the INPUT file... */
    OPTTRAN_OUTPUT_VERBOSE((7, "--- parsing input file"));
    
    bzero(buffer, BUFFER_SIZE);
    while (NULL != fgets(buffer, BUFFER_SIZE - 1, inputfile_p)) {
        node_t *node;
        int temp;
        
        input_line++;
        
        /* Ignore comments */
        if (0 == strncmp("#", buffer, 1)) {
            continue;
        }
        
        /* Is this line a new recommendation? */
        if (0 == strncmp("%", buffer, 1)) {
            char temp_str[BUFFER_SIZE];
            
            OPTTRAN_OUTPUT_VERBOSE((5, "(%d) --- %s", input_line,
                                    _GREEN("new fragment found")));
            
            /* Create a list item for this code bottleneck */
            fragment = (fragment_t *)malloc(sizeof(fragment_t));
            if (NULL == fragment) {
                OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
                exit(OPTTRAN_ERROR);
            }
            opttran_list_item_construct((opttran_list_item_t *)fragment);
            
            /* Initialize some elements on segment */
            fragment->filename = NULL;
            fragment->line_number = 0;
            fragment->code_type = NULL;
            fragment->loop_depth = 0;
            fragment->fragment_file = NULL;
            fragment->outer_loop_fragment_file = NULL;
            fragment->outer_outer_loop_fragment_file = NULL;
            fragment->outer_loop = 0;
            fragment->outer_outer_loop = 0;
            opttran_list_construct((opttran_list_t *)&(fragment->recommendations));

            /* Add this item to 'segments' */
            opttran_list_append(fragments_p, (opttran_list_item_t *)fragment);
            
            continue;
        }
        
        node = (node_t *)malloc(sizeof(node_t) + strlen(buffer) + 1);
        if (NULL == node) {
            OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
            exit(OPTTRAN_ERROR);
        }
        bzero(node, sizeof(node_t) + strlen(buffer) + 1);
        node->key = strtok(strcpy((char*)(node + 1), buffer), "=\r\n");
        node->value = strtok(NULL, "\r\n");
        
        /* OK, now it is time to check which parameter is this, and add it to
         * 'patterns'.
         */
        
        /* Code param: code.filename */
        if (0 == strncmp("code.filename", node->key, 13)) {
            fragment->filename = (char *)malloc(strlen(node->value) + 1);
            if (NULL == fragment->filename) {
                OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
                exit(OPTTRAN_ERROR);
            }
            bzero(fragment->filename, strlen(node->value) + 1);
            strcpy(fragment->filename, node->value);
            OPTTRAN_OUTPUT_VERBOSE((10, "(%d)  \\- %s [%s]", input_line,
                                    _MAGENTA("filename:"), fragment->filename));
            free(node);
            continue;
        }
        /* Code param: code.line_number */
        if (0 == strncmp("code.line_number", node->key, 16)) {
            fragment->line_number = atoi(node->value);
            OPTTRAN_OUTPUT_VERBOSE((10, "(%d)  \\- %s [%d]", input_line,
                                    _MAGENTA("line number:"),
                                    fragment->line_number));
            free(node);
            continue;
        }
        /* Code param: code.type */
        if (0 == strncmp("code.type", node->key, 9)) {
            fragment->code_type = (char *)malloc(strlen(node->value) + 1);
            if (NULL == fragment->code_type) {
                OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
                exit(OPTTRAN_ERROR);
            }
            bzero(fragment->code_type, strlen(node->value) + 1);
            strcpy(fragment->code_type, node->value);
            OPTTRAN_OUTPUT_VERBOSE((10, "(%d)  \\- %s [%s]", input_line,
                                    _MAGENTA("type:"), fragment->code_type));
            free(node);
            continue;
        }
        /* Code param: code.loop_depth */
        if (0 == strncmp("code.loop_depth", node->key, 15)) {
            fragment->loop_depth = atoi(node->value);
            OPTTRAN_OUTPUT_VERBOSE((10, "(%d)  \\- %s [%d]",
                                    input_line, _MAGENTA("loop depth:"),
                                    fragment->loop_depth));
            free(node);
            continue;
        }
        /* Code param: code.outer_loop */
        if (0 == strncmp("code.outer_loop", node->key, 15)) {
            fragment->outer_loop = atoi(node->value);
            OPTTRAN_OUTPUT_VERBOSE((10, "(%d)  \\- %s [%d]", input_line,
                                    _MAGENTA("outer loop:"),
                                    fragment->outer_loop));
            free(node);
            continue;
        }
        /* Code param: code.outer_outer_loop */
        if (0 == strncmp("code.outer_outer_loop", node->key, 21)) {
            fragment->outer_outer_loop = atoi(node->value);
            OPTTRAN_OUTPUT_VERBOSE((10, "(%d)  \\- %s [%d]", input_line,
                                    _MAGENTA("outer outer loop:"),
                                    fragment->outer_outer_loop));
            free(node);
            continue;
        }
        /* Recommender param: recommender.code_fragment */
        if (0 == strncmp("recommender.code_fragment", node->key, 25)) {
            fragment->fragment_file = (char *)malloc(strlen(node->value) + 1);
            if (NULL == fragment->fragment_file) {
                OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
                exit(OPTTRAN_ERROR);
            }
            bzero(fragment->fragment_file, strlen(node->value) + 1);
            strcpy(fragment->fragment_file, node->value);
            OPTTRAN_OUTPUT_VERBOSE((10, "(%d)  \\- %s [%s]", input_line,
                                    _MAGENTA("fragment file:"),
                                    fragment->fragment_file));
            free(node);
            continue;
        }
        /* Recommender param: recommender.outer_loop_fragment */
        if (0 == strncmp("recommender.outer_loop_fragment", node->key, 31)) {
            fragment->outer_loop_fragment_file = (char *)malloc(strlen(node->value) + 1);
            if (NULL == fragment->outer_loop_fragment_file) {
                OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
                exit(OPTTRAN_ERROR);
            }
            bzero(fragment->outer_loop_fragment_file, strlen(node->value) + 1);
            strcpy(fragment->outer_loop_fragment_file, node->value);
            OPTTRAN_OUTPUT_VERBOSE((10, "(%d)  \\- %s [%s]", input_line,
                                    _MAGENTA("outer loop fragment file:"),
                                    fragment->outer_loop_fragment_file));
            free(node);
            continue;
        }
        /* Recommender param: recommender.outer_outer_loop_fragment */
        if (0 == strncmp("recommender.outer_outer_loop_fragment", node->key, 31)) {
            fragment->outer_outer_loop_fragment_file = (char *)malloc(strlen(node->value) + 1);
            if (NULL == fragment->outer_outer_loop_fragment_file) {
                OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
                exit(OPTTRAN_ERROR);
            }
            bzero(fragment->outer_outer_loop_fragment_file, strlen(node->value) + 1);
            strcpy(fragment->outer_outer_loop_fragment_file, node->value);
            OPTTRAN_OUTPUT_VERBOSE((10, "(%d)  \\- %s [%s]", input_line,
                                    _MAGENTA("outer loop fragment file:"),
                                    fragment->outer_outer_loop_fragment_file));
            free(node);
            continue;
        }
        /* It is expected that after a 'recommender.recommendation_id' there
         * will be a list of recognizer (no recognizers is also valid).
         */
        /* Recommender param: recommender.recommendation_id */
        if (0 == strncmp("recommender.recommendation_id", node->key, 29)) {
            recommendation = (recommendation_t *)malloc(sizeof(recommendation_t));
            if (NULL == recommendation) {
                OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
                exit(OPTTRAN_ERROR);
            }
            opttran_list_item_construct((opttran_list_item_t *)recommendation);
            recommendation->id = atoi(node->value);
            opttran_list_construct((opttran_list_t *)&(recommendation->recognizers));
            opttran_list_append((opttran_list_t *)&(fragment->recommendations),
                                (opttran_list_item_t *)recommendation);
            OPTTRAN_OUTPUT_VERBOSE((10, "(%d)  | \\- %s [%d]", input_line,
                                    _YELLOW("recommendation ID:"),
                                    recommendation->id));
            free(node);
            continue;
        }
        /* If there is a new recognizer, the ID should become first, them the
         * recognizer name. This allow us to correctly allocate the struct mem.
         */
        /* Recommender param: recommender.recognizer_id */
        if (0 == strncmp("recommender.recognizer_id", node->key, 25)) {
            recognizer = (recognizer_t *)malloc(sizeof(recognizer_t));
            if (NULL == recognizer) {
                OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
                exit(OPTTRAN_ERROR);
            }
            opttran_list_item_construct((opttran_list_item_t *)recognizer);
            recognizer->id = atoi(node->value);
            recognizer->test_result = OPTTRAN_UNDEFINED;
            opttran_list_append((opttran_list_t *)&(recommendation->recognizers),
                                (opttran_list_item_t *)recognizer);
            OPTTRAN_OUTPUT_VERBOSE((10, "(%d)  | | \\- %s [%d]", input_line,
                                    _CYAN("recognizer ID:"), recognizer->id));
            free(node);
            continue;
        }
        /* Recommender param: recommender.recognizer */
        if (0 == strncmp("recommender.recognizer", node->key, 22)) {
            recognizer->program = (char *)malloc(strlen(node->value) + 1);
            if (NULL == recognizer->program) {
                OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
                exit(OPTTRAN_ERROR);
            }
            bzero(recognizer->program, strlen(node->value) + 1);
            strcpy(recognizer->program, node->value);
            OPTTRAN_OUTPUT_VERBOSE((10, "(%d)  | | \\- %s [%s]", input_line,
                                    _CYAN("recognizer:"), recognizer->program));
            free(node);
            continue;
        }
        
        /* Should never reach this point, only if there is garbage in the input
         * file.
         */
        OPTTRAN_OUTPUT_VERBOSE((4, "(%d) %s (%s = %s)", input_line,
                                _RED("ignored line"), node->key, node->value));
        free(node);
    }
    
    /* print a summary of 'segments' */
    OPTTRAN_OUTPUT_VERBOSE((4, "%d %s", opttran_list_get_size(fragments_p),
                            _GREEN("code fragment(s) found")));
    
    fragment = (fragment_t *)opttran_list_get_first(fragments_p);
    while ((opttran_list_item_t *)fragment != &(fragments_p->sentinel)) {
        OPTTRAN_OUTPUT_VERBOSE((4, "   %s:%d", fragment->filename,
                                fragment->line_number));
        fragment = (fragment_t *)opttran_list_get_next(fragment);
    }
    
    OPTTRAN_OUTPUT_VERBOSE((4, "==="));
    
    return OPTTRAN_SUCCESS;
}

/* test_recognizers */
static int test_recognizers(opttran_list_t *fragments_p) {
    opttran_list_t *recommendations;
    opttran_list_t *recognizers;
    recommendation_t *recommendation;
    recognizer_t *recognizer;
    fragment_t *fragment;
    opttran_list_t *tests;
    test_t *test;

    tests = (opttran_list_t *)malloc(sizeof(opttran_list_t));
    if (NULL == tests) {
        OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
        exit(OPTTRAN_ERROR);
    }
    opttran_list_construct(tests);

    OPTTRAN_OUTPUT_VERBOSE((4, "=== %s", _BLUE("Testing pattern recognizers")));

    OPTTRAN_OUTPUT_VERBOSE((8, "creating a list of tests to run..."));

    /* Create a list of all pattern recognizers we have to test */
    fragment = (fragment_t *)opttran_list_get_first(fragments_p);
    while ((opttran_list_item_t *)fragment != &(fragments_p->sentinel)) {
        /* For all code fragments ... */
        recommendations = (opttran_list_t *)&(fragment->recommendations);
        recommendation = (recommendation_t *)opttran_list_get_first(&(fragment->recommendations));
        while ((opttran_list_item_t *)recommendation != &(recommendations->sentinel)) {
            /* For all recommendations ... */
            recognizers = (opttran_list_t *)&(recommendation->recognizers);
            recognizer = (recognizer_t *)opttran_list_get_first(&(recommendation->recognizers));
            while ((opttran_list_item_t *)recognizer != &(recognizers->sentinel)) {
                /* For all fragment recognizers ... */
                test = (test_t *)malloc(sizeof(test_t));
                if (NULL == test) {
                    OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
                    exit(OPTTRAN_ERROR);
                }
                opttran_list_item_construct((opttran_list_item_t *)test);
                test->program = recognizer->program;
                test->fragment_file = fragment->fragment_file;
                test->test_result = &(recognizer->test_result);

                OPTTRAN_OUTPUT_VERBOSE((10, "[%s] %s", test->program,
                                        test->fragment_file));

                /* Add this item to to-'tests' */
                opttran_list_append(tests, (opttran_list_item_t *)test);

                /* It we're testing for a loop, check for the outer loop */
                if ((0 == strncmp("loop", fragment->code_type, 4)) &&
                    (2 <= fragment->loop_depth) &&
                    (0 != fragment->outer_loop) &&
                    (NULL != fragment->outer_loop_fragment_file)) {

                    /* Add the outer loop test */
                    test = (test_t *)malloc(sizeof(test_t));
                    if (NULL == test) {
                        OPTTRAN_OUTPUT(("%s", _ERROR("Error: out of memory")));
                        exit(OPTTRAN_ERROR);
                    }
                    opttran_list_item_construct((opttran_list_item_t *)test);
                    test->program = recognizer->program;
                    test->fragment_file = fragment->outer_loop_fragment_file;
                    test->test_result = &(recognizer->test2_result);

                    OPTTRAN_OUTPUT_VERBOSE((10, "[%s] %s", test->program,
                                            test->fragment_file));

                    /* Add this item to to-'tests' */
                    opttran_list_append(tests, (opttran_list_item_t *)test);
                    
                    /* And test for the outer outer loop too */
                    if ((3 <= fragment->loop_depth) &&
                        (0 != fragment->outer_outer_loop) &&
                        (NULL != fragment->outer_outer_loop_fragment_file)) {

                        /* Add the outer outer loop test */
                        test = (test_t *)malloc(sizeof(test_t));
                        if (NULL == test) {
                            OPTTRAN_OUTPUT(("%s",
                                            _ERROR("Error: out of memory")));
                            exit(OPTTRAN_ERROR);
                        }
                        opttran_list_item_construct((opttran_list_item_t *)test);
                        test->program = recognizer->program;
                        test->fragment_file = fragment->outer_outer_loop_fragment_file;
                        test->test_result = &(recognizer->test3_result);

                        OPTTRAN_OUTPUT_VERBOSE((10, "[%s] %s", test->program,
                                                test->fragment_file));

                        /* Add this item to to-'tests' */
                        opttran_list_append(tests, (opttran_list_item_t *)test);
                    }
                }
                recognizer = (recognizer_t *)opttran_list_get_next(recognizer);
            }
            recommendation = (recommendation_t *)opttran_list_get_next(recommendation);
        }
        fragment = (fragment_t *)opttran_list_get_next(fragment);
    }
    OPTTRAN_OUTPUT_VERBOSE((8, "...done!"));

    /* Print a summary of 'tests' */
    OPTTRAN_OUTPUT_VERBOSE((4, "%d %s", opttran_list_get_size(tests),
                            _GREEN("test(s) should be run")));

    /* Run the tests */
    test = (test_t *)opttran_list_get_first(tests);
    while ((opttran_list_item_t *)test != &(tests->sentinel)) {
        if (OPTTRAN_SUCCESS != test_one(test)) {
            OPTTRAN_OUTPUT(("   %s [%s] >> [%s]", _RED("Error: running test"),
                            test->program, test->fragment_file));
        }

        switch ((int)*(test->test_result)) {
            case OPTTRAN_UNDEFINED:
                OPTTRAN_OUTPUT_VERBOSE((8, "   %s [%s] >> [%s]",
                                        _BOLDRED("UNDEF"), test->program,
                                        test->fragment_file));
                break;

            case OPTTRAN_FAILURE:
                OPTTRAN_OUTPUT_VERBOSE((8, "   %s  [%s] >> [%s]",
                                        _ERROR("FAIL"), test->program,
                                        test->fragment_file));
                break;
                
            case OPTTRAN_SUCCESS:
                OPTTRAN_OUTPUT_VERBOSE((8, "   %s    [%s] >> [%s]",
                                        _BOLDGREEN("OK"), test->program,
                                        test->fragment_file));
                break;
                
            case OPTTRAN_ERROR:
                OPTTRAN_OUTPUT_VERBOSE((8, "   %s [%s] >> [%s]",
                                        _BOLDYELLOW("ERROR"), test->program,
                                        test->fragment_file));
                break;
                
            default:
                break;
        }
        
        /* Break the loop if 'testall' is not set */
        if ((0 == globals.testall) &&
            (OPTTRAN_SUCCESS == (int)*(test->test_result))) {
            OPTTRAN_OUTPUT_VERBOSE((7, "   %s", _YELLOW("passed test")));
            break;
        }
        
        /* Move on to the next test... */
        test = (test_t *)opttran_list_get_next(test);
    }
    /* Free 'tests' structure' */
    while (OPTTRAN_FALSE == opttran_list_is_empty(tests)) {
        test = (test_t *)opttran_list_get_first(tests);
        opttran_list_remove_item(tests, (opttran_list_item_t *)test);
        free(test); // Some version of GCC will complain about this
    }
    opttran_list_destruct(tests);
    // TODO: in case of empty lists (no recognizers) this causes a segfault :-/
    free(tests);

    OPTTRAN_OUTPUT_VERBOSE((4, "==="));

    return OPTTRAN_SUCCESS;
}

/* test_one */
static int test_one(test_t *test) {
    int  pipe1[2];
    int  pipe2[2];
    int  pid = 0;
    int  file = 0;
    int  r_bytes = 0;
    int  w_bytes = 0;
    int  rc = OPTTRAN_UNDEFINED;
    char temp_str[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];

#define	PARENT_READ  pipe1[0]
#define	CHILD_WRITE  pipe1[1]
#define CHILD_READ   pipe2[0]
#define PARENT_WRITE pipe2[1]
    
    /* Creating pipes */
    if (-1 == pipe(pipe1)) {
        OPTTRAN_OUTPUT(("%s", _ERROR("Error: unable to create pipe1")));
        return OPTTRAN_ERROR;
    }
    if (-1 == pipe(pipe2)) {
        OPTTRAN_OUTPUT(("%s", _ERROR("Error: unable to create pipe2")));
        return OPTTRAN_ERROR;
    }
    
    /* Forking child */
    pid = fork();
    if (-1 == pid) {
        OPTTRAN_OUTPUT(("%s", _ERROR("Error: unable to fork")));
        return OPTTRAN_ERROR;
    }

    if (0 == pid) {
        /* Child */
        bzero(temp_str, BUFFER_SIZE);
        sprintf(temp_str, "%s/%s", OPTTRAN_BINDIR, test->program);
        OPTTRAN_OUTPUT_VERBOSE((10, "   running %s", _CYAN(temp_str)));
        
        close(PARENT_WRITE);
        close(PARENT_READ);

        if (-1 == dup2(CHILD_READ, STDIN_FILENO)) {
            OPTTRAN_OUTPUT(("%s", _ERROR("Error: unable to DUP STDIN")));
            return OPTTRAN_ERROR;
        }
        if (-1 == dup2(CHILD_WRITE, STDOUT_FILENO)) {
            OPTTRAN_OUTPUT(("%s", _ERROR("Error: unable to DUP STDOUT")));
            return OPTTRAN_ERROR;
        }

        execl(temp_str, test->program, NULL);
        
        OPTTRAN_OUTPUT(("child process failed to run, check if program exists"));
        exit(127);
    } else {
        /* Parent */
        close(CHILD_READ);
        close(CHILD_WRITE);
        
        /* Open input file and sent if to the child process */
        if (-1 == (file = open(test->fragment_file, O_RDONLY))) {
            OPTTRAN_OUTPUT(("%s (%s)",
                            _ERROR("Error: unable to open fragment file"),
                            test->fragment_file));
            return OPTTRAN_ERROR;
        } else {
            bzero(buffer, BUFFER_SIZE);
            while (0 != (r_bytes = read(file, buffer, BUFFER_SIZE))) {
                w_bytes = write(PARENT_WRITE, buffer, r_bytes);
                bzero(buffer, BUFFER_SIZE);
            }
            close(file);
            close(PARENT_WRITE);
        }
        
        /* Read child process' answer and write it to output file */
        bzero(temp_str, BUFFER_SIZE);
        sprintf(temp_str, "%s.%s.recognizer_output", test->fragment_file,
                test->program);
        OPTTRAN_OUTPUT_VERBOSE((10, "   output  %s", _CYAN(temp_str)));

        if (-1 == (file = open(temp_str, O_CREAT|O_WRONLY, 0644))) {
            OPTTRAN_OUTPUT(("%s (%s)",
                            _ERROR("Error: unable to open output file"),
                            temp_str));
            return OPTTRAN_ERROR;
        } else {
            bzero(buffer, BUFFER_SIZE);
            while (0 != (r_bytes = read(PARENT_READ, buffer, BUFFER_SIZE))) {
                w_bytes = write(file, buffer, r_bytes);
                bzero(buffer, BUFFER_SIZE);
            }
            close(file);
            close(PARENT_READ);
        }
        wait(&rc);
        OPTTRAN_OUTPUT_VERBOSE((10, "   result  %s %d", _CYAN("return code"),
                                rc >> 8));
    }

    /* Evaluating the result */
    switch (rc >> 8) {
        /* The pattern matches */
        case 0:
            *test->test_result = OPTTRAN_SUCCESS;
            break;

        /* The pattern doesn't match */
        case 255:
            *test->test_result = OPTTRAN_ERROR;
            break;
        
        /* Error during fork() or waitpid() */
        case -1:
            *test->test_result = OPTTRAN_FAILURE;
            break;
        
        /* Execution failed */
        case 127:
            *test->test_result = OPTTRAN_FAILURE;
            break;
        
        /* Not sure what happened */
        default:
            *test->test_result = OPTTRAN_UNDEFINED;
            break;
    }

    return OPTTRAN_SUCCESS;
}

/* output results */
static int output_results(opttran_list_t *fragments_p) {
    opttran_list_t *recommendations;
    opttran_list_t *recognizers;
    recommendation_t *recommendation;
    recognizer_t *recognizer;
    fragment_t *fragment;
#if HAVE_SQLITE3 == 1
    int  r_bytes = 0;
    int  fragment_FP;
    char sql[MAX_FRAGMENT_DATA];
    char *error_msg = NULL;
    char temp_str[MAX_FRAGMENT_DATA/4];
    char fragment_data[MAX_FRAGMENT_DATA/4];
    char parent_fragment_data[MAX_FRAGMENT_DATA/4];
    char grandparent_fragment_data[MAX_FRAGMENT_DATA/4];
#endif

    OPTTRAN_OUTPUT_VERBOSE((4, "=== %s", _BLUE("Outputting results")));

    /* Output tests result */
    fragment = (fragment_t *)opttran_list_get_first(fragments_p);
    while ((opttran_list_item_t *)fragment != &(fragments_p->sentinel)) {
        /* For all code fragments ... */
        recommendations = (opttran_list_t *)&(fragment->recommendations);
        recommendation = (recommendation_t *)opttran_list_get_first(&(fragment->recommendations));

        if (0 == globals.use_stdout) {
            fprintf(globals.outputfile_FP, "%% transformation for %s:%d\n",
                    fragment->filename, fragment->line_number);
        } else {
            fprintf(globals.outputfile_FP,
                    "#--------------------------------------------------\n");
            fprintf(globals.outputfile_FP, "# Transformations for %s:%d\n",
                    fragment->filename, fragment->line_number);
            fprintf(globals.outputfile_FP,
                    "#--------------------------------------------------\n");
            fprintf(globals.outputfile_FP, "Fragment file: %s\n",
                    fragment->fragment_file);
        }

        while ((opttran_list_item_t *)recommendation != &(recommendations->sentinel)) {
            /* For all recommendations ... */
            recognizers = (opttran_list_t *)&(recommendation->recognizers);
            recognizer = (recognizer_t *)opttran_list_get_first(&(recommendation->recognizers));
            while ((opttran_list_item_t *)recognizer != &(recognizers->sentinel)) {
                /* For all fragment recognizers ... */
                if (0 == globals.use_stdout) {
                    if (OPTTRAN_SUCCESS == recognizer->test_result) {
                        fprintf(globals.outputfile_FP,
                                "recommender.code_fragment=%s\n",
                                fragment->fragment_file);
                        fprintf(globals.outputfile_FP, "pr.transformation=%s\n",
                                recognizer->program);
                    }
                    /* If it is a loop, check parent loop test result */
                    if ((0 == strncmp(fragment->code_type, "loop", 4)) &&
                        (OPTTRAN_SUCCESS == recognizer->test2_result)) {
                        fprintf(globals.outputfile_FP,
                                "recommender.code_fragment=%s\n",
                                fragment->outer_loop_fragment_file);
                        fprintf(globals.outputfile_FP, "pr.transformation=%s\n",
                                recognizer->program);
                    }
                    /* If it is a loop, check grandparent loop test result */
                    if ((0 == strncmp(fragment->code_type, "loop", 4)) &&
                        (OPTTRAN_SUCCESS == recognizer->test3_result)) {
                        fprintf(globals.outputfile_FP,
                                "recommender.code_fragment=%s\n",
                                fragment->outer_outer_loop_fragment_file);
                        fprintf(globals.outputfile_FP, "pr.transformation=%s\n",
                                recognizer->program);
                    }
                } else {
                    fprintf(globals.outputfile_FP, "Transformation: %s ",
                            recognizer->program);
                    fprintf(globals.outputfile_FP, "%s\n",
                            recognizer->test_result ? "(not valid)" : "(valid)");
                    /* If it is a loop, check parent loop test result */
                    if ((0 == strncmp(fragment->code_type, "loop", 4)) &&
                        (0 != fragment->outer_loop)) {
                        fprintf(globals.outputfile_FP, "Transformation: %s ",
                                recognizer->program);
                        fprintf(globals.outputfile_FP, "%s\n",
                                recognizer->test2_result ?
                                "(not valid on parent loop)" :
                                "(valid on parent loop)");
                    }
                    if ((0 == strncmp(fragment->code_type, "loop", 4)) &&
                        (0 != fragment->outer_outer_loop)) {
                        fprintf(globals.outputfile_FP, "Transformation: %s ",
                                recognizer->program);
                        fprintf(globals.outputfile_FP, "%s\n",
                                recognizer->test3_result ?
                                "(not valid on grandparent loop)" :
                                "(valid on grandparent loop)");
                    }
                }
#if HAVE_SQLITE3 == 1
                /* Log result on SQLite: 3 steps */
                /* Step 1: connect to database */
                if (OPTTRAN_SUCCESS != database_connect()) {
                    OPTTRAN_OUTPUT(("%s",
                                    _ERROR("Error: connecting to database")));
                    return OPTTRAN_ERROR;
                }

                /* Step 2: read fragment file content */
                if (-1 == (fragment_FP = open(fragment->fragment_file, O_RDONLY))) {
                    OPTTRAN_OUTPUT(("%s (%s)",
                                    _ERROR("Error: unable to open fragment file"),
                                    fragment->fragment_file));
                    return OPTTRAN_ERROR;
                } else {
                    bzero(fragment_data, MAX_FRAGMENT_DATA/4);
                    r_bytes = read(fragment_FP, fragment_data,
                                   MAX_FRAGMENT_DATA/4);
                    // TODO: escape single quotes from fragment_data
                    close(fragment_FP);
                }
                /* parent fragment data */
                if ((0 == strncmp(fragment->code_type, "loop", 4)) &&
                    (0 != fragment->outer_loop)) {
                    if (-1 == (fragment_FP = open(fragment->outer_loop_fragment_file,
                                                  O_RDONLY))) {
                        OPTTRAN_OUTPUT(("%s (%s)",
                                        _ERROR("Error: unable to open fragment file"),
                                        fragment->outer_loop_fragment_file));
                        return OPTTRAN_ERROR;
                    } else {
                        bzero(parent_fragment_data, MAX_FRAGMENT_DATA/4);
                        r_bytes = read(fragment_FP, parent_fragment_data,
                                       MAX_FRAGMENT_DATA/4);
                        // TODO: escape single quotes from parent_fragment_data
                        close(fragment_FP);
                    }
                }
                /* grandparent fragment data */
                if ((0 == strncmp(fragment->code_type, "loop", 4)) &&
                    (0 != fragment->outer_outer_loop)) {
                    if (-1 == (fragment_FP = open(fragment->outer_outer_loop_fragment_file,
                                                  O_RDONLY))) {
                        OPTTRAN_OUTPUT(("%s (%s)",
                                        _ERROR("Error: unable to open fragment file"),
                                        fragment->outer_outer_loop_fragment_file));
                        return OPTTRAN_ERROR;
                    } else {
                        bzero(grandparent_fragment_data, MAX_FRAGMENT_DATA/4);
                        r_bytes = read(fragment_FP, grandparent_fragment_data,
                                       MAX_FRAGMENT_DATA/4);
                        // TODO: escape single quotes from grandparent_fragment_data
                        close(fragment_FP);
                    }
                }

                /* Step 3: insert data into DB */
                bzero(sql, MAX_FRAGMENT_DATA);
                strcat(sql, "INSERT INTO log_pr (pid, code_filename,");
                strcat(sql, "\n                        ");
                strcat(sql, "code_line_number, code_fragment, id_recommendation,");
                strcat(sql, "\n                        ");
                strcat(sql, "id_pattern, result) VALUES (");
                strcat(sql, "\n                        ");
                bzero(temp_str, MAX_FRAGMENT_DATA/4);
                sprintf(temp_str, "%llu, '%s', %d, '%s', %d, %d, %d);",
                        globals.opttran_pid, fragment->filename,
                        fragment->line_number, fragment_data, recommendation->id,
                        recognizer->id, recognizer->test_result);
                strcat(sql, temp_str);

                /* Add the parent loop test result */
                if ((0 == strncmp(fragment->code_type, "loop", 4)) &&
                    (0 != fragment->outer_loop)) {
                    strcat(sql, "\n                     ");
                    strcat(sql, "INSERT INTO log_pr (pid, code_filename,");
                    strcat(sql, "\n                        ");
                    strcat(sql, "code_line_number, code_fragment, id_recommendation,");
                    strcat(sql, "\n                        ");
                    strcat(sql, "id_pattern, result) VALUES (");
                    strcat(sql, "\n                        ");
                    bzero(temp_str, MAX_FRAGMENT_DATA/4);
                    sprintf(temp_str, "%llu, '%s', %d, '%s', %d, %d, %d);",
                            globals.opttran_pid, fragment->filename,
                            fragment->outer_loop, parent_fragment_data,
                            recommendation->id, recognizer->id,
                            recognizer->test2_result);
                    strcat(sql, temp_str);
                }
                /* Add the grandparent loop test result */
                if ((0 == strncmp(fragment->code_type, "loop", 4)) &&
                    (0 != fragment->outer_outer_loop)) {
                    strcat(sql, "\n                     ");
                    strcat(sql, "INSERT INTO log_pr (pid, code_filename,");
                    strcat(sql, "\n                        ");
                    strcat(sql, "code_line_number, code_fragment, id_recommendation,");
                    strcat(sql, "\n                        ");
                    strcat(sql, "id_pattern, result) VALUES (");
                    strcat(sql, "\n                        ");
                    bzero(temp_str, MAX_FRAGMENT_DATA/4);
                    sprintf(temp_str, "%llu, '%s', %d, '%s', %d, %d, %d);",
                            globals.opttran_pid, fragment->filename,
                            fragment->outer_outer_loop,
                            grandparent_fragment_data, recommendation->id,
                            recognizer->id, recognizer->test3_result);
                    strcat(sql, temp_str);
                }

                OPTTRAN_OUTPUT_VERBOSE((10, "%s",
                                        _YELLOW("logging results into DB")));
                OPTTRAN_OUTPUT_VERBOSE((10, "   SQL: %s", _CYAN(sql)));

                if (SQLITE_OK != sqlite3_exec(globals.db, sql, NULL, NULL,
                                              &error_msg)) {
                    fprintf(stderr, "Error: SQL error: %s\n", error_msg);
                    sqlite3_free(error_msg);
                    sqlite3_close(globals.db);
                    exit(OPTTRAN_ERROR);
                }
#endif
                recognizer = (recognizer_t *)opttran_list_get_next(recognizer);
            }
            recommendation = (recommendation_t *)opttran_list_get_next(recommendation);
        }
        fragment = (fragment_t *)opttran_list_get_next(fragment);
    }

    OPTTRAN_OUTPUT_VERBOSE((4, "==="));
    return OPTTRAN_SUCCESS;
}

#if HAVE_SQLITE3 == 1
/* database_connect */
static int database_connect(void) {
    OPTTRAN_OUTPUT_VERBOSE((4, "=== %s", _BLUE("Connecting to database")));

    /* Connect to the DB */
    if (NULL == globals.dbfile) {
        globals.dbfile = "./recommendation.db";
    }
    if (-1 == access(globals.dbfile, F_OK)) {
        OPTTRAN_OUTPUT(("%s (%s)",
                        _ERROR("Error: recommendation database doesn't exist"),
                        globals.dbfile));
        return OPTTRAN_ERROR;
    }
    if (-1 == access(globals.dbfile, R_OK)) {
        OPTTRAN_OUTPUT(("%s (%s)",
                        _ERROR("Error: you don't have permission to read"),
                        globals.dbfile));
        return OPTTRAN_ERROR;
    }

    if (SQLITE_OK != sqlite3_open(globals.dbfile, &(globals.db))) {
        OPTTRAN_OUTPUT(("%s (%s), %s", _ERROR("Error: openning database"),
                        globals.dbfile, sqlite3_errmsg(globals.db)));
        sqlite3_close(globals.db);
        exit(OPTTRAN_ERROR);
    } else {
        OPTTRAN_OUTPUT_VERBOSE((4, "connected to %s", globals.dbfile));
    }
    return OPTTRAN_SUCCESS;
}
#endif
    
#ifdef __cplusplus
}
#endif

// EOF
