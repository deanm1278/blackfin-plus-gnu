SUBDIRS = gnulib/lib tests

EXTRA_DIST = HACKING gnulib/m4/gnulib-cache.m4

ACLOCAL_AMFLAGS = -I gnulib/m4
AM_CPPFLAGS = -DLOCALSTATEDIR=\"$(localstatedir)\"

bin_PROGRAMS = ldr
if USE_RC_SOURCES
RC_SOURCES = icon.rc
else
RC_SOURCES =
endif
ldr_SOURCES = helpers.c ldr.c ldr_elf.c lfd.c lfd_bf506.c lfd_bf518.c lfd_bf527.c lfd_bf533.c lfd_bf537.c lfd_bf548.c lfd_bf561.c lfd_bf592.c lfd_bf609.c lfd_bf706.c termios-mingw.c arg-nonnull.h blackfin_defines.h c++defs.h dxe_jump.h dxes.h headers.h helpers.h ldr.h ldr_elf.h lfd.h link-warning.h sdp.h warn-on-use.h $(RC_SOURCES)
ldr_LDADD = $(LIB_PTHREAD)

.rc.o:
	$(AM_V_GEN)$(WINDRES) -I $(srcdir) $< -o $@
%.o : %.rc
	$(AM_V_GEN)$(WINDRES) -I $(srcdir) $< -o $@
