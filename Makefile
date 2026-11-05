#
# Info:
#
# This makefile expects the following directly layout:
# .
# ├── 3rdParty
# │   ├── bin
# |   |   ├── sqlite3 -> ../sqlite.build/sqlite3
# │   │   └── ... etc: more links to 3rdParty executables
# │   ├── <3rdPart tool>        : src dir
# │   ├── <3rdPart tool>.build  : build dir
# │   ├── include
# │   │   ├── sqlite3.h -> ../sqlite.build/sqlite3.h
# │   │   └── ... etc: more links to 3rdParty headers
# │   ├── lib
# │   │   ├── libgtest.a -> ../googletest.build/lib/libgtest.a
# │   │   └── ... etc: more links to 3rdParty libraries
# │   ├── Makefile
# │   ├── README.md
# │   └── src
# │       └── sqlite3.c -> ../sqlite.build/sqlite3.c
# ├── build : build dir
# ├── data  : data for tests
# ├── Makefile
# ├── README.md
# └── src
#     ├── tests : unit tests src, one file per test 
#     └── <other> : executable / library. All directories become libraries.
#                   If a main.cpp is present, it's linked as exe together with the library.
#
# All .cpp files generate an .o file (standard object file)
# All .o filed generate an .d file (defining dependencies)
# All executable/test targets get a .lnk file with their build rules
#
# to build:
#   make target=debug/release/profile all
#
# to run the unit tests:
#   make target=debug/release/profile run_UTs
#

# tools
AR=ar
ARFLAGS=crT
CC=gcc
CPP=g++
CFLAGS=-W -Wall -std=c17
CPPFLAGS=-W -Wall -std=c++20
DEFINES=
INCLUDE=3rdParty/include
PROJECT_INCLUDE=$(patsubst src/%,-I src/% ,$(filter-out src/tests/,$(wildcard src/*/)))
GTEST_LIBS=3rdParty/lib/libgtest.a
SQLITE=3rdParty/lib/sqlite3.o

# Select the optimization level with `make target=release all`.
target ?= $(shell if [ -f build/.target ]; then cat build/.target; else echo debug; fi)
ifeq ($(filter $(target),debug release profile),)
$(error Unknown target '$(target)'; choose debug, release, or profile)
endif
ifeq ($(target),release)
CPPFLAGS += -O3
else ifeq ($(target),profile)
CPPFLAGS += -g -O2
else
CPPFLAGS += -g3 -O0
endif

# Add .d / .lnk to Make's recognized suffixes.
SUFFIXES += .d .lnk

# make $$ work
.SECONDEXPANSION:

# Keep recursive make calls quiet about directory changes.
MAKEFLAGS += --no-print-directory

NEED_FILE_GENERATION := $(filter-out message clean,$(MAKECMDGOALS))

#
# Issue a warning if people just execute this makefile without any idea of what havock it can wreak.
#
.PHONY: message
message:
	@echo "Unless you know what you are doing you probably want to do this:"
	@echo ""
	@echo "compile:        make target=<debug|profile|release> 3rdParty all"
	@echo "run unit tests: make run_UTs"
	@echo ""
	@echo "'make all' uses the previously selected target, or debug if none was selected"
	@echo "Any other argument: Here Be Dragons"

.PHONY: all

# When both are requested, make all wait for clean before starting its build.
ifneq ($(filter clean,$(MAKECMDGOALS)),)
ifneq ($(filter all,$(MAKECMDGOALS)),)
all: clean
all: 3rdParty
3rdParty: clean
endif
endif

ifneq ($(filter proper,$(MAKECMDGOALS)),)
ifneq ($(filter all,$(MAKECMDGOALS)),)
all: proper
all: 3rdParty
3rdParty: proper
endif
endif

ifneq ($(filter 3rdParty,$(MAKECMDGOALS)),)
ifneq ($(filter all,$(MAKECMDGOALS)),)
all: 3rdParty
endif
endif

ifeq ($(wildcard build/.3rdParty),)
all: 3rdParty
endif

# Keep the shared build directory safe when optimization flags change. The
# recursive build starts only after stale objects and generated link rules are removed.
all:
	@mkdir -p build temp
	@previous=$$(if [ -f build/.target ]; then cat build/.target; else echo $(target); fi); \
	if [ "$$previous" != "$(target)" ]; then \
		echo "Switching build target to $(target), cleaning previous build..."; \
		$(MAKE) clean; \
	fi
	@mkdir -p build temp
	@printf '%s\n' '$(target)' > build/.target
	@$(MAKE) target=$(target) binaries
		
.PHONY: clean
clean:
	@echo "Cleaning ..."
	@rm -rf bin build *.out
	@find . -name "*~" -exec rm -f "{}" \;

.PHONY: proper
proper: clean
	@rm -f temp
	@$(MAKE) -C 3rdParty clean

#
# call the makefile for the 3dParty libraries
#

.PHONY: 3rdParty
3rdParty:
	@mkdir -p build temp
	@echo "Building 3rdParty libraries..."
	@$(MAKE) -C 3rdParty all
	@touch build/.3rdParty

#
# turn all CPP files into objects / executables and create a dependency file for all
#
CPP_FILES := $(wildcard src/*.cpp src/*/*.cpp)
OBJ_FILES := $(patsubst src/%.cpp,build/%.o,$(CPP_FILES))
DEP_FILES := $(OBJ_FILES:.o=.d)

LIB_DIRS := $(filter-out src/tests/,$(wildcard src/*/))
LIB_FILES := $(foreach d,$(LIB_DIRS),build/$(notdir $(patsubst %/,%,$(d)))/$(notdir $(patsubst %/,%,$(d))).a)

TST_FILES := $(patsubst src/tests/%.cpp,bin/%,$(wildcard src/tests/*.cpp))
EXE_FILES := $(patsubst src/%/main.cpp,bin/%,$(wildcard src/*/main.cpp))

TST_LNK_FILES := $(patsubst src/tests/%.cpp,build/tests/%.lnk,$(wildcard src/tests/*.cpp))
EXE_LNK_FILES := $(patsubst src/%/main.cpp,build/%/main.lnk,$(wildcard src/*/main.cpp))
LIB_LNK_FILES := $(patsubst %.a,%.lnk,$(LIB_FILES))

#
# expensive targets, which should only be ran when the target is actually needed
#

#
# dependency files
#
.PRECIOUS: build/%.d
build/%.d: src/%.cpp Makefile
	@mkdir -p $(dir $@)
	@echo "Creating $@"
	@$(CPP) $(CPPFLAGS) ${DEFINES} -I ${INCLUDE} ${PROJECT_INCLUDE} -MM -MT '$(patsubst src/%.cpp,build/%.o,$<)' $< -MF $@

#
# object files
#
.PRECIOUS: build/%.o 
build/%.o: src/%.cpp build/%.d Makefile
	@mkdir -p $(dir $@)
	@echo "Compiling $< to $@"
	@${CPP} ${CPPFLAGS} ${DEFINES} -isystem ${INCLUDE} ${PROJECT_INCLUDE} -fPIC -o $@ -c $<

ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(DEP_FILES)
endif

#
# functions for .lnk file generation
#

# entry point, calls gcc to generate the dependencies and generate lnk files for executables/tests
# inputs: target, lnk file, cpp file
# $(info create_lnk_file: input=[$1] [$2] [$3])
define create_lnk_file
$(call create_object_file,$1,$2,$3,$(shell $(CPP) $(CPPFLAGS) ${DEFINES} -I ${INCLUDE} ${PROJECT_INCLUDE} -MM -MT $1 $3 -MF -))
endef

# subfunction which generates the formatting for the .lnk file
# inputs: target, lnk file, cpp file, dependencies
# $(info create_object_file: input=[$1] [$2] [$3] [$4])
define create_object_file
$1: $(patsubst build/%.lnk,build/%.o,$2) $(call convert_dependencies_to_libraries,$4) $(call collect_special_libs,$4,$3) Makefile
	@mkdir -p $$(dir $$@)
	@echo "Linking $$@"
	@$${CPP} $${CPPFLAGS} -o $$@ $(patsubst build/%.lnk,build/%.o,$2) $(call convert_dependencies_to_libraries,$4) $(call collect_special_libs,$4,$3)

.PRECIOUS: $2
$2: $3 Makefile
	@mkdir -p $(dir $2)
	@echo Creating $2
	@$$(file >$2,$$(call create_lnk_file,$1,$2,$3))
endef

# subfunction which converts dependencies in other directories to library dependencies
# inputs: dependecies cpp file
# $(info convert_dependencies_to_libraries: input=[$1] [$2])
define convert_dependencies_to_libraries
$(sort $(foreach d,$(LIB_DIRS),$(if $(filter $(d)%,$(filter src/%,$(1))),build/$(notdir $(patsubst %/,%,$(d)))/$(notdir $(patsubst %/,%,$(d))).a)))
endef

# subfunction which adds 3rdparty libraries if the headers are used
# inputs: dependecies 
# $(info collect_special_libs: input=[$1] [$2])
define collect_special_libs
$(if $(filter %/sqlite3.h,$1),${SQLITE}) \
$(if $(filter src/tests/%,$2),${GTEST_LIBS}) 
endef

# Generate one .lnk rule per library directory. Its archive members and
# dependent libraries come from the sources' compiler dependency scan.
define library_source_files
$(filter-out $1main.cpp,$(wildcard $1*.cpp))
endef

define library_object_files
$(patsubst src/%.cpp,build/%.o,$(call library_source_files,$1))
endef

define library_source_dependencies
$(shell $(CPP) $(CPPFLAGS) ${DEFINES} -I ${INCLUDE} ${PROJECT_INCLUDE} -MM $(call library_source_files,$1) -MF -)
endef

define library_archive_target
build/$(notdir $(patsubst %/,%,$1))/$(notdir $(patsubst %/,%,$1)).a
endef

define library_link_target
$(patsubst %.a,%.lnk,$(call library_archive_target,$1))
endef

define library_archive_dependencies
$(filter-out $(call library_archive_target,$1),$(call convert_dependencies_to_libraries,$(call library_source_dependencies,$1)))
endef

define library_external_objects
$(if $(filter %/sqlite3.h,$(call library_source_dependencies,$1)),${SQLITE})
endef

define library_source_prerequisites
$(call library_source_files,$1) $(filter src/%,$(call library_source_dependencies,$1))
endef

define create_library_rule
$(call library_archive_target,$1): $(call library_object_files,$1) $(call library_archive_dependencies,$1) $(call library_external_objects,$1) Makefile
	@mkdir -p $$(dir $$@)
	@rm -f $$@
	@echo "Archiving ... to $$@"
	@$$(AR) $$(ARFLAGS) $$@ $(call library_object_files,$1) $(call library_archive_dependencies,$1) $(call library_external_objects,$1)

.PRECIOUS: $2
$2: $1 $(call library_source_prerequisites,$1) Makefile
	@mkdir -p $$(dir $2)
	@$$(file >$2,$$(call create_library_rule,$1,$2))
endef

define generate_library_lnk
$(if $(wildcard $2),,$(shell mkdir -p $(dir $2))$(file >$2,$(call create_library_rule,$1,$2)))
endef

ifneq ($(strip $(NEED_FILE_GENERATION)),)
$(foreach d,$(LIB_DIRS),$(call generate_library_lnk,$(d),$(call library_link_target,$(d))))
endif

#
# generate the lnk files if not available yet
#
ifneq ($(strip $(NEED_FILE_GENERATION)),)
$(foreach f,$(TST_LNK_FILES) $(EXE_LNK_FILES),\
  $(if $(wildcard $(f)),,\
    $(shell mkdir -p $(dir $(f)))\
    $(file >$(f),$(call create_lnk_file,\
      $(patsubst build/tests/%.lnk,bin/%,$(patsubst build/%/main.lnk,bin/%,$(f))),\
      $(f),\
      $(patsubst build/tests/%.lnk,src/tests/%.cpp,$(patsubst build/%/main.lnk,src/%/main.cpp,$(f)))))))
endif

ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(TST_LNK_FILES)
-include $(EXE_LNK_FILES)
-include $(LIB_LNK_FILES)
endif

.PHONY: binaries
binaries: tests executables

#
# build & execute tests
#		
.PHONY: tests	
tests: ${TST_FILES}

.PHONY: run_UTs
run_UTs:
	@$(MAKE) target=$(target) all
	@for test in ${TST_FILES}; do \
		echo "=== $$test ==="; \
		$$test --gtest_catch_exceptions=0 || exit 1; \
	done

#
# build tools
#	
.PHONY: executables
executables: ${EXE_FILES}

.PHONY: run_tests
run_tests:
	@$(MAKE) target=$(target) all
	@mkdir -p temp
	bin/Tokenizer -s -o temp/all.sq3 data/*.txt

.PHONY: run_callgrind
run_callgrind:
	@$(MAKE) target=$(target) all
	@mkdir -p temp
	valgrind --tool=callgrind --callgrind-out-file=callgrind.out bin/Tokenizer -o temp/alice_in_wonderland.sq3 data/alice_in_wonderland.txt

.PHONY: run_benchmark
run_benchmark:
	@$(MAKE) target=$(target) all
	@mkdir -p temp
	bin/Tokenizer -s -o temp/alice_in_wonderland.sq3 data/alice_in_wonderland.txt

.PHONY: run_linked_list_benchmark
run_linked_list_benchmark:
	@$(MAKE) target=$(target) all
	@mkdir -p temp
	bin/LinkedList_UT --gtest_also_run_disabled_tests --gtest_filter='*DISABLED_PushBackAndReplaceTwo*'


