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
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#undef  NDEBUG
#define NDEBUG
#define PAM_SM_ACCOUNT
#define PAM_SM_AUTH
#define PAM_SM_SESSION
#define PAM_SM_PASSWORD

#include <stdbool.h>
#include <string.h>

#include <syslog.h>

#ifdef HAVE_SECURITY_PAM_APPL_H
#	include <security/pam_appl.h>
#endif
#ifdef HAVE_SECURITY_PAM_EXT_H
#	include <security/pam_ext.h>
#endif
#ifdef HAVE_SECURITY_PAM_MODULES_H
#	include <security/pam_modules.h>
#endif

#include "line_tokens_match.h"
#include "pam_syslog.h"

/* Check if a string is in a list separated by separators.
 */
static bool
in_list(char const *list, char sep, char const *s) {
	size_t const n = strlen(s);
	for (char const *p = strstr(list, s); p; p = strstr(p + 1, s)) {
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
	bool any = false;
	bool debug = false;
	char const *disable = NULL;
	char const *enable = NULL;
	bool quiet_fail = false;
	bool quiet_success = false;
	for (; argc > 0; --argc, ++argv) {
		if (strcmp(*argv, "all") == 0)
			any = false;
		else if (strcmp(*argv, "any") == 0)
			any = true;
		else if (strcmp(*argv, "debug") == 0)
			debug = true;
		else if (strncmp(*argv, "disable=", 8) == 0)
			disable = *argv + 8;
		else if (strncmp(*argv, "enable=", 7) == 0)
			enable = *argv + 7;
		else if (strcmp(*argv, "quiet") == 0)
			quiet_fail = quiet_success = true;
		else if (strcmp(*argv, "quiet_fail") == 0)
			quiet_fail = true;
		else if (strcmp(*argv, "quiet_success") == 0)
			quiet_success = true;
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
	bool matches = !any;
	for (; argc > 0; --argc, ++argv) {
		for (char const *s = ssh_auth_info; *s; s = next_line(s)) {
			matches = initial_first_line_tokens_match(s, *argv);
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
		if (matches == any)
			break;
	}
	if (!(matches ? quiet_success : quiet_fail)) {
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
			matches ? "met" : "not met",
			user
			);
	}
	return matches ? PAM_SUCCESS : PAM_AUTH_ERR;
}

int
pam_sm_setcred(
	pam_handle_t *pamh,
	int flags,
	int argc,
	char const **argv
	) {
	(void)pamh;
	(void)flags;
	(void)argc;
	(void)argv;
	return PAM_IGNORE;
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
