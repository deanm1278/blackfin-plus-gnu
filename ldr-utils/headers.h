/*
 * File: headers.h
 *
 * Copyright 2006-2010 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * I'm too lazy to update headers in multiple files ...
 * just move all that crap here ;)
 */

#ifndef __HEADERS_H__
#define __HEADERS_H__

/* Some sane defaults for POSIX systems.  Everyone else can bite it. */
#ifndef HAVE_CONFIG_H
# define HAVE_ASSERT_H 1
# define HAVE_CTYPE_H 1
# define HAVE_ELF_H 1
# define HAVE_ERRNO_H 1
# define HAVE_FCNTL_H 1
# define HAVE_GETOPT_H 1
# define HAVE_INTTYPES_H 1
# define HAVE_LIBGEN_H 1
# define HAVE_NETDB_H 1
# define HAVE_PTHREAD_H 1
# define HAVE_SIGNAL_H 1
# define HAVE_STDBOOL_H 1
# define HAVE_STDINT_H 1
# define HAVE_STDIO_H 1
# define HAVE_STDLIB_H 1
# define HAVE_STRING_H 1
# define HAVE_STRINGS_H 1
# define HAVE_TERMIOS_H 1
# define HAVE_TIME_H 1
# define HAVE_UNISTD_H 1
# define HAVE_ARPA_INET_H 1
# define HAVE_NETINET_IN_H 1
# define HAVE_NETINET_TCP_H 1
# define HAVE_SYS_MMAN_H 1
# define HAVE_SYS_SOCKET_H 1
# define HAVE_SYS_STAT_H 1
# define HAVE_SYS_TYPES_H 1
# define HAVE_SYS_WAIT_H 1
# define HAVE_ALARM 1
# define HAVE_FDATASYNC 1
# define HAVE_FORK 1
# define HAVE_FSEEKO 1
# define HAVE_FTELLO 1
# define HAVE_GETADDRINFO 1
# define HAVE_MMAP 1
# define HAVE_USLEEP 1
# ifdef __linux__
#  define HAVE_ENDIAN_H 1
#  define HAVE_PTY_H 1
# endif
# define LOCALSTATEDIR "/var"
# define _UNUSED_PARAMETER_ __attribute__((__unused__))
# ifdef __LFD_INTERNAL
#  define HAVE_LIBUSB 1
# endif
#else
# include <config.h>
#endif

#ifdef __MINGW64__
# define _POSIX
# define __USE_MINGW_ALARM
#endif

#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif
#ifdef HAVE_CTYPE_H
# include <ctype.h>
#endif
#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_LOCAL_ELF_H
# include "elf.h"
#endif
#ifdef HAVE_ENDIAN_H
# include <endian.h>
#endif
#ifdef HAVE_ERRNO_H
# include <errno.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_GETOPT_H
# include <getopt.h>
#endif
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_IO_H
# include <io.h>
#endif
#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif
#ifdef HAVE_PTY_H
# include <pty.h>
#endif
#ifdef HAVE_SIGNAL_H
# include <signal.h>
#endif
#ifdef HAVE_STDBOOL_H
# include <stdbool.h>
#endif
#ifdef HAVE_STDINT_H
# include <stdint.h>
#endif
#ifdef HAVE_STDIO_H
# include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif
#ifdef HAVE_TERMIOS_H
# include <termios.h>
#endif
#ifdef HAVE_TIME_H
# include <time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_UTIL_H
# include <util.h>
#endif
#ifdef HAVE_WINSOCK2_H
# include <winsock2.h>
#endif
#ifdef HAVE_WS2TCPIP_H
# include <ws2tcpip.h>
#endif
#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_TCP_H
# include <netinet/tcp.h>
#endif
#ifdef HAVE_SYS_ENDIAN_H
# include <sys/endian.h>
#endif
#ifdef HAVE_SYS_MMAN_H
# include <sys/mman.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifdef HAVE_LIBUSB
# include <libusb.h>
#endif

#define _LDR_UTILS_BIG_ENDIAN 4321
#define _LDR_UTILS_LITTLE_ENDIAN 1234
#ifndef HAVE_CONFIG_H
# ifndef BYTE_ORDER
#  error unable to detect endian
# endif
# if BYTE_ORDER != BIG_ENDIAN && BYTE_ORDER != LITTLE_ENDIAN
#  error unknown endian
# endif
# if BYTE_ORDER == BIG_ENDIAN
#  define _LDR_UTILS_ENDIAN _LDR_UTILS_BIG_ENDIAN
# else
#  define _LDR_UTILS_ENDIAN _LDR_UTILS_LITTLE_ENDIAN
# endif
#else
# ifdef WORDS_BIGENDIAN
#  define _LDR_UTILS_ENDIAN _LDR_UTILS_BIG_ENDIAN
# else
#  define _LDR_UTILS_ENDIAN _LDR_UTILS_LITTLE_ENDIAN
# endif
#endif

#if !defined(bswap_16)
# if defined(bswap16)
#  define bswap_16 bswap16
#  define bswap_32 bswap32
# else
#  define bswap_16(x) \
			((((x) & 0xff00) >> 8) | \
			 (((x) & 0x00ff) << 8))
#  define bswap_32(x) \
			((((x) & 0xff000000) >> 24) | \
			 (((x) & 0x00ff0000) >>  8) | \
			 (((x) & 0x0000ff00) <<  8) | \
			 (((x) & 0x000000ff) << 24))
# endif
#endif

#if _LDR_UTILS_ENDIAN == _LDR_UTILS_BIG_ENDIAN
# define ldr_make_little_endian_16(x) ({ ((x) = bswap_16(x)); })
# define ldr_make_little_endian_32(x) ({ ((x) = bswap_32(x)); })
#elif _LDR_UTILS_ENDIAN == _LDR_UTILS_LITTLE_ENDIAN
# define ldr_make_little_endian_16(x) ({ (x); })
# define ldr_make_little_endian_32(x) ({ (x); })
#endif

#ifndef ELF_DATA
# if _LDR_UTILS_ENDIAN == _LDR_UTILS_BIG_ENDIAN
#  define ELF_DATA ELFDATA2MSB
# elif _LDR_UTILS_ENDIAN == _LDR_UTILS_LITTLE_ENDIAN
#  define ELF_DATA ELFDATA2LSB
# endif
#endif

#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

/* Crappy Windows compatibility for moronic behavior. */
#ifndef O_BINARY
# define O_BINARY 0
#endif

#endif
