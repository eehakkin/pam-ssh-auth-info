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

#ifdef HAVE_CONFIG_H
#	include "config.h"
#endif

#undef  NDEBUG
#define NDEBUG
#define PAM_SM_ACCOUNT
#define PAM_SM_AUTH
#define PAM_SM_SESSION
#define PAM_SM_PASSWORD

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <syslog.h>

#if defined(HAVE_SECURITY_PAM_APPL_H) || !defined(PACKAGE_NAME)
#	include <security/pam_appl.h>
#endif
#if defined(HAVE_SECURITY_PAM_EXT_H) || !defined(PACKAGE_NAME)
#	include <security/pam_ext.h>
#endif
#if defined(HAVE_SECURITY_PAM_MODULES_H) || !defined(PACKAGE_NAME)
#	include <security/pam_modules.h>
#endif

#include "line_tokens_match.h"
#include "pam_syslog.h"

/* Check if a string is in a list separated by separators.
 */
static bool
in_list(char const *list, char sep, char const *s) {
	size_t const n = strlen(s);
	for (char const *p = strstr(list, s); p && *p; p = strstr(p + 1, s)) {
		/* Check that the string is either the first item in the list
		 * or is preceded by a separator and that the string is either
		 * the last item in the list or is followed by a separator (so
		 * a mere substring within another list item is not enough).
		 */
		if ((p == list || p[-1] == sep) && (!p[n] || p[n] == sep))
			return true;
	}
	return false;
}

/* Locate the beginning of the next line or the NUL byte.
 */
static char const *
next_line(char const *s) {
	s += strcspn(s, "\n");
	if (*s == '\n')
		++s;
	return s;
}

int
pam_sm_authenticate(
	pam_handle_t *pamh,
	int flags,
	int argc,
	char const **argv
	) {
	(void)flags;
	/* Parse options.
	 */
	bool debug = false;
	char const *disable = NULL;
	char const *enable = NULL;
	enum {
		MATCH_ALL_OF,
		MATCH_ANY_OF,
		MATCH_NONE_OF
	} match_style = MATCH_ALL_OF;
	bool quiet_fail = false;
	bool quiet_success = false;
	unsigned recursion_limit = 100u;
	for (; argc > 0; --argc, ++argv) {
		if (strncmp(*argv, "all", 3) == 0 && (
			strcmp(*argv + 3, "_of") == 0 ||
			strcmp(*argv + 3, "") == 0
			))
			match_style = MATCH_ALL_OF;
		else if (strncmp(*argv, "any", 3) == 0 && (
			strcmp(*argv + 3, "_of") == 0 ||
			strcmp(*argv + 3, "") == 0
			))
			match_style = MATCH_ANY_OF;
		else if (strcmp(*argv, "debug") == 0)
			debug = true;
		else if (strncmp(*argv, "disable=", 8) == 0)
			disable = *argv + 8;
		else if (strncmp(*argv, "enable=", 7) == 0)
			enable = *argv + 7;
		else if (strcmp(*argv, "none_of") == 0)
			match_style = MATCH_NONE_OF;
		else if (strcmp(*argv, "quiet") == 0)
			quiet_fail = quiet_success = true;
		else if (strcmp(*argv, "quiet_fail") == 0)
			quiet_fail = true;
		else if (strcmp(*argv, "quiet_success") == 0)
			quiet_success = true;
		else if (strncmp(*argv, "recursion_limit=", 16) == 0)
			recursion_limit = strtoul(*argv + 16, NULL, 0);
		else
			break;
	}
	/* Process options.
	 */
	if (disable || enable) {
		int ret;
		char const *service = NULL;
		if ((ret = pam_get_item(
			pamh,
			PAM_SERVICE,
			(void const **)&service
			)) != PAM_SUCCESS)
			return ret;
		if (!service || !*service) {
			if (debug)
				pam_syslog(pamh, LOG_DEBUG, "no service");
			return PAM_IGNORE;
		}
		if (disable && in_list(disable, ':', service)) {
			if (debug)
				pam_syslog(
					pamh,
					LOG_DEBUG,
					"disabled"
					" for service %s"
					" due to disable=%s",
					service,
					disable
					);
			return PAM_IGNORE;
		}
		if (enable && !in_list(enable, ':', service)) {
			if (debug)
				pam_syslog(
					pamh,
					LOG_DEBUG,
					"not enabled"
					" for service %s"
					" due to enable=%s",
					service,
					enable
					);
			return PAM_IGNORE;
		}
	}
	/* Retrieve SSH authentication information.
	 */
	char const* ssh_auth_info = pam_getenv(pamh, "SSH_AUTH_INFO_0");
	if (!ssh_auth_info || !*ssh_auth_info) {
		if (debug)
			pam_syslog(
				pamh,
				LOG_DEBUG,
				!ssh_auth_info ? "no %s" : "empty %s",
				"SSH_AUTH_INFO_0"
				);
		return PAM_IGNORE;
	}
	/* Process SSH authentication information patterns.
	 */
	bool success = match_style != MATCH_ANY_OF;
	for (; argc > 0; --argc, ++argv) {
		bool matches = false;
		for (char const *s = ssh_auth_info; *s; s = next_line(s)) {
			matches = initial_first_line_tokens_match(
				s,
				*argv,
				recursion_limit
				);
			if (debug)
				pam_syslog(
					pamh,
					LOG_DEBUG,
					"ssh auth info"
					" line \"%.*s\""
					" %s"
					" pattern \"%s\"",
					(int)strcspn(s, "\n"),
					s,
					matches ? "matches" : "does not match",
					*argv
					);
			if (matches)
				break;
		}
		switch (match_style) {
		case MATCH_ALL_OF:
			if (matches)
				continue;
			success = false;
			break;
		case MATCH_ANY_OF:
			if (!matches)
				continue;
			success = true;
			break;
		case MATCH_NONE_OF:
			if (!matches)
				continue;
			success = false;
			break;
		}
		break;
	}
	if (!(success ? quiet_success : quiet_fail)) {
		char const *user = NULL;
		if (pam_get_item(
			pamh,
			PAM_USER,
			(void const **)&user
			) != PAM_SUCCESS || user == NULL)
			user = "(unknown)";
		pam_syslog(
			pamh,
			LOG_INFO,
			"ssh auth info %s%s%s%s %s by user %s",
			argc ? "pattern requirement" : "pattern requirements",
			argc ? " \"" : "",
			argc ? *argv : "",
			argc ? "\""  : "",
			success ? "met" : "not met",
			user
			);
	}
	return success ? PAM_SUCCESS : PAM_AUTH_ERR;
}

int
pam_sm_setcred(
	pam_handle_t *pamh,
	int flags,
	int argc,
	char const **argv
	) {
	return pam_sm_authenticate(pamh, flags, argc, argv);
}

int
pam_sm_acct_mgmt(
	pam_handle_t *pamh,
	int flags,
	int argc,
	char const **argv
	) {
	return pam_sm_authenticate(pamh, flags, argc, argv);
}

int
pam_sm_open_session(
	pam_handle_t *pamh,
	int flags,
	int argc,
	char const **argv
	) {
	return pam_sm_authenticate(pamh, flags, argc, argv);
}

int
pam_sm_close_session(
	pam_handle_t *pamh,
	int flags,
	int argc,
	char const **argv
	) {
	return pam_sm_authenticate(pamh, flags, argc, argv);
}

int
pam_sm_chauthtok(
	pam_handle_t *pamh,
	int flags,
	int argc,
	char const **argv
	) {
	return pam_sm_authenticate(pamh, flags, argc, argv);
}
