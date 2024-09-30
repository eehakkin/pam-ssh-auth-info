/*
 * Copyright © 2021 - 2024 Eero Häkkinen <Eero+pam-ssh-auth-info@Häkkinen.fi>
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

#include "pattern.h"

struct tokens_match_config {
	bool allow_prefix_match;
	struct {
		struct character_byte_set pattern;
		struct character_byte_set token;
	} separators;
};

/* Check if the tokens match the pattern.
 *
 * Any character byte that appears in a pattern, other than the extended
 * patterns and the special pattern characters described below and
 * the separator characters, matches itself.
 *
 * The separator character bytes match themselves and token separator character
 * bytes.
 *
 * The special pattern characters:
 *
 *  *  Matches any number of (including zero) token character bytes but not
 *     a token separator.
 *  ?  Matches any token character byte but not a token separator.
 *  [  A pair of brackets introduces a character byte class.
 *     A character byte class ([...]) matches any token character byte in
 *     the class. A complemented character byte class ([!...]) matches any
 *     token character byte not in the class. Neither matches a token
 *     separator. A class may contain character bytes ([abcde]) and character
 *     byte ranges ([a-e]).
 *  \  Preserves the literal meaning of the following character byte.
 *
 * The extended patterns:
 *
 *  ?(pattern|pattern|...)  Matches zero or one occurence of the given
 *                          patterns.
 *                          Does not match a token separator.
 *  *(pattern|pattern|...)  Matches zero or more occurences of the given
 *                          patterns.
 *                          Does not match a token separator.
 *  +(pattern|pattern|...)  Matches one or more occurences of the given
 *                          patterns.
 *                          Does not match a token separator.
 *  @(pattern|pattern|...)  Matches one of the given patterns.
 *                          Does not match a token separator.
 *  !(pattern|pattern|...)  Matches anything except one occurence of the given
 *                          patterns.
 *                          Does not match a token separator.
 */
static bool
tokens_match(
	struct tokens_match_config const *const config,
	char const *tokens,
	char const *const tokens_end,
	char const *pattern,
	char const *const pattern_end,
	unsigned const recursion_limit
	);

static bool
character_byte_matches_character_byte_class(
	struct character_byte_class_info const *const info,
	char const ch
	) {
	for (char const *p = info->begin;;) {
		if (p[1] == '-' && p[2] != ']') {
			/* A character byte range.
			 */
			if (p[0] <= ch && ch <= p[2])
				return !info->negation;
			p += 3;
		}
		else {
			/* A character byte.
			 */
			if (ch == p[0])
				return !info->negation;
			++p;
		}
		if (p >= info->end) {
			assert(p == info->end);
			return info->negation;
		}
	}
}

static bool
is_token_separator(
	struct tokens_match_config const *const config,
	char const ch
	) {
	return in_character_byte_set(&config->separators.token, ch);
}

static bool
is_end_of_token(
	struct tokens_match_config const *const config,
	char const *const tokens,
	char const *const tokens_end
	) {
	assert(tokens <= tokens_end);
	return tokens >= tokens_end || is_token_separator(config, *tokens);
}

static bool
token_match_pattern_list(
	char const *const token,
	char const *const token_end,
	char const *const pattern_list,
	char const *const pattern_list_end,
	unsigned const recursion_limit
	) {
	/* The token does not contain separators.
	 * Therefore, a zero config is enough.
	 */
	static struct tokens_match_config const config = {
		false,
		{{0, ""}, {0, ""}}
	};
	char const *pattern = pattern_list;
	for (;;) {
		char const *pattern_end = find_in_pattern(
			pattern,
			pattern_list_end,
			'|',
			pattern_list_end
			);
		if (tokens_match(
			&config,
			token,
			token_end,
			pattern,
			pattern_end,
			recursion_limit
			))
			return true;
		if (pattern_end == pattern_list_end)
			break;
		pattern = pattern_end + 1;
	}
	return false;
}

static bool
tokens_match_extended_pattern(
	struct tokens_match_config const *const config,
	char const *tokens,
	char const *const tokens_end,
	struct extended_pattern_info const *const info,
	char const *const pattern,
	char const *const pattern_end,
	unsigned const recursion_limit,
	unsigned count
	) {
	char const *tokens_tail;
	if (info->count.max == 0) {  /* !(...) */
		/* Try to split the tokens to a head and a tail so that
		 *  1) the tokens head is a token or a token prefix
		 *     (does not contain a token separator),
		 *  2) the tokens head does not match any of the patterns in
		 *     the extended pattern and
		 *  3) the tokens tail matches the rest of the pattern.
		 */
		for (tokens_tail = tokens;; ++tokens_tail) {
			if (
				!token_match_pattern_list(
					tokens,
					tokens_tail,
					info->begin,
					info->end,
					recursion_limit
					) &&
				tokens_match(
					config,
					tokens_tail,
					tokens_end,
					pattern,
					pattern_end,
					recursion_limit
					)
				)
				return true;
			if (is_end_of_token(config, tokens_tail, tokens_end))
				return false;
		}
	}
	/* If there are not enough occurences or
	 * if the tokens do not match the rest of the pattern,
	 * try to split the tokens to a head and a tail so that
	 *  1) the tokens head is a token or a token prefix
	 *     (does not contain a token separator),
	 *  2) the tokens head matches at least one of the patterns in
	 *     the extended pattern and
	 *  3) the tokens tail matches the extended pattern with increased
	 *     occurence count.
	 */
	for (tokens_tail = tokens;;) {
		size_t const match_len = (size_t)(tokens_tail - tokens);
		if (tokens_tail == tokens) {
			if (
				count >= info->count.min &&
				tokens_match(
					config,
					tokens,
					tokens_end,
					pattern,
					pattern_end,
					recursion_limit
					)
				)
				/* There are enough occurences and
				 * the tokens match the rest of the pattern.
				 */
				return true;
			if (count >= info->count.max)
				/* No more occurences can be found.
				 */
				return false;
		}
		if (match_len > info->match_len.max)
			/* No more matches can be found.
			 */
			return false;
		if (
			match_len >= info->match_len.min &&
			(match_len || count < info->count.min) &&
			token_match_pattern_list(
				tokens,
				tokens_tail,
				info->begin,
				info->end,
				recursion_limit
				)
			) {
			/* The tokens head matches the pattern list and
			 * either the tokens head is not empty or the occurence
			 * count must be increased.
			 */
			if (
				match_len == info->match_len.max ||
				is_end_of_token(
					config,
					tokens_tail,
					tokens_end
					)
				) {
				/* Tail call optimization.
				 */
				++count;
				tokens = tokens_tail;
				continue;
			}
			if (
				recursion_limit &&
				tokens_match_extended_pattern(
					config,
					tokens_tail,
					tokens_end,
					info,
					pattern,
					pattern_end,
					recursion_limit - 1,
					count + 1
					)
				)
				/* The tokens tail matches the extended pattern
				 * with increased occurence count.
				 */
				return true;
		}
		if (is_end_of_token(config, tokens_tail, tokens_end))
			return false;
		++tokens_tail;
	}
}

static bool
tokens_match(
	struct tokens_match_config const *const config,
	char const *tokens,
	char const *const tokens_end,
	char const *pattern,
	char const *const pattern_end,
	unsigned const recursion_limit
	) {
	while (pattern < pattern_end) {
		assert(tokens <= tokens_end);
		char character_byte;
		struct character_byte_class_info character_byte_class;
		struct extended_pattern_info extended_pattern;
		struct wildcard_pattern_info wildcard_pattern;
		bool const measure_extended_patterns_on = true;
		switch (parse_next_pattern_entity(
			&pattern,
			pattern_end,
			&config->separators.pattern,
			&config->separators.token,
			&character_byte,
			&character_byte_class,
			&extended_pattern,
			&wildcard_pattern,
			measure_extended_patterns_on
			)) {
		case EXTENDED_PATTERN:
			if (!recursion_limit)
				return false;
			return tokens_match_extended_pattern(
				config,
				tokens,
				tokens_end,
				&extended_pattern,
				pattern,
				pattern_end,
				recursion_limit - 1,
				0
				);
		case WILDCARD_PATTERN_MATCH_ANY:
			/* An asterisk (*) matches any number of (including
			 * zero) token character bytes but not a token
			 * separator.
			 */
			while (pattern < pattern_end && pattern[0] == '*')
				++pattern;
			if (pattern == pattern_end) {
				assert(tokens <= tokens_end);
				if (config->allow_prefix_match)
					/* An asterisk matches remaining token
					 * character bytes to the end of
					 * the token and an empty pattern
					 * matches there.
					 */
					return true;
				for (; tokens < tokens_end; ++tokens) {
					if (is_token_separator(
						config,
						*tokens
						))
						return false;
				}
				return true;
			}
			if (!recursion_limit)
				return false;
			for (;; ++tokens) {
				if (tokens_match(
					config,
					tokens,
					tokens_end,
					pattern,
					pattern_end,
					recursion_limit - 1
					))
					return true;
				if (is_end_of_token(
					config,
					tokens,
					tokens_end
					))
					return false;
			}
			assert(false);
		case WILDCARD_PATTERN_MATCH_ONE:
			/* A question mark (?) matches any token character byte
			 * but not a token separator.
			 */
			if (tokens >= tokens_end)
				return false;
			break;
		case CHARACTER_BYTE_CLASS_PATTERN:
			/* A character byte class ([...]) matches any token
			 * character byte in the class.
			 * A complemented character byte class ([!...]) matches
			 * any token character byte not in the class.
			 * Neither matches a token separator.
			 */
			if (tokens >= tokens_end)
				return false;
			if (!character_byte_matches_character_byte_class(
				&character_byte_class,
				*tokens
				))
				return false;
			break;
		case CHARACTER_BYTE_PATTERN:
			if (tokens >= tokens_end)
				return false;
			if (*tokens != character_byte)
				return false;
			break;
		case PATTERN_SEPARATOR_PATTERN:
		case TOKEN_SEPARATOR_PATTERN:
			/* A separator character byte matches itself or
			 * a token separator character byte.
			 */
			if (tokens >= tokens_end)
				return false;
			if (
				*tokens != character_byte &&
				!is_token_separator(config, *tokens)
				)
				return false;
			++tokens;
			continue;
		}
		assert(tokens < tokens_end);
		if (is_token_separator(config, *tokens))
			return false;
		++tokens;
	}
	/* The end of the pattern.
	 */
	assert(tokens <= tokens_end);
	if (tokens >= tokens_end)
		return true;
	if (config->allow_prefix_match && is_token_separator(config, *tokens))
		return true;
	return false;
}
