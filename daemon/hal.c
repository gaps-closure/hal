/*
 * Hardware Abstraction Layer (HAL) between Applicaitons and GAP XD Guards
 *   March 2020, Perspecta Labs
 *
 * TODO:
 *  XXX: Fix README.md and figure
 *  XXX: Log to specified logfile, not stderr
 *  XXX: Properly daemonize: close standard fds, trap signals, Exit only when needed (not to debug), etc.
 *  XXX: Dynamically change HAL daemon conifg file
 *  XXX: Add frag/defrag and other functionality.
 *  XXX: Codec ADU transformation (using halmap entry)
 */

/**********************************************************************/
/* HAL Library Includes and Deinitions */
/**********************************************************************/

#include "hal.h"
#include "config.h"
#include "device_open.h"
#include "device_read_write.h"
#include "map.h"
#include "packetize.h"

int hal_verbose=0;        /* print debug flag */
int hal_wait_us=1000;     /* select (EAGAIN) wait loop (in microseconds) */

/**********************************************************************/
/* HAL get options */
/*********t************************************************************/
void opts_print(void) {
  printf("Hardware Abstraction Layer (HAL) for gaps CLOSURE project\n");
  printf("Usage: hal [OPTION]... CONFIG-FILE\n");
  printf("OPTION: one of the following options:\n");
  printf(" -h --help : print this message\n");
  printf(" -v --hal_verbose : print debug messages in stderr\n");
  printf(" -w --hal_verbose : select wait time (in microseconds) when device not ready (EAGAIN) - default to 1000us (-1 will exit if device is not ready)\n");
  printf("CONFIG-FILE: path to file with HAL configuration information (e.g., sample.cfg)\n");
}

/* Parse the configuration file */
char *opts_get (int argc, char **argv) {
  int opt;
  char  *file_name= NULL;
  
  if (argc < 2) {
    opts_print();
    exit(EXIT_FAILURE);
  }
  while((opt =  getopt(argc, argv, "hvw:")) != EOF)
  {
    switch (opt)
    {
      case 'h':
        opts_print();
        exit(0);
      case 'v':
        hal_verbose = 1;
        break;
      case 'w':
        hal_wait_us = atoi(optarg);
        break;
      default:
        printf("\nSkipping undefined Option (%d)\n", opt);
        opts_print();
    }
  }
  if(optind<argc) {
     file_name = argv[optind++];
  }
  if(hal_verbose) fprintf(stderr, "Read script options: v=%d conifg-file=%s (optind=%d)\n", hal_verbose, file_name, optind);
  return (file_name);
}

/**********************************************************************/
/* get confifguration inofrmation                                     */
/*********t************************************************************/
/* Get coniguration, then call read_wait_loop */
int main(int argc, char **argv) {
  config_t  cfg;           /* Configuration */
  char     *file_name;     /* Path to conifg file */
  device   *devs;          /* Linked list of enabled devices */
  halmap   *map;           /* Linked list of selector mappings */
  
  file_name = opts_get(argc, argv);
  cfg_read(&cfg, file_name);
  if(hal_verbose) fprintf(stderr, "Config file %s exists\n", file_name);
  devs = get_devices(&cfg);
  if(hal_verbose) {fprintf(stderr, "Config file "); devices_print_all(devs);}
  map  = get_mappings(&cfg);
  if(hal_verbose) {fprintf(stderr, "Config file "); halmap_print_all(map);}
  config_destroy(&cfg);
  devices_open(devs, hal_verbose);
  devices_print_all(devs);
  halmap_print_all(map);
  read_wait_loop(devs, map, hal_verbose, hal_wait_us);
  return 0;
}
