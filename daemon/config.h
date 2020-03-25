/* Read (libconfig) config file information into HAL */

extern void cfg_read(config_t *, char  *);
extern device *get_devices(config_t *);
extern halmap *get_mappings(config_t *);
