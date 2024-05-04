/*
 * Copyright © 2021 - 2025 Eero Häkkinen <Eero+pam-ssh-auth-info@Häkkinen.fi>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "tokens_match.h"

/* Check if the tokens on the line match the pattern.
 *
 * Any character byte that appears in a pattern, other than the extended
 * patterns and the special pattern characters described below, matches
 * itself.
 *
 * The special pattern characters:
 *
 *  *  Matches any number of (including zero) token character bytes but not
 *     a token separator (space).
 *  =  Matches an equal-sign (=) or a token separator (space).
 *  ?  Matches any token character byte but not a token separator (space).
 *  [  A pair of brackets introduces a character byte class.
 *     A character byte class ([...]) matches any token character byte in
 *     the class. A complemented character byte class ([!...]) matches any
 *     token character byte not in the class. Neither matches a token separator
 *     (space). A class may contain character bytes ([abcde]) and character
 *     byte ranges ([a-e]).
 *  \  Preserves the literal meaning of the following character byte.
 *
 * The extended patterns:
 *
 *  ?(pattern|pattern|...)  Matches zero or one occurence of the given
 *                          patterns within a token.
 *                          Does not match a token separator (space).
 *  *(pattern|pattern|...)  Matches zero or more occurences of the given
 *                          patterns within a token.
 *                          Does not match a token separator (space).
 *  @(pattern|pattern|...)  Matches one occurence of the given patterns
 *                          within a token.
 *                          Does not match a token separator (space).
 *  +(pattern|pattern|...)  Matches one or more occurences of the given
 *                          patterns within a token.
 *                          Does not match a token separator (space).
 *  !(pattern|pattern|...)  Matches anything within a token except one
 *                          occurence of the given patterns.
 *                          Does not match a token separator (space).
 */
static bool
line_tokens_match(
	char const *line,
	char const *const line_end,
	char const *pattern,
	char const *const pattern_end,
	bool const allow_prefix_match,
	unsigned const recursion_limit
	) {
	struct tokens_match_config const config = {
		allow_prefix_match,
		{{1, "="}, {1, " "}}
	};
	assert(line <= line_end);
	assert(pattern <= pattern_end);
	return tokens_match(
		&config,
		line,
		line_end,
		pattern,
		pattern_end,
		recursion_limit
		);
}

static bool
first_line_tokens_match(
	char const *const lines,
	char const *const pattern,
	bool const allow_prefix_match,
	unsigned const recursion_limit
	) {
	return line_tokens_match(
		lines,
		lines + strcspn(lines, "\n"),
		pattern,
		pattern + strlen(pattern),
		allow_prefix_match,
		recursion_limit
		);
}
