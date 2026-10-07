# tools
AR=ar
CC=gcc
CPP=g++
CFLAGS=-W -Wall
CPPFLAGS=-W -Wall
DEFINES=
INCLUDE=-I 3rdParty/include -I src/tokenizer
GTEST_LIBS=3rdParty/lib/libgtest_main.a 3rdParty/lib/libgtest.a
SQLITE=build/3rdParty/sqlite3.o

# Add .d / .lnkto Make's recognized suffixes.
SUFFIXES += .d .lnk

#
# turn all CPP files into objects / executables
#
CPP_FILES := $(wildcard src/*.cpp src/*/*.cpp)
OBJ_FILES := $(patsubst src/%.cpp,build/%.o,$(CPP_FILES))
TST_FILES := $(patsubst src/tests/%.cpp,bin/tests/%,$(wildcard src/tests/*.cpp))
EXE_FILES := $(patsubst src/%/main.cpp,bin/%,$(wildcard src/*/main.cpp))
DEP_FILES := $(OBJ_FILES:.o=.d)
LNK_FILES := $(patsubst bin/tests/%,build/tests/%.lnk,$(TST_FILES)) $(patsubst bin/%,build/%/main.lnk,$(EXE_FILES))

#
# define which libraries are needed for each executable
#
bin/tests/sqlite_smoketest: LDLIBS += ${GTEST_LIBS} ${SQLITE}
#bin/tests/sqlite_smoketest: OBJECTS += build/tests/sqlite_smoketest.o
bin/tests/tokenizer_ut: LDLIBS += ${GTEST_LIBS} ${SQLITE}
#bin/tests/tokenizer_ut: OBJECTS += build/tests/tokenizer_ut.o
bin/tokenizer: LDLIBS += ${SQLITE}
#bin/tokenizer: OBJECTS += build/tokenizer/main.o build/tokenizer/tokenizer.o

#
# generic targets
#

all: 3rdParty executables tests

.PHONY: 3rdParty
3rdParty:
	make -C 3rdParty all
		
clean:
	rm -rf bin build
	find . -name "*~" -exec rm -f "{}" \;

#
# include generated dependencies/objects for linking
#
-include $(DEP_FILES)
-include $(LNK_FILES)
    
#
# functions
#

# Convert a dependency list into candidate source files
define deps_to_sources
$(sort \
  $(filter $(CPP_FILES), \
    $(patsubst %.h,%.cpp,$(filter %.h %.cpp,$1)) \
  ) \
)
endef

define sources_to_objects
$(patsubst src/%.cpp,build/%.o,$1)
endef

#
# sqlite
#
	
${SQLITE}: 
	@mkdir -p $(dir $@)
	${CC} ${CFLAGS} ${DEFINES} ${INCLUDE} -fPIC -o ${SQLITE} -c 3rdParty/src/sqlite3.c

#
# generic build rules
#

.SECONDEXPANSION:

# generic

.PRECIOUS: build/%.d
build/%.d: src/%.cpp Makefile
	@mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) ${DEFINES} ${INCLUDE} -MM -MT '$(patsubst src/%.cpp,build/%.o,$<)' $< -MF $@

.PRECIOUS: build/%.o 
build/%.o: src/%.cpp build/%.d Makefile
	@mkdir -p $(dir $@)
	${CPP} ${CPPFLAGS} ${DEFINES} ${INCLUDE} -fPIC -o $@ -c $<

# executables

define sources_to_objects
    $(info sources_to_objects: [$1])
    $(call process_sources,$(shell $(CPP) $(CPPFLAGS) ${DEFINES} ${INCLUDE} -MM -MT '$(patsubst src/tests/%.cpp,build/tests/%,$(patsubst src/%/main.cpp,build/%,$1))' $1 -MF -))
endef
define process_sources
    $(info process_sources: [$1])
	echo $(subst :,: OBJECTS += ,$(patsubst build/%,bin/%,$(firstword $1))) \
	     $(patsubst src/%.cpp,build/%.o,$(sort $(wildcard $(patsubst %.h,%.cpp,$(filter src/%,$1)))))
endef

.PRECIOUS: build/%/main.lnk
build/%/main.lnk: src/%/main.cpp Makefile
	@mkdir -p $(dir $@)
	@echo $(call sources_to_objects,$<) > $@

.PRECIOUS: build/%
bin/%: build/%/main.lnk $${LDLIBS} $${OBJECTS} Makefile
	@mkdir -p $(dir $@)
	$(if $(strip $(LDLIBS)),,$(error Please adjust Makefile: no LDLIBS defined for $@))
	${CPP} ${CPPFLAGS} -o $@ ${LDLIBS} ${OBJECTS} 

#tests 

.PRECIOUS: build/tests/%.lnk
build/tests/%.lnk: src/tests/%.cpp Makefile
	@mkdir -p $(dir $@)
	@echo $(call sources_to_objects,$<) > $@

.PRECIOUS: build/tests/%
bin/tests/%: build/tests/%.lnk $${LDLIBS} $${OBJECTS} Makefile
	@mkdir -p $(dir $@)
	$(if $(strip $(LDLIBS)),,$(error Please adjust Makefile: no LDLIBS defined for $@))
	${CPP} ${CPPFLAGS} -o $@ ${LDLIBS} ${OBJECTS} 

#
# build & execute tests
#		
.PHONY: tests	
tests: ${TST_FILES}
	@for test in $^; do \
		echo "=== $$test ==="; \
		$$test || exit 1; \
	done
	
#
# build tools
#	
.PHONY: executables
executables: ${EXE_FILES}

