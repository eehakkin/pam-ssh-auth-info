ACLOCAL_AMFLAGS	= -I m4
EXTRA_DIST	= LICENSE LICENSE.LESSER
LIBS		=

TESTS		= $(check_PROGRAMS)

check_PROGRAMS	= line_tokens_match_test

pamdir		= $(libdir)/security
pam_LTLIBRARIES	= pam_ssh_auth_info.la

line_tokens_match_SOURCES	= \
	line_tokens_match.h
line_tokens_match_test_SOURCES	= \
	line_tokens_match_test.c \
	$(line_tokens_match_SOURCES)
pam_ssh_auth_info_la_LDFLAGS	= $(AM_LDFLAGS) -avoid-version -module -shared
pam_ssh_auth_info_la_LIBADD	= -lpam
pam_ssh_auth_info_la_SOURCES	= \
	pam_ssh_auth_info.c \
	$(line_tokens_match_SOURCES)
