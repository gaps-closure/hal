/* Read (libconfig) config file information into HAL */

#include "hal.h"

/**********************************************************************/
/* HAL Configuration file (read and parse) */
/*********t************************************************************/
/* Read conifg file */
void cfg_read (config_t *cfg, char  *file_name) {
  config_init(cfg);
  if(! config_read_file(cfg, file_name))
  {
    fprintf(stderr, "%s:%d - %s\n", config_error_file(cfg), config_error_line(cfg), config_error_text(cfg));
    config_destroy(cfg);
    exit(EXIT_FAILURE);
  }
}

/* Get a top-level string item from config */
char *cfg_get_top_str(config_t *cfg, char *fld) {
  const char *ret;
  if(!config_lookup_string(cfg, fld, &ret)) {
    fprintf(stderr, "No '%s' setting in configuration file.\n", fld);
    exit(EXIT_FAILURE);
  }
  return strdup(ret);
}

/* Get field parameter from config (exit if param is not found and non-optional) */
const char * get_param_str(config_setting_t *dev, const char *name, int optional, int field_num) {
  const char *val="";         /* not use NULL to avoid lookup failure */

//  fprintf(stderr, "%s %s\n", __func__, name);
  if( (!config_setting_lookup_string(dev, name, &val)) && (!optional) ) {
    fprintf(stderr, "Missing a non-optional field '%s' (for device %d)\n", name, field_num);
    exit(EXIT_FAILURE);
  }
  return (strdup(val));
}

/* Get field parameter from config (exit if param is not found and non-optional) */
int get_param_int(config_setting_t *dev, const char *name, int optional, int field_num) {
  int  val = -1;
  
  if ( (!config_setting_lookup_int(dev, name, &val)) && (!optional) ) {
    fprintf(stderr, "Missing a non-optional field '%s' (for device %d)\n", name, field_num);
    exit(EXIT_FAILURE);
  }
  return (val);
}

/* Construct linked list of devices from config */
device *get_devices(config_t *cfg) {
  device *ret = NULL;
  config_setting_t *devs = config_lookup(cfg, "devices");
  if(devs != NULL) {
    int count = config_setting_length(devs);
    ret = malloc(count * sizeof(device));
    if (ret == NULL) {
      fprintf(stderr, "Memory allocation failed");
      exit(EXIT_FAILURE);
    }
    for(int i = 0; i < count; i++) {
      config_setting_t *dev = config_setting_get_elem(devs, i);
      /* required parameters */
      ret[i].enabled  = get_param_int(dev, "enabled",  0, i);
      ret[i].id       = get_param_str(dev, "id",       0, i);
      ret[i].path     = get_param_str(dev, "path",     0, i);
      ret[i].model    = get_param_str(dev, "model",    0, i);
      ret[i].comms    = get_param_str(dev, "comms",    0, i);
      /* optional parameters */
      ret[i].addr_in  = get_param_str(dev, "addr_in",  1, i);
      ret[i].addr_out = get_param_str(dev, "addr_out", 1, i);
      ret[i].mode_in  = get_param_str(dev, "mode_in",  1, i);
      ret[i].mode_out = get_param_str(dev, "mode_out", 1, i);
      ret[i].port_in  = get_param_int(dev, "port_in",  1, i);
      ret[i].port_out = get_param_int(dev, "port_out", 1, i);
      ret[i].readfd   = -1; /* to be set when opened */
      ret[i].writefd  = -1; /* to be set when opened */
      ret[i].next     = i < count - 1 ? &ret[i+1] : (device *) NULL;
    }
  }
  return ret;
}

/* Construct linked list of HAL map entries from config */
halmap *get_mappings(config_t *cfg) {
  halmap *ret = NULL;

  config_setting_t *hmaps = config_lookup(cfg, "maps");
  if(hmaps != NULL) {
    int count = config_setting_length(hmaps);
    ret = malloc(count * sizeof(halmap));
    if (ret == NULL) {
      fprintf(stderr, "Memory allocation failed");
      exit(EXIT_FAILURE);
    }
    for(int i = 0; i < count; i++) {
      config_setting_t *map = config_setting_get_elem(hmaps, i);
      ret[i].from.dev     = get_param_str(map, "from_dev",  0, i);
      ret[i].from.ctag    = get_param_int(map, "from_ctag", 1, i);
      ret[i].from.tag.mux = get_param_int(map, "from_mux",  1, i);
      ret[i].from.tag.sec = get_param_int(map, "from_sec",  1, i);
      ret[i].from.tag.typ = get_param_int(map, "from_typ",  1, i);
      ret[i].to.dev       = get_param_str(map, "to_dev",    0, i);
      ret[i].to.ctag      = get_param_int(map, "to_ctag",   1, i);
      ret[i].to.tag.mux   = get_param_int(map, "to_mux",    1, i);
      ret[i].to.tag.sec   = get_param_int(map, "to_sec",    1, i);
      ret[i].to.tag.typ   = get_param_int(map, "to_typ",    1, i);
      ret[i].codec        = get_param_str(map, "codec",     1, i);
      ret[i].next         = i < count - 1 ? &ret[i+1] : (halmap *) NULL;
      fprintf(stderr, "ctags = %d %d\n", ret[i].from.ctag, ret[i].to.ctag);
    }
  }
  return ret;
}
