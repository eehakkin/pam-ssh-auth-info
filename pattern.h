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
#include <stdint.h>
#include <string.h>

#ifndef SIZE_MAX
#	define SIZE_MAX (~(size_t)0)
#endif

struct pattern_count_info {
	unsigned min;
	unsigned max;
};

struct pattern_length_info {
	size_t min;
	size_t max;
};

static char const *
find_in_pattern(
	char const *pattern,
	char const *const pattern_end,
	char const ch,
	char const *const not_found
	);

static void
measure_pattern(
	char const *pattern,
	char const *const pattern_end,
	struct pattern_length_info *const match_len
	);

struct character_byte_class_info {
	char const *begin;
	char const *end;
	bool negation;
};

static bool
parse_character_byte_class_pattern(
	char const **const pattern_ptr,
	char const *const pattern_end,
	struct character_byte_class_info *const info
	) {
	assert(*pattern_ptr < pattern_end);
	if (pattern_end - *pattern_ptr < 3)
		return false;
	if ((*pattern_ptr)[0] != '[')
		return false;
	if ((*pattern_ptr)[1] == '!') {  /* [!...] */
		info->begin = *pattern_ptr + 2;
		info->negation = true;
	}
	else {  /* [...] */
		info->begin = *pattern_ptr + 1;
		info->negation = false;
	}
	info->end = (char const *)memchr(
		info->begin + 1,
		']',
		(size_t)(pattern_end - (info->begin + 1))
		);
	if (!info->end)
		return false;
	*pattern_ptr = info->end + 1;
	return true;
}

struct character_byte_set {
	size_t len;
	char const *ptr;
};

static bool
in_character_byte_set(
	struct character_byte_set const *const set,
	char const ch
	) {
	return memchr(set->ptr, ch, set->len) != NULL;
}

struct extended_pattern_info {
	char const *begin;
	char const *end;
	struct pattern_count_info count;
	struct pattern_length_info match_len;
	struct pattern_length_info total_len;
	char const *next_character_byte;
};

static bool
parse_extended_pattern(
	char const **const pattern_ptr,
	char const *const pattern_end,
	struct extended_pattern_info *const info,
	bool const measure
	) {
	assert(*pattern_ptr < pattern_end);
	if (pattern_end - *pattern_ptr < 3)
		return false;
	if ((*pattern_ptr)[1] != '(')
		return false;
	switch ((*pattern_ptr)[0]) {
	case '?':  /* zero or one occurence */
		info->count.min = 0u;
		info->count.max = 1u;
		break;
	case '*':  /* zero or more occurences */
		info->count.min = 0u;
		info->count.max = ~0u;
		break;
	case '@':  /* one occurence */
		info->count.min = info->count.max = 1u;
		break;
	case '+':  /* one or more occurences */
		info->count.min = 1u;
		info->count.max = ~0u;
		break;
	case '!':  /* anything except one occurence */
		info->count.min = info->count.max = 0u;
		break;
	default:
		return false;
	}
	info->begin = *pattern_ptr + 2;
	info->end = find_in_pattern(info->begin, pattern_end, ')', NULL);
	if (!info->end)
		return false;
	info->next_character_byte = NULL;
	*pattern_ptr = info->end + 1;
	if (measure) {
		info->match_len.min = SIZE_MAX;
		info->match_len.max = 0u;
		for (char const *pattern2 = info->begin;;) {
			struct pattern_length_info match_len;
			char const *const pattern2_end = find_in_pattern(
				pattern2,
				info->end,
				'|',
				info->end
				);
			measure_pattern(pattern2, pattern2_end, &match_len);
			if (info->match_len.min > match_len.min)
				info->match_len.min = match_len.min;
			if (info->match_len.max < match_len.max)
				info->match_len.max = match_len.max;
			if (pattern2_end == info->end)
				break;
			pattern2 = pattern2_end + 1;
		}
		if (info->count.max == 0u) {  /* !(...) */
			info->total_len.min =
				info->match_len.min == 0u ? 1u : 0u;
			info->total_len.max = SIZE_MAX;
		}
		else {
			info->total_len.min =
				info->count.min * info->match_len.min;
			/* Ditto for the maximum,
			 * but do not let it overflow.
			 */
			if (info->match_len.max <= SIZE_MAX / info->count.max)
				info->total_len.max =
					info->count.max * info->match_len.max;
			else
				info->total_len.max = SIZE_MAX;
		}
	}
	return true;
}

struct wildcard_pattern_info {
	char const *next_character_byte;
};

enum pattern_type {
	EXTENDED_PATTERN,
	PATTERN_SEPARATOR_PATTERN,
	TOKEN_SEPARATOR_PATTERN,
	CHARACTER_BYTE_PATTERN,
	CHARACTER_BYTE_CLASS_PATTERN = '[',
	WILDCARD_PATTERN_MATCH_ANY = '*',
	WILDCARD_PATTERN_MATCH_ONE = '?',
};

static enum pattern_type
parse_next_pattern_entity(
	char const **const pattern_ptr,
	char const *const pattern_end,
	struct character_byte_set const *const pattern_separators,
	struct character_byte_set const *const token_separators,
	char *const character_byte,
	struct character_byte_class_info *const character_byte_class,
	struct extended_pattern_info *const extended_pattern,
	struct wildcard_pattern_info *const wildcard_pattern,
	bool const measure_extended_patterns
	) {
	switch (**pattern_ptr) {
	case '?':
		if (extended_pattern && parse_extended_pattern(
			pattern_ptr,
			pattern_end,
			extended_pattern,
			measure_extended_patterns
			))
			return EXTENDED_PATTERN;
		if (wildcard_pattern) {
			++*pattern_ptr;
			wildcard_pattern->next_character_byte = NULL;
			return WILDCARD_PATTERN_MATCH_ONE;
		}
		break;
	case '*':
		if (extended_pattern && parse_extended_pattern(
			pattern_ptr,
			pattern_end,
			extended_pattern,
			measure_extended_patterns
			))
			return EXTENDED_PATTERN;
		if (wildcard_pattern) {
			++*pattern_ptr;
			wildcard_pattern->next_character_byte = NULL;
			return WILDCARD_PATTERN_MATCH_ANY;
		}
		break;
	case '@':
	case '+':
	case '!':
		if (extended_pattern && parse_extended_pattern(
			pattern_ptr,
			pattern_end,
			extended_pattern,
			measure_extended_patterns
			))
			return EXTENDED_PATTERN;
		break;
	case '[':
		if (character_byte_class && parse_character_byte_class_pattern(
			pattern_ptr,
			pattern_end,
			character_byte_class
			))
			return CHARACTER_BYTE_CLASS_PATTERN;
		break;
	case '\\':
		if (pattern_end - *pattern_ptr >= 2)
			++*pattern_ptr;
		break;
	default:
		if (pattern_separators && in_character_byte_set(
			pattern_separators,
			**pattern_ptr
			)) {
			*character_byte = *(*pattern_ptr)++;
			return PATTERN_SEPARATOR_PATTERN;
		}
		if (token_separators && in_character_byte_set(
			token_separators,
			**pattern_ptr
			)) {
			*character_byte = *(*pattern_ptr)++;
			return TOKEN_SEPARATOR_PATTERN;
		}
	}
	*character_byte = *(*pattern_ptr)++;
	return CHARACTER_BYTE_PATTERN;
}

static char const *
find_in_pattern(
	char const *pattern,
	char const *const pattern_end,
	char const ch,
	char const *const not_found
	) {
	assert(pattern <= pattern_end);
	while (pattern < pattern_end) {
		char character_byte;
		struct character_byte_class_info character_byte_class;
		struct extended_pattern_info extended_pattern;
		struct wildcard_pattern_info wildcard_pattern;
		bool const measure_extended_patterns_off = false;
		if (*pattern == ch)
			return pattern;
		/* Skip an extended pattern, a character byte class,
		 * a backslash escaped character byte or a character byte.
		 */
		parse_next_pattern_entity(
			&pattern,
			pattern_end,
			NULL,
			NULL,
			&character_byte,
			&character_byte_class,
			&extended_pattern,
			&wildcard_pattern,
			measure_extended_patterns_off
			);
	}
	return not_found;
}

static void
measure_pattern(
	char const *pattern,
	char const *const pattern_end,
	struct pattern_length_info *const len
	) {
	assert(pattern <= pattern_end);
	len->min = len->max = 0u;
	while (pattern < pattern_end) {
		char character_byte;
		struct character_byte_class_info character_byte_class;
		struct extended_pattern_info extended_pattern;
		struct wildcard_pattern_info wildcard_pattern;
		bool const measure_extended_patterns_on = true;
		switch (parse_next_pattern_entity(
			&pattern,
			pattern_end,
			NULL,
			NULL,
			&character_byte,
			&character_byte_class,
			&extended_pattern,
			&wildcard_pattern,
			measure_extended_patterns_on
			)) {
		case EXTENDED_PATTERN:
			len->min += extended_pattern.total_len.min;
			/* Ditto for the maximum,
			 * but do not let it overflow.
			 */
			if (len->max <= (
				SIZE_MAX - extended_pattern.total_len.max
				))
				len->max += extended_pattern.total_len.max;
			else
				len->max = SIZE_MAX;
			continue;
		case WILDCARD_PATTERN_MATCH_ANY:
			len->max = SIZE_MAX;
			continue;
		case WILDCARD_PATTERN_MATCH_ONE:
		case CHARACTER_BYTE_CLASS_PATTERN:
		case CHARACTER_BYTE_PATTERN:
			break;
		case PATTERN_SEPARATOR_PATTERN:
		case TOKEN_SEPARATOR_PATTERN:
			assert(false);
		}
		++len->min;
		/* Ditto for the maximum,
		 * but do not let it overflow.
		 */
		if (len->max != SIZE_MAX)
			++len->max;
	}
}
