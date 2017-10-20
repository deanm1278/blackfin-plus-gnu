/*
 * File: helpers.c
 *
 * Copyright 2006-2012 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * some common utility functions
 */

#include "headers.h"
#include "helpers.h"

static void poison_it(void *ptr, size_t size)
{
#ifndef NDEBUG
	memset(ptr, 0xFB, size);
#endif
}

void *xmalloc(size_t size)
{
	void *ret = malloc(size);
	if (ret == NULL)
		errp("malloc(%zu) returned NULL!", size);
	poison_it(ret, size);
	return ret;
}

void *xzalloc(size_t size)
{
	void *ret = xmalloc(size);
	memset(ret, 0x00, size);
	return ret;
}

void *xrealloc(void *ptr, size_t size)
{
	void *ret = realloc(ptr, size);
	if (ret == NULL)
		errp("realloc(%p, %zu) returned NULL!", ptr, size);
	if (ptr == NULL)
		poison_it(ret, size);
	return ret;
}

char *xstrdup(const char *s)
{
	char *ret = strdup(s);
	if (ret == NULL)
		errp("strdup(%p) returned NULL!", s);
	return ret;
}

bool parse_bool(const char *boo)
{
	if (strcmp(boo, "1") == 0 || strcasecmp(boo, "yes") == 0 ||
	    strcasecmp(boo, "y") == 0 || strcasecmp(boo, "true") == 0)
		return true;
	if (strcmp(boo, "0") == 0 || strcasecmp(boo, "no") == 0 ||
	    strcasecmp(boo, "n") == 0 || strcasecmp(boo, "false") == 0)
		return false;
	err("Invalid boolean: '%s'", boo);
}

ssize_t read_retry(int fd, void *buf, size_t count)
{
	ssize_t ret = 0, temp_ret;
	while (count > 0) {
		/*
		 * clear errno to avoid confusing output with higher layers and
		 * short reads (which aren't technically errors)
		 */
		errno = 0;
		temp_ret = read(fd, buf, count);
		if (temp_ret > 0) {
			ret += temp_ret;
			buf += temp_ret;
			count -= temp_ret;
		} else if (temp_ret == 0) {
			break;
		} else {
			if (errno == EINTR)
				continue;
			ret = -1;
			break;
		}
	}
	return ret;
}

size_t fread_retry(void *buf, size_t size, size_t nmemb, FILE *fp)
{
	size_t ret = 0, temp_ret;
	while (nmemb > 0) {
		temp_ret = fread(buf, size, nmemb, fp);
		if (temp_ret > 0) {
			ret += temp_ret;
			buf += (temp_ret * size);
			nmemb -= temp_ret;
		} else if (temp_ret == 0) {
			break;
		} else {
			if (errno == EINTR)
				continue;
			ret = 0;
			break;
		}
	}
	return ret;
}

#ifdef HAVE_TERMIOS_H

/*
 * tty_speed_to_baud() / tty_baud_to_speed()
 * Annoying function for translating the termios baud representation
 * into the actual decimal value and vice versa.
 */
static const struct {
	speed_t s;
	size_t b;
} speeds[] = {
	{B0, 0}, {B50, 50}, {B75, 75}, {B110, 110}, {B134, 134}, {B150, 150},
	{B200, 200}, {B300, 300}, {B600, 600}, {B1200, 1200}, {B1800, 1800},
	{B2400, 2400}, {B4800, 4800}, {B9600, 9600}, {B19200, 19200},
	{B38400, 38400}, {B57600, 57600}, {B115200, 115200}, {B230400, 230400}
};
__attribute__((const))
static inline size_t tty_speed_to_baud(const speed_t speed)
{
	size_t i;
	for (i = 0; i < ARRAY_SIZE(speeds); ++i)
		if (speeds[i].s == speed)
			return speeds[i].b;
	return 0;
}
__attribute__((const))
static inline size_t tty_baud_to_speed(const size_t baud)
{
	size_t i;
	for (i = 0; i < ARRAY_SIZE(speeds); ++i)
		if (speeds[i].b == baud)
			return speeds[i].s;
	return 0;
}

/*
 * tty_get_baud()
 * Helper function to return the baud rate the specified fd is running at.
 */
size_t tty_get_baud(const int fd)
{
	struct termios term;
	tcgetattr(fd, &term);
	return tty_speed_to_baud(cfgetispeed(&term));
}

int tty_open(const char *filename, int flags)
{
	/*
	 * We need to open in non-blocking mode because if the serial
	 * device has CTS/RTS enabled, but the cable or device doesn't
	 * actually support it, we can easily hang.  The kernel itself
	 * waits for carrier detection before returning to userspace.
	 * Once we've applied our own settings in tty_init(), we'll
	 * undo the non-blocking mode of the fd.
	 */
	return open(filename, flags | O_BINARY | O_NONBLOCK);
}

/*
 * tty_init()
 * Make sure the tty we're going to be working with is properly setup.
 *  - make sure we are not running in ICANON mode
 *  - set speed to 115200 so transfers go fast
 */
bool tty_init(const int fd, const size_t baud, const bool ctsrts)
{
	const speed_t speed = tty_baud_to_speed(baud);
	struct termios term;
	if (speed == B0) {
		errno = EINVAL;
		return false;
	}
	if (verbose)
		printf("[getattr] ");
	if (tcgetattr(fd, &term))
		return false;
	term.c_iflag = IGNBRK | IGNPAR;
	term.c_oflag = 0;
	term.c_cflag = CS8 | CREAD | CLOCAL | (ctsrts ? CRTSCTS : 0);
	term.c_lflag = 0;
	if (verbose)
		printf("[setattr] ");
	if (tcsetattr(fd, TCSANOW, &term))
		return false;
	if (verbose)
		printf("[speed:%zu] ", baud);
	if (cfgetispeed(&term) != speed || cfgetospeed(&term) != speed) {
		if (cfsetispeed(&term, speed) || cfsetospeed(&term, speed))
			return false;
		if (tcsetattr(fd, TCSANOW, &term))
			return false;
	}

	/* See comment in tty_open() */
	int flags = fcntl(fd, F_GETFL);
	if (flags == -1)
		return false;
	fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

	return true;
}

/*
 * _tty_get_lock_dir()
 * Return the directory for lock files.
 */
static const char *_tty_get_lock_dir(void)
{
	return LOCALSTATEDIR "/lock";
}

/*
 * _tty_check_lock_dir()
 * Make sure the lock dir is usable.
 */
static bool _tty_check_lock_dir(void)
{
	const char *lockdir = _tty_get_lock_dir();
	if (access(lockdir, R_OK)) {
		if (verbose)
			warn("tty lock dir '%s' is not usable", lockdir);
		return false;
	}
	return true;
}

/*
 * _tty_get_lock_name()
 * Transform the specified tty path to its lock name.
 * Note: not reentrant by any means.
 */
static const char *_tty_get_lock_name(const char *tty)
{
	const char *base_tty;
	static char lockfile[1024];
	base_tty = strrchr(tty, '/');
	if (base_tty == NULL)
		base_tty = tty;
	else
		++base_tty;
	snprintf(lockfile, sizeof(lockfile), "%s/LCK..%s", _tty_get_lock_dir(), base_tty);
	return lockfile;
}

/*
 * tty_lock()
 * Try to lock the specified tty.  Return value indicates
 * whether locking was successful.
 */
bool tty_lock(const char *tty)
{
	int fd, mask;
	bool ret = false;
	FILE *fp;
	const char *lockfile;

	/* if the lock dir is not available, just lie and say
	 * the locking worked out for us.
	 */
	if (!_tty_check_lock_dir())
		return true;

	/* see if the lock is stale */
	lockfile = _tty_get_lock_name(tty);
	fp = fopen(lockfile, "rb");
	if (fp != NULL) {
		unsigned long pid;
		if (fscanf(fp, "%lu", &pid) == 1) {
			if (kill(pid, 0) == -1 && errno == ESRCH) {
				if (!quiet)
					printf("Removing stale lock '%s'\n", lockfile);
				unlink(lockfile);
			} else if (verbose)
				printf("TTY '%s' is locked by pid '%lu'\n", tty, pid);
		}
		fclose(fp);
	}

	/* now create a new lock and write our pid into it */
	mask = umask(022);
	fd = open(lockfile, O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0666);
	umask(mask);
	if (fd != -1) {
		fp = fdopen(fd, "wb");
		if (fp != NULL) {
			fprintf(fp, "%lu\n", (unsigned long)getpid());
			fclose(fp);
			ret = true;
		} else
			/* the fclose() above also calls close() on the underlying fd */
			close(fd);
	}

	return ret;
}

/*
 * tty_unlock()
 * Unlock the specified tty.
 * TODO: maybe make sure this lock belongs to us ?
 */
bool tty_unlock(const char *tty)
{
	/* if the lock dir is not available, just lie and say
	 * the locking worked out for us.
	 */
	if (!_tty_check_lock_dir())
		return true;

	const char *lockfile = _tty_get_lock_name(tty);
	return (unlink(lockfile) == 0 ? true : false);
}

/*
 * tty_stdin_init()
 */
static struct termios stdin_orig_attrs;
static void tty_stdin_restore(void)
{
	tcsetattr(0, TCSANOW, &stdin_orig_attrs);
}
void tty_stdin_init(void)
{
	struct termios stdin_attrs;
	tcgetattr(0, &stdin_orig_attrs);
	atexit(tty_stdin_restore);
	stdin_attrs = stdin_orig_attrs;
	stdin_attrs.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(0, TCSADRAIN, &stdin_attrs);
}

#endif /* HAVE_TERMIOS_H */

#ifdef HAVE_BACKTRACE
# include <execinfo.h>
void error_backtrace(void)
{
	void *funcs[10];
	int num_funcs;
	if (getenv("LDR_UTILS_TESTING"))
		return;
	puts("\nBacktrace:");
	num_funcs = backtrace(funcs, sizeof(funcs));
	backtrace_symbols_fd(funcs, num_funcs, 1);
}
void error_backtrace_maybe(void)
{
	if (debug) error_backtrace();
}
#endif
