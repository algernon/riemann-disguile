# -*- Autoconf -*-

AC_INIT([riemann-disguile], [0.0.1], [algernon@madhouse-project.org])
AM_INIT_AUTOMAKE([foreign subdir-objects dist-xz no-dist-gzip])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_SRCDIR([lib/disguile.c])
AC_CONFIG_HEADERS([lib/platform.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_CXX

LT_INIT([shared])

AC_CHECK_HEADERS([])
AC_CHECK_FUNCS([])
AC_FUNC_MALLOC
AC_FUNC_REALLOC

# Check for pkg-config-enabled libraries
PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES(GUILE, guile-2.0)
AC_SUBST(extlibdir, [`pkg-config guile-2.0 --variable extensiondir`])
AC_SUBST(scmdir, [`pkg-config guile-2.0 --variable sitedir`])

PKG_CHECK_MODULES(RIEMANN_C, riemann-client >= 1.4.0)

AC_CONFIG_FILES([
        Makefile
])
AC_OUTPUT
