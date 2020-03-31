/*
 * Hardware Abstraction Layer (HAL) between Applicaitons and GAP XD Guards
 *   April 2020, Perspecta Labs
 *
 * TODO:
 *  XXX: Fix README.md and figure
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

/* Signal Handler for SIGINT - print statistics */
device   *root_dev;
void sigintHandler(int sig_num)
{
  char        s[256]="", str_new[64];

  for(device *d = root_dev; d != NULL; d = d->next) {
    if (d->enabled != 0) {
      sprintf(str_new, "%s[r=%d w=%d] ", d->id, d->count_r, d->count_w);
      strcat(s, str_new);
    }
  }
  fprintf(stderr, "\nDevice read-write summary: %s\n", s);
  exit(0);
}

/**********************************************************************/
/* Initialize using confifguration file and user defined options     */
/*********t************************************************************/
void hal_init(char *file_name_config, char *file_name_log, char *file_name_stats,
              int log_level, int hal_quiet, int hal_wait_us) {
  config_t  cfg;           /* Configuration */
  device   *devs;          /* Linked list of enabled devices */
  halmap   *map;           /* Linked list of selector mappings */
  FILE     *fp=NULL;
  
  /* a) Logging */
  log_set_quiet(hal_quiet);
  if (log_level < LOG_TRACE) log_level = LOG_TRACE;
  log_set_level(log_level);
  if (file_name_log != NULL) {
    log_trace("Openning Log file: %s", file_name_log);
    fp = fopen(file_name_log, "w+");
    log_set_fp(fp);
  }
  if (file_name_stats != NULL) {
    log_trace("Openning Stats file: %s", file_name_stats);
    fp = fopen(file_name_stats, "w+");
    log_set_stats_fp(fp);
  }
  log_debug("HAL options: fc=%s fl=%s fs=%s lev=%d quiet=%d wait_us=%d", file_name_config, file_name_log, file_name_stats, log_level, hal_quiet, hal_wait_us);
  
  /* b) Load coniguration */
  cfg_read(&cfg, file_name_config);
  devs = get_devices(&cfg);
  devices_print_all(devs, LOG_TRACE, __func__);
  map  = get_mappings(&cfg);
  halmap_print_all(map, LOG_DEBUG, __func__);
  config_destroy(&cfg);

  /* c) Open devices */
  devices_open(devs);
  devices_print_all(devs, LOG_DEBUG, __func__);
  
  /* d) Initialize signal handler, then Wait for input */
  signal(SIGINT, sigintHandler);
  root_dev = devs;
  read_wait_loop(devs, map, hal_quiet, hal_wait_us);
}

/**********************************************************************/
/* HAL Process User Defined options */
/*********t************************************************************/
void opts_print(void) {
  printf("Hardware Abstraction Layer (HAL) for GAPS CLOSURE project (version 0.11)\n");
  printf("Usage: hal [OPTIONS]... CONFIG-FILE\n");
  printf("OPTIONS: are one of the following:\n");
  printf(" -f : log file name (default = no log file)\n");
  printf(" -h : print this message\n");
  printf(" -l : log level: 0=TRACE, 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR, 5=FATAL (default = 0)\n");
  printf(" -q : quiet: disable logging on stderr (default = enabled)");
  printf(" -s : statistics file name (default = no log file)\n");
  printf(" -w : device not ready (EAGAIN) wait time in microseconds (default = 1000us): -1 exits if not ready\n");
  printf("CONFIG-FILE: path to HAL configuration file (e.g., test/sample.cfg)\n");
}

/* Get user defined options */
int main(int argc, char **argv) {
  int    opt;
  int    log_level=3, hal_quiet=0, hal_wait_us=1000;        /* option defaults */
  char  *file_name_config = NULL;
  char  *file_name_log    = NULL;
  char  *file_name_stats  = NULL;

  if (argc < 2) {
    opts_print();
    exit(EXIT_FAILURE);
  }
  while((opt =  getopt(argc, argv, ":f:hl:s:vw:")) != EOF)
  {
    switch (opt)
    {
      case 'f':
        file_name_log = optarg;
        break;
      case 'h':
        opts_print();
        exit(0);
      case 'l':
        log_level = atoi(optarg);
        break;
      case 'q':
        hal_quiet = 1;
        break;
      case 's':
        file_name_stats = optarg;
        break;
      case 'w':
        hal_wait_us = atoi(optarg);
        break;
      case ':':
        log_fatal("Option -%c needs a value\n", optopt);
        opts_print();
        exit(EXIT_FAILURE);
        break;
      default:
        log_error("Skipping undefined Option (%d)\n", optopt);
        opts_print();
    }
  }

  if(optind<argc) {
    file_name_config = argv[optind++];
  }
  else {
    log_fatal("Must specify a CONFIG-FILE");
    opts_print();
    exit(EXIT_FAILURE);
  }
  
  hal_init(file_name_config, file_name_log, file_name_stats, log_level, hal_quiet, hal_wait_us);
  return (0);
}
