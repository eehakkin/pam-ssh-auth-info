<link href="index.css" rel="stylesheet" type="text/css" />
<h1 align="center">pam_ssh_auth_info</h1>

<hr>


<h2>NAME
</h2>



<p style="margin-left:11%; margin-top: 1em">pam_ssh_auth_info
- test SSH authentication information</p>

<h2>SYNOPSIS
</h2>



<p style="margin-left:11%; margin-top: 1em"><b>pam_ssh_auth_info.so</b>
[<i>flag</i>...] [<i>pattern</i>[=<i>pattern</i>]...]</p>

<h2>DESCRIPTION
</h2>


<p style="margin-left:11%; margin-top: 1em">The
pam_ssh_auth_info.so PAM module is designed to succeed or
fail authentication based on SSH authentication information
consisting of a list of successfully completed
authentication methods and public credentials (e.g. keys)
used to authenticate the user. One use is to select whether
to load other modules based on this test.</p>

<p style="margin-left:11%; margin-top: 1em">Normally, the
module should be given zero or more patterns as module
arguments, and authentication will succeed if SSH
authentication information matches all of the patterns (or
any of or none of the patterns depending on the options). If
there are no previous successfully completed authentication
methods, SSH authentication information is not available but
missing and authentication will neither succeed nor fail
(the module will return <b>PAM_IGNORE</b>).</p>

<h2>OPTIONS
</h2>


<p style="margin-left:11%; margin-top: 1em">The following
<i>flag</i>s are supported:</p>

<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="9%">


<p><b>all_of</b></p></td>
<td width="2%"></td>
<td width="78%">


<p>All of the <i>pattern</i>s must match. If zero
<i>pattern</i>s are given as module arguments,
authentication will succeed if SSH authentication
information is available. This is the default.</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="9%">


<p><b>any_of</b></p></td>
<td width="2%"></td>
<td width="78%">


<p>At least one of the <i>pattern</i>s must match. If zero
<i>pattern</i>s are given as module arguments,
authentication will fail if SSH authentication information
is available.</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="9%">


<p><b>debug</b></p></td>
<td width="2%"></td>
<td width="78%">


<p>Log debugging messages to syslog.</p></td></tr>
</table>


<p style="margin-left:11%;"><b>disable</b>=<i>service</i>[:<i>service</i>[...]]</p>

<p style="margin-left:22%;">Disable pattern matching for
the services listed in the colon separator service list.</p>


<p style="margin-left:11%;"><b>enable</b>=<i>service</i>[:<i>service</i>[...]]</p>

<p style="margin-left:22%;">Enable pattern matching only
for the services listed in the colon separator service
list.</p>

<p style="margin-left:11%;"><b>none_of</b></p>

<p style="margin-left:22%;">None of the <i>pattern</i>s may
match. If zero <i>pattern</i>s are given as module
arguments, authentication will succeed if SSH authentication
information is available.</p>

<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="7%">


<p><b>quiet</b></p></td>
<td width="4%"></td>
<td width="75%">


<p>Do not log failure or success messages to syslog.</p></td>
<td width="3%">
</td></tr>
</table>

<p style="margin-left:11%;"><b>quiet_fail</b></p>

<p style="margin-left:22%;">Do not log failure messages to
syslog.</p>

<p style="margin-left:11%;"><b>quiet_success</b></p>

<p style="margin-left:22%;">Do not log success messages to
syslog.</p>


<p style="margin-left:11%;"><b>recursion_limit</b>=<i>limit</i></p>

<p style="margin-left:22%;">Change the recursion limit.
This affects extended patterns and <b>*</b> wildcard
patterns.</p>


<p style="margin-left:11%; margin-top: 1em"><b>PATTERNS</b>
<br>
Any character byte that appears in a pattern, other than the
extended patterns and the special pattern characters
described below, matches itself.</p>

<p style="margin-left:11%; margin-top: 1em">The special
pattern characters have the following meanings:</p>

<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p><b>*</b></p></td>
<td width="10%"></td>
<td width="78%">


<p>Matches any number of (including zero) word character
bytes but not a word separator (space).</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p><b>=</b></p></td>
<td width="10%"></td>
<td width="78%">


<p>Matches equal-sign (<b>=</b>) or a word separator
(space).</p> </td></tr>
</table>

<p style="margin-left:22%; margin-top: 1em">This enables
more natural looking patterns and allows patterns to be
written without spaces avoiding the need for square bracket
quoting in PAM configuration files.</p>

<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p style="margin-top: 1em"><b>?</b></p></td>
<td width="10%"></td>
<td width="78%">


<p style="margin-top: 1em">Matches any word character byte
but not a word separator (space).</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p><b>[</b></p></td>
<td width="10%"></td>
<td width="78%">


<p>A pair of brackets introduces a character byte class. A
character byte class (<b>[</b>...<b>]</b>) matches any word
character byte in the class. A complemented character byte
class (<b>[!</b>...<b>]</b>) matches any word character byte
not in the class. Neither matches a word separator (space).
A class may contain character bytes (<b>[abcde]</b>) and
character byte ranges (<b>[a-e]</b>).</p></td></tr>
<tr valign="top" align="left">
<td width="11%"></td>
<td width="1%">


<p><b>\</b></p></td>
<td width="10%"></td>
<td width="78%">


<p>Preserves the literal meaning of the following character
byte.</p> </td></tr>
</table>

<p style="margin-left:11%; margin-top: 1em">The extended
patterns have the following meanings: <b><br>

?(</b><i>pattern</i><b>|</b><i>pattern</i><b>|</b>...<b>)</b></p>

<p style="margin-left:22%;">Matches zero or one occurence
of the given patterns. Does not match a word separator
(space).</p>


<p style="margin-left:11%;"><b>*(</b><i>pattern</i><b>|</b><i>pattern</i><b>|</b>...<b>)</b></p>

<p style="margin-left:22%;">Matches zero or more occurences
of the given patterns. Does not match a word separator
(space).</p>


<p style="margin-left:11%;"><b>+(</b><i>pattern</i><b>|</b><i>pattern</i><b>|</b>...<b>)</b></p>

<p style="margin-left:22%;">Matches one or more occurences
of the given patterns. Does not match a word separator
(space).</p>


<p style="margin-left:11%;"><b>@(</b><i>pattern</i><b>|</b><i>pattern</i><b>|</b>...<b>)</b></p>

<p style="margin-left:22%;">Matches one of the given
patterns. Does not match a word separator (space).</p>


<p style="margin-left:11%;"><b>!(</b><i>pattern</i><b>|</b><i>pattern</i><b>|</b>...<b>)</b></p>

<p style="margin-left:22%;">Matches anything except one
occurence of the given patterns. Does not match a word
separator (space).</p>

<p style="margin-left:11%; margin-top: 1em">SSH
authentication information consists of lines having a format
like</p>


<p style="margin-left:22%; margin-top: 1em"><i>method</i>[/<i>submethod</i>]
[<i>key-type key-data</i>] [<i>info-word</i>]...</p>

<p style="margin-left:11%; margin-top: 1em">SSH
authentication information matches a pattern if on any such
line all the words or the initial words match the
pattern.</p>

<h2>MODULE SERVICES PROVIDED
</h2>


<p style="margin-left:11%; margin-top: 1em">All module
types (<b>account</b>, <b>auth</b>, <b>password</b> and
<b>session</b>) are provided. That said, the <b>auth</b>
type is still undoubtedly the most useful one.</p>

<h2>RETURN VALUES
</h2>



<p style="margin-left:11%; margin-top: 1em"><b>PAM_AUTH_ERR</b></p>

<p style="margin-left:22%;">Pattern requirements are not
met. SSH authentication information does not match all of or
any of the patterns (see the <b>all_of</b> and the
<b>any_of</b> options) or matches some of the patterns (see
the <b>none_of</b> option).</p>

<p style="margin-left:11%;"><b>PAM_IGNORE</b></p>

<p style="margin-left:22%;">The pattern matching is
disabled for the service (see the <b>disable</b> option) or
not enabled for the service (see the <b>enable</b> option)
or SSH authentication information is missing.</p>

<p style="margin-left:11%;"><b>PAM_SUCCESS</b></p>

<p style="margin-left:22%;">Pattern requirements are met.
SSH authentication information matches all of or any of the
patterns (see the <b>all_of</b> and the <b>any_of</b>
options) or matches none of the patterns (see the
<b>none_of</b> option).</p>

<h2>EXAMPLES
</h2>


<p style="margin-left:11%; margin-top: 1em">Always load an
OATH module rule (to request and check a one-time password)
but load a password module rule (to request and check the
user password) only if there are no previous successfully
completed authentications (in which case SSH authentication
information is missing):</p>

<p style="margin-left:22%; margin-top: 1em">auth requisite
oath-module.so option... <br>
auth [success=1 \ <br>
&nbsp; default=ignore] pam_ssh_auth_info.so <br>
auth required password-module.so try_first_pass
option...</p>

<p style="margin-left:11%; margin-top: 1em">Load an OATH
module rule (to request and check a one-time password) only
if there are no previous successfully completed FIDO
authenticator algorithm based public key authentications (in
which case SSH authentication information either is missing
or does not match the pattern) but always load a password
module rule (to request and check the user password):</p>

<p style="margin-left:22%; margin-top: 1em">auth [success=1
\ <br>
&nbsp; ignore=ignore \ <br>
&nbsp; auth_err=ignore \ <br>
&nbsp; default=die] pam_ssh_auth_info.so quiet \ <br>
&nbsp; &nbsp; &nbsp; publickey=*sk-*@openssh.com <br>
auth requisite oath-module.so option... <br>
auth required password-module.so try_first_pass
option...</p>

<p style="margin-left:11%; margin-top: 1em">Require that
there is at least one previous successfully completed FIDO
authenticator algorithm based public key authentication (in
which case the key type contains &ldquo;sk-&rdquo; and ends
with &ldquo;@openssh.com&rdquo;). If multiple public key
authentications are required, only one of them is required
to be a FIDO authenticator algorithm based one.</p>

<p style="margin-left:22%; margin-top: 1em">auth requisite
pam_ssh_auth_info.so quiet \ <br>
&nbsp; &nbsp; &nbsp; publickey=*sk-*@openssh.com</p>

<p style="margin-left:11%; margin-top: 1em">Require that
there is at least one previous successfully completed FIDO
authenticator algorithm based public key authentication (in
which case the key type contains &ldquo;sk-&rdquo; and ends
with &ldquo;@openssh.com&rdquo;) and at least one previous
successfully completed non-FIDO public key authentication
(in which case the key type does not contain
&ldquo;sk-&rdquo; or does not end with
&ldquo;@openssh.com&rdquo;).</p>

<p style="margin-left:22%; margin-top: 1em">auth requisite
pam_ssh_auth_info.so quiet \ <br>
&nbsp; &nbsp; &nbsp; publickey=*sk-*@openssh.com \ <br>
&nbsp; &nbsp; &nbsp; publickey=!(*sk-*@openssh.com)</p>

<h2>ENVIRONMENT
</h2>



<p style="margin-left:11%; margin-top: 1em"><b>SSH_AUTH_INFO_0</b></p>

<p style="margin-left:22%;">SSH authentication information
consisting of a list of successfully completed
authentication methods and public credentials (e.g. keys)
used to authenticate the user. This environment variable is
visible to PAM modules but not to user sessions and is
provided by OpenSSH server since version 7.8p1.</p>

<p style="margin-left:11%;"><b>SSH_USER_AUTH</b></p>

<p style="margin-left:22%;">A location of a file containing
SSH authentication information consisting of a list of
successfully completed authentication methods and public
credentials (e.g. keys) used to authenticate the user. This
environment variable is visible to user sessions but not to
PAM modules and is provided by OpenSSH server since version
7.8p1 if the <b>ExposeAuthInfo</b> server option is enabled.
This may be a useful source for creating proper
patterns.</p>

<h2>NOTES
</h2>


<p style="margin-left:11%; margin-top: 1em">In order to
make this module useful, the following <b>sshd_config</b>(5)
options should be set (either explicitly or via defaults):
<b><br>
AuthenticationMethods</b></p>

<p style="margin-left:22%;">Should contain comma-separated
lists of authentication method names with
<b>keyboard-interactive</b>, <b>keyboard-interactive:pam</b>
or <b>password</b> as the last list items so that there are
successfully completed authentication methods and public
credentials (e.g. keys) during the PAM authentication. For
example</p>

<p style="margin-left:32%;"><b>AuthenticationMethods
publickey,keyboard-interactive</b></p>

<p style="margin-left:22%;">for mandatory public key
authentication and mandatory PAM authentication using
keyboard-interactive challenge-response authentication
or</p>

<p style="margin-left:32%;"><b>AuthenticationMethods
publickey,keyboard-interactive keyboard-interactive</b></p>

<p style="margin-left:22%;">for optional public key
authentication and mandatory PAM authentication using
keyboard-interactive challenge-response authentication.</p>


<p style="margin-left:11%;"><b>ChallengeResponseAuthentication</b>,
<b>KbdInteractiveAuthentication</b>, <b><br>
PasswordAuthentication</b>, etc.</p>

<p style="margin-left:22%;">The relevant ones should be set
to <b>yes</b>.</p>

<table width="100%" border="0" rules="none" frame="void"
       cellspacing="0" cellpadding="0">
<tr valign="top" align="left">
<td width="11%"></td>
<td width="9%">


<p><b>UsePAM</b></p></td>
<td width="2%"></td>
<td width="32%">


<p>Should be set to <b>yes</b>.</p></td>
<td width="46%">
</td></tr>
</table>

<h2>SEE ALSO
</h2>


<p style="margin-left:11%; margin-top: 1em"><b>pam</b>(7),
<b>sshd_config</b>(5) <br>

<a href="https://github.Eero.xn--Hkkinen-5wa.fi/pam-ssh-auth-info/">Home
Page for pam_ssh_auth_info</a></p>

<h2>AUTHOR
</h2>


<p style="margin-left:11%; margin-top: 1em">Eero
H&auml;kkinen
&lt;Eero+pam-ssh-auth-info@H&auml;kkinen.fi&gt;</p>

<h2>COPYRIGHT
</h2>


<p style="margin-left:11%; margin-top: 1em">Copyright
&copy; 2021 - 2022 Eero H&auml;kkinen
&lt;Eero+pam-ssh-auth-info@H&auml;kkinen.fi&gt;</p>

<p style="margin-left:11%; margin-top: 1em">This manual
page is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3
of the License, or (at your option) any later version.</p>
<hr>
