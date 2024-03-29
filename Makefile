#
# make
# make all      -- build everything
#
# make test     -- run unit tests
#
# make install  -- install binaries to /usr/local
#
# make clean    -- remove build files
#

all:    build
	$(MAKE) -C build $@

test:   build
	$(MAKE) -C build test

install: build
	$(MAKE) -C build $@

clean:
	rm -rf build

build:
	mkdir $@
	cmake -B $@ .

.PHONY: clean test install
