LIB_DIR=lib
EXE_DIR=bin

# Set SILENT to 's' if --quiet/-s set, otherwise ''.
SILENT := $(findstring s,$(word 1, $(MAKEFLAGS)))

.PHONY: all
all:
	@mkdir -p build; \
	cd build; \
	cmake .. && make

.PHONY: verbose
verbose: 
	@mkdir -p build; \
	cd build; \
	cmake .. && make VERBOSE=1

.PHONY: clean
clean:
	rm -rf $(LIB_DIR) $(EXE_DIR) Log.txt build

.PHONY: install
install:
	mkdir -p build/docs/html
	mkdir -p build/docs/latex
	cd build && sudo make install

# clang-format
.PHONY: cf
cf:
	@clang-format -style=file -i include/*.h
	@clang-format -style=file -i src/*.cpp

.PHONY: list
list:
	@$(MAKE) -pRrq -f $(lastword $(MAKEFILE_LIST)) : 2>/dev/null | awk -v RS= -F: '/^# File/,/^# Finished Make data base/ {if ($$1 !~ "^[#.]") {print $$1}}' | sort | egrep -v -e '^[^[:alnum:]]' -e '^$@$$'




