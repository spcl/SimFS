/** @file dv_cmdline.h
 *  @brief The header file for the command line option parser
 *  generated by GNU Gengetopt version 2.22.6
 *  http://www.gnu.org/software/gengetopt.
 *  DO NOT modify this file, since it can be overwritten
 *  @author GNU Gengetopt by Lorenzo Bettini */

#ifndef DV_CMDLINE_H
#define DV_CMDLINE_H

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h> /* for FILE */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef CMDLINE_PARSER_PACKAGE
/** @brief the program name (used for printing errors) */
#define CMDLINE_PARSER_PACKAGE PACKAGE
#endif

#ifndef CMDLINE_PARSER_PACKAGE_NAME
/** @brief the complete program name (used for help and version) */
#ifdef PACKAGE_NAME
#define CMDLINE_PARSER_PACKAGE_NAME PACKAGE_NAME
#else
#define CMDLINE_PARSER_PACKAGE_NAME PACKAGE
#endif
#endif

#ifndef CMDLINE_PARSER_VERSION
/** @brief the program version */
#define CMDLINE_PARSER_VERSION VERSION
#endif

/** @brief Where the command line options are stored */
struct gengetopt_args_info
{
  const char *help_help; /**< @brief Print help and exit help description.  */
  const char *version_help; /**< @brief Print version and exit help description.  */
  char * hostname_arg;	/**< @brief host name or IP address.  */
  char * hostname_orig;	/**< @brief host name or IP address original value given at command line.  */
  const char *hostname_help; /**< @brief host name or IP address help description.  */
  char * simulator_port_arg;	/**< @brief simulator port.  */
  char * simulator_port_orig;	/**< @brief simulator port original value given at command line.  */
  const char *simulator_port_help; /**< @brief simulator port help description.  */
  char * client_port_arg;	/**< @brief client port.  */
  char * client_port_orig;	/**< @brief client port original value given at command line.  */
  const char *client_port_help; /**< @brief client port help description.  */
  char * simulator_arg;	/**< @brief simulator configuration path.  */
  char * simulator_orig;	/**< @brief simulator configuration path original value given at command line.  */
  const char *simulator_help; /**< @brief simulator configuration path help description.  */
  char * checkpoints_arg;	/**< @brief checkpoint/restart files path.  */
  char * checkpoints_orig;	/**< @brief checkpoint/restart files path original value given at command line.  */
  const char *checkpoints_help; /**< @brief checkpoint/restart files path help description.  */
  char * results_arg;	/**< @brief result files path.  */
  char * results_orig;	/**< @brief result files path original value given at command line.  */
  const char *results_help; /**< @brief result files path help description.  */
  char * dummy_arg;	/**< @brief temporary redirect dummy path.  */
  char * dummy_orig;	/**< @brief temporary redirect dummy path original value given at command line.  */
  const char *dummy_help; /**< @brief temporary redirect dummy path help description.  */
  char * filecache_type_arg;	/**< @brief cache type.  */
  char * filecache_type_orig;	/**< @brief cache type original value given at command line.  */
  const char *filecache_type_help; /**< @brief cache type help description.  */
  long filecache_size_arg;	/**< @brief cache size.  */
  char * filecache_size_orig;	/**< @brief cache size original value given at command line.  */
  const char *filecache_size_help; /**< @brief cache size help description.  */
  long filecache_fifo_size_arg;	/**< @brief size of the fifo queue (default 0).  */
  char * filecache_fifo_size_orig;	/**< @brief size of the fifo queue (default 0) original value given at command line.  */
  const char *filecache_fifo_size_help; /**< @brief size of the fifo queue (default 0) help description.  */
  long filecache_lir_set_size_arg;	/**< @brief LIR set size for LIRS.  */
  char * filecache_lir_set_size_orig;	/**< @brief LIR set size for LIRS original value given at command line.  */
  const char *filecache_lir_set_size_help; /**< @brief LIR set size for LIRS help description.  */
  long filecache_mrus_arg;	/**< @brief reserved MRUs.  */
  char * filecache_mrus_orig;	/**< @brief reserved MRUs original value given at command line.  */
  const char *filecache_mrus_help; /**< @brief reserved MRUs help description.  */
  double filecache_penalty_arg;	/**< @brief penalty factor.  */
  char * filecache_penalty_orig;	/**< @brief penalty factor original value given at command line.  */
  const char *filecache_penalty_help; /**< @brief penalty factor help description.  */
  long prefetch_intervals_arg;	/**< @brief max. number of intervals to be prefetched (sets vertical to value and horizontal to 1).  */
  char * prefetch_intervals_orig;	/**< @brief max. number of intervals to be prefetched (sets vertical to value and horizontal to 1) original value given at command line.  */
  const char *prefetch_intervals_help; /**< @brief max. number of intervals to be prefetched (sets vertical to value and horizontal to 1) help description.  */
  long horizontal_prefetch_interval_arg;	/**< @brief max. number of intervals to be prefetched in one simulation (horizontal).  */
  char * horizontal_prefetch_interval_orig;	/**< @brief max. number of intervals to be prefetched in one simulation (horizontal) original value given at command line.  */
  const char *horizontal_prefetch_interval_help; /**< @brief max. number of intervals to be prefetched in one simulation (horizontal) help description.  */
  long vertical_prefetch_interval_arg;	/**< @brief max. number of intervals to be prefetched by multiple simulations (vertical).  */
  char * vertical_prefetch_interval_orig;	/**< @brief max. number of intervals to be prefetched by multiple simulations (vertical) original value given at command line.  */
  const char *vertical_prefetch_interval_help; /**< @brief max. number of intervals to be prefetched by multiple simulations (vertical) help description.  */
  long parallel_simulators_arg;	/**< @brief max. number of simulators running in parallel.  */
  char * parallel_simulators_orig;	/**< @brief max. number of simulators running in parallel original value given at command line.  */
  const char *parallel_simulators_help; /**< @brief max. number of simulators running in parallel help description.  */
  long sim_kill_threshold_arg;	/**< @brief threshold for stopping a running simulator.  */
  char * sim_kill_threshold_orig;	/**< @brief threshold for stopping a running simulator original value given at command line.  */
  const char *sim_kill_threshold_help; /**< @brief threshold for stopping a running simulator help description.  */
  
  unsigned int help_given ;	/**< @brief Whether help was given.  */
  unsigned int version_given ;	/**< @brief Whether version was given.  */
  unsigned int hostname_given ;	/**< @brief Whether hostname was given.  */
  unsigned int simulator_port_given ;	/**< @brief Whether simulator-port was given.  */
  unsigned int client_port_given ;	/**< @brief Whether client-port was given.  */
  unsigned int simulator_given ;	/**< @brief Whether simulator was given.  */
  unsigned int checkpoints_given ;	/**< @brief Whether checkpoints was given.  */
  unsigned int results_given ;	/**< @brief Whether results was given.  */
  unsigned int dummy_given ;	/**< @brief Whether dummy was given.  */
  unsigned int filecache_type_given ;	/**< @brief Whether filecache-type was given.  */
  unsigned int filecache_size_given ;	/**< @brief Whether filecache-size was given.  */
  unsigned int filecache_fifo_size_given ;	/**< @brief Whether filecache-fifo-size was given.  */
  unsigned int filecache_lir_set_size_given ;	/**< @brief Whether filecache-lir-set-size was given.  */
  unsigned int filecache_mrus_given ;	/**< @brief Whether filecache-mrus was given.  */
  unsigned int filecache_penalty_given ;	/**< @brief Whether filecache-penalty was given.  */
  unsigned int prefetch_intervals_given ;	/**< @brief Whether prefetch-intervals was given.  */
  unsigned int horizontal_prefetch_interval_given ;	/**< @brief Whether horizontal-prefetch-interval was given.  */
  unsigned int vertical_prefetch_interval_given ;	/**< @brief Whether vertical-prefetch-interval was given.  */
  unsigned int parallel_simulators_given ;	/**< @brief Whether parallel-simulators was given.  */
  unsigned int sim_kill_threshold_given ;	/**< @brief Whether sim-kill-threshold was given.  */

  char **inputs ; /**< @brief unamed options (options without names) */
  unsigned inputs_num ; /**< @brief unamed options number */
} ;

/** @brief The additional parameters to pass to parser functions */
struct cmdline_parser_params
{
  int override; /**< @brief whether to override possibly already present options (default 0) */
  int initialize; /**< @brief whether to initialize the option structure gengetopt_args_info (default 1) */
  int check_required; /**< @brief whether to check that all required options were provided (default 1) */
  int check_ambiguity; /**< @brief whether to check for options already specified in the option structure gengetopt_args_info (default 0) */
  int print_errors; /**< @brief whether getopt_long should print an error message for a bad option (default 1) */
} ;

/** @brief the purpose string of the program */
extern const char *gengetopt_args_info_purpose;
/** @brief the usage string of the program */
extern const char *gengetopt_args_info_usage;
/** @brief the description string of the program */
extern const char *gengetopt_args_info_description;
/** @brief all the lines making the help output */
extern const char *gengetopt_args_info_help[];

/**
 * The command line parser
 * @param argc the number of command line options
 * @param argv the command line options
 * @param args_info the structure where option information will be stored
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser (int argc, char **argv,
  struct gengetopt_args_info *args_info);

/**
 * The command line parser (version with additional parameters - deprecated)
 * @param argc the number of command line options
 * @param argv the command line options
 * @param args_info the structure where option information will be stored
 * @param override whether to override possibly already present options
 * @param initialize whether to initialize the option structure my_args_info
 * @param check_required whether to check that all required options were provided
 * @return 0 if everything went fine, NON 0 if an error took place
 * @deprecated use cmdline_parser_ext() instead
 */
int cmdline_parser2 (int argc, char **argv,
  struct gengetopt_args_info *args_info,
  int override, int initialize, int check_required);

/**
 * The command line parser (version with additional parameters)
 * @param argc the number of command line options
 * @param argv the command line options
 * @param args_info the structure where option information will be stored
 * @param params additional parameters for the parser
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser_ext (int argc, char **argv,
  struct gengetopt_args_info *args_info,
  struct cmdline_parser_params *params);

/**
 * Save the contents of the option struct into an already open FILE stream.
 * @param outfile the stream where to dump options
 * @param args_info the option struct to dump
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser_dump(FILE *outfile,
  struct gengetopt_args_info *args_info);

/**
 * Save the contents of the option struct into a (text) file.
 * This file can be read by the config file parser (if generated by gengetopt)
 * @param filename the file where to save
 * @param args_info the option struct to save
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser_file_save(const char *filename,
  struct gengetopt_args_info *args_info);

/**
 * Print the help
 */
void cmdline_parser_print_help(void);
/**
 * Print the version
 */
void cmdline_parser_print_version(void);

/**
 * Initializes all the fields a cmdline_parser_params structure 
 * to their default values
 * @param params the structure to initialize
 */
void cmdline_parser_params_init(struct cmdline_parser_params *params);

/**
 * Allocates dynamically a cmdline_parser_params structure and initializes
 * all its fields to their default values
 * @return the created and initialized cmdline_parser_params structure
 */
struct cmdline_parser_params *cmdline_parser_params_create(void);

/**
 * Initializes the passed gengetopt_args_info structure's fields
 * (also set default values for options that have a default)
 * @param args_info the structure to initialize
 */
void cmdline_parser_init (struct gengetopt_args_info *args_info);
/**
 * Deallocates the string fields of the gengetopt_args_info structure
 * (but does not deallocate the structure itself)
 * @param args_info the structure to deallocate
 */
void cmdline_parser_free (struct gengetopt_args_info *args_info);

/**
 * Checks that all the required options were specified
 * @param args_info the structure to check
 * @param prog_name the name of the program that will be used to print
 *   possible errors
 * @return
 */
int cmdline_parser_required (struct gengetopt_args_info *args_info,
  const char *prog_name);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* DV_CMDLINE_H */
