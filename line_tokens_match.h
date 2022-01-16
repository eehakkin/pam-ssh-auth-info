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

static bool
is_line_token_separator(char ch) {
	return ch == ' ';
}

static bool
is_end_of_line_or_token(
	char const *const line,
	char const *const line_end
	) {
	assert(line <= line_end);
	return line >= line_end || is_line_token_separator(*line);
}

/* Check if the initial tokens on the line matches the pattern.
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
	char const *const line_end,
	char const *prefix_pattern,
	char const *const prefix_pattern_end,
	unsigned const recursion_limit
	);

static bool
initial_line_tokens_match_pattern_list(
	char const *const line,
	char const *const line_end,
	char const *const prefix_pattern_list,
	char const *const prefix_pattern_list_end,
	unsigned const recursion_limit
	) {
	char const *prefix_pattern = prefix_pattern_list;
	for (;;) {
		char const *prefix_pattern_end = find_in_pattern(
			prefix_pattern,
			prefix_pattern_list_end,
			'|',
			prefix_pattern_list_end
			);
		if (initial_line_tokens_match(
			line,
			line_end,
			prefix_pattern,
			prefix_pattern_end,
			recursion_limit
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
	unsigned const recursion_limit,
	unsigned const count
	) {
	char const *line_tail;
	if (info->count.max == 0) {  /* !(...) */
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
					info->begin,
					info->end,
					recursion_limit
					) &&
				initial_line_tokens_match(
					line_tail,
					line_end,
					prefix_pattern,
					prefix_pattern_end,
					recursion_limit
					)
				)
				return true;
			if (is_end_of_line_or_token(line_tail, line_end))
				return false;
		}
	}
	else {
		if (
			count >= info->count.min &&
			initial_line_tokens_match(
				line,
				line_end,
				prefix_pattern,
				prefix_pattern_end,
				recursion_limit
				)
			)
			/* There are enough occurences and
			 * the line matches the rest of the prefix
			 * pattern.
			 */
			return true;
		if (count >= info->count.max)
			/* No more occurences can be found.
			 */
			return false;
		if (!recursion_limit)
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
		if (count >= info->count.min) {
			/* Enough occurences have been found
			 * thus skip empty occurences as they would not change
			 * anything.
			 */
			if (is_end_of_line_or_token(line_tail, line_end))
				return false;
			++line_tail;
		}
		for (;; ++line_tail) {
			size_t const match_len = (size_t)(line_tail - line);
			if (
				match_len >= info->match_len.min &&
				initial_line_tokens_match_pattern_list(
					line,
					line_tail,
					info->begin,
					info->end,
					recursion_limit
					) &&
				initial_line_tokens_match_extended_pattern(
					line_tail,
					line_end,
					info,
					prefix_pattern,
					prefix_pattern_end,
					recursion_limit - 1,
					count + 1
					)
				)
				return true;
			if (is_end_of_line_or_token(line_tail, line_end))
				return false;
		}
	}
}

static bool
initial_line_tokens_match(
	char const *line,
	char const *const line_end,
	char const *prefix_pattern,
	char const *const prefix_pattern_end,
	unsigned const recursion_limit
	) {
	for (; prefix_pattern < prefix_pattern_end; ++prefix_pattern) {
		struct character_class_info character_class;
		struct extended_pattern_info extended_pattern;
		if (is_extended_pattern(
			prefix_pattern,
			prefix_pattern_end,
			&extended_pattern
			)) {
			if (!recursion_limit)
				return false;
			prefix_pattern = extended_pattern.end + 1;
			return initial_line_tokens_match_extended_pattern(
				line,
				line_end,
				&extended_pattern,
				prefix_pattern,
				prefix_pattern_end,
				recursion_limit - 1,
				0
				);
		}
		switch (prefix_pattern[0]) {
		case '*':
			/* An asterisk matches any number of (including zero)
			 * token character bytes but not a token separator
			 * (space).
			 */
			while (
				prefix_pattern < prefix_pattern_end &&
				prefix_pattern[0] == '*'
				)
				++prefix_pattern;
			if (!recursion_limit)
				return false;
			for (;; ++line) {
				if (initial_line_tokens_match(
					line,
					line_end,
					prefix_pattern,
					prefix_pattern_end,
					recursion_limit - 1
					))
					return true;
				if (is_end_of_line_or_token(line, line_end))
					return false;
			}
			assert(false);
		case '=':
			/* An equal sign matches an equal sign or a token
			 * separator (space).
			 */
			if (line >= line_end)
				return false;
			if (*line != '=' && !is_line_token_separator(*line))
				return false;
			++line;
			continue;
		case '?':
			/* A question mark matches any token character byte but
			 * not a token separator (space).
			 */
			if (is_end_of_line_or_token(line, line_end))
				return false;
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
			if (is_end_of_line_or_token(line, line_end))
				return false;
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
			if (prefix_pattern_end - prefix_pattern >= 2)
				++prefix_pattern;
			break;
		}
		if (line >= line_end)
			return false;
		if (*line != prefix_pattern[0])
			return false;
		++line;
	}
	/* The end of the pattern.
	 * Accept at the end of a line and at the end of a token.
	 */
	return is_end_of_line_or_token(line, line_end);
}

static bool
initial_first_line_tokens_match(
	char const *const lines,
	char const *const prefix_pattern,
	unsigned const recursion_limit
	) {
	return initial_line_tokens_match(
		lines,
		lines + strcspn(lines, "\n"),
		prefix_pattern,
		prefix_pattern + strlen(prefix_pattern),
		recursion_limit
		);
}
