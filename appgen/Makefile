DIRS = $(wildcard 6month-demo apptest)

default: clean sub_appgen

all:
	for d in $(DIRS); do $(MAKE) -C "$$d" $@ || exit 1; done

static:
	for d in $(DIRS); do $(MAKE) -C "$$d" $@ || exit 1; done

clean:
	for d in $(DIRS); do $(MAKE) -C "$$d" $@ || exit 1; done
