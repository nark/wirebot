# Makefile.in

abs_top_srcdir	= /Users/nark/Development/Me/Cocoa/Wired/wired2/wirebot
datarootdir		= ${prefix}/share
exec_prefix		= ${prefix}
objdir			= obj
rundir			= run
bindir			= ${exec_prefix}/bin
docdir			= ${datarootdir}/doc/${PACKAGE_TARNAME}
mandir			= ${datarootdir}/man
prefix			= /usr/local

WR_VERSION		= 2.0
WR_MAINTAINER	= 0

DISTFILES		= INSTALL LICENSE NEWS README Makefile Makefile.in \
				  config.guess config.status config.h.in config.sub configure \
				  configure.in install-sh libwired man run wirebot
SUBDIRS			= libwired

WIREOBJECTS		= $(addprefix $(objdir)/wirebot/,$(notdir $(patsubst %.c,%.o,$(shell find $(abs_top_srcdir)/wirebot -name "[a-z]*.c"))))

DEFS			= -DHAVE_CONFIG_H
CC				= gcc
CFLAGS			= -g -O2
CPPFLAGS		= -I/usr/local/include -DWI_PTHREADS -DWI_CORESERVICES -DWI_CARBON -DWI_DIGESTS -DWI_CIPHERS -DWI_RSA -I/usr/include/libxml2 -DWI_LIBXML2 -DWI_PLIST -DWI_ZLIB -DWI_P7 -DWI_ICONV -DWI_TERMCAP -DWI_READLINE
LDFLAGS			= -L$(rundir)/libwired/lib -L/usr/local/lib
LIBS			= -lwired -framework CoreServices -framework Carbon -lcrypto -lxml2 -lz -liconv -ltermcap -lreadline -lcurl
INCLUDES		= -I$(abs_top_srcdir) -I$(rundir)/libwired/include

INSTALL			= /usr/bin/install -c
COMPILE			= $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
PREPROCESS		= $(CC) -E $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
DEPEND			= $(CC) -MM $(INCLUDES)
LINK			= $(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
ARCHIVE			= ar rcs $@

.PHONY: all all-recursive clean-recursive distclean-recursive install install-only install-wirebot install-man dist clean distclean scmclean
.NOTPARALLEL:

all: all-recursive $(rundir)/wirebot

ifeq ($(WR_MAINTAINER), 1)
all: Makefile configure config.h.in

Makefile: Makefile.in config.status
	./config.status
	            
configure: configure.in
	autoconf

config.h.in: configure.in
	autoheader
	touch $@
	rm -f $@~
endif

all-recursive clean-recursive distclean-recursive:
	@list='$(SUBDIRS)'; \
	for subdir in $$list; do \
		target=`echo $@ | sed s/-recursive//`; \
		(cd $$subdir && $(MAKE) -e $$target) || exit 1; \
	done

$(rundir)/wirebot: $(abs_top_srcdir)/wirebot/wired.xml.h $(WIREOBJECTS) $(rundir)/libwired/lib/libwired.a
	@test -d $(@D) || mkdir -p $(@D)
	$(LINK) $(WIREOBJECTS) $(LIBS)

$(objdir)/wirebot/%.o: $(abs_top_srcdir)/wirebot/%.c
	@test -d $(@D) || mkdir -p $(@D)
	$(COMPILE) -I$(<D) -c $< -o $@

$(objdir)/wirebot/%.d: $(abs_top_srcdir)/wirebot/%.c
	@test -d $(@D) || mkdir -p $(@D)
	($(DEPEND) $< | sed 's,$*.o,$(@D)/&,g'; echo "$@: $<") > $@

$(abs_top_srcdir)/wirebot/wired.xml.h: $(rundir)/wired.xml
	sed -e 's/\"/\\\"/g' -e 's/^/\"/g' -e 's/$$/\"/g' $< > $@

install: all install-man install-wirebot

install-only: install-man install-wirebot

install-wirebot:
	$(INSTALL) -m 755 -d $(bindir)
	$(INSTALL) -m 755 -d $(docdir)/wirebot
	$(INSTALL) -m 755 run/wirebot $(bindir)
	$(INSTALL) -m 755 wirebot.xml $(docdir)/wirebot/wirebot.xml

	@echo ""
	@echo "Installation complete!"
	@echo ""
	@echo "wirebot has been installed as $(bindir)/wirebot."
	@echo ""
	@echo "Manual pages have been installed into $(mandir)."
	@echo ""
	@echo "Default dictionnary stored in $(docdir)."

install-man:
	$(INSTALL) -m 755 -d $(mandir)/man1/
	$(INSTALL) -m 644 man/wirebot.1 $(mandir)/man1/

dist:
	rm -rf wirebot-$(WR_VERSION)
	rm -f wirebot-$(WR_VERSION).tar.gz
	mkdir wirebot-$(WR_VERSION)

	@for i in $(DISTFILES); do \
		if [ -e $$i ]; then \
			echo cp -LRp $$i wirebot-$(WR_VERSION)/$$i; \
			cp -LRp $$i wirebot-$(WR_VERSION)/$$i; \
		fi \
	done

	$(SHELL) -ec "cd wirebot-$(WR_VERSION) && WR_MAINTAINER=0 WI_MAINTAINER=0 $(MAKE) -e distclean scmclean"

	tar -czf wirebot-$(WR_VERSION).tar.gz wirebot-$(WR_VERSION)
	rm -rf wirebot-$(WR_VERSION)

clean: clean-recursive
	rm -f $(objdir)/wirebot/*.o
	rm -f $(objdir)/*.d
	rm -f $(rundir)/wirebot

distclean: clean distclean-recursive
	rm -rf $(objdir)
	rm -f Makefile config.h config.log config.status
	rm -f wirebot-$(WR_VERSION).tar.gz

scmclean:
	find . -name .DS_Store -print0 | xargs -0 rm -f
	find . -name CVS -print0 | xargs -0 rm -rf
	find . -name .svn -print0 | xargs -0 rm -rf

ifeq ($(WR_MAINTAINER), 1)
-include $(WIREOBJECTS:.o=.d)
endif
