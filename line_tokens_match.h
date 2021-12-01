/*
 * Copyright © 2021 Eero Häkkinen <Eero+pam-ssh-auth-info@Häkkinen.fi>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * as published by the Free Software Foundation.
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

/* Check if the initial tokens on the first line matches the pattern.
 *
 * The special pattern characters:
 *
 *  *  Matches any number of (including zero) token character bytes but not
 *     a token separator (space).
 *  =  Matches equal-sign (=) or a token separator (space).
 *  ?  Matches any token character byte but not a token separator (space).
 *  [  A pair of brackets introduces a character byte class.
 *     A character byte class ([...]) matches any token character byte in
 *     the class. A complemented character byte class ([!...]) matches any
 *     token character byte not in the class. Neither matches a token separator
 *     (space). A class may contain character bytes ([abcde]) and character
 *     byte ranges ([a-e]).
 *  \  Preserves the literal meaning of the following character byte.
 */
static bool
initial_first_line_tokens_match(
	char const *lines,
	char const *prefix_pattern
	) {
	for (; prefix_pattern[0]; ++lines, ++prefix_pattern) {
		switch (*lines) {
		case ' ':
			/* A token separator must be matched explicitly.
			 */
			if (
				prefix_pattern[0] == '\\' &&
				prefix_pattern[1] == ' '
				)
				++prefix_pattern;
			if (
				prefix_pattern[0] == ' ' ||
				prefix_pattern[0] == '='
				)
				continue;
			/* Fall through. */
		case '\n':
		case '\0':
			/* The end of a line or the end of a token without
			 * explicitly matched token separator.
			 * This is the end of the initial tokens thus accept
			 * an empty pattern and patterns consisting only
			 * asterisks.
			 */
			return (
				!prefix_pattern[0] ||
				!prefix_pattern[strspn(prefix_pattern, "*")]
				);
		}
		switch (prefix_pattern[0]) {
		case '*':
			/* An asterisk matches any number of (including zero)
			 * token character bytes but not a token separator
			 * (space).
			 */
			prefix_pattern += strspn(prefix_pattern, "*");
			for (;; ++lines) {
				if (initial_first_line_tokens_match(
					lines,
					prefix_pattern
					))
					return true;
				/* A token separator must be matched
				 * explicitly.
				 */
				if (*lines == ' ')
					return false;
				if (*lines == '\n' || !*lines)
					break;
			}

			/* The end of a line but not the end of the pattern.
			 */
			assert(prefix_pattern[0]);
			return false;
		case '?':
			/* A question mark matches any token character byte but
			 * not a token separator (space).
			 */
			assert(*lines != ' ');
			assert(*lines != '\n');
			assert(*lines);
			continue;
		case '[': {
			bool const negation = prefix_pattern[1] == '!';
			char const *const begin =
				prefix_pattern + (negation ? 2 : 1);
			char const *const end =
				*begin ? strchr(begin + 1, ']') : NULL;
			if (!end)
				/* An opening bracket ([) without a matching
				 * closing bracket (]) is not a character byte
				 * class.
				 */
				break;
			/* A character byte class ([...]) matches any token
			 * character byte in the class.
			 * A complemented character byte class ([!...]) matches
			 * any token character byte not in the class.
			 * Neither matches a token separator (space).
			 */
			assert(*lines != ' ');
			assert(*lines != '\n');
			assert(*lines);
			prefix_pattern = begin;
			do {
				if (
					prefix_pattern[1] == '-' &&
					prefix_pattern + 2 < end
					) {
					/* A character byte range.
					 */
					if (
						*lines >= prefix_pattern[0] &&
						*lines <= prefix_pattern[2]
						)
						break;
					prefix_pattern += 3;
				}
				else {
					/* A character byte.
					 */
					if (*lines == prefix_pattern[0])
						break;
					++prefix_pattern;
				}
			} while (prefix_pattern < end);
			assert(prefix_pattern <= end);
			if (
				negation
					? prefix_pattern < end
					: prefix_pattern >= end
				)
				return false;
			prefix_pattern = end;
			continue;
		}
		case '\\':
			/* A backslash preserves the literal meaning of
			 * the following character byte.
			 */
			assert(*lines != ' ');
			assert(*lines != '\n');
			assert(*lines);
			if (prefix_pattern[1])
				++prefix_pattern;
			break;
		default:
			break;
		}
		if (*lines != prefix_pattern[0])
			return false;
	}
	/* The end of the pattern.
	 * Accept at the end of a token and at the end of a line.
	 */
	return *lines == ' ' || *lines == '\n' || !*lines;
}
