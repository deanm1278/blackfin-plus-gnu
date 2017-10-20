/*
 * File: helpers.h
 *
 * Copyright 2006-2010 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * some common utility functions
 */

#ifndef __HELPERS_H__
#define __HELPERS_H__

#ifndef VERSION
# define VERSION "live"
#endif

extern int force, verbose, quiet, debug;

extern const char *argv0;

#if defined(__GLIBC__) && !defined(__UCLIBC__) && !defined(NDEBUG)
# define HAVE_BACKTRACE
void error_backtrace(void);
void error_backtrace_maybe(void);
#else
# define error_backtrace()
# define error_backtrace_maybe()
#endif

#define warn(fmt, args...) \
	fprintf(stderr, "%s: " fmt "\n", argv0 , ## args)
#define warnf(fmt, args...) warn("%s(): " fmt, __func__ , ## args)
#define warnp(fmt, args...) warn(fmt ": %s" , ## args , strerror(errno))
#define _err(wfunc, fmt, args...) \
	do { \
		wfunc(fmt, ## args); \
		error_backtrace_maybe(); \
		exit(EXIT_FAILURE); \
	} while (0)
#define err(fmt, args...) _err(warn, fmt, ## args)
#define errf(fmt, args...) _err(warnf, fmt, ## args)
#define errp(fmt, args...) _err(warnp, fmt , ## args)

#define container_of(ptr, type, member) \
	({ \
		const typeof( ((type *)0)->member ) *__mptr = (ptr); \
		(type *)( (char *)__mptr - offsetof(type,member) ); \
	})
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

void *xmalloc(size_t);
void *xzalloc(size_t);
void *xrealloc(void *, size_t);
char *xstrdup(const char *);
bool parse_bool(const char *);
ssize_t read_retry(int, void *, size_t);
size_t fread_retry(void *, size_t, size_t, FILE *);

size_t tty_get_baud(const int);
int tty_open(const char *, int);
bool tty_init(const int, const size_t, const bool);
bool tty_lock(const char *);
bool tty_unlock(const char *);
void tty_stdin_init(void);

#ifndef HAVE_ALARM
# define alarm(seconds) 0
# define SIGALRM 0
#endif
#ifndef HAVE_FDATASYNC
# define fdatasync(fd) 0
#endif
#ifndef HAVE_USLEEP
# define usleep(usecs) 0
#endif

#ifndef HAVE_PTHREAD_H
typedef int pthread_t;
# define pthread_cancel(thread)
# define pthread_create(thread, attr, func, arg)
#endif

#endif
