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

#include <stdint.h>
#include <stdio.h>

#include "line_tokens_match_test.h"
#include "pattern.h"

char const *
size2str(size_t const size, char *buf, size_t n) {
	if (size == SIZE_MAX)
		return "SIZE_MAX";
	size_t const quot = size / UINT_MAX;
	size_t const rem = size % UINT_MAX;
	switch (quot) {
	case 0:
		snprintf(buf, n, "%zu", rem);
		break;
	case 1:
		if (!rem)
			return "UINT_MAX";
		else
			snprintf(buf, n, "UINT_MAX + %zu", rem);
		break;
	default:
		if (!rem)
			snprintf(buf, n, "%zu * UINT_MAX", quot);
		else
			snprintf(buf, n, "%zu * UINT_MAX + %zu", quot, rem);
	}
	return buf;
}

#define SIZE2STR(size_var) size2str( \
	size_var, \
	size_var##_buf, \
	sizeof size_var##_buf / sizeof *size_var##_buf \
	)

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
			char expected_max_buf[256];
			size_t actual_min, actual_max;
			char actual_max_buf[256];
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
				", max == %s %s %s"
				"\n",
				pattern,
				actual_min,
				actual_min == expected_min ? "==" : "!=",
				expected_min,
				SIZE2STR(actual_max),
				actual_max == expected_max ? "==" : "!=",
				SIZE2STR(expected_max)
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
