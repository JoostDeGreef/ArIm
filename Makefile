# tools
AR=ar
CC=gcc
CPP=g++
CFLAGS=-W -Wall
CPPFLAGS=-W -Wall
DEFINES=
INCLUDE=-I 3rdParty/include
GTEST_LIBS=3rdParty/lib/libgtest_main.a 3rdParty/lib/libgtest.a
SQLITE=build/3rdParty/sqlite3.o

# Add .d to Make's recognized suffixes.
SUFFIXES += .d

#
# turn all CPP files into objects / executables
#
CPP_FILES := $(wildcard src/*.cpp src/*/*.cpp)
OBJ_FILES := $(patsubst src/%.cpp,build/%.o,$(CPP_FILES))
TST_FILES := $(patsubst src/tests/%.cpp,bin/tests/%,$(wildcard src/tests/*.cpp))
EXE_FILES := $(patsubst src/%/main.cpp,bin/%,$(wildcard src/*/main.cpp))
DEP_FILES := $(OBJ_FILES:.o=.d)

#
# define which libraries are needed for each executable
#
bin/tests/sqlite_smoketest: LDLIBS += ${GTEST_LIBS} ${SQLITE}
bin/tokenizer: LDLIBS += ${SQLITE}

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
# include generated dependencies
#
-include $(DEP_FILES)
    
#
# sqlite
#
	
${SQLITE}:
	@mkdir -p $(dir $@)
	${CC} ${CFLAGS} ${DEFINES} ${INCLUDE} -fPIC -o ${SQLITE} -c 3rdParty/src/sqlite3.c

#
# generic build rules
#

.PRECIOUS: build/%.d
build/%.d: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MM -MT '$(patsubst src/%.cpp,build/%.o,$<)' $< -MF $@
    
.PRECIOUS: build/%.o
build/%.o: src/%.cpp build/%.d
	@mkdir -p $(dir $@)
	${CPP} ${CPPFLAGS} ${DEFINES} ${INCLUDE} -fPIC -o $@ -c $<

.PRECIOUS: build/tests/%
bin/tests/%: build/tests/%.o ${LDLIBS} 
	@mkdir -p $(dir $@)
	$(if $(strip $(LDLIBS)),,$(error Please adjust Makefile: no LDLIBS defined for $@))
	${CPP} ${CPPFLAGS} -o $@ $< ${LDLIBS}

.PRECIOUS: build/%
bin/%: build/%/main.o ${LDLIBS} 
	@mkdir -p $(dir $@)
	$(if $(strip $(LDLIBS)),,$(error Please adjust Makefile: no LDLIBS defined for $@))
	${CPP} ${CPPFLAGS} -o $@ $< ${LDLIBS}

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

