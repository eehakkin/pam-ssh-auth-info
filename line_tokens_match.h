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

#include "pattern.h"

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
initial_line_tokens_match(
	char const *line,
	char const *line_end,
	char const *prefix_pattern
	) {
	for (; prefix_pattern[0]; ++prefix_pattern) {
		struct character_class_info character_class;
		if (line < line_end && *line == ' ') {
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
				) {
				++line;
				continue;
			}
			/* The end of a token without an explicitly matched
			 * token separator is the end of the initial tokens.
			 */
			line_end = line;
		}
		if (prefix_pattern[0] == '*') {
			/* An asterisk matches any number of (including zero)
			 * token character bytes but not a token separator
			 * (space).
			 */
			prefix_pattern += strspn(prefix_pattern, "*");
			for (;; ++line) {
				if (initial_line_tokens_match(
					line,
					line_end,
					prefix_pattern
					))
					return true;
				if (line >= line_end || *line == ' ')
					break;
			}
			/* The end of a line or the end of a token without
			 * an explicitly matched token separator (space)
			 * but not the end of the pattern.
			 */
			assert(prefix_pattern[0]);
			return false;
		}
		if (line >= line_end) {
			/* The end of a line but not the end of the pattern.
			 */
			assert(prefix_pattern[0]);
			return false;
		}
		switch (prefix_pattern[0]) {
		case '?':
			/* A question mark matches any token character byte but
			 * not a token separator (space).
			 */
			assert(line < line_end);
			assert(*line != ' ');
			++line;
			continue;
		case '[':
			if (!is_character_class(
				prefix_pattern,
				&character_class
				))
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
			assert(line < line_end);
			assert(*line != ' ');
			prefix_pattern = character_class.begin;
			do {
				if (
					prefix_pattern[1] == '-' &&
					prefix_pattern[2] != ']'
					) {
					/* A character byte range.
					 */
					if (
						*line >= prefix_pattern[0] &&
						*line <= prefix_pattern[2]
						)
						break;
					prefix_pattern += 3;
				}
				else {
					/* A character byte.
					 */
					if (*line == prefix_pattern[0])
						break;
					++prefix_pattern;
				}
			} while (prefix_pattern < character_class.end);
			assert(prefix_pattern <= character_class.end);
			if (
				character_class.negation
					? prefix_pattern < character_class.end
					: prefix_pattern >= character_class.end
				)
				return false;
			prefix_pattern = character_class.end;
			++line;
			continue;
		case '\\':
			/* A backslash preserves the literal meaning of
			 * the following character byte.
			 */
			assert(line < line_end);
			assert(*line != ' ');
			if (prefix_pattern[1])
				++prefix_pattern;
			break;
		default:
			break;
		}
		if (*line != prefix_pattern[0])
			return false;
		++line;
	}
	/* The end of the pattern.
	 * Accept at the end of a line and at the end of a token.
	 */
	assert(line <= line_end);
	return line >= line_end || *line == ' ';
}

static bool
initial_first_line_tokens_match(
	char const *lines,
	char const *prefix_pattern
	) {
	return initial_line_tokens_match(
		lines,
		lines + strcspn(lines, "\n"),
		prefix_pattern
		);
}
