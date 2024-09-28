/*
 * Copyright © 2021 - 2023 Eero Häkkinen <Eero+pam-ssh-auth-info@Häkkinen.fi>
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

#undef NDEBUG

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "line_tokens_match.h"

#include "line_tokens_match_test.h"

int
main() {
	unsigned const recursion_limit = 6u;
	for (int i = 0; test_data[i].lines; ++i) {
		char const *const lines = test_data[i].lines;
		size_t const m = strcspn(lines, "\n");
		size_t const n = strspn(lines + m, "\n");
		assert(n <= 1u);
		assert(!lines[m+n]);
		for (int j = 0; test_data[i].pattern_data[j].pattern; ++j) {
			char const *const pattern =
				test_data[i].pattern_data[j].pattern;
			for (int k = 0; k < 2; ++k) {
				bool const allow_prefix_match = (bool)k;
				bool const expected =
					(
						allow_prefix_match ||
						!test_data[i].pattern_data[j].allow_prefix_match
						) &&
					test_data[i].pattern_data[j].expected;
				bool const actual = first_line_tokens_match(
					lines,
					pattern,
					allow_prefix_match,
					recursion_limit
					);
				fprintf(
					stderr,
					"first_line_tokens_match"
					"(\"%.*s%.*s\", \"%s\", %s, %u)"
					" %s %s\n",
					(int)m,
					lines,
					2 * (int)n,
					"\\n",
					pattern,
					allow_prefix_match ? "true" : "false",
					recursion_limit,
					actual == expected ? "==" : "!=",
					expected ? "true" : "false"
					);
				if (actual != expected)
					return 1;
			}
		}
	}
	fprintf(stderr, "OK\n");
	return 0;
}
