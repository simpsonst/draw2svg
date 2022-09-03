all::

CAT=cat
FIND=find
XARGS=xargs
SED=sed
MKDIR=mkdir -p
CP=cp
CMP=cmp -s
HTML2TXT=lynx -dump
MARKDOWN=markdown


VWORDS:=$(shell src/getversion.sh --prefix=v MAJOR MINOR PATCH)
VERSION:=$(word 1,$(VWORDS))
BUILD:=$(word 2,$(VWORDS))

## Provide a version of $(abspath) that can cope with spaces in the
## current directory.
myblank:=
myspace:=$(myblank) $(myblank)
MYCURDIR:=$(subst $(myspace),\$(myspace),$(CURDIR)/)
MYABSPATH=$(foreach f,$1,$(if $(patsubst /%,,$f),$(MYCURDIR)$f,$f))

-include $(call MYABSPATH,config.mk)
-include draw2svg-env.mk

#VERSION:=$(shell $(CAT) docs/VERSION)

CPPFLAGS += -DVERSION='"$(file <VERSION)"'

#TEXTFILES += HISTORY
TEXTFILES += VERSION
TEXTFILES += COPYING

DRAWFILES += cbook
DRAWFILES += die
DRAWFILES += home
DRAWFILES += space
DRAWFILES += sptst1

binaries.c += draw2svg
draw2svg_obj += draw2svg
draw2svg_obj += files
draw2svg_obj += indent
draw2svg_obj += theconv
draw2svg_obj += units
draw2svg_obj += version


SOURCES:=$(filter-out $(headers),$(shell $(FIND) src/obj \( -name "*.c" -o -name "*.h" \) -printf '%P\n'))


riscos_zips += draw2svg
draw2svg_zrof += !ReadMe,fff

draw2svg_apps += boot
boot_appname=!Boot
boot_rof += $(call riscos_bin,$(binaries.c))

draw2svg_apps += source
source_appname=Draw2SVG
source_rof += Extras/Sprites,ff9
source_rof += Extras/draw2svg,bae
source_rof += $(TEXTFILES:%=Docs/%,fff)
source_rof += Tests/convert,feb
source_rof += $(DRAWFILES:%=Tests/DrawFiles/%,aff)
source_rof += $(call riscos_src,$(SOURCES))
source_rof += Tests/SVGFiles

include binodeps.mk

tmp/obj/version.o: VERSION

all:: BUILD VERSION installed-binaries riscos-zips

install:: install-binaries install-riscos

tidy::
	$(FIND) . -name "*~" -exec $(RM) {} \;

distclean: blank
	$(RM) VERSION BUILD

out/riscos/Draw2SVG/Tests/SVGFiles:
	$(MKDIR) '$@'

$(BINODEPS_OUTDIR)/riscos/Draw2SVG/Docs/VERSION,fff: VERSION
	$(MKDIR) "$(@D)"
	$(CP) "$<" "$@"

$(BINODEPS_OUTDIR)/riscos/!ReadMe,fff: tmp/README.html
	$(MKDIR) "$(@D)"
	$(HTML2TXT) "$<" > "$@"

$(BINODEPS_OUTDIR)/riscos/Draw2SVG/Docs/COPYING,fff: LICENSE.txt
	$(MKDIR) "$(@D)"
	$(CP) "$<" "$@"

tmp/README.html: README.md
	$(MKDIR) '$(@D)'
	$(MARKDOWN) '$<' > '$@'


MYCMPCP=$(CMP) -s '$1' '$2' || $(CP) '$1' '$2'
.PHONY: prepare-version
mktmp:
	@$(MKDIR) tmp/
prepare-version: mktmp
	$(file >tmp/BUILD,$(BUILD))
	$(file >tmp/VERSION,$(VERSION))
BUILD: prepare-version
	@$(call MYCMPCP,tmp/BUILD,$@)
VERSION: prepare-version
	@$(call MYCMPCP,tmp/VERSION,$@)

# Set this to the comma-separated list of years that should appear in
# the licence.  Do not use characters other than [0-9,] - no spaces.
YEARS=2000-1,2005-6,2012,2019

update-licence:
	$(FIND) . -name ".svn" -prune -or -type f -print0 | $(XARGS) -0 \
	$(SED) -i 's/Copyright (C) [0-9,-]\+  Steven Simpson/Copyright (C) $(YEARS)  Steven Simpson/g'
