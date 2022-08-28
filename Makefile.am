ACLOCAL_AMFLAGS			= -I m4
EXTRA_DIST			= README.md debian
LIBS				=

TESTS				= $(check_PROGRAMS)

check_PROGRAMS			= line_tokens_match_test pattern_test

dist_man8_MANS			= pam_ssh_auth_info.8

pamdir				= $(libdir)/security
pam_LTLIBRARIES			= pam_ssh_auth_info.la

line_tokens_match_SOURCES	= \
	line_tokens_match.h \
	$(pattern_SOURCES)
line_tokens_match_test_SOURCES	= \
	line_tokens_match_test.c \
	line_tokens_match_test.h \
	$(line_tokens_match_SOURCES)
pam_ssh_auth_info_la_LDFLAGS	= \
	$(AM_LDFLAGS) -avoid-version -module -shared
pam_ssh_auth_info_la_LIBADD	= -lpam
pam_ssh_auth_info_la_SOURCES	= \
	pam_ssh_auth_info.c \
	pam_syslog.h \
	$(line_tokens_match_SOURCES)
pattern_SOURCES			= \
	pattern.h
pattern_test_SOURCES		= \
	line_tokens_match_test.h \
	pattern_test.c \
	$(pattern_SOURCES)
