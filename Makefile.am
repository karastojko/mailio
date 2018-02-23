pkgconfig_DATA = libmailio.pc
ACLOCAL_AMFLAGS = -I m4
SUBDIRS = src examples

EXTRA_DIST = Doxyfile.in

# simple doxygen target
doc: Doxyfile.in
	doxygen Doxyfile

# clean up the extras including doxygen
clean-local:
	cd ${top_srcdir}
	rm -rf doc
