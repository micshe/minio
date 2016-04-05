DESTDIR=
PREFIX=/usr/local
CFLAGS=-Wall

all: static shared

minio.o: minio.c
	@$(CC) -c minio.c $(CFLAGS) -o minio.o 

libminio.a: minio.o
	@echo "building static minio library (libminio.a)"
	@$(AR) rcs libminio.a minio.o

libminio.so: minio.o
	@echo "building shared minio library (libminio.so)"
	@$(CC) -shared -fPIC minio.o -o libminio.so $(LDFLAGS) 

clean:
	@echo "removing minio.o, libminio.a, libminio.so, test"
	@rm -f minio.o 2> /dev/null || true
	@rm -f libminio.a 2> /dev/null || true
	@rm -f libminio.so 2> /dev/null || true
	@rm -f test 2> /dev/null || true
	@chmod -R 755 tmpdir0 2> /dev/null || true
	@rm -r tmpdir0 2> /dev/null || true
	@rm tmpsrv 2> /dev/null || true
	@rm -r tmpdir 2> /dev/null || true

test: minio.o test.c
	$(CC) test.c minio.o $(CFLAGS) -o test
	@chmod -R 755 tmpdir0 2> /dev/null || true
	@rm -r tmpdir0 2>  /dev/null || true
	@rm tmpsrv 2> /dev/null || true
	@rm -r tmpdir 2> /dev/null || true 
	./test 

branch: 
	$(CC) test.c branch.minio.c -o test
	./test

static: libminio.a
shared: libminio.so

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

install: install-static install-shared 

uninstall-header:
	@echo uninstalling minio.h from $(DESTDIR)$(PREFIX)/include
	@rm -f $(DESTDIR)$(PREFIX)/include/minio.h || true

uninstall-static: uninstall-header
	@echo uninstalling libminio.a from $(DESTDIR)$(PREFIX)/lib
	@rm -f $(DESTDIR)$(PREFIX)/lib/libminio.a || true

uninstall-shared: uninstall-header
	@echo uninstalling libminio.so from $(DESTDIR)$(PREFIX)/lib
	@rm -f $(DESTDIR)$(PREFIX)/lib/libminio.so || true

uninstall: uninstall-static uninstall-shared

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
	@echo \'make\' will build libminio.so and libminio.a
	@echo \'make clean\' will delete all the compiled minio files from this directory.  it will *not* uninstall libminio.so or libminio.a
	@echo \'make test\' build minio and test the library.  it will not install anything.
	@echo \'make help\' will show this options page
	@echo \'make install\' will install libminio.so and libminio.a into $(DESTDIR)$(PREFIX)/lib, and minio.h insto $(DESTDIR)$(PREFIX)/include
	@echo \'make uninstall\' will delete $(DESTDIR)$(PREFIX)/lib/libminio.so, $(DESTDIR)$(PREFIX)/lib/libminio.a, and $(DESTDIR)$(PREFIX)/include/minio.h 
	@echo 
	@echo \'make static\' will only build libminio.a
	@echo \'make shared\' will only build libminio.so
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
	@echo \'make install-static\', \'make install-shared\', \'make install-header\',
	@echo \'make uninstall-static\', \'make uninstall-shared\', \'make uninstall-header\'
	@echo \(which all do as their names suggest\)
	@echo

.PHONY: \
all options help test clean \
uninstall uninstall-shared uninstall-static uninstall-header \
install install-shared install-staic install-header

