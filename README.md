# PAM SSH Authentication Information Module

The pam_ssh_auth_info.so PAM module is designed to succeed or fail authentication based on SSH authentication information.
One use is to select whether to load other modules based on this test.

## Requirements

The following packages are required in order to build and install this module:

* PAM development files (libpam-dev, pam-devel or such)
* Automake (automake)
* Autoconf (autoconf)

The following packages are required in order to make use of this module:

* OpenSSH server (openssh-server >= 7.8p1)

## Building and Installation

The following commands can be used to build this module:

1. autoreconf --install
2. ./configure
3. make
4. sudo make install

It may be necessary to pass
--libdir=DIR,
--exec-prefix=EPREFIX or
--prefix=PREFIX
option to ./configure in order to get the PAM module to be installed in the right directory.

## SSH Server Configuration

It is possible to enable PAM authentication in SSH server using
keyboard interactive challenge response authentication or
password authentication.

### Challenge Response Authentication

In order to enable PAM authentication using
keyboard interactive challenge response authentication,
the following SSH server options must be set
in **/etc/ssh/sshd_config** or such
(either explicitly or via defaults):

* AuthenticationMethods
  - must not be set or must contain **keyboard-interactive**
* ChallengeResponseAuthentication yes
* KeyboardInteractiveAuthentication yes
* UsePAM yes

### Password Authentication

In order to enable PAM authentication using
password authentication,
the following SSH server options must be set
in **/etc/ssh/sshd_config** or such
(either explicitly or via defaults):

* AuthenticationMethods
  - must not be set or must contain **password**
* PasswordAuthentication yes
* UsePAM yes

## PAM Configuration

In order to enable this module,
an appropriate file in the **/etc/pam.d/** directory
(such as **/etc/pam.d/ssh**)
must be modified
to contain a line in form of

    auth [auth_err=fail-action, default=error-action, ignore=ignore-action, success=success-action] pam_ssh_auth_info.so [option...] [pattern...]

For more information,
see
[pam_ssh_auth_info(8)](https://github.Eero.Häkkinen.fi/pam-ssh-auth-info) and
[pam.conf(5)](https://manpages.debian.org/pam.conf.5).
