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

#undef NDEBUG

#include <stdint.h>
#include <stdio.h>

#include "line_tokens_match_test.h"
#include "pattern.h"

int
main() {
	for (int i = 0; test_data[i].lines; ++i) {
		for (int j = 0; test_data[i].pattern_data[j].pattern; ++j) {
			char const *const pattern =
				test_data[i].pattern_data[j].pattern;
			size_t const expected_min =
				test_data[i].pattern_data[j].expected_min;
			size_t const expected_max =
				test_data[i].pattern_data[j].expected_max;
			size_t actual_min, actual_max;
			measure_pattern(
				pattern,
				pattern + strlen(pattern),
				&actual_min,
				&actual_max
				);
			fprintf(
				stderr,
				"measure_pattern(\"%s\", \"\", &min, &max)"
				", min == %zu %s %zu"
				", max == %zu %s %zu"
				"\n",
				pattern,
				actual_min,
				actual_min == expected_min ? "==" : "!=",
				expected_min,
				actual_max,
				actual_max == expected_max ? "==" : "!=",
				expected_max
				);
			if (actual_min != expected_min)
				return 1;
			if (actual_max != expected_max)
				return 1;
		}
	}
	fprintf(stderr, "OK\n");
	return 0;
}
