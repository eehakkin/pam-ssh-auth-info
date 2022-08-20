/*
 * Copyright © 2021 - 2022 Eero Häkkinen <Eero+pam-ssh-auth-info@Häkkinen.fi>
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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#ifndef SIZE_MAX
#	define SIZE_MAX (~(size_t)0)
#endif

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
	size_t *min,
	size_t *max
	);

struct character_class_info {
	char const *begin;
	char const *end;
	bool negation;
};

static bool
is_character_class(
	char const *const pattern,
	char const *const pattern_end,
	struct character_class_info *info
	) {
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
	info->end = memchr(
		info->begin + 1,
		']',
		(size_t)(pattern_end - (info->begin + 1))
		);
	return info->end != NULL;
}

struct extended_pattern_info {
	char const *begin;
	char const *end;
	struct {
		unsigned min;
		unsigned max;
	} count;
	struct {
		size_t min;
		size_t max;
	} match_len;
};

static bool
is_extended_pattern(
	char const *const pattern,
	char const *const pattern_end,
	struct extended_pattern_info *info
	) {
	if (pattern_end - pattern < 3)
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
	if (pattern[1] != '(')
		return false;
	info->begin = pattern + 2;
	info->end = find_in_pattern(info->begin, pattern_end, ')', NULL);
	if (!info->end)
		return false;
	info->match_len.min = SIZE_MAX;
	info->match_len.max = 0u;
	/* A match length is not meaningful for !(...).
	 */
	if (info->count.max != 0) { /* Not !(...) */
		char const *pattern2 = info->begin;
		for (;;) {
			size_t min, max;
			char const *pattern2_end = find_in_pattern(
				pattern2,
				info->end,
				'|',
				info->end
				);
			measure_pattern(pattern2, pattern2_end, &min, &max);
			if (info->match_len.min > min)
				info->match_len.min = min;
			if (info->match_len.max < max)
				info->match_len.max = max;
			if (pattern2_end == info->end)
				break;
			pattern2 = pattern2_end + 1;
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
	for (; pattern < pattern_end; ++pattern) {
		struct character_class_info character_class;
		struct extended_pattern_info extended_pattern;
		if (*pattern == ch)
			return pattern;
		/* Skip extended patterns, character classes and backslash
		 * escaped character bytes.
		 */
		if (is_extended_pattern(
			pattern,
			pattern_end,
			&extended_pattern
			))
			pattern = extended_pattern.end;
		else if (is_character_class(
			pattern,
			pattern_end,
			&character_class
			))
			pattern = character_class.end;
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
	size_t *min,
	size_t *max
	) {
	*min = *max = 0u;
	for (; pattern < pattern_end; ++pattern) {
		struct character_class_info character_class;
		struct extended_pattern_info extended_pattern;
		if (is_extended_pattern(
			pattern,
			pattern_end,
			&extended_pattern
			)) {
			if (extended_pattern.count.max == 0)
				/* !(...) */
				*max = SIZE_MAX;
			else {
				*min +=
					extended_pattern.count.min *
					extended_pattern.match_len.min;
				/* Ditto for the maximum,
				 * but do not let it overflow.
				 */
				if (
					extended_pattern.count.max <=
					(SIZE_MAX - *max) /
					extended_pattern.match_len.max
					)
					*max +=
						extended_pattern.count.max *
						extended_pattern.match_len.max;
				else
					*max = SIZE_MAX;
			}
			pattern = extended_pattern.end;
			continue;
		}
		else if (is_character_class(
			pattern,
			pattern_end,
			&character_class
			))
			pattern = character_class.end;
		else if (*pattern == '*') {
			*max = SIZE_MAX;
			continue;
		}
		else if (*pattern == '\\') {
			if (pattern_end - pattern >= 2)
				++pattern;
		}
		++*min;
		/* Ditto for the maximum,
		 * but do not let it overflow.
		 */
		if (*max != SIZE_MAX)
			++*max;
	}
}
