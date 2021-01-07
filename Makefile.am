ACLOCAL_AMFLAGS	= -I m4
EXTRA_DIST	= LICENSE LICENSE.LESSER
LIBS		=

TESTS		= $(check_PROGRAMS)

check_PROGRAMS	= line_tokens_match_test

line_tokens_match_SOURCES	= \
	line_tokens_match.h
line_tokens_match_test_SOURCES	= \
	line_tokens_match_test.c \
	$(line_tokens_match_SOURCES)
