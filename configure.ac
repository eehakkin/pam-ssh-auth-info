#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.69])
AC_INIT([pam-ssh-auth-info], [2.0.20250421], [Eero+pam-ssh-auth-info@Häkkinen.fi], [], [https://github.Eero.Häkkinen.fi/pam-ssh-auth-info/])
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([line_tokens_match.h])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC
AM_PROG_AR
LT_INIT([disable-static])

# Checks for libraries.
AC_CHECK_LIB([pam], [pam_get_item], [], [AC_MSG_ERROR([cannot find -lpam])])

# Checks for header files.
AC_CHECK_HEADERS([security/pam_appl.h security/pam_ext.h security/pam_modules.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_CHECK_FUNCS([pam_syslog vsyslog])

AC_CONFIG_FILES([
	Makefile
	contrib/pam-ssh-auth-info.spec
	])
AC_OUTPUT
