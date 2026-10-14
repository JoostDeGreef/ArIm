# tools
AR=ar
CC=gcc
CPP=g++
CFLAGS=-W -Wall -std=c17
CPPFLAGS=-W -Wall -std=c++17
DEFINES=
INCLUDE=3rdParty/include
PROJECT_INCLUDE=-I src/tokenizer
GTEST_LIBS=3rdParty/lib/libgtest_main.a 3rdParty/lib/libgtest.a
SQLITE=build/3rdParty/sqlite3.o

# Add .d / .lnk to Make's recognized suffixes.
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
# generic targets
#

message:
	@echo "Either use 'make release' or 'make debug', possibly augmented with 'run'"
	@echo "(so 'make debug run' would run a debug build of the unit tests)"
	@echo "Any other argument: Here Be Dragons"

all: 3rdParty executables tests

run: 3rdParty run_tests

release:
	if [ -f build/.debug ]; then make clean; fi
	mkdir -p build
	touch build/.release
	$(MAKE) CPPFLAGS="$(CPPFLAGS) -O3" all

debug:
	if [ -f build/.release ]; then make clean; fi
	mkdir -p build
	touch build/.debug
	$(MAKE) CPPFLAGS="$(CPPFLAGS) -g3 -O0" all

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
# sqlite
#
	
${SQLITE}: 
	@mkdir -p $(dir $@)
	${CC} ${CFLAGS} ${DEFINES} -isystem ${INCLUDE} -fPIC -o ${SQLITE} -c 3rdParty/src/sqlite3.c

#
# generic build rules
#

.SECONDEXPANSION:

# generic

.PRECIOUS: build/%.d
build/%.d: src/%.cpp Makefile
	@mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) ${DEFINES} -I ${INCLUDE} ${PROJECT_INCLUDE} -MM -MT '$(patsubst src/%.cpp,build/%.o,$<)' $< -MF $@

.PRECIOUS: build/%.o 
build/%.o: src/%.cpp build/%.d Makefile
	@mkdir -p $(dir $@)
	${CPP} ${CPPFLAGS} ${DEFINES} -isystem ${INCLUDE} ${PROJECT_INCLUDE} -fPIC -o $@ -c $<

#
# helper functions
#

define lnk_file_from_src
    $(call process_compiler_output,$(shell $(CPP) $(CPPFLAGS) ${DEFINES} -I ${INCLUDE} ${PROJECT_INCLUDE} -MM -MT '$(patsubst src/tests/%.cpp,build/tests/%,$(patsubst src/%/main.cpp,build/%,$1))' $1 -MF -))
endef
define process_compiler_output
	$(subst :,: OBJECTS += ,$(patsubst build/%,bin/%,$(firstword $1))) \
	$(patsubst src/%.cpp,build/%.o,$(sort $(wildcard $(patsubst %.h,%.cpp,$(filter src/%,$1))))) \
	"\n" \
	$(subst :,: LDLIBS += ,$(patsubst build/%,bin/%,$(firstword $1))) \
	$(if $(filter %/sqlite3.h,$1),${SQLITE}) \
	$(if $(filter build/tests/%,$1),${GTEST_LIBS})
endef

# executables

.PRECIOUS: build/%/main.lnk
build/%/main.lnk: src/%/main.cpp Makefile
	@mkdir -p $(dir $@)
	@echo Creating $@
	@echo $(call lnk_file_from_src,$<) | awk '{$$1=$$1;print}' > $@

.PRECIOUS: build/%
bin/%: build/%/main.lnk $${LDLIBS} $${OBJECTS} Makefile
	@mkdir -p $(dir $@)
	${CPP} ${CPPFLAGS} -o $@ ${LDLIBS} ${OBJECTS} 

#tests 

.PRECIOUS: build/tests/%.lnk
build/tests/%.lnk: src/tests/%.cpp Makefile
	@mkdir -p $(dir $@)
	@echo Creating $@
	@echo $(call lnk_file_from_src,$<) | awk '{$$1=$$1;print}' > $@

.PRECIOUS: build/tests/%
bin/tests/%: build/tests/%.lnk $${LDLIBS} $${OBJECTS} Makefile
	@mkdir -p $(dir $@)
	${CPP} ${CPPFLAGS} -o $@ ${LDLIBS} ${OBJECTS} 

#
# build & execute tests
#		
.PHONY: tests	
tests: ${TST_FILES}

.PHONY: run_tests
run_tests: ${TST_FILES}
	@for test in $^; do \
		echo "=== $$test ==="; \
		$$test || exit 1; \
	done
	
#
# build tools
#	
.PHONY: executables
executables: ${EXE_FILES}

