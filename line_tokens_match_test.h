/*
 * Copyright © 2021 - 2024 Eero Häkkinen <Eero+pam-ssh-auth-info@Häkkinen.fi>
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
		bool allow_prefix_match;
		bool expected;
		struct pattern_length_info expected_match_len;
	} pattern_data[103];
} test_data[] = {
	/* The first line contains special pattern character bytes.
	 */
	{"[abbccc]", {
		{"[[]abbccc[]]", false, true, {8u, 8u}},
		{"[][]abbccc[][]", false, true, {8u, 8u}},
		{"\\[abbccc\\]", false, true, {8u, 8u}},
		{"[[]abbccc?(dddd)[]]", false, true, {8u, 12u}},
		{"[[]?(a)?(bb)?(ccc)?(dddd)?()[]]", false, true, {2u, 12u}},
		{"[[]*(|a)*(|b)*(|c)*(|d)*()[]]", false, true, {2u, MAX(4u * (size_t)UINT_MAX + 2u, UINT_MAX)}},
		{"[[]@(a)@(bb)@(ccc)@()[]]", false, true, {8u, 8u}},
		{"[[]+(|a)+(|b)+(|c)+(|d)+()[]]", false, true, {2u, MAX(4u * (size_t)UINT_MAX + 2u, UINT_MAX)}},
		{"[[]!(|*b*|*c*)!(|*a*|*c*)!(|*a*|*b*)[]]", false, true, {2u, SIZE_MAX}},
		{"[[][-a-z][-a-z-][-a-z-][a-z-][a-z-][a-z-][]]", false, true, {8u, 8u}},
		{"[[][!b-z][!ac-z][!ac-z][!abd-z][!abd-z][!abd-z][]]", false, true, {8u, 8u}},
		{"[![]abbccc[]]", false, false, {8u, 8u}},
		{"[[][!a]bbccc[]]", false, false, {8u, 8u}},
		{"[[]a[!a-z]bccc[]]", false, false, {8u, 8u}},
		{"[[]abb[!ab-yz]cc[]]", false, false, {8u, 8u}},
		{"[[]abbccc[!]]", false, false, {8u, 8u}},
		{"[[]?(a)?(b)?(ccc)[]]", false, false, {2u, 7u}},
		{"[[]@(a)@(b)@(ccc)[]]", false, false, {7u, 7u}},
		{"[[]+(|a)+(|b)+(|c)+(d)[]]", false, false, {3u, MAX(4u * (size_t)UINT_MAX + 2u, UINT_MAX)}},
		{"[[]!(|*b*|*c*)!(|*b*)!(|*a*|*b*)[]]", false, false, {2u, SIZE_MAX}},
		{NULL, false, false, {0u, 0u}}
	}},
	{"\\", {
		{"[\\]", false, true, {1u, 1u}},
		{"\\\\", false, true, {1u, 1u}},
		{"\\", false, true, {1u, 1u}},
		{"[!\\]", false, false, {1u, 1u}},
		{NULL, false, false, {0u, 0u}}
	}},
	{"\\-", {
		{"[\\]-", false, true, {2u, 2u}},
		{"\\\\-", false, true, {2u, 2u}},
		{"\\-", false, false, {1u, 1u}},
		{"[!\\]-", false, false, {2u, 2u}},
		{NULL, false, false, {0u, 0u}}
	}},
	/* The first line is empty.
	 */
	{"", {
		{"", false, true, {0u, 0u}},
		{"*", false, true, {0u, SIZE_MAX}},
		{"@(*)", false, true, {0u, SIZE_MAX}},
		{"* *", false, false, {1u, SIZE_MAX}},
		{"@(* *)", false, false, {1u, SIZE_MAX}},
		{"*?", false, false, {1u, SIZE_MAX}},
		{"?", false, false, {1u, 1u}},
		{"?*", false, false, {1u, SIZE_MAX}},
		{NULL, false, false, {0u, 0u}}
	}},
	/* The first line is empty.
	 */
	{"\n", {
		{"", false, true, {0u, 0u}},
		{"*", false, true, {0u, SIZE_MAX}},
		{"@(*)", false, true, {0u, SIZE_MAX}},
		{"* *", false, false, {1u, SIZE_MAX}},
		{"@(* *)", false, false, {1u, SIZE_MAX}},
		{"*?", false, false, {1u, SIZE_MAX}},
		{"?", false, false, {1u, 1u}},
		{"?*", false, false, {1u, SIZE_MAX}},
		{NULL, false, false, {0u, 0u}}
	}},
	/* The first line consists of one token.
	 */
	{"method\n", {
		/* Patterns for the only token.
		 */
		{"", false, false, {0u, 0u}},
		{"*", false, true, {0u, SIZE_MAX}},
		{"@(*)", false, true, {0u, SIZE_MAX}},
		{"* *", false, false, {1u, SIZE_MAX}},
		{"@(* *)", false, false, {1u, SIZE_MAX}},
		{"*?", false, true, {1u, SIZE_MAX}},
		{"?", false, false, {1u, 1u}},
		{"?*", false, true, {1u, SIZE_MAX}},
		{"*thod", false, true, {4u, SIZE_MAX}},
		{"*?thod", false, true, {5u, SIZE_MAX}},
		{"*??thod", false, true, {6u, SIZE_MAX}},
		{"?thod", false, false, {5u, 5u}},
		{"?*thod", false, true, {5u, SIZE_MAX}},
		{"??thod", false, true, {6u, 6u}},
		{"??*thod", false, true, {6u, SIZE_MAX}},
		{"me*od", false, true, {4u, SIZE_MAX}},
		{"me*?od", false, true, {5u, SIZE_MAX}},
		{"me*??od", false, true, {6u, SIZE_MAX}},
		{"me?od", false, false, {5u, 5u}},
		{"me?*od", false, true, {5u, SIZE_MAX}},
		{"me??od", false, true, {6u, 6u}},
		{"me??*od", false, true, {6u, SIZE_MAX}},
		{"meth", false, false, {4u, 4u}},
		{"meth*", false, true, {4u, SIZE_MAX}},
		{"meth*?", false, true, {5u, SIZE_MAX}},
		{"meth*??", false, true, {6u, SIZE_MAX}},
		{"meth?", false, false, {5u, 5u}},
		{"meth?*", false, true, {5u, SIZE_MAX}},
		{"meth??", false, true, {6u, 6u}},
		{"meth??*", false, true, {6u, SIZE_MAX}},
		{"method", false, true, {6u, 6u}},
		{"method*", false, true, {6u, SIZE_MAX}},
		{"method*?", false, false, {7u, SIZE_MAX}},
		{"method?", false, false, {7u, 7u}},
		{"method?*", false, false, {7u, SIZE_MAX}},
		{NULL, false, false, {0u, 0u}}
	}},
	/* The first line consists of three tokens.
	 */
	{"method key-type abcdef==\n", {
		/* Patterns for the first token.
		 */
		{"", true, false, {0u, 0u}},
		{"*", true, true, {0u, SIZE_MAX}},
		{"@(*)", true, true, {0u, SIZE_MAX}},
		{"*?", true, true, {1u, SIZE_MAX}},
		{"?", true, false, {1u, 1u}},
		{"?*", true, true, {1u, SIZE_MAX}},
		{"+(?)", true, true, {1u, UINT_MAX}},
		{"*thod", true, true, {4u, SIZE_MAX}},
		{"*?thod", true, true, {5u, SIZE_MAX}},
		{"*??thod", true, true, {6u, SIZE_MAX}},
		{"?thod", true, false, {5u, 5u}},
		{"?*thod", true, true, {5u, SIZE_MAX}},
		{"??thod", true, true, {6u, 6u}},
		{"??*thod", true, true, {6u, SIZE_MAX}},
		{"me*od", true, true, {4u, SIZE_MAX}},
		{"me*?od", true, true, {5u, SIZE_MAX}},
		{"me*??od", true, true, {6u, SIZE_MAX}},
		{"me?od", true, false, {5u, 5u}},
		{"me?*od", true, true, {5u, SIZE_MAX}},
		{"me??od", true, true, {6u, 6u}},
		{"me??*od", true, true, {6u, SIZE_MAX}},
		{"meth", true, false, {4u, 4u}},
		{"meth*", true, true, {4u, SIZE_MAX}},
		{"meth*?", true, true, {5u, SIZE_MAX}},
		{"meth*??", true, true, {6u, SIZE_MAX}},
		{"meth?", true, false, {5u, 5u}},
		{"meth?*", true, true, {5u, SIZE_MAX}},
		{"meth??", true, true, {6u, 6u}},
		{"meth??*", true, true, {6u, SIZE_MAX}},
		{"method", true, true, {6u, 6u}},
		{"method*", true, true, {6u, SIZE_MAX}},
		{"method*?", true, false, {7u, SIZE_MAX}},
		{"method?", true, false, {7u, 7u}},
		{"method?*", true, false, {7u, SIZE_MAX}},
		/* Patterns for the first two tokens.
		 */
		{"* ", true, false, {1u, SIZE_MAX}},
		{"* *", true, true, {1u, SIZE_MAX}},
		{"@(* *)", true, false, {1u, SIZE_MAX}},
		{"* *?", true, true, {2u, SIZE_MAX}},
		{"* ?", true, false, {2u, SIZE_MAX}},
		{"* ?*", true, true, {2u, SIZE_MAX}},
		{"+(?)=+(?)", true, true, {3u, MAX(2u * (size_t)UINT_MAX + 1u, UINT_MAX)}},
		{"method=*-type", true, true, {12u, SIZE_MAX}},
		{"method=*?-type", true, true, {13u, SIZE_MAX}},
		{"method=*???"/* no trigraphs */"-type", true, true, {15u, SIZE_MAX}},
		{"method=?-type", true, false, {13u, 13u}},
		{"method=?*-type", true, true, {13u, SIZE_MAX}},
		{"method=???"/* no trigraphs */"-type", true, true, {15u, 15u}},
		{"method=???*-type", true, true, {15u, SIZE_MAX}},
		{"method=key*type", true, true, {14u, SIZE_MAX}},
		{"method=key*?type", true, true, {15u, SIZE_MAX}},
		{"method=key*??type", true, false, {16u, SIZE_MAX}},
		{"method=key?type", true, true, {15u, 15u}},
		{"method=key?*type", true, true, {15u, SIZE_MAX}},
		{"method=key??type", true, false, {16u, 16u}},
		{"method=key??*type", true, false, {16u, SIZE_MAX}},
		{"method=key-", true, false, {11u, 11u}},
		{"method=key-*", true, true, {11u, SIZE_MAX}},
		{"method=key-*?", true, true, {12u, SIZE_MAX}},
		{"method=key-*????", true, true, {15u, SIZE_MAX}},
		{"method=key-?", true, false, {12u, 12u}},
		{"method=key-?*", true, true, {12u, SIZE_MAX}},
		{"method=key-????", true, true, {15u, 15u}},
		{"method=key-????*", true, true, {15u, SIZE_MAX}},
		{"method=key-type", true, true, {15u, 15u}},
		{"method=key-type*", true, true, {15u, SIZE_MAX}},
		{"method=key-type*?", true, false, {16u, SIZE_MAX}},
		{"method=key-type?", true, false, {16u, 16u}},
		{"method=key-type?*", true, false, {16u, SIZE_MAX}},
		/* Patterns for all three tokens.
		 */
		{"* * ", false, false, {2u, SIZE_MAX}},
		{"* * *", false, true, {2u, SIZE_MAX}},
		{"* * * *", false, false, {3u, SIZE_MAX}},
		{"* * *?", false, true, {3u, SIZE_MAX}},
		{"* * ?", false, false, {3u, SIZE_MAX}},
		{"* * ?*", false, true, {3u, SIZE_MAX}},
		{"+(?)=+(?)=+(?)", false, true, {5u, MAX(3u * (size_t)UINT_MAX + 2u, UINT_MAX)}},
		{"method=key-type=*cdef==", false, true, {22u, SIZE_MAX}},
		{"method=key-type=*?cdef==", false, true, {23u, SIZE_MAX}},
		{"method=key-type=*??cdef==", false, true, {24u, SIZE_MAX}},
		{"method=key-type=?cdef==", false, false, {23u, 23u}},
		{"method=key-type=?*cdef==", false, true, {23u, SIZE_MAX}},
		{"method=key-type=??cdef==", false, true, {24u, 24u}},
		{"method=key-type=??*cdef==", false, true, {24u, SIZE_MAX}},
		{"method=key-type=ab*==", false, true, {20u, SIZE_MAX}},
		{"method=key-type=ab*?==", false, true, {21u, SIZE_MAX}},
		{"method=key-type=ab*????"/* no trigraphs */"==", false, true, {24u, SIZE_MAX}},
		{"method=key-type=ab?==", false, false, {21u, 21u}},
		{"method=key-type=ab?*==", false, true, {21u, SIZE_MAX}},
		{"method=key-type=ab????"/* no trigraphs */"==", false, true, {24u, 24u}},
		{"method=key-type=ab????*==", false, true, {24u, SIZE_MAX}},
		{"method=key-type=abcdef", false, false, {22u, 22u}},
		{"method=key-type=abcdef*", false, true, {22u, SIZE_MAX}},
		{"method=key-type=abcdef*?", false, true, {23u, SIZE_MAX}},
		{"method=key-type=abcdef*??", false, true, {24u, SIZE_MAX}},
		{"method=key-type=abcdef?", false, false, {23u, 23u}},
		{"method=key-type=abcdef?*", false, true, {23u, SIZE_MAX}},
		{"method=key-type=abcdef??", false, true, {24u, 24u}},
		{"method=key-type=abcdef??*", false, true, {24u, SIZE_MAX}},
		{"method=key-type=abcdef==", false, true, {24u, 24u}},
		{"method=key-type=abcdef==*", false, true, {24u, SIZE_MAX}},
		{"method=key-type=abcdef==*?", false, false, {25u, SIZE_MAX}},
		{"method=key-type=abcdef==?", false, false, {25u, 25u}},
		{"method=key-type=abcdef==?*", false, false, {25u, SIZE_MAX}},
		{NULL, false, false, {0u, 0u}}
	}},
	{NULL, {
		{NULL, false, false, {0u, 0u}}
	}}
};
