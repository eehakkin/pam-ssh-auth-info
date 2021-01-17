MV	= mv -f
RM	= rm -f
SED	= sed

all: all-html
all-html: pam_ssh_auth_info.html

all: all-md
all-md: index.md

clean: clean-html
clean-html:
	$(RM) -- *.html

clean: clean-md
clean-md:
	$(RM) -- index.md

index.md: pam_ssh_auth_info.html
	$(SED) \
		-e '1a\
	<link href="index.css" rel="stylesheet" type="text/css" />' \
		-e '\|<body>|N' \
		-e '1,\|<body>|d' \
		-e '\|</body>|,$$d' \
		-- pam_ssh_auth_info.html \
		> '$@~'
	$(MV) -- '$@~' '$@'

%.html: %.8
	man -Thtml './$*.8' > '$@~'
	$(SED) -i -- '/^<!DOCTYPE/,$$!d' '$@~'
	$(MV) -- '$@~' '$@'
