AC_PREREQ(2.63)
AC_INIT(ldr-utils, [svn-5853], [vapier@blackfin.uclinux.org])
AM_INIT_AUTOMAKE([1.10.1 dist-bzip2 dist-xz no-dist-gzip])
AC_CONFIG_HEADER(config.h)

dnl silent rules requires automake-1.11+
m4_ifdef([AM_SILENT_RULES],
	[AM_SILENT_RULES([yes])],
	[AC_SUBST([AM_DEFAULT_VERBOSITY], [1])])

AC_PROG_CC
gl_EARLY
gl_INIT
AM_MISSING_PROG([AUTOM4TE], [autom4te])
PKG_PROG_PKG_CONFIG

AC_USE_SYSTEM_EXTENSIONS
AC_C_BIGENDIAN

AC_CHECK_HEADERS_ONCE(m4_flatten([
	assert.h
	ctype.h
	endian.h
	errno.h
	fcntl.h
	getopt.h
	inttypes.h
	io.h
	libgen.h
	netdb.h
	pthread.h
	pty.h
	signal.h
	stdbool.h
	stdint.h
	stdio.h
	string.h
	strings.h
	termios.h
	time.h
	unistd.h
	util.h
	winsock2.h
	ws2tcpip.h
	arpa/inet.h
	netinet/in.h
	netinet/tcp.h
	sys/endian.h
	sys/mman.h
	sys/socket.h
	sys/stat.h
	sys/types.h
	sys/wait.h
]))

AC_CHECK_FUNCS_ONCE(m4_flatten([
	alarm
	fdatasync
	fork
	fseeko
	ftello
	getaddrinfo
	mmap
	usleep
]))

AC_CHECK_LIB([util], [openpty], [PTY_LIB="-lutil"], [PTY_LIB=""])
AC_SUBST([PTY_LIB])

dnl
dnl look for libusb for SDP/USB loading
dnl
PKG_CHECK_MODULES([LIBUSB], libusb-1.0, [
	AS_VAR_APPEND([CFLAGS], [" $LIBUSB_CFLAGS"])
	AS_VAR_APPEND([LIBS], [" $LIBUSB_LIBS"])
	AC_DEFINE([HAVE_LIBUSB], 1, [Whether we support libusb])
],[
	:
])

dnl
dnl for non-ELF systems, steal elf.h from local uClibc
dnl
AC_CHECK_HEADER([elf.h], [
	AC_DEFINE([HAVE_ELF_H], [1], [Define to 1 if you have a <elf.h> header file.])
],[
	if test -f "$srcdir/../uClibc/include/elf.h" ; then
		dnl the DECLS stuff does not matter as all of our code is C
		sed \
			-e "/features.h/d" \
			-e "/endian.h/d" \
			-e "/__BEGIN_DECLS/d" \
			-e "/__END_DECLS/d" \
			"$srcdir/../uClibc/include/elf.h" > elf.h
	else
		AC_MSG_ERROR([unable to locate a usable elf.h])
	fi
	AC_DEFINE([HAVE_LOCAL_ELF_H], [1], [Define to 1 if you have a local "elf.h" header file.])
])

AC_CONFIG_TESTDIR([tests])

if test "$GCC" = yes; then
	CFLAGS="$CFLAGS -Wall"
fi
#if test "$USER" = vapier; then
#	CFLAGS="$CFLAGS -Werror"
#fi

dnl
dnl complain to the user if their setup sucks
dnl
fail_at_life=no

if test "x$ac_cv_header_termios_h" = "xno" ; then
	fail_at_life=yes
AC_MSG_WARN([

Your host system does not provide termios.h as required by POSIX.
Support for loading LDRs over the UART is disabled.
])
fi

case $host_os in
	*darwin*)
	LIBS="${LIBS} -lobjc"
	;;
esac

if test "x$ac_cv_header_pthread_h" = "xno" ; then
	fail_at_life=yes
AC_MSG_WARN([

Your host system does not provide POSIX threads support.
])
	LIB_PTHREAD=""
else
	LIB_PTHREAD="-lpthread"
fi
AC_SUBST(LIB_PTHREAD)

if test "x$ac_cv_func_mmap" = "xno" ; then
	fail_at_life=yes
AC_MSG_WARN([

Your host system does not provide mmap() as required by POSIX.
This function will be emulated by a big fat read().  I hope you
have bunches of free memory.
])
fi

if test "x$fail_at_life" = "xyes" ; then
AC_MSG_WARN([

Please purchase a Linux machine.  Seriously.  Wal-Mart sells them
for \$200.  You can afford one.
])
	CPPFLAGS="$CPPFLAGS "'-I$(top_builddir)/gnulib/lib -I$(top_srcdir)/gnulib/lib'
	LIBS="$LIBS "'$(top_builddir)/gnulib/lib/libgnu.a'
fi

USE_RC_SOURCES="false"
case $host_os in
	mingw*)
	AC_CHECK_TOOL([WINDRES], [windres], :)
	USE_RC_SOURCES="true"
	;;
esac
AM_CONDITIONAL(USE_RC_SOURCES, $USE_RC_SOURCES)

AC_CONFIG_FILES([
	Makefile
	gnulib/lib/Makefile
	tests/atlocal
	tests/Makefile
	tests/package.m4
])

AC_OUTPUT
