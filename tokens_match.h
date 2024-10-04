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

#include "pattern.h"

struct tokens_match_config {
	bool allow_prefix_match;
	struct {
		struct character_byte_set pattern;
		struct character_byte_set token;
	} separators;
};

struct tokens_pattern {
	char const *tokens;
	char const *pattern;
};

struct tokens_pattern_end {
	char const *tokens_min;
	char const *tokens_max;
	char const *pattern;
	char const *next_character_byte;
};

static bool
tokens_match_partially(
	struct tokens_match_config const *const config,
	struct tokens_pattern const *const begin,
	struct tokens_pattern *const current_out,
	struct tokens_pattern_end const *const end,
	char const *token_end,
	unsigned const recursion_limit
	);

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
	) {
	assert(tokens <= tokens_end);
	assert(pattern <= pattern_end);
	struct tokens_pattern const begin = {tokens, pattern};
	struct tokens_pattern_end const end = {
		tokens_end,
		tokens_end,
		pattern_end,
		NULL
	};
	return tokens_match_partially(
		config,
		&begin,
		NULL,
		&end,
		NULL,
		recursion_limit
		);
}

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

static char const *
find_end_of_token(
	struct tokens_match_config const *const config,
	struct tokens_pattern const *const begin,
	struct tokens_pattern_end const *const end
	) {
	assert(begin->tokens <= end->tokens_max);
	assert(begin->pattern <= end->pattern);
	assert(end->tokens_min <= end->tokens_max);
	if (config->separators.token.len > 0u) {
		char const *tokens = begin->tokens;
		for (; tokens < end->tokens_max; ++tokens) {
			if (in_character_byte_set(
				&config->separators.token,
				*tokens
				))
				return tokens;
		}
	}
	return end->tokens_max;
}

/* 1) Find the tail.
 * In addition, while doing that,
 * 2) Process consecutive initial wildcard patterns.
 * 3) Try to record the next character byte.
 */
static bool
find_tokens_pattern_tail(
	struct tokens_match_config const *const config,
	struct tokens_pattern *const current,
	struct tokens_pattern_end *const tail,
	struct tokens_pattern_end const *const end,
	char const *const token_end,
	struct extended_pattern_info *const extended_pattern,
	struct wildcard_pattern_info *const wildcard_pattern
	) {
	assert(current->tokens <= token_end && token_end <= end->tokens_max);
	assert(current->pattern <= end->pattern);
	assert(end->tokens_min <= end->tokens_max);
	tail->tokens_min = current->tokens;
	tail->pattern = current->pattern;
	tail->next_character_byte = NULL;
	if (extended_pattern) {
		if ((size_t)(
			token_end - tail->tokens_min
			) < extended_pattern->total_len.min)
			return false;
		tail->tokens_min += extended_pattern->total_len.min;
		if (
			extended_pattern->total_len.min ==
			extended_pattern->total_len.max
			) {
			/* The end of a fixed length extended pattern is
			 * a pattern tail.
			 */
			tail->tokens_max = tail->tokens_min;
			return true;
		}
	}
	bool has_complex_patterns = !!extended_pattern;
	while (tail->pattern < end->pattern) {
		char character_byte;
		struct character_byte_class_info character_byte_class;
		struct extended_pattern_info extended_pattern2;
		struct wildcard_pattern_info wildcard_pattern2;
		bool const measure_extended_patterns_on = true;
		char const *const original_tail_pattern = tail->pattern;
		switch (parse_next_pattern_entity(
			&tail->pattern,
			end->pattern,
			token_end < end->tokens_max
				? &config->separators.pattern
				: NULL,
			&config->separators.token,
			&character_byte,
			&character_byte_class,
			&extended_pattern2,
			&wildcard_pattern2,
			measure_extended_patterns_on
			)) {
		case EXTENDED_PATTERN:
			if ((size_t)(
				token_end - tail->tokens_min
				) < extended_pattern2.total_len.min)
				return false;
			tail->tokens_min += extended_pattern2.total_len.min;
			if (
				extended_pattern2.total_len.min !=
				extended_pattern2.total_len.max
				)
				has_complex_patterns = true;
			continue;
		case WILDCARD_PATTERN_MATCH_ANY:
			if (wildcard_pattern && (
				current->pattern == original_tail_pattern
				))
				/* More consecutive initial wildcard patterns.
				 */
				current->pattern = tail->pattern;
			else if (!has_complex_patterns) {
				/* The beginning of the next wildcard pattern
				 * is a pattern tail.
				 */
				tail->tokens_max = token_end;
				tail->pattern = original_tail_pattern;
				return true;
			}
			continue;
		case WILDCARD_PATTERN_MATCH_ONE:
			if (wildcard_pattern && (
				current->pattern == original_tail_pattern
				))
				/* More consecutive initial wildcard patterns.
				 */
				current->pattern = tail->pattern;
			break;
		case CHARACTER_BYTE_CLASS_PATTERN:
			break;
		case CHARACTER_BYTE_PATTERN:
			if (current->pattern == original_tail_pattern) {
				/* Record the next character byte.
				 */
				if (extended_pattern)
					extended_pattern->next_character_byte =
						tail->pattern - 1;
				if (wildcard_pattern)
					wildcard_pattern->next_character_byte =
						tail->pattern - 1;
			}
			break;
		case PATTERN_SEPARATOR_PATTERN:
			if (memchr(
				tail->tokens_min,
				character_byte,
				(size_t)(token_end - tail->tokens_min)
				)) {
				/* Too complex pattern.
				 * The pattern separator can match either
				 * itself or a token separator.
				 */
				*tail = *end;
				return true;
			}
			/* FALLTHROUGH */
		case TOKEN_SEPARATOR_PATTERN:
			if (token_end >= end->tokens_max)
				/* There are no more tokens.
				 */
				return false;
			tail->tokens_min = token_end;
			tail->tokens_max = token_end;
			tail->pattern = original_tail_pattern;
			return true;
		}
		if (tail->tokens_min >= token_end)
			return false;
		assert(tail->tokens_min < end->tokens_max);
		++tail->tokens_min;
	}
	*tail = *end;
	return true;
}

static char const *
token_matches_pattern_list_partially(
	struct extended_pattern_info const *const info,
	char const *const token,
	char const *const token_end_min,
	char const *const token_end_max,
	char const *const next_character_byte,
	unsigned const recursion_limit
	) {
	assert(token <= token_end_min && token_end_min <= token_end_max);
	if ((size_t)(token_end_max - token) < info->match_len.min)
		return NULL;
	if ((size_t)(token_end_min - token) > info->match_len.max)
		return NULL;
	if (token == token_end_min && info->match_len.min == 0u)
		return token_end_min;
	/* The token does not contain separators.
	 * Therefore, a zero config is enough.
	 */
	static struct tokens_match_config const config = {
		false,
		{{0, ""}, {0, ""}}
	};
	for (struct tokens_pattern current = {token, info->begin};;) {
		struct tokens_pattern_end const end = {
			token_end_min,
			token_end_max,
			find_in_pattern(
				current.pattern,
				info->end,
				'|',
				info->end
				),
			next_character_byte
		};
		if (tokens_match_partially(
			&config,
			&current,
			&current,
			&end,
			token_end_max,
			recursion_limit
			))
			return current.tokens;
		if (end.pattern == info->end)
			return NULL;
		current.pattern = end.pattern + 1;
	}
}

static bool
tokens_match_extended_pattern_partially(
	struct tokens_match_config const *const config,
	struct extended_pattern_info const *const info,
	struct tokens_pattern const *const begin,
	struct tokens_pattern *const current_out,
	struct tokens_pattern_end const *const end,
	char const *const token_end,
	unsigned const recursion_limit,
	unsigned count
	) {
	assert(begin->tokens <= token_end && token_end <= end->tokens_max);
	assert(begin->pattern <= end->pattern);
	assert(end->tokens_min <= end->tokens_max);
	struct tokens_pattern tail = *begin;
	size_t const token_tail_len_min = info->next_character_byte ? 1u : 0u;
	for (;; ++count) {
		struct pattern_length_info const *const head_len =
			info->count.max == 0u
				? &info->total_len
				: &info->match_len;
		char const *const head_tokens = tail.tokens;
		if ((size_t)(token_end - head_tokens) < token_tail_len_min)
			return false;
		char const *tail_tokens_max =
			(size_t)(token_end - head_tokens) > head_len->max
				? head_tokens + head_len->max
				: token_end - token_tail_len_min;
		if (info->count.max > 0u && (
			count >= info->count.min || info->match_len.min == 0u
			)) {
			if (tokens_match_partially(
				config,
				&tail,
				current_out,
				end,
				token_end,
				recursion_limit
				))
				/* There are enough occurences (or there could
				 * be enough empty occurences) and
				 * the tokens match the rest of the pattern.
				 */
				return true;
			if (count >= info->count.max)
				/* No more occurences can be found.
				 */
				return false;
			/* Ignore empty tokens heads.
			 * They are irrelevant (they would only increased
			 * the occurence count but do nothing else).
			 */
			if (tail.tokens >= tail_tokens_max)
				return false;
			++tail.tokens;
		}
		if ((size_t)(
			token_end - head_tokens
			) < head_len->min + token_tail_len_min)
			return false;
		if ((size_t)(tail.tokens - head_tokens) < head_len->min)
			tail.tokens = head_tokens + head_len->min;
		if (info->count.max == 0u) {  /* !(...) */
			/* Try to split the tokens to a head and a tail so that
			 *  1) the tokens head is a token or a token prefix
			 *     (does not contain a token separator),
			 *  2) the tokens head does not match any of
			 *     the patterns in the extended pattern and
			 *  3) the tokens tail matches the rest of the pattern.
			 */
			for (;; ++tail.tokens) {
				if (info->next_character_byte) {
					if (!(tail.tokens = memchr(
						tail.tokens,
						*info->next_character_byte,
						(size_t)(
							tail_tokens_max -
							tail.tokens
							) + 1
						)))
						return false;
				}
				assert(tail.tokens <= tail_tokens_max);
				if (
					!token_matches_pattern_list_partially(
						info,
						head_tokens,
						tail.tokens,
						tail.tokens,
						info->next_character_byte,
						recursion_limit
						) &&
					tokens_match_partially(
						config,
						&tail,
						current_out,
						end,
						token_end,
						recursion_limit
						)
					)
					return true;
				if (tail.tokens >= tail_tokens_max)
					return false;
			}
		}
		/* Try to split the tokens to a head and a tail so that
		 *  1) the tokens head is a token or a token prefix
		 *     (does not contain a token separator),
		 *  2) the tokens head matches at least one of the patterns in
		 *     the extended pattern and
		 *  3) the tokens tail matches the extended pattern with
		 *     an increased occurence count.
		 */
		char const *const next_character_byte =
			count + 1 >= info->count.max
				? info->next_character_byte
				: NULL;
		assert(tail.tokens <= tail_tokens_max);
		if (next_character_byte) {
			if (!(tail.tokens = memchr(
				tail.tokens,
				*next_character_byte,
				(size_t)(tail_tokens_max - tail.tokens) + 1
				)))
				return false;
			while (*tail_tokens_max != *next_character_byte)
				--tail_tokens_max;
		}
		char const *const tail_tokens_min = tail.tokens;
		char const *const tail_tokens_initial =
			token_matches_pattern_list_partially(
				info,
				head_tokens,
				tail_tokens_min,
				tail_tokens_max,
				next_character_byte,
				recursion_limit
				);
		if (!tail_tokens_initial)
			/* There are no matches.
			 */
			return false;
		/* Repeat with different tokens tails.
		 * First with tail_tokens_initial.
		 * Then with every other tokens tail from tail_tokens_min
		 * to tail_tokens_max.
		 */
		for (tail.tokens = tail_tokens_initial;;) {
			assert(tail.tokens <= tail_tokens_max);
			char const *tail_tokens_next;
			if (
				tail.tokens == tail_tokens_initial &&
				tail_tokens_initial > tail_tokens_min
				)
				tail_tokens_next = tail_tokens_min;
			else {
				tail_tokens_next = tail.tokens;
				do {
					if (tail_tokens_next == tail_tokens_max) {
						tail_tokens_next = NULL;
						break;
					}
					++tail_tokens_next;
					if (next_character_byte) {
						tail_tokens_next = memchr(
							tail_tokens_next,
							*next_character_byte,
							(size_t)(
								tail_tokens_max -
								tail_tokens_next
								) + 1
							);
						assert(tail_tokens_next);
					}
				} while (tail_tokens_next == tail_tokens_initial);
			}
			if (
				tail.tokens == tail_tokens_initial ||
				token_matches_pattern_list_partially(
					info,
					head_tokens,
					tail.tokens,
					tail.tokens,
					next_character_byte,
					recursion_limit
					) == tail.tokens
				) {
				if (!tail_tokens_next)
					/* Tail call optimization.
					 */
					break;
				if (
					recursion_limit &&
					tokens_match_extended_pattern_partially(
						config,
						info,
						&tail,
						current_out,
						end,
						token_end,
						recursion_limit - 1,
						count + 1
						)
					)
					/* The tokens tail matches the extended
					 * pattern with an increased occurence
					 * count.
					 */
					return true;
			}
			if (!tail_tokens_next)
				return false;
			tail.tokens = tail_tokens_next;
		}
	}
}

static bool
tokens_match_wildcard_pattern_partially(
	struct tokens_match_config const *const config,
	struct wildcard_pattern_info const *const info,
	struct tokens_pattern const *const begin,
	struct tokens_pattern *const current_out,
	struct tokens_pattern_end const *const end,
	char const *const token_end,
	unsigned const recursion_limit
	) {
	assert(begin->tokens <= token_end && token_end <= end->tokens_max);
	assert(begin->pattern <= end->pattern);
	assert(end->tokens_min <= end->tokens_max);
	struct tokens_pattern current = *begin;
	if (current.pattern >= end->pattern && !end->next_character_byte) {
		assert(current.tokens <= token_end);
		assert(current.pattern == end->pattern);
		if (current.tokens < end->tokens_min) {
			if (
				!config->allow_prefix_match &&
				token_end < end->tokens_min
				)
				/* An asterisk matches the remaining token
				 * character bytes to the end of the token but
				 * not further to the tokens min end.
				 */
				return false;
			current.tokens = end->tokens_min;
		}
		if (current_out)
			*current_out = current;
		return true;
	}
	if (!recursion_limit)
		return false;
	for (;; ++current.tokens) {
		if (info->next_character_byte && !(current.tokens = memchr(
			current.tokens,
			*info->next_character_byte,
			(size_t)(token_end - current.tokens)
			)))
			return false;
		assert(current.tokens <= token_end);
		if (tokens_match_partially(
			config,
			&current,
			current_out,
			end,
			token_end,
			recursion_limit - 1
			))
			return true;
		if (current.tokens >= token_end)
			return false;
	}
}

static bool
tokens_match_partially(
	struct tokens_match_config const *const config,
	struct tokens_pattern const *const begin,
	struct tokens_pattern *const current_out,
	struct tokens_pattern_end const *const end,
	char const *token_end,
	unsigned const recursion_limit
	) {
	if (token_end == NULL)
		token_end = find_end_of_token(config, begin, end);
	assert(begin->tokens <= token_end && token_end <= end->tokens_max);
	assert(begin->pattern <= end->pattern);
	assert(end->tokens_min <= end->tokens_max);
	struct tokens_pattern current = *begin;
	while (current.pattern < end->pattern) {
		char character_byte;
		struct character_byte_class_info character_byte_class;
		struct extended_pattern_info extended_pattern;
		struct wildcard_pattern_info wildcard_pattern;
		bool const measure_extended_patterns_on = true;
		struct tokens_pattern_end tail;
		if (token_end < current.tokens)
			token_end = find_end_of_token(config, &current, end);
		switch (parse_next_pattern_entity(
			&current.pattern,
			end->pattern,
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
			if (!find_tokens_pattern_tail(
				config,
				&current,
				&tail,
				end,
				token_end,
				&extended_pattern,
				NULL
				))
				return false;
			if (!tokens_match_extended_pattern_partially(
				config,
				&extended_pattern,
				&current,
				&current,
				&tail,
				token_end <= tail.tokens_max
					? token_end
					: tail.tokens_max,
				recursion_limit - 1,
				0
				))
				return false;
			continue;
		case WILDCARD_PATTERN_MATCH_ANY:
			/* An asterisk (*) matches any number of (including
			 * zero) token character bytes but not a token
			 * separator.
			 */
			if (!find_tokens_pattern_tail(
				config,
				&current,
				&tail,
				end,
				token_end,
				NULL,
				&wildcard_pattern
				))
				return false;
			assert(token_end <= tail.tokens_max);
			if (!tokens_match_wildcard_pattern_partially(
				config,
				&wildcard_pattern,
				&current,
				&current,
				&tail,
				token_end,
				recursion_limit
				))
				return false;
			continue;
		case WILDCARD_PATTERN_MATCH_ONE:
			/* A question mark (?) matches any token character byte
			 * but not a token separator.
			 */
			if (current.tokens >= token_end)
				return false;
			break;
		case CHARACTER_BYTE_CLASS_PATTERN:
			/* A character byte class ([...]) matches any token
			 * character byte in the class.
			 * A complemented character byte class ([!...]) matches
			 * any token character byte not in the class.
			 * Neither matches a token separator.
			 */
			if (current.tokens >= token_end)
				return false;
			if (!character_byte_matches_character_byte_class(
				&character_byte_class,
				*current.tokens
				))
				return false;
			break;
		case CHARACTER_BYTE_PATTERN:
			if (current.tokens >= token_end)
				return false;
			if (*current.tokens != character_byte)
				return false;
			break;
		case PATTERN_SEPARATOR_PATTERN:
		case TOKEN_SEPARATOR_PATTERN:
			/* A separator character byte matches itself or
			 * a token separator character byte.
			 */
			if (current.tokens >= end->tokens_max)
				return false;
			if (
				*current.tokens != character_byte &&
				current.tokens != token_end
				)
				return false;
			++current.tokens;
			continue;
		}
		assert(current.tokens < end->tokens_max);
		++current.tokens;
	}
	/* The end of the pattern.
	 */
	assert(current.tokens <= end->tokens_max);
	if (end->next_character_byte) {
		if (*current.tokens != *end->next_character_byte)
			return false;
	}
	if (current.tokens < end->tokens_min) {
		if (!config->allow_prefix_match)
			return false;
		if (current.tokens != token_end)
			return false;
		current.tokens = end->tokens_min;
	}
	if (current_out)
		*current_out = current;
	return true;
}
