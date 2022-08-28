/*
 * Copyright © 2021 - 2022 Eero Häkkinen <Eero+pam-ssh-auth-info@Häkkinen.fi>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef MAX
#	define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

static const
struct {
	char const *lines;
	struct {
		char const *pattern;
		bool expected;
		size_t expected_min;
		size_t expected_max;
	} pattern_data[103];
} test_data[] = {
	/* The first line contains special pattern character bytes.
	 */
	{"[abbccc]", {
		{"[[]abbccc[]]", true, 8u, 8u},
		{"[][]abbccc[][]", true, 8u, 8u},
		{"\\[abbccc\\]", true, 8u, 8u},
		{"[[]abbccc?(dddd)[]]", true, 8u, 12u},
		{"[[]?(a)?(bb)?(ccc)?(dddd)[]]", true, 2u, 12u},
		{"[[]*(|a)*(|b)*(|c)*(|d)[]]", true, 2u, MAX(2u + 4u * (size_t)UINT_MAX, UINT_MAX)},
		{"[[]@(a)@(bb)@(ccc)[]]", true, 8u, 8u},
		{"[[]+(|a)+(|b)+(|c)+(|d)[]]", true, 2u, MAX(2u + 4u * (size_t)UINT_MAX, UINT_MAX)},
		{"[[]!(|*b*|*c*)!(|*a*|*c*)!(|*a*|*b*)[]]", true, 2u, SIZE_MAX},
		{"[[][-a-z][-a-z-][-a-z-][a-z-][a-z-][a-z-][]]", true, 8u, 8u},
		{"[[][!b-z][!ac-z][!ac-z][!abd-z][!abd-z][!abd-z][]]", true, 8u, 8u},
		{"[![]abbccc[]]", false, 8u, 8u},
		{"[[][!a]bbccc[]]", false, 8u, 8u},
		{"[[]a[!a-z]bccc[]]", false, 8u, 8u},
		{"[[]abb[!ab-yz]cc[]]", false, 8u, 8u},
		{"[[]abbccc[!]]", false, 8u, 8u},
		{"[[]?(a)?(b)?(ccc)[]]", false, 2u, 7u},
		{"[[]@(a)@(b)@(ccc)[]]", false, 7u, 7u},
		{"[[]+(|a)+(|b)+(|c)+(d)[]]", false, 3u, MAX(2u + 4u * (size_t)UINT_MAX, UINT_MAX)},
		{"[[]!(|*b*|*c*)!(|*b*)!(|*a*|*b*)[]]", false, 2u, SIZE_MAX},
		{NULL}
	}},
	{"\\", {
		{"[\\]", true, 1u, 1u},
		{"\\\\", true, 1u, 1u},
		{"\\", true, 1u, 1u},
		{"[!\\]", false, 1u, 1u},
		{NULL}
	}},
	{"\\-", {
		{"[\\]-", true, 2u, 2u},
		{"\\\\-", true, 2u, 2u},
		{"\\-", false, 1u, 1u},
		{"[!\\]-", false, 2u, 2u},
		{NULL}
	}},
	/* The first line is empty.
	 */
	{"", {
		{"", true, 0u, 0u},
		{"*", true, 0u, SIZE_MAX},
		{"@(*)", true, 0u, SIZE_MAX},
		{"* *", false, 1u, SIZE_MAX},
		{"@(* *)", false, 1u, SIZE_MAX},
		{"*?", false, 1u, SIZE_MAX},
		{"?", false, 1u, 1u},
		{"?*", false, 1u, SIZE_MAX},
		{NULL}
	}},
	/* The first line is empty.
	 */
	{"\n", {
		{"", true, 0u, 0u},
		{"*", true, 0u, SIZE_MAX},
		{"@(*)", true, 0u, SIZE_MAX},
		{"* *", false, 1u, SIZE_MAX},
		{"@(* *)", false, 1u, SIZE_MAX},
		{"*?", false, 1u, SIZE_MAX},
		{"?", false, 1u, 1u},
		{"?*", false, 1u, SIZE_MAX},
		{NULL}
	}},
	/* The first line consists of one token.
	 */
	{"method\n", {
		/* Patterns for the only token.
		 */
		{"", false, 0u, 0u},
		{"*", true, 0u, SIZE_MAX},
		{"@(*)", true, 0u, SIZE_MAX},
		{"* *", false, 1u, SIZE_MAX},
		{"@(* *)", false, 1u, SIZE_MAX},
		{"*?", true, 1u, SIZE_MAX},
		{"?", false, 1u, 1u},
		{"?*", true, 1u, SIZE_MAX},
		{"*thod", true, 4u, SIZE_MAX},
		{"*?thod", true, 5u, SIZE_MAX},
		{"*??thod", true, 6u, SIZE_MAX},
		{"?thod", false, 5u, 5u},
		{"?*thod", true, 5u, SIZE_MAX},
		{"??thod", true, 6u, 6u},
		{"??*thod", true, 6u, SIZE_MAX},
		{"me*od", true, 4u, SIZE_MAX},
		{"me*?od", true, 5u, SIZE_MAX},
		{"me*??od", true, 6u, SIZE_MAX},
		{"me?od", false, 5u, 5u},
		{"me?*od", true, 5u, SIZE_MAX},
		{"me??od", true, 6u, 6u},
		{"me??*od", true, 6u, SIZE_MAX},
		{"meth", false, 4u, 4u},
		{"meth*", true, 4u, SIZE_MAX},
		{"meth*?", true, 5u, SIZE_MAX},
		{"meth*??", true, 6u, SIZE_MAX},
		{"meth?", false, 5u, 5u},
		{"meth?*", true, 5u, SIZE_MAX},
		{"meth??", true, 6u, 6u},
		{"meth??*", true, 6u, SIZE_MAX},
		{"method", true, 6u, 6u},
		{"method*", true, 6u, SIZE_MAX},
		{"method*?", false, 7u, SIZE_MAX},
		{"method?", false, 7u, 7u},
		{"method?*", false, 7u, SIZE_MAX},
		{NULL}
	}},
	/* The first line consists of three tokens.
	 */
	{"method key-type abcdef==\n", {
		/* Patterns for the first token.
		 */
		{"", false, 0u, 0u},
		{"*", true, 0u, SIZE_MAX},
		{"@(*)", true, 0u, SIZE_MAX},
		{"*?", true, 1u, SIZE_MAX},
		{"?", false, 1u, 1u},
		{"?*", true, 1u, SIZE_MAX},
		{"+(?)", true, 1u, UINT_MAX},
		{"*thod", true, 4u, SIZE_MAX},
		{"*?thod", true, 5u, SIZE_MAX},
		{"*??thod", true, 6u, SIZE_MAX},
		{"?thod", false, 5u, 5u},
		{"?*thod", true, 5u, SIZE_MAX},
		{"??thod", true, 6u, 6u},
		{"??*thod", true, 6u, SIZE_MAX},
		{"me*od", true, 4u, SIZE_MAX},
		{"me*?od", true, 5u, SIZE_MAX},
		{"me*??od", true, 6u, SIZE_MAX},
		{"me?od", false, 5u, 5u},
		{"me?*od", true, 5u, SIZE_MAX},
		{"me??od", true, 6u, 6u},
		{"me??*od", true, 6u, SIZE_MAX},
		{"meth", false, 4u, 4u},
		{"meth*", true, 4u, SIZE_MAX},
		{"meth*?", true, 5u, SIZE_MAX},
		{"meth*??", true, 6u, SIZE_MAX},
		{"meth?", false, 5u, 5u},
		{"meth?*", true, 5u, SIZE_MAX},
		{"meth??", true, 6u, 6u},
		{"meth??*", true, 6u, SIZE_MAX},
		{"method", true, 6u, 6u},
		{"method*", true, 6u, SIZE_MAX},
		{"method*?", false, 7u, SIZE_MAX},
		{"method?", false, 7u, 7u},
		{"method?*", false, 7u, SIZE_MAX},
		/* Patterns for the first two tokens.
		 */
		{"* ", false, 1u, SIZE_MAX},
		{"* *", true, 1u, SIZE_MAX},
		{"@(* *)", false, 1u, SIZE_MAX},
		{"* *?", true, 2u, SIZE_MAX},
		{"* ?", false, 2u, SIZE_MAX},
		{"* ?*", true, 2u, SIZE_MAX},
		{"+(?)=+(?)", true, 3u, MAX(1u + 2u * (size_t)UINT_MAX, UINT_MAX)},
		{"method=*-type", true, 12u, SIZE_MAX},
		{"method=*?-type", true, 13u, SIZE_MAX},
		{"method=*???"/* no trigraphs */"-type", true, 15u, SIZE_MAX},
		{"method=?-type", false, 13u, 13u},
		{"method=?*-type", true, 13u, SIZE_MAX},
		{"method=???"/* no trigraphs */"-type", true, 15u, 15u},
		{"method=???*-type", true, 15u, SIZE_MAX},
		{"method=key*type", true, 14u, SIZE_MAX},
		{"method=key*?type", true, 15u, SIZE_MAX},
		{"method=key*??type", false, 16u, SIZE_MAX},
		{"method=key?type", true, 15u, 15u},
		{"method=key?*type", true, 15u, SIZE_MAX},
		{"method=key??type", false, 16u, 16u},
		{"method=key??*type", false, 16u, SIZE_MAX},
		{"method=key-", false, 11u, 11u},
		{"method=key-*", true, 11u, SIZE_MAX},
		{"method=key-*?", true, 12u, SIZE_MAX},
		{"method=key-*????", true, 15u, SIZE_MAX},
		{"method=key-?", false, 12u, 12u},
		{"method=key-?*", true, 12u, SIZE_MAX},
		{"method=key-????", true, 15u, 15u},
		{"method=key-????*", true, 15u, SIZE_MAX},
		{"method=key-type", true, 15u, 15u},
		{"method=key-type*", true, 15u, SIZE_MAX},
		{"method=key-type*?", false, 16u, SIZE_MAX},
		{"method=key-type?", false, 16u, 16u},
		{"method=key-type?*", false, 16u, SIZE_MAX},
		/* Patterns for all three tokens.
		 */
		{"* * ", false, 2u, SIZE_MAX},
		{"* * *", true, 2u, SIZE_MAX},
		{"* * * *", false, 3u, SIZE_MAX},
		{"* * *?", true, 3u, SIZE_MAX},
		{"* * ?", false, 3u, SIZE_MAX},
		{"* * ?*", true, 3u, SIZE_MAX},
		{"+(?)=+(?)=+(?)", true, 5u, MAX(2u + 3u * (size_t)UINT_MAX, UINT_MAX)},
		{"method=key-type=*cdef==", true, 22u, SIZE_MAX},
		{"method=key-type=*?cdef==", true, 23u, SIZE_MAX},
		{"method=key-type=*??cdef==", true, 24u, SIZE_MAX},
		{"method=key-type=?cdef==", false, 23u, 23u},
		{"method=key-type=?*cdef==", true, 23u, SIZE_MAX},
		{"method=key-type=??cdef==", true, 24u, 24u},
		{"method=key-type=??*cdef==", true, 24u, SIZE_MAX},
		{"method=key-type=ab*==", true, 20u, SIZE_MAX},
		{"method=key-type=ab*?==", true, 21u, SIZE_MAX},
		{"method=key-type=ab*????"/* no trigraphs */"==", true, 24u, SIZE_MAX},
		{"method=key-type=ab?==", false, 21u, 21u},
		{"method=key-type=ab?*==", true, 21u, SIZE_MAX},
		{"method=key-type=ab????"/* no trigraphs */"==", true, 24u, 24u},
		{"method=key-type=ab????*==", true, 24u, SIZE_MAX},
		{"method=key-type=abcdef", false, 22u, 22u},
		{"method=key-type=abcdef*", true, 22u, SIZE_MAX},
		{"method=key-type=abcdef*?", true, 23u, SIZE_MAX},
		{"method=key-type=abcdef*??", true, 24u, SIZE_MAX},
		{"method=key-type=abcdef?", false, 23u, 23u},
		{"method=key-type=abcdef?*", true, 23u, SIZE_MAX},
		{"method=key-type=abcdef??", true, 24u, 24u},
		{"method=key-type=abcdef??*", true, 24u, SIZE_MAX},
		{"method=key-type=abcdef==", true, 24u, 24u},
		{"method=key-type=abcdef==*", true, 24u, SIZE_MAX},
		{"method=key-type=abcdef==*?", false, 25u, SIZE_MAX},
		{"method=key-type=abcdef==?", false, 25u, 25u},
		{"method=key-type=abcdef==?*", false, 25u, SIZE_MAX},
		{NULL}
	}},
	{NULL}
};
