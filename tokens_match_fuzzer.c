/*
 * Copyright © 2024 Eero Häkkinen <Eero+pam-ssh-auth-info@Häkkinen.fi>
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

#include <string.h>

#include "tokens_match.h"

static char const *
memchr_or_end(char const *s, int c, char const *end) {
	char const *p = (char const *)memchr(s, c, end - s);
	return p ? p : end;
}

static unsigned const recursion_limit = 3u;

int LLVMFuzzerTestOneInput(uint8_t const* const data, size_t const size) {
	if (size < 3)
		return -1;  /* Reject. */
	char const *const data_end = (char const *)data + size;
	/* Split [data, data_end) to
	 *     flags pat_sep_len tok_sep_len tokens [\0 pattern [\0 [rest]]]
	 * The rest is ignored.
	 * If pattern is missing, pattern = pattern_end = data_end.
	 */
	struct tokens_match_config const config = {
		.allow_prefix_match = !!(data[0] & 0x1u),
		.separators = {
			.pattern = {
				.len = data[1] & 0x3u,  /* From 0 to 3. */
				.ptr = "=:,"
			},
			.token = {
				.len = data[2] & 0x3u,  /* From 0 to 3. */
				.ptr = " \t/"
			}
		}
	};
	char const *const tokens = (char const *)(data + 3);
	char const *const tokens_end = memchr_or_end(tokens, '\0', data_end);
	char const *const first_line_tokens = tokens;
	char const *const first_line_tokens_end =
		memchr_or_end(first_line_tokens, '\n', tokens_end);
	char const *const pattern =
		tokens_end < data_end ? tokens_end + 1 : data_end;
	char const *const pattern_end = memchr_or_end(pattern, '\0', data_end);
	if ((tokens_end - tokens) > 127 || (pattern_end - pattern) > 127)
		return -1;  /* Reject. */
	tokens_match(
		&config,
		first_line_tokens,
		first_line_tokens_end,
		pattern,
		pattern_end,
		recursion_limit
		);
	return 0;  /* Accept. The input may be added to the corpus. */
}
