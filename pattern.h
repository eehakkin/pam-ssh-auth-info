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
	char ch,
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
is_character_byte_class(
	char const *const pattern,
	char const *const pattern_end,
	struct character_byte_class_info *const info
	) {
	assert(pattern < pattern_end);
	if (pattern_end - pattern < 3)
		return false;
	if (pattern[0] != '[')
		return false;
	if (pattern[1] == '!') {  /* [!...] */
		info->begin = pattern + 2;
		info->negation = true;
	}
	else {  /* [...] */
		info->begin = pattern + 1;
		info->negation = false;
	}
	info->end = (char const *)memchr(
		info->begin + 1,
		']',
		(size_t)(pattern_end - (info->begin + 1))
		);
	return info->end != NULL;
}

struct extended_pattern_info {
	char const *begin;
	char const *end;
	struct pattern_count_info count;
	struct pattern_length_info match_len;
	struct pattern_length_info total_len;
};

static bool
is_extended_pattern(
	char const *const pattern,
	char const *const pattern_end,
	struct extended_pattern_info *const info,
	bool const measure
	) {
	assert(pattern < pattern_end);
	if (pattern_end - pattern < 3)
		return false;
	if (pattern[1] != '(')
		return false;
	switch (pattern[0]) {
	case '?':  /* zero or one occurence */
		info->count.min = 0u;
		info->count.max = 1u;
		break;
	case '*':  /* zero or more occurences */
		info->count.min = 0u;
		info->count.max = ~0u;
		break;
	case '+':  /* one or more occurences */
		info->count.min = 1u;
		info->count.max = ~0u;
		break;
	case '@':  /* one occurence */
		info->count.min = info->count.max = 1u;
		break;
	case '!':  /* anything except one occurence */
		info->count.min = info->count.max = 0u;
		break;
	default:
		return false;
	}
	info->begin = pattern + 2;
	info->end = find_in_pattern(info->begin, pattern_end, ')', NULL);
	if (!info->end)
		return false;
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

static char const *
find_in_pattern(
	char const *pattern,
	char const *const pattern_end,
	char ch,
	char const *const not_found
	) {
	assert(pattern <= pattern_end);
	for (; pattern < pattern_end; ++pattern) {
		struct character_byte_class_info character_byte_class;
		struct extended_pattern_info extended_pattern;
		bool const measure_extended_patterns_off = false;
		if (*pattern == ch)
			return pattern;
		/* Skip extended patterns, character classes and backslash
		 * escaped character bytes.
		 */
		if (is_extended_pattern(
			pattern,
			pattern_end,
			&extended_pattern,
			measure_extended_patterns_off
			))
			pattern = extended_pattern.end;
		else if (is_character_byte_class(
			pattern,
			pattern_end,
			&character_byte_class
			))
			pattern = character_byte_class.end;
		else if (*pattern == '\\') {
			if (pattern_end - pattern >= 2)
				++pattern;
		}
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
	for (; pattern < pattern_end; ++pattern) {
		struct character_byte_class_info character_byte_class;
		struct extended_pattern_info extended_pattern;
		bool const measure_extended_patterns_on = true;
		if (is_extended_pattern(
			pattern,
			pattern_end,
			&extended_pattern,
			measure_extended_patterns_on
			)) {
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
			pattern = extended_pattern.end;
			continue;
		}
		else if (is_character_byte_class(
			pattern,
			pattern_end,
			&character_byte_class
			))
			pattern = character_byte_class.end;
		else if (*pattern == '*') {
			len->max = SIZE_MAX;
			continue;
		}
		else if (*pattern == '\\') {
			if (pattern_end - pattern >= 2)
				++pattern;
		}
		++len->min;
		/* Ditto for the maximum,
		 * but do not let it overflow.
		 */
		if (len->max != SIZE_MAX)
			++len->max;
	}
}
