#
#

DESTDIR =
VPATH = ../.././../bfin ../.././../bfin/..
srcdir = ../.././../bfin
objdir = .
srcroot = $(srcdir)/../..
objroot = $(objdir)/../..
top_srcdir = ../.././../bfin

prefix = /usr/local
exec_prefix = ${prefix}

host_alias = 
target_alias = bfin-elf

bindir = ${exec_prefix}/bin
libdir = ${exec_prefix}/lib
tooldir = $(exec_prefix)/$(target_alias)

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644

# Multilib support variables.
# TOP is used instead of MULTI{BUILD,SRC}TOP.
MULTIDIRS = 
MULTISUBDIR = /32

SHELL =	/bin/sh

mkinstalldirs = $(SHELL) $(top_srcdir)/../../mkinstalldirs

CC = gcc  -m32

AS = as
AR = ar
LD = ld
RANLIB = ranlib

OBJDUMP = `if [ -f ${objroot}/../binutils/objdump ] ; \
	then echo ${objroot}/../binutils/objdump ; \
	else t='$(program_transform_name)'; echo objdump | sed -e $$t ; fi`
OBJCOPY = `if [ -f ${objroot}/../binutils/objcopy ] ; \
	then echo ${objroot}/../binutils/objcopy ; \
	else t='$(program_transform_name)'; echo objcopy | sed -e $$t ; fi`

OBJS		=
CFLAGS		=
SCRIPTS		=

# Here is all of the simulator stuff
SIM_SCRIPTS	=
SIM_LDFLAGS	=
SIM_BSP		= libsim.a
SIM_CRT0	= crt0.o
SIM_OBJS	= syscalls.o clear_cache_range.o
SIM_TEST	= sim-test
SIM_INSTALL	= install-sim

# Here is all of the development board stuff
BOARD_SCRIPTS	= bfin-common-sc.ld  bfin-common-mc.ld bfin-common-mc0.ld \
	bf504.ld bf506.ld \
	bf512.ld bf514.ld bf516.ld bf518.ld \
	bf522.ld bf523.ld bf524.ld bf525.ld bf526.ld bf527.ld \
	bf531.ld bf532.ld bf533.ld \
	bf534.ld bf536.ld bf537.ld \
	bf538.ld bf539.ld \
	bf542.ld bf544.ld bf547.ld bf548.ld bf549.ld \
	bf561.ld bf561a.ld bf561b.ld bf561m.ld \
	bf606.ld bf606c0.ld bf606c1.ld bf606m.ld \
	bf607.ld bf607c0.ld bf607c1.ld bf607m.ld \
	bf608.ld bf608c0.ld bf608c1.ld bf608m.ld \
	bf609.ld bf609c0.ld bf609c1.ld bf609m.ld \
	bf592.ld \
	bf706.ld
BOARD_LDFLAGS	=
BOARD_BSP	= libbfinbsp.a
BOARD_CRT0S	= basiccrt.o basiccrts.o
BOARD_CRT0S	+= basiccrt561.o basiccrt561s.o basiccrt561b.o
# BOARD_CRT0S	+= basiccrt60x.o basiccrt60xs.o basiccrt60xc1.o
BOARD_OBJS	= clear_cache_range.o _exit.o
BOARD_TEST	=
BOARD_INSTALL	= install-board

# Host specific makefile fragment comes in here.
NEWLIB_CFLAGS = `if [ -d ${objroot}/newlib ]; then echo -I${objroot}/newlib/targ-include -I${srcroot}/newlib/libc/include; fi`
NEWLIB_LDFLAGS = `if [ -d ${objroot}/newlib ]; then echo -B${objroot}/newlib/ -L${objroot}/newlib/; fi`

INCLUDES = -I. -I$(srcdir)/..
# Note that when building the library, ${MULTILIB} is not the way multilib
# options are passed; they're passed in $(CFLAGS).
CFLAGS_FOR_TARGET = -O2 -g ${MULTILIB} ${INCLUDES} ${NEWLIB_CFLAGS}
LDFLAGS_FOR_TARGET = ${MULTILIB} ${NEWLIB_LDFLAGS}
AR_FLAGS = rc

.c.o:
	$(CC) $(CFLAGS_FOR_TARGET) -O2 $(INCLUDES) -c $(CFLAGS) $<

.C.o:
	$(CC) $(CFLAGS_FOR_TARGET) -O2 $(INCLUDES) -c $(CFLAGS) $<
.s.o:
	$(AS) $(ASFLAGS_FOR_TARGET) $(INCLUDES) $(ASFLAGS) -o $*.o $<

#
# GCC knows to run the preprocessor on .S files before it assembles them.
#
.S.o:
	$(CC) $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -c $<

#
# this is a bogus target that'll produce an assembler from the
# C source with the right compiler options. this is so we can
# track down code generation or debug symbol bugs.
#
.c.s:
	$(CC) $(CFLAGS_FOR_TARGET) -S $(INCLUDES) $(CFLAGS) $<

INCLUDES += -I$(srcdir)/include
#
# build a test program for each target board. Just trying to get
# it to link is a good test, so we ignore all the errors for now.
#
all: ${SIM_CRT0} ${SIM_BSP} ${BOARD_CRT0S} ${BOARD_BSP}

#
# here's where we build the board support packages for each target
#
${BOARD_BSP}: ${OBJS} ${BOARD_OBJS}
	${AR} ${ARFLAGS} $@ $^
	${RANLIB} $@

${SIM_BSP}: ${OBJS} ${SIM_OBJS}
	${AR} ${ARFLAGS} ${SIM_BSP} ${SIM_OBJS} ${OBJS}
	${RANLIB} ${SIM_BSP}

#
#
#
crt0.o: crt0.S

basiccrt.o: basiccrt.S
ifeq (,$(findstring mcpu=,$(CFLAGS)))
	$(CC) $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf532-any -o $@ -c $<
else
	$(CC) $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -o $@ -c $<
endif

ifeq (,$(findstring mcpu=bf706-none,$(CFLAGS)))

	basiccrts.o: basiccrt.S
	ifeq (,$(findstring mcpu=,$(CFLAGS)))
		$(CC) -D__BFIN_SDRAM $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf532-any -o $@ -c $<
	else
		$(CC) -D__BFIN_SDRAM $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -o $@ -c $<
	endif

	basiccrt561.o: basiccrt.S
	ifneq (,$(findstring mcpu=bf532-none,$(CFLAGS)))
		$(CC) $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf561-none -o $@ -c $<
	else
		$(CC) $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf561-any -o $@ -c $<
	endif

	basiccrt561s.o: basiccrt.S
	ifneq (,$(findstring mcpu=bf532-none,$(CFLAGS)))
		$(CC) -D__BFIN_SDRAM $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf561-none -o $@ -c $<
	else
		$(CC) -D__BFIN_SDRAM $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf561-any -o $@ -c $<
	endif

	basiccrt561b.o: basiccrt.S
	ifneq (,$(findstring mcpu=bf532-none,$(CFLAGS)))
		$(CC) -D__ADSPBF561_COREB__ $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf561-none -o $@ -c $<
	else
		$(CC) -D__ADSPBF561_COREB__ $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf561-any -o $@ -c $<
	endif

	basiccrt60x.o: basiccrt.S
	ifneq (,$(findstring mcpu=bf532-none,$(CFLAGS)))
		$(CC) $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf608-none -o $@ -c $<
	else
		$(CC) $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf608-any -o $@ -c $<
	endif

	basiccrt60xs.o: basiccrt.S
	ifneq (,$(findstring mcpu=bf532-none,$(CFLAGS)))
		$(CC) -D__BFIN_SDRAM $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf608-none -o $@ -c $<
	else
		$(CC) -D__BFIN_SDRAM $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf608-any -o $@ -c $<
	endif

	basiccrt60xc1.o: basiccrt.S
	ifneq (,$(findstring mcpu=bf532-none,$(CFLAGS)))
		$(CC) -D__ADSPBF60x_CORE1__ $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf608-none -o $@ -c $<
	else
		$(CC) -D__ADSPBF60x_CORE1__ $(CFLAGS_FOR_TARGET) $(INCLUDES) $(CFLAGS) -mcpu=bf608-any -o $@ -c $<
	endif

endif

clean mostlyclean:
	rm -f a.out core *.i *.o ${SIM_BSP} ${BOARD_BSP}

distclean maintainer-clean realclean: clean
	rm -f Makefile config.status *~

.PHONY: install info install-info clean-info
install: ${SIM_INSTALL} ${BOARD_INSTALL}

install-sim:
	for x in ${SIM_CRT0} ${SIM_BSP} ${SIM_SCRIPTS}; do \
	 ${INSTALL_DATA} $$x $(DESTDIR)${tooldir}/lib${MULTISUBDIR}/$$x || exit $$?; \
	done

install-board:
	for x in ${BOARD_CRT0S} ${BOARD_BSP}; do \
	 ${INSTALL_DATA} $$x $(DESTDIR)${tooldir}/lib${MULTISUBDIR}/$$x || exit $$?; \
	done
	-if [ -z "${MULTISUBDIR}" ]; then \
	  for x in ${BOARD_SCRIPTS}; do \
	   ${INSTALL_DATA} ${srcdir}/$$x $(DESTDIR)${tooldir}/lib/$$x; \
	  done; \
	  ${mkinstalldirs} ${DESTDIR}${tooldir}/include; \
	  for i in ${srcdir}/include/*.h; do \
	   ${INSTALL_DATA} $$i ${DESTDIR}${tooldir}/include/`basename $$i`; \
	  done; \
	  ${mkinstalldirs} ${DESTDIR}${tooldir}/include/sys; \
	  for i in ${srcdir}/include/sys/*.h; do \
	   ${INSTALL_DATA} $$i ${DESTDIR}${tooldir}/include/sys/`basename $$i`; \
	  done; \
	else true; fi

doc:
info:
install-info:
clean-info:

Makefile: Makefile.in config.status ../.././../bfin/../config/default.mh
	$(SHELL) config.status

config.status: configure
	$(SHELL) config.status --recheck
