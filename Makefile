
AR=ar
CC=gcc
CPP=g++
CFLAGS=-W -Wall
CPPFLAGS=-W -Wall
#CFLAGS=
#CPPFLAGS=
DEFINES=
INCLUDE=-I 3rdParty/include
GTEST_LIBS=3rdParty/lib/libgtest_main.a 3rdParty/lib/libgtest.a
SQLITE=build/3rdParty/sqlite3.o

all: 3rdParty executables tests

.PHONY: 3rdParty
3rdParty:
	make -C 3rdParty all
		
clean:
	rm -rf bin build

bin: 
	mkdir -p bin

build:
	mkdir -p build
	
build/tests: build
	mkdir -p build/tests	
	
build/tokenizer: build
	mkdir -p build/tokenizer

build/3rdParty: build
	mkdir -p build/3rdParty

#
# sqlite
#
	
${SQLITE}: build/3rdParty
	${CC} ${CCFLAGS} ${DEFINES} ${INCLUDE} -fPIC -o ${SQLITE} -c 3rdParty/src/sqlite3.c

#
# sqlite_smoketest
#			
bin/sqlite_smoketest: bin build/tests/sqlite_smoketest.o ${SQLITE}
	${CPP} ${CPPFLAGS} -o bin/sqlite_smoketest build/tests/sqlite_smoketest.o ${GTEST_LIBS} ${SQLITE}

build/tests/sqlite_smoketest.o: build/tests src/tests/sqlite_smoketest.cpp
	${CPP} ${CPPFLAGS} ${DEFINES} ${INCLUDE} -fPIC -o build/tests/sqlite_smoketest.o -c src/tests/sqlite_smoketest.cpp

#
# tokenizer
#
bin/tokenizer: bin build/tokenizer/main.o ${SQLITE}
	${CPP} ${CPPFLAGS} -o bin/tokenizer build/tokenizer/main.o ${SQLITE}

build/tokenizer/main.o: build/tokenizer src/tokenizer/main.cpp
	${CPP} ${CPPFLAGS} ${DEFINES} ${INCLUDE} -fPIC -o build/tokenizer/main.o -c src/tokenizer/main.cpp

#
# build & execute tests
#		
.PHONY: tests	
tests: bin/sqlite_smoketest
	bin/sqlite_smoketest
	
#
# build tools
#	
.PHONY: executables
executables: bin/tokenizer
	
