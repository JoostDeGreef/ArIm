
all: lib bin src include

clean:
	rm -rf include lib src bin *~ *.build

proper: clean
	rm -rf sqlite
	
googletest.build: googletest
	mkdir -p googletest.build
	(cd googletest.build && cmake ../googletest && make) # || rm -rf googletest.build

sqlite: 
	mkdir -p sqlite
	curl 'https://www.sqlite.org/src/tarball/sqlite.tar.gz?r=release' > sqlite/sqlite.tgz
	(cd sqlite; tar -xzf sqlite.tgz --strip-components=1)

sqlite.build: sqlite
	rm -rf sqlite.build
	mkdir -p sqlite.build
	(cd sqlite.build; sh ../sqlite/configure && make sqlite3 sqlite3.c) # || rm -rf sqlite.build

bin: sqlite.build
	rm -rf bin
	mkdir -p bin
	ln -s ../sqlite.build/sqlite3 bin/
	
src: sqlite.build
	rm -rf src
	mkdir -p src
	ln -sf ../sqlite.build/sqlite3.c src/
	
lib: googletest.build
	rm -rf lib
	mkdir -p lib
	ln -sf ../googletest.build/lib/libgmock.a lib/
	ln -sf ../googletest.build/lib/libgmock_main.a lib/
	ln -sf ../googletest.build/lib/libgtest.a lib/
	ln -sf ../googletest.build/lib/libgtest_main.a lib/

include: googletest.build sqlite.build
	rm -rf include
	mkdir -p include
	ln -sf ../googletest/googletest/include/gtest include/
	ln -sf ../googletest/googlemock/include/gmock include/
	ln -sf ../sqlite.build/sqlite3.h include/

		