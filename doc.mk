MV	= mv -f
RM	= rm -f
SED	= sed

all: all-html
all-html: pam_ssh_auth_info.html

clean: clean-html
clean-html:
	$(RM) -- *.html

%.html: %.8
	man -Thtml './$*.8' > '$@~'
	$(SED) -i -- '/^<!DOCTYPE/,$$!d' '$@~'
	$(MV) -- '$@~' '$@'
