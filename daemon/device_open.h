#if LOG_DEBUG >= LOG_LEVEL_MIN
  #define log_devs_debug(root, fn) log_log_devs(LOG_DEBUG, root, fn)
#else
  #define log_devs_debug(root, fn)
#endif

extern void devices_print_one(device *, FILE *);
extern device *find_device_by_read_fd(device *, int);
extern device *find_device_by_read_soc(device *, void *socket);
extern device *find_device_by_id(device *, const char *);
extern void devices_open(device *);
void log_log_devs(int level, device *root, const char *fn);
