/*
 * Copyright © 2021 - 2023 Eero Häkkinen <Eero+pam-ssh-auth-info@Häkkinen.fi>
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
	struct {
		struct tokens_match_config_set {
			size_t len;
			char const *ptr;
		} pattern_separator, token_separator;
	} sets;
};

static bool
in_tokens_match_config_set(
	struct tokens_match_config_set const *const set,
	char const ch
	) {
	return memchr(set->ptr, ch, set->len) != NULL;
}

static bool
is_pattern_separator(
	struct tokens_match_config const *const config,
	char const ch
	) {
	return in_tokens_match_config_set(&config->sets.pattern_separator, ch);
}

static bool
is_token_separator(
	struct tokens_match_config const *const config,
	char const ch
	) {
	return in_tokens_match_config_set(&config->sets.token_separator, ch);
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

/* Check if the initial tokens match the pattern.
 *
 * Any character byte that appears in a pattern, other than the extended
 * patterns and the special pattern characters described below and the pattern
 * separator characters, matches itself.
 *
 * The pattern separator character bytes match themselves and token
 * separator character bytes.
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
initial_tokens_match(
	struct tokens_match_config const *const config,
	char const *tokens,
	char const *const tokens_end,
	char const *prefix_pattern,
	char const *const prefix_pattern_end,
	unsigned const recursion_limit
	);

static bool
initial_token_match_pattern_list(
	char const *const token,
	char const *const token_end,
	char const *const prefix_pattern_list,
	char const *const prefix_pattern_list_end,
	unsigned const recursion_limit
	) {
	/* The token does not contain separators.
	 * Therefore, a zero config is enough.
	 */
	static struct tokens_match_config const config = {
		{{0, ""}, {0, ""}}
	};
	char const *prefix_pattern = prefix_pattern_list;
	for (;;) {
		char const *prefix_pattern_end = find_in_pattern(
			prefix_pattern,
			prefix_pattern_list_end,
			'|',
			prefix_pattern_list_end
			);
		if (initial_tokens_match(
			&config,
			token,
			token_end,
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
initial_tokens_match_extended_pattern(
	struct tokens_match_config const *const config,
	char const *tokens,
	char const *const tokens_end,
	struct extended_pattern_info const *const info,
	char const *const prefix_pattern,
	char const *const prefix_pattern_end,
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
		 *  3) the tokens tail matches the rest of the prefix pattern.
		 */
		for (tokens_tail = tokens;; ++tokens_tail) {
			if (
				!initial_token_match_pattern_list(
					tokens,
					tokens_tail,
					info->begin,
					info->end,
					recursion_limit
					) &&
				initial_tokens_match(
					config,
					tokens_tail,
					tokens_end,
					prefix_pattern,
					prefix_pattern_end,
					recursion_limit
					)
				)
				return true;
			if (is_end_of_token(config, tokens_tail, tokens_end))
				return false;
		}
	}
	/* If there are not enough occurences or
	 * if the tokens do not match the rest of the prefix pattern,
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
				initial_tokens_match(
					config,
					tokens,
					tokens_end,
					prefix_pattern,
					prefix_pattern_end,
					recursion_limit
					)
				)
				/* There are enough occurences and
				 * the tokens match the rest of
				 * the prefix pattern.
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
			initial_token_match_pattern_list(
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
			if (match_len == info->match_len.max) {
				/* Tail call optimization.
				 */
				++count;
				tokens = tokens_tail;
				continue;
			}
			if (
				recursion_limit &&
				initial_tokens_match_extended_pattern(
					config,
					tokens_tail,
					tokens_end,
					info,
					prefix_pattern,
					prefix_pattern_end,
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
initial_tokens_match(
	struct tokens_match_config const *const config,
	char const *tokens,
	char const *const tokens_end,
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
			return initial_tokens_match_extended_pattern(
				config,
				tokens,
				tokens_end,
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
			 * token character bytes but not a token separator.
			 */
			while (
				prefix_pattern < prefix_pattern_end &&
				prefix_pattern[0] == '*'
				)
				++prefix_pattern;
			if (prefix_pattern == prefix_pattern_end)
				/* An asterisk matches remaining token
				 * character bytes to the end of the token and
				 * an empty prefix pattern matches there.
				 */
				return true;
			if (!recursion_limit)
				return false;
			for (;; ++tokens) {
				if (initial_tokens_match(
					config,
					tokens,
					tokens_end,
					prefix_pattern,
					prefix_pattern_end,
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
		case '?':
			/* A question mark matches any token character byte but
			 * not a token separator.
			 */
			if (is_end_of_token(config, tokens, tokens_end))
				return false;
			++tokens;
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
			 * Neither matches a token separator.
			 */
			if (is_end_of_token(config, tokens, tokens_end))
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
						*tokens >= prefix_pattern[0] &&
						*tokens <= prefix_pattern[2]
						)
						break;
					prefix_pattern += 3;
				}
				else {
					/* A character byte.
					 */
					if (*tokens == prefix_pattern[0])
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
			++tokens;
			continue;
		case '\\':
			/* A backslash preserves the literal meaning of
			 * the following character byte.
			 */
			if (prefix_pattern_end - prefix_pattern >= 2)
				++prefix_pattern;
			break;
		default:
			if (is_pattern_separator(config, prefix_pattern[0])) {
				/* A pattern separator character byte matches
				 * itself or a token separator character byte.
				 */
				assert(tokens <= tokens_end);
				if (tokens >= tokens_end)
					return false;
				if (!(
					*tokens == prefix_pattern[0] ||
					is_token_separator(config, *tokens)
					))
					return false;
				++tokens;
				continue;
			}
		}
		assert(tokens <= tokens_end);
		if (tokens >= tokens_end)
			return false;
		if (*tokens != prefix_pattern[0])
			return false;
		++tokens;
	}
	/* The end of the pattern.
	 * Accept at the end of the tokens and at the end of a token.
	 */
	return is_end_of_token(config, tokens, tokens_end);
}
