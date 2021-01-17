MAN	= man
MV	= mv -f
RM	= rm -f
SED	= sed

all: all-html
all-html: pam_ssh_auth_info.html

all: all-md
all-md: index.md

# all: all-pdf
all-pdf: pam_ssh_auth_info.pdf

# all: all-ps
all-ps: pam_ssh_auth_info.ps

clean: clean-html
clean-html:
	$(RM) -- *.html

clean: clean-md
clean-md:
	$(RM) -- index.md

clean: clean-pdf
clean-pdf:
	$(RM) -- *.pdf

clean: clean-ps
clean-ps:
	$(RM) -- *.ps

clean: clean-tmp
clean-tmp:
	$(RM) -- *.tmp

index.md: pam_ssh_auth_info.html
	$(SED) \
		-e '1i<link href="index.css" rel="stylesheet" type="text/css" />' \
		-e '\|<body>|N' \
		-e '1,\|<body>|d' \
		-e '\|</body>|,$$d' \
		-- pam_ssh_auth_info.html \
		> '$@.tmp'
	$(MV) -- '$@.tmp' '$@'

%.html: %.8
	$(MAN) -Txhtml -- './$*.8' > '$@.tmp'
	$(SED) \
		-i \
		-e '\|^<html|i<!DOCTYPE html>' \
		-e '\|^<html|,$$!d' \
		-e '\|^<!-- CreationDate: .* -->$$|d' \
		-e '\|^<!-- Creator     : .* -->$$|d' \
		-e 's|></col>| />|g' \
		-- '$@.tmp'
	$(MV) -- '$@.tmp' '$@'

%.pdf: %.8
	$(MAN) -Tpdf -- './$*.8' > '$@.tmp'
	$(MV) -- '$@.tmp' '$@'

%.ps: %.8
	$(MAN) -Tps -- './$*.8' > '$@.tmp'
	$(MV) -- '$@.tmp' '$@'
