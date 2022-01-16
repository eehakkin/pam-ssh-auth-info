/*
 * Copyright © 2021 - 2022 Eero Häkkinen <Eero+pam-ssh-auth-info@Häkkinen.fi>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#undef NDEBUG

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "line_tokens_match.h"

struct {
	char const *lines;
	struct {
		char const *pattern;
		bool expected;
	} pattern_data[103];
} test_data[] = {
	/* The first line contains special pattern character bytes.
	 */
	{"[abbccc]", {
		{"[[]abbccc[]]", true},
		{"[][]abbccc[][]", true},
		{"\\[abbccc\\]", true},
		{"[[]abbccc?(dddd)[]]", true},
		{"[[]?(a)?(bb)?(ccc)?(dddd)[]]", true},
		{"[[]*(|a)*(|b)*(|c)*(|d)[]]", true},
		{"[[]@(a)@(bb)@(ccc)[]]", true},
		{"[[]+(|a)+(|b)+(|c)+(|d)[]]", true},
		{"[[]!(|*b*|*c*)!(|*a*|*c*)!(|*a*|*b*)[]]", true},
		{"[[][-a-z][-a-z-][-a-z-][a-z-][a-z-][a-z-][]]", true},
		{"[[][!b-z][!ac-z][!ac-z][!abd-z][!abd-z][!abd-z][]]", true},
		{"[![]abbccc[]]", false},
		{"[[][!a]bbccc[]]", false},
		{"[[]a[!a-z]bccc[]]", false},
		{"[[]abb[!ab-yz]cc[]]", false},
		{"[[]abbccc[!]]", false},
		{"[[]?(a)?(b)?(ccc)[]]", false},
		{"[[]@(a)@(b)@(ccc)[]]", false},
		{"[[]+(|a)+(|b)+(|c)+(d)[]]", false},
		{"[[]!(|*b*|*c*)!(|*b*)!(|*a*|*b*)[]]", false},
		{NULL}
	}},
	{"\\", {
		{"[\\]", true},
		{"\\\\", true},
		{"\\", true},
		{"[!\\]", false},
		{NULL}
	}},
	{"\\-", {
		{"[\\]-", true},
		{"\\\\-", true},
		{"\\-", false},
		{"[!\\]-", false},
		{NULL}
	}},
	/* The first line is empty.
	 */
	{"", {
		{"", true},
		{"*", true},
		{"@(*)", true},
		{"* *", false},
		{"@(* *)", false},
		{"*?", false},
		{"?", false},
		{"?*", false},
		{NULL}
	}},
	/* The first line is empty.
	 */
	{"\n", {
		{"", true},
		{"*", true},
		{"@(*)", true},
		{"* *", false},
		{"@(* *)", false},
		{"*?", false},
		{"?", false},
		{"?*", false},
		{NULL}
	}},
	/* The first line consists of one token.
	 */
	{"method\n", {
		/* Patterns for the only token.
		 */
		{"", false},
		{"*", true},
		{"@(*)", true},
		{"* *", false},
		{"@(* *)", false},
		{"*?", true},
		{"?", false},
		{"?*", true},
		{"*thod", true},
		{"*?thod", true},
		{"*??thod", true},
		{"?thod", false},
		{"?*thod", true},
		{"??thod", true},
		{"??*thod", true},
		{"me*od", true},
		{"me*?od", true},
		{"me*??od", true},
		{"me?od", false},
		{"me?*od", true},
		{"me??od", true},
		{"me??*od", true},
		{"meth", false},
		{"meth*", true},
		{"meth*?", true},
		{"meth*??", true},
		{"meth?", false},
		{"meth?*", true},
		{"meth??", true},
		{"meth??*", true},
		{"method", true},
		{"method*", true},
		{"method*?", false},
		{"method?", false},
		{"method?*", false},
		{NULL}
	}},
	/* The first line consists of three tokens.
	 */
	{"method key-type abcdef==\n", {
		/* Patterns for the first token.
		 */
		{"", false},
		{"*", true},
		{"@(*)", true},
		{"*?", true},
		{"?", false},
		{"?*", true},
		{"+(?)", true},
		{"*thod", true},
		{"*?thod", true},
		{"*??thod", true},
		{"?thod", false},
		{"?*thod", true},
		{"??thod", true},
		{"??*thod", true},
		{"me*od", true},
		{"me*?od", true},
		{"me*??od", true},
		{"me?od", false},
		{"me?*od", true},
		{"me??od", true},
		{"me??*od", true},
		{"meth", false},
		{"meth*", true},
		{"meth*?", true},
		{"meth*??", true},
		{"meth?", false},
		{"meth?*", true},
		{"meth??", true},
		{"meth??*", true},
		{"method", true},
		{"method*", true},
		{"method*?", false},
		{"method?", false},
		{"method?*", false},
		/* Patterns for the first two tokens.
		 */
		{"* ", false},
		{"* *", true},
		{"@(* *)", false},
		{"* *?", true},
		{"* ?", false},
		{"* ?*", true},
		{"+(?)=+(?)", true},
		{"method=*-type", true},
		{"method=*?-type", true},
		{"method=*???"/* no trigraphs */"-type", true},
		{"method=?-type", false},
		{"method=?*-type", true},
		{"method=???"/* no trigraphs */"-type", true},
		{"method=???*-type", true},
		{"method=key*type", true},
		{"method=key*?type", true},
		{"method=key*??type", false},
		{"method=key?type", true},
		{"method=key?*type", true},
		{"method=key??type", false},
		{"method=key??*type", false},
		{"method=key-", false},
		{"method=key-*", true},
		{"method=key-*?", true},
		{"method=key-*????", true},
		{"method=key-?", false},
		{"method=key-?*", true},
		{"method=key-????", true},
		{"method=key-????*", true},
		{"method=key-type", true},
		{"method=key-type*", true},
		{"method=key-type*?", false},
		{"method=key-type?", false},
		{"method=key-type?*", false},
		/* Patterns for all three tokens.
		 */
		{"* * ", false},
		{"* * *", true},
		{"* * * *", false},
		{"* * *?", true},
		{"* * ?", false},
		{"* * ?*", true},
		{"+(?)=+(?)=+(?)", true},
		{"method=key-type=*cdef==", true},
		{"method=key-type=*?cdef==", true},
		{"method=key-type=*??cdef==", true},
		{"method=key-type=?cdef==", false},
		{"method=key-type=?*cdef==", true},
		{"method=key-type=??cdef==", true},
		{"method=key-type=??*cdef==", true},
		{"method=key-type=ab*==", true},
		{"method=key-type=ab*?==", true},
		{"method=key-type=ab*????"/* no trigraphs */"==", true},
		{"method=key-type=ab?==", false},
		{"method=key-type=ab?*==", true},
		{"method=key-type=ab????"/* no trigraphs */"==", true},
		{"method=key-type=ab????*==", true},
		{"method=key-type=abcdef", false},
		{"method=key-type=abcdef*", true},
		{"method=key-type=abcdef*?", true},
		{"method=key-type=abcdef*??", true},
		{"method=key-type=abcdef?", false},
		{"method=key-type=abcdef?*", true},
		{"method=key-type=abcdef??", true},
		{"method=key-type=abcdef??*", true},
		{"method=key-type=abcdef==", true},
		{"method=key-type=abcdef==*", true},
		{"method=key-type=abcdef==*?", false},
		{"method=key-type=abcdef==?", false},
		{"method=key-type=abcdef==?*", false},
		{NULL}
	}},
	{NULL}
};

int
main() {
	unsigned const recursion_limit = 25u;
	for (int i = 0; test_data[i].lines; ++i) {
		char const *const lines = test_data[i].lines;
		for (int j = 0; test_data[i].pattern_data[j].pattern; ++j) {
			char const *const pattern =
				test_data[i].pattern_data[j].pattern;
			bool const expected =
				test_data[i].pattern_data[j].expected;
			size_t const m = strcspn(lines, "\n");
			size_t const n = strspn(lines + m, "\n");
			assert(!lines[m+n]);
			bool const actual = initial_first_line_tokens_match(
				lines,
				pattern,
				recursion_limit
				);
			fprintf(
				stderr,
				"initial_first_line_tokens_match"
				"(\"%.*s%.*s\", \"%s\", %u) %s %s\n",
				(int)m,
				lines,
				2 * (int)n,
				"\\n",
				pattern,
				recursion_limit,
				actual == expected ? "==" : "!=",
				expected ? "true" : "false"
				);
			if (actual != expected)
				return 1;
		}
	}
	fprintf(stderr, "OK\n");
	return 0;
}
