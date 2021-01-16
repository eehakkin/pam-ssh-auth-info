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

#include <stdio.h>
#include <syslog.h>

#ifndef HAVE_PAM_SYSLOG
static void
pam_syslog(pam_handle_t *pamh, int priority, char const *format, ...) {
	char buffer[1024];
	char const *service = NULL;
	if (pam_get_item(
		pamh,
		PAM_SERVICE,
		(void const **)&service
		) != PAM_SUCCESS || !service)
		service = "<unknown>";
	snprintf(
		buffer,
		sizeof buffer,
		"%s(%s:%s)",
		PACKAGE_NAME,
		service,
		"auth"
		);
	va_list args;
	va_start(args, format);
	openlog(buffer, LOG_CONS | LOG_PID, LOG_AUTHPRIV);
#ifdef HAVE_VSYSLOG
	vsyslog(priority, format, args);
#else
	vsnprintf(buffer, sizeof buffer, format, args);
	syslog(priority, "%s", buffer);
#endif
	closelog();
	va_end(args);
}
#endif  /* !HAVE_PAM_SYSLOG */
