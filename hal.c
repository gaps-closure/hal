#include <stdio.h>
#include <stdlib.h>
#include <libconfig.h>

char *handle_args(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s path-to-config-file\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  return argv[1];
}

void read_config (char *cfname, config_t *cfg) {
  fprintf(stderr, "Reading configuration file: %s\n", cfname);
  config_init(cfg);
  if(! config_read_file(cfg, cfname))
  {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(cfg), config_error_line(cfg), config_error_text(cfg));
    config_destroy(cfg);
    exit(EXIT_FAILURE);
  }
}

int main(int argc, char **argv) {
  config_t         cfg;
  config_setting_t *setting;
  const char       *str;

  read_config(handle_args(argc, argv), &cfg);

  if(config_lookup_string(&cfg, "zcpath", &str)) 
    printf("About to exec: %s\n", str);
    /* Fork, then exec zc in child and get in and out handles for child in parent */
  else
    fprintf(stderr, "No 'zcpath' setting in configuration file.\n");

  setting = config_lookup(&cfg, "devices");
  if(setting != NULL) {
    int count = config_setting_length(setting);
    int i;
    for(i = 0; i < count; ++i)
    {
      const config_setting_t *dev;
      const char *devpath;
      str = config_setting_get_string_elem(setting, i);
      dev = config_lookup(&cfg, str);
      config_setting_lookup_string(dev, "path", &devpath);

      printf("About to open device: %s %s\n", str, devpath);
      /* Open device and get in and out handles for device */
    }
  }

  /* Get bijection of MUX label to device/port */
  /* Get bijection of SEC label to device-specific message num */
  /* Get bijection of TYP label to device-specific datatype num */
	
  /* While true, select across file descriptors for zc and devices */

  /* On select, determine codec to use for fd, and invoke on PDU, use SEC and TYP mappings */

  /* Write code result to appropriate fd based on MUX mapping */

  /* XXX: Log to specified logfile, not stderr */
  /* XXX: Properly daemonize, close stadnard fds, trap signals etc. */
  /* XXX: Deal with frag/defrag, buffering on blocking I/O, etc. */

  return 0;
}

