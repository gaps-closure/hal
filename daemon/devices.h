#define PARENT_READ  read_pipe[0]
#define PARENT_WRITE write_pipe[1]
#define CHILD_WRITE  read_pipe[1]
#define CHILD_READ   write_pipe[0]

extern void devices_print_one(device *);
extern void devices_print_all(device *);
extern device *find_device_by_readfd(device *, int);
extern device *find_device_by_id(device *, const char *);
extern void devices_open(device *);
