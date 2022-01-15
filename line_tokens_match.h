/*
 * Copyright © 2021 - 2022 Eero Häkkinen <Eero+pam-ssh-auth-info@Häkkinen.fi>
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
 * Any character byte that appears in a pattern, other than the extended
 * patterns and the special pattern characters described below, matches
 * itself.
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
 *
 * The extended patterns:
 *
 *  ?(pattern|pattern|...)  Matches zero or one occurence of the given
 *                          patterns.
 *                          Does not match a token separator (space).
 *  *(pattern|pattern|...)  Matches zero or more occurences of the given
 *                          patterns.
 *                          Does not match a token separator (space).
 *  +(pattern|pattern|...)  Matches one or more occurences of the given
 *                          patterns.
 *                          Does not match a token separator (space).
 *  @(pattern|pattern|...)  Matches one of the given patterns.
 *                          Does not match a token separator (space).
 *  !(pattern|pattern|...)  Matches anything except one of the given
 *                          patterns or a token separator (space).
 */
static bool
initial_line_tokens_match(
	char const *line,
	char const *line_end,
	char const *prefix_pattern,
	char const *const prefix_pattern_end
	);

static bool
initial_line_tokens_match_pattern_list(
	char const *const line,
	char const *const line_end,
	char const *const prefix_pattern_list,
	char const *const prefix_pattern_list_end
	) {
	char const *prefix_pattern = prefix_pattern_list;
	for (;;) {
		char const *prefix_pattern_end = find_in_pattern(
			prefix_pattern,
			prefix_pattern_list_end,
			'|'
			);
		if (!prefix_pattern_end)
			prefix_pattern_end = prefix_pattern_list_end;
		if (initial_line_tokens_match(
			line,
			line_end,
			prefix_pattern,
			prefix_pattern_end
			))
			return true;
		if (prefix_pattern_end == prefix_pattern_list_end)
			break;
		prefix_pattern = prefix_pattern_end + 1;
	}
	return false;
}

static bool
initial_line_tokens_match_extended_pattern(
	char const *const line,
	char const *const line_end,
	struct extended_pattern_info const *const info,
	char const *const prefix_pattern,
	char const *const prefix_pattern_end,
	unsigned const count
	) {
	char const *line_tail;
	if (info->max == 0) {  /* !(...) */
		/* Try to split the line to a head and a tail so that
		 *  1) the line head is a token or a token prefix
		 *     (does not contain a space),
		 *  2) the line head does not match any of the patterns in
		 *     the extended pattern and
		 *  3) the line tail matches the rest of the prefix pattern.
		 */
		for (line_tail = line;; ++line_tail) {
			if (
				!initial_line_tokens_match_pattern_list(
					line,
					line_tail,
					info->list,
					info->list_end
					) &&
				initial_line_tokens_match(
					line_tail,
					line_end,
					prefix_pattern,
					prefix_pattern_end
					)
				)
				return true;
			if (line_tail >= line_end || *line_tail == ' ')
				/* The end of a line or the end of a token.
				 */
				return false;
		}
	}
	else {
		if (
			count >= info->min &&
			initial_line_tokens_match(
				line,
				line_end,
				prefix_pattern,
				prefix_pattern_end
				)
			)
			/* There are enough occurences and
			 * the line matches the rest of the prefix
			 * pattern.
			 */
			return true;
		if (count >= info->max)
			/* No more occurences can be found.
			 */
			return false;
		/* Try to split the line to a head and a tail so that
		 *  1) the line head is a token or a token prefix
		 *     (does not contain a space),
		 *  2) the line head matches at least one of the patterns in
		 *     the extended pattern and
		 *  3) the line tail matches the extended pattern with
		 *     increased occurence count.
		 */
		line_tail = line;
		if (count >= info->min) {
			/* Enough occurences have been found
			 * thus skip empty occurences as they would not change
			 * anything.
			 */
			if (line_tail >= line_end || *line_tail == ' ')
				/* The end of a line or the end of a token.
				 */
				return false;
			++line_tail;
		}
		for (;; ++line_tail) {
			if (
				initial_line_tokens_match_pattern_list(
					line,
					line_tail,
					info->list,
					info->list_end
					) &&
				initial_line_tokens_match_extended_pattern(
					line_tail,
					line_end,
					info,
					prefix_pattern,
					prefix_pattern_end,
					count + 1
					)
				)
				return true;
			if (line_tail >= line_end || *line_tail == ' ')
				/* The end of a line or the end of a token.
				 */
				return false;
		}
	}
}

static bool
initial_line_tokens_match(
	char const *line,
	char const *line_end,
	char const *prefix_pattern,
	char const *const prefix_pattern_end
	) {
	for (; prefix_pattern < prefix_pattern_end; ++prefix_pattern) {
		struct character_class_info character_class;
		struct extended_pattern_info extended_pattern;
		if (line < line_end && *line == ' ') {
			/* A token separator must be matched explicitly.
			 */
			if (
				prefix_pattern_end - prefix_pattern >= 2 &&
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
		if (is_extended_pattern(
			prefix_pattern,
			prefix_pattern_end,
			&extended_pattern
			)) {
			prefix_pattern = extended_pattern.list_end + 1;
			return initial_line_tokens_match_extended_pattern(
				line,
				line_end,
				&extended_pattern,
				prefix_pattern,
				prefix_pattern_end,
				0
				);
		}
		if (prefix_pattern[0] == '*') {
			/* An asterisk matches any number of (including zero)
			 * token character bytes but not a token separator
			 * (space).
			 */
			while (
				prefix_pattern < prefix_pattern_end &&
				prefix_pattern[0] == '*'
				)
				++prefix_pattern;
			for (;; ++line) {
				if (initial_line_tokens_match(
					line,
					line_end,
					prefix_pattern,
					prefix_pattern_end
					))
					return true;
				if (line >= line_end || *line == ' ')
					break;
			}
			/* The end of a line or the end of a token without
			 * an explicitly matched token separator (space)
			 * but not the end of the pattern.
			 */
			assert(prefix_pattern < prefix_pattern_end);
			return false;
		}
		if (line >= line_end) {
			/* The end of a line but not the end of the pattern.
			 */
			assert(prefix_pattern < prefix_pattern_end);
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
				prefix_pattern_end,
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
			if (prefix_pattern_end - prefix_pattern >= 2)
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
	char const *const lines,
	char const *const prefix_pattern
	) {
	return initial_line_tokens_match(
		lines,
		lines + strcspn(lines, "\n"),
		prefix_pattern,
		prefix_pattern + strlen(prefix_pattern)
		);
}
