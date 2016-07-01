DESTDIR=
PREFIX=/usr/local
CFLAGS=-Wall -Wextra

all: static shared hlib

minio.o: minio.c
	@$(CC) -c minio.c $(CFLAGS) -o minio.o 

libminio.a: minio.o
	@echo "building static minio library (libminio.a)"
	@$(AR) rcs libminio.a minio.o

libminio.so: minio.o
	@echo "building shared minio library (libminio.so)"
	@$(CC) -shared -fPIC minio.o -o libminio.so $(LDFLAGS) 

minio-hlib.h: minio.c minio.h
	@echo "generating #include -hlib library (minio-hlib.h)"
	
	@cp minio.h minio-hlib.h
	
	@echo                        >> minio-hlib.h
	@echo "#ifndef MINIO_HLIB_H" >> minio-hlib.h
	@echo "#define MINIO_HLIB_H" >> minio-hlib.h
	@echo "#ifdef __cplusplus"   >> minio-hlib.h
	@echo 'extern "C" {'         >> minio-hlib.h
	@echo "#endif"               >> minio-hlib.h
	
	@cat minio.c                 >> minio-hlib.h
	
	@echo "#ifdef __cplusplus"   >> minio-hlib.h
	@echo '}'                    >> minio-hlib.h
	@echo "#endif"               >> minio-hlib.h
	@echo "#endif"               >> minio-hlib.h

clean:
	@echo "removing minio.o, libminio.a, libminio.so, minio-hlib.h, test"
	@rm -f minio.o 2> /dev/null || true
	@rm -f libminio.a 2> /dev/null || true
	@rm -f libminio.so 2> /dev/null || true
	@rm -f minio-hlib.h 2> /dev/null || true
	@rm -f test 2> /dev/null || true
	@rm -f test-object 2> /dev/null || true
	@rm -f test-static 2> /dev/null || true
	@rm -f test-shared 2> /dev/null || true
	@rm -f test-hlib 2> /dev/null || true
	@chmod -R 755 tmpdir0 2> /dev/null || true
	@rm -r tmpdir0 2> /dev/null || true
	@rm tmpsrv 2> /dev/null || true
	@rm -r tmpdir 2> /dev/null || true

test-object: minio.o test.c
	$(CC) test.c minio.o $(CFLAGS) -o test-object
	@chmod -R 755 tmpdir0 2> /dev/null || true
	@rm -r tmpdir0 2>  /dev/null || true
	@rm tmpsrv 2> /dev/null || true
	@rm -r tmpdir 2> /dev/null || true 
	./test-object

test-hlib: hlib test.c
	$(CC) test.c $(CFLAGS) -o test-hlib -DMINIO_TEST_LOCAL_HLIB
	@chmod -R 755 tmpdir0 || true
	@rm -r tmpdir0 || true
	@rm tmpsrv|| true
	@rm -r tmpdir|| true
	./test-hlib

test-static: static test.c
	$(CC) -static test.c $(CFLAGS) -o test-static -L. -lminio
	@chmod -R 755 tmpdir0 2> /dev/null || true
	@rm -r tmpdir0 2>  /dev/null || true
	@rm tmpsrv 2> /dev/null || true
	@rm -r tmpdir 2> /dev/null || true 
	./test-static 

test-shared: shared test.c
	#pass "-R ." to the linker so that it will search for shared
	#libraries in the current working directory during runtime.
	$(CC) test.c libminio.so $(CFLAGS) -Wl,-R -Wl,. -o test-shared
	@chmod -R 755 tmpdir0 2> /dev/null || true
	@rm -r tmpdir0 2>  /dev/null || true
	@rm tmpsrv 2> /dev/null || true
	@rm -r tmpdir 2> /dev/null || true 
	./test-shared 

test: test-object test-hlib test-static test-shared

static: libminio.a
shared: libminio.so
hlib: minio-hlib.h

install-header:
	@echo installing minio.h into $(DESTDIR)$(PREFIX)/include
	@mkdir -p $(DESTDIR)$(PREFIX)/include
	@cp minio.h $(DESTDIR)$(PREFIX)/include
	@chmod 644 $(DESTDIR)$(PREFIX)/include/minio.h 
	@echo 

install-static: install-header static
	@echo installing libminio.a into $(DESTDIR)$(PREFIX)/lib
	@mkdir -p $(DESTDIR)$(PREFIX)/lib
	@cp libminio.a $(DESTDIR)$(PREFIX)/lib/libminio.a
	@chmod 644 $(DESTDIR)$(PREFIX)/lib/libminio.a
	@echo '  to use libminio.a in another program,'
	@echo '  set LDFLAGS="-L$(DESTDIR)$(PREFIX)/lib -lminio"'
	@echo '  and CFLAGS="-I$(DESTDIR)$(PREFIX)/include"'
	@echo 

install-shared: install-header shared
	@echo installing libminio.so into $(DESTDIR)$(PREFIX)/lib
	@mkdir -p $(DESTDIR)$(PREFIX)/lib
	@cp libminio.so $(DESTDIR)$(PREFIX)/lib/libminio.so
	@chmod 755 $(DESTDIR)$(PREFIX)/lib/libminio.so
	@echo '  to use libminio.so in another program,'
	@echo '  set LDFLAGS="-lminio"'
	@echo '  and CFLAGS="-I$(DESTDIR)$(PREFIX)/include"'
	@echo 
	@echo run \'ldconfig\' now to add libminio.so to the shared library database.

install-hlib: install-header hlib
	@echo installing minio-hlib.h into $(DESTDIR)$(PREFIX)/include
	@mkdir -p $(DESTDIR)$(PREFIX)/include
	@cp minio-hlib.h $(DESTDIR)$(PREFIX)/include/minio-hlib.h
	@chmod 644 $(DESTDIR)$(PREFIX)/include/minio-hlib.h
	@echo '  to use minio-hlib.h in another program,'
	@echo '  set CFLAGS="-I$(DESTDIR)$(PREFIX)/include"'
	@echo '  and #include<minio-hlib.h> in *one* of your .c files'
	@echo '(just #include<minio.h> as normal in the other .c files)'
	@echo 

install: install-static install-shared install-hlib

uninstall-header:
	@echo uninstalling minio.h from $(DESTDIR)$(PREFIX)/include
	@rm -f $(DESTDIR)$(PREFIX)/include/minio.h || true

uninstall-static:
	@echo uninstalling libminio.a from $(DESTDIR)$(PREFIX)/lib
	@rm -f $(DESTDIR)$(PREFIX)/lib/libminio.a || true

uninstall-shared: 
	@echo uninstalling libminio.so from $(DESTDIR)$(PREFIX)/lib
	@rm -f $(DESTDIR)$(PREFIX)/lib/libminio.so || true

uninstall-hlib:
	@echo uninstalling minio-hlib.h from $(DESTDIR)$(PREFIX)/include
	@rm -f $(DESTDIR)$(PREFIX)/include/minio-hlib.h || true 

uninstall: uninstall-static uninstall-shared uninstall-hlib uninstall-header

options:
	@echo these are the current configuration options for minio:
	@echo CC=$(CC)
	@echo AR=$(AR)
	@echo CFLAGS=$(CFLAGS)
	@echo LDFLAGS=$(LDFLAGS)
	@echo PREFIX=$(PREFIX)
	@echo DESTDIR=$(DESTDIR)

help:
	@echo minio makefile help 
	@echo
	@echo \'make\' will build libminio.so, libminio.a, minio-hlib.h
	@echo \'make clean\' will delete all the compiled minio files from this directory.  it will *not* uninstall libminio.so or libminio.a
	@echo \'make test\' build minio and test the library.  it will not install anything.
	@echo \'make help\' will show this options page
	@echo \'make install\' will install libminio.so and libminio.a into $(DESTDIR)$(PREFIX)/lib, and minio.h into $(DESTDIR)$(PREFIX)/include
	@echo \'make uninstall\' will delete $(DESTDIR)$(PREFIX)/lib/libminio.so, $(DESTDIR)$(PREFIX)/lib/libminio.a, and $(DESTDIR)$(PREFIX)/include/minio.h 
	@echo 
	@echo \'make static\' will only build libminio.a
	@echo \'make shared\' will only build libminio.so
	@echo \'make hlib\' will only generatre minio-hlib.h
	@echo
	@echo \'make options\' will list the defaults that control how minio will be built, and where it will install to.
	@echo these defaults can be overridden by assinging them values from the commandline, like so:
	@echo '  to compile minio using the gcc compiler instead of the default cc, type "make CC=gcc"'
	@echo '  to compile minio using the clang compiler instead of the default cc, type "make CC=clang"'
	@echo '  to compile minio using the tcc compiler compiler instead of the default cc, type "make CC=tcc"'
	@echo '  to install into your home directory, type: "make install PREFIX=$$HOME"'
	@echo '  to uninstall from your home directory, type: "make uninstall PREFIX=$$HOME"'
	@echo '  (remember to pass the same PREFIX value for uninstalling that you did for installing)'
	@echo
	@echo more fine-grained control is provided by:
	@echo \'make install-static\', \'make install-shared\', \'make install-hlib\', \'make install-header\',
	@echo \'make uninstall-static\', \'make uninstall-shared\', \'make uninstall-hlib\', \'make uninstall-header\'
	@echo \(which all do as their names suggest\)
	@echo

.PHONY: \
all options help test clean \
uninstall uninstall-shared uninstall-static uninstall-header \
install install-shared install-staic install-header \
hlib install-hlib uninstall-hlib \
test-object test-static test-shared test-hlib

