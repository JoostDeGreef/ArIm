# tools
AR=ar
ARFLAGS=cr
CC=gcc
CPP=g++
CFLAGS=-W -Wall -std=c17
CPPFLAGS=-W -Wall -std=c++20
DEFINES=
INCLUDE=3rdParty/include
PROJECT_INCLUDE=-I src/tokenizer -I src/db
GTEST_LIBS=3rdParty/lib/libgtest_main.a 3rdParty/lib/libgtest.a
SQLITE=build/3rdParty/sqlite3.o

# Add .d / .lnk to Make's recognized suffixes.
SUFFIXES += .d .lnk

# make $$ work
.SECONDEXPANSION:

#
# Issue a warning if people just execute this makefile without any idea of what havock it can wreak.
#
message:
	@echo "Either use 'make release' or 'make debug', possibly augmented with 'run'"
	@echo "(so 'make debug run' would run a debug build of the unit tests)"
	@echo "Any other argument: Here Be Dragons"

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

	
clean:
	rm -rf bin build
	find . -name "*~" -exec rm -f "{}" \;

#
# call the makefile for the 3dParty libraries
#

.PHONY: 3rdParty
3rdParty:
	make -C 3rdParty all

${SQLITE}: 
	@mkdir -p $(dir $@)
	${CC} ${CFLAGS} ${DEFINES} -isystem ${INCLUDE} -fPIC -o ${SQLITE} -c 3rdParty/src/sqlite3.c

#
# turn all CPP files into objects / executables and create a dependency file for all
#
CPP_FILES := $(wildcard src/*.cpp src/*/*.cpp)
OBJ_FILES := $(patsubst src/%.cpp,build/%.o,$(CPP_FILES))
DEP_FILES := $(OBJ_FILES:.o=.d)

LIB_DIRS := $(filter-out src/tests/,$(wildcard src/*/))
LIB_FILES := $(foreach d,$(LIB_DIRS),build/$(notdir $(d))/$(notdir $(d)).a)

TST_LNK_FILES := $(patsubst src/tests/%.cpp,build/tests/%.lnk,$(wildcard src/tests/*.cpp))
EXE_LNK_FILES := $(patsubst src/%/main.cpp,build/%/main.lnk,$(wildcard src/*/main.cpp))
LIB_LNK_FILES := $(foreach d,$(LIB_DIRS),build/$(notdir $(d))/$(notdir $(d)).lnk)

#
# dependency files
#
.PRECIOUS: build/%.d
build/%.d: src/%.cpp Makefile
	@mkdir -p $(dir $@)
	$(CPP) $(CPPFLAGS) ${DEFINES} -I ${INCLUDE} ${PROJECT_INCLUDE} -MM -MT '$(patsubst src/%.cpp,build/%.o,$<)' $< -MF $@

#
# object files
#
.PRECIOUS: build/%.o 
build/%.o: src/%.cpp build/%.d Makefile
	@mkdir -p $(dir $@)
	${CPP} ${CPPFLAGS} ${DEFINES} -isystem ${INCLUDE} ${PROJECT_INCLUDE} -fPIC -o $@ -c $<

#
# archive files
#
define LIB_template
$1/$(patsubst %.a,build/%/,$(1)).a: $$(patsubst src/%.cpp,build/%.o,$$(filter-out $(1)/main.cpp,$$(wildcard $(1)/*.cpp)))
	@mkdir -p $$(dir $$@)
	$$(AR) $$(ARFLAGS) $$@ $$^
endef
#$(info LIB_DIRS=[$(LIB_DIRS)])
$(foreach d,$(LIB_DIRS),$(eval $(call LIB_template,$(d))))

-include $(DEP_FILES)
-include $(TST_LNK_FILES)
-include $(EXE_LNK_FILES)
-include $(LIB_LNK_FILES)

# inputs target, lnk file, cpp file
# $(info create_lnk_file: input=[$1] [$2] [$3])
define create_lnk_file
$(call create_object_file,$1,$2,$3,$(shell $(CPP) $(CPPFLAGS) ${DEFINES} -I ${INCLUDE} ${PROJECT_INCLUDE} -MM -MT $1 $3 -MF -))
endef
# inputs: target, lnk file, cpp file, dependencies
# $(info create_object_file: input=[$1] [$2] [$3] [$4])
define create_object_file
$1: OBJECTS += $(patsubst build/%.lnk,build/%.o,$2)
$1: LDLIBS += $(call convert_dependencies_to_libraries,$4) $(call collect_special_libs,$4,$3)
$1: $3 $${LDLIBS} $${OBJECTS} Makefile
	@mkdir -p $$(dir $$@) 
	$${CPP} $${CPPFLAGS} -o $$@ $${OBJECTS} $${LDLIBS}
endef
# inputs: dependecies cpp file
# $(info convert_dependencies_to_libraries: input=[$1] [$2])
define convert_dependencies_to_libraries
$(sort $(foreach d,$(LIB_DIRS),$(if $(filter $(d)%,$(filter src/%,$(1))),build/$(notdir $(patsubst %/,%,$(d)))/$(notdir $(patsubst %/,%,$(d))).a)))
endef
# inputs: dependecies 
# $(info collect_special_libs: input=[$1] [$2])
define collect_special_libs
$(if $(filter %/sqlite3.h,$1),${SQLITE}) \
$(if $(filter src/tests/%,$2),${GTEST_LIBS}) 
endef

.PRECIOUS: build/%/main.lnk
build/%/main.lnk: src/%/main.cpp Makefile
	@mkdir -p $(dir $@)
	@echo Creating $@
	@$(file >$@,$(call create_lnk_file,$(patsubst build/%/main.lnk,bin/%,$@),$@,$<))

.PRECIOUS: build/tests/%.lnk
build/tests/%.lnk: src/tests/%.cpp Makefile
	@mkdir -p $(dir $@)
	@echo Creating $@
	@$(file >$@,$(call create_lnk_file,$(patsubst build/tests/%.lnk,bin/%,$@),$@,$<))

# .PRECIOUS: build/%/%.lnk
# build/%/%.lnk: Makefile
# 	@mkdir -p $(dir $@)
# 	@echo Creating $@
# 	@$(file >$@,$(call create_lnk_file,$(patsubst build/tests/%.lnk,bin/%,$@),$@,$<))


#all: 

# #
# # build & execute tests
# #		
# .PHONY: tests	
# tests: ${TST_FILES}

# .PHONY: run_tests
# run_tests: ${TST_FILES}
# 	@for test in $^; do \
# 		echo "=== $$test ==="; \
# 		$$test || exit 1; \
# 	done
	
# #
# # build tools
# #	
# .PHONY: executables
# executables: ${EXE_FILES}

