# -- Global settings --
AM_MAKEFLAGS				= --no-print-directory
AM_CFLAGS				= -I${top_srcdir}/lib -I${top_builddir}/lib
ACLOCAL_AMFLAGS				= -Im4

VERSION					= @VERSION@${SNAPSHOT_VERSION}

# -- Libraries --
extlib_LTLIBRARIES				= lib/libriemann-disguile.la

lib_libriemann_disguile_la_CFLAGS	= ${AM_CFLAGS} ${GUILE_CFLAGS} ${RIEMANN_C_CFLAGS}
lib_libriemann_disguile_la_LDFLAGS	= \
	-avoid-version -module
lib_libriemann_disguile_la_LIBADD	= ${GUILE_LIBS} ${RIEMANN_C_LIBS}
lib_libriemann_disguile_la_SOURCES	= \
	lib/disguile.c

extlibdir				= @extlibdir@

scmdir					= @scmdir@
scm_DATA				= lib/disguile.scm

# -- Testcases --
TESTS					= tests/run_tests

# -- Extra files to distribute --
EXTRA_DIST				= README.md NEWS GPL LGPL \
					  lib/disguile.scm \
					  ${TESTS} tests/test_disguile.scm

# -- Custom targets --
coverage: coverage.info.html

coverage.info.html: coverage.info
	$(AM_V_GEN)genhtml --quiet $^ --output-directory $@

coverage.info: check
	$(AM_V_GEN)lcov --quiet --capture --directory ${top_builddir}/lib --output $@ -b ${top_builddir}

.PHONY: coverage
CLEANFILES				= coverage.info

clean-local:
	rm -rf coverage.info.html
	find ${top_builddir} -name '*.gcda' -or -name '*.gcno' | xargs rm -f
