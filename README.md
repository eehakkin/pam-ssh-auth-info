[![Build CI](https://github.com/eehakkin/pam-ssh-auth-info/actions/workflows/build.yml/badge.svg)](https://github.com/eehakkin/pam-ssh-auth-info/actions/workflows/build.yml)

# PAM SSH Authentication Information Module

The pam_ssh_auth_info.so PAM module is designed to succeed or fail
authentication based on SSH authentication information consisting of
a list of successfully completed authentication methods and public
credentials (e.g. keys) used to authenticate the user.
One use is to select whether to load other modules based on this test.

## Requirements

The following packages are required in order to build and install this
module:

* PAM development files (libpam-dev, pam-devel or such)
* Autoconf (autoconf)
* Automake (automake)
* C development files and tools (build-essential, Development tools or such)
  consisting at least of:
  * C compiler (such as gcc)
  * C development files (libc6-dev, glibc-devel or such)
  * GNU Libtool (libtool)
  * Make (make)

The following packages are required in order to make use of this module:

* OpenSSH server (openssh-server >= 7.8p1 or >= 9.8p1 recommended)

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
option to ./configure in order to get the PAM module to be installed in
the right directory.

## SSH Server Configuration

It is possible to enable PAM authentication in SSH server using
keyboard-interactive challenge-response authentication or
password authentication.

### Keyboard-Interactive Authentication

If keyboard-interactive challenge-response authentication is used, SSH
authentication information is available to PAM **auth** modules
beginning with OpenSSH version 7.8p1.

In order to enable PAM authentication using keyboard-interactive
challenge-response authentication, the following SSH server options must
be set in **/etc/ssh/sshd_config** or such (either explicitly or via
defaults):

* AuthenticationMethods
  - must contain **keyboard-interactive** (or
    **keyboard-interactive:pam**) or must not be set
  - should contain comma-separated lists of authentication method names
    with **keyboard-interactive** (or **keyboard-interactive:pam**) as
    the last list items so that there are successfully completed
    authentication methods and public credentials (e.g. keys) during
    the PAM authentication, for example
    ```
    AuthenticationMethods publickey,keyboard-interactive
    ```
    for mandatory public key authentication and mandatory PAM
    authentication using keyboard-interactive challenge-response
    authentication or
    ```
    AuthenticationMethods publickey,keyboard-interactive keyboard-interactive
    ```
    for optional public key authentication and mandatory PAM
    authentication using keyboard-interactive challenge-response
    authentication
* KbdInteractiveAuthentication yes
  - NOTE: Before OpenSSH version 8.7p1,
    **ChallengeResponseAuthentication yes** was needed for
    **KbdInteractiveAuthentication yes** to have an effect.
* UsePAM yes

### Password Authentication

If password authentication is used, SSH authentication information is
available to PAM **auth** modules beginning with OpenSSH version 9.8p1.

In order to enable PAM authentication using password authentication,
the following SSH server options must be set in **/etc/ssh/sshd_config**
or such (either explicitly or via defaults):

* AuthenticationMethods
  - must contain **password** or must not be set
  - should contain comma-separated lists of authentication method names
    with **password** as the last list items so that there are
    successfully completed authentication methods and public credentials
    (e.g. keys) during the PAM authentication, for example
    ```
    AuthenticationMethods publickey,password
    ```
    for mandatory public key authentication and mandatory PAM
    authentication using password authentication or
    ```
    AuthenticationMethods publickey,password password
    ```
    for optional public key authentication and mandatory PAM
    authentication using password authentication
* PasswordAuthentication yes
* UsePAM yes

### Account Management, Session Management and Authentication Token Update

SSH authentication information is available to PAM **account**,
**session** and **password** modules beginning with OpenSSH version
7.6p1. However, these are undoubtedly less usefull module types than
the **auth** type described above.

In order to enable PAM account management, session management and
authentication token update, the following SSH server options must be
set in **/etc/ssh/sshd_config** or such (either explicitly or via
defaults):

* UsePAM yes

## PAM Configuration

In order to enable this module, an appropriate file in
the **/etc/pam.d/** directory (such as **/etc/pam.d/sshd**) must be
modified to contain a line in form of

    auth [auth_err=fail-action, default=error-action, ignore=ignore-action, success=success-action] pam_ssh_auth_info.so [option]... [pattern]...

For more information,
see
[pam_ssh_auth_info(8)](https://github.Eero.Häkkinen.fi/pam-ssh-auth-info/),
[pam.conf(5)](https://manpages.debian.org/pam.conf.5) and
[sshd_config(5)](https://manpages.debian.org/sshd_config.5).
