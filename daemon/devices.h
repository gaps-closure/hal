extern void devices_print_one(device *);
extern void devices_print_all(device *);
extern device *find_device_by_readfd(device *, int);
extern device *find_device_by_id(device *, const char *);
extern void devices_open(device *);
