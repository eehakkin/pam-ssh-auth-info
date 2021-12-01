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

#include <string.h>

struct character_class_info {
	bool negation;
	char const *begin;
	char const *end;
};

static bool
is_character_class(
	char const *pattern,
	char const *const pattern_end,
	struct character_class_info *info
	) {
	if (pattern_end - pattern < 3)
		return false;
	if (pattern[0] != '[')
		return false;
	if (pattern[1] == '!') {  /* [!...] */
		info->negation = true;
		info->begin = pattern + 2;
	}
	else {  /* [...] */
		info->negation = false;
		info->begin = pattern + 1;
	}
	info->end = memchr(
		info->begin + 1,
		']',
		(size_t)(pattern_end - info->begin - 1)
		);
	return info->end != NULL;
}
