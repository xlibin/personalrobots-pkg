include $(shell rospack find mk)/cmake.mk

#all: lib/libjpeg.a
#	cd src && make $(PARALLEL_JOBS)
#
#build/jpeg-6b:
#	mkdir build
#	cd build; tar xzf ../jpeg-6b.tar.gz
#
#lib/libjpeg.a: build/jpeg-6b
#	-mkdir lib
#	cd build/jpeg-6b && CFLAGS=-fPIC ./configure
#	cd build/jpeg-6b && make $(PARALLEL_JOBS)
#	# don't do a make install. instead, the following lines take exactly what we need
#	cp build/jpeg-6b/libjpeg.a lib/libjpeg.a
#	cp build/jpeg-6b/jpeglib.h include/ijg_libjpeg/jpeglib.h
#	cp build/jpeg-6b/jmorecfg.h include/ijg_libjpeg/jmorecfg.h
#	cp build/jpeg-6b/jerror.h include/ijg_libjpeg/jerror.h
#	cp build/jpeg-6b/jpegint.h include/ijg_libjpeg/jpegint.h
#	sed -e '/HAVE_STDLIB_H/d' -e '/HAVE_STDDEF_H/d' <build/jpeg-6b/jconfig.h >include/ijg_libjpeg/jconfig.h
#
#clean:
#	cd src && make clean
#	-rm -rf build lib/libjpeg.a
#	-rm include/ijg_libjpeg/jpeglib.h
#	-rm include/ijg_libjpeg/jmorecfg.h
#	-rm include/ijg_libjpeg/jerror.h
#	-rm include/ijg_libjpeg/jpegint.h
#	-rm include/ijg_libjpeg/jconfig.h
#
#wipe: clean
#
#.PHONY : clean
