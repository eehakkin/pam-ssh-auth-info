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

#include <string.h>

static char const *
find_in_pattern(
	char const *pattern,
	char const *const pattern_end,
	char ch
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
	info->end = find_in_pattern(info->begin, pattern_end, ')');
	return info->end != NULL;
}

static char const *
find_in_pattern(
	char const *pattern,
	char const *const pattern_end,
	char ch
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
	return NULL;
}
