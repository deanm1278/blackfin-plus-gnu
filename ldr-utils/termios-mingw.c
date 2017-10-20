/*
 * File: termios-mingw.c
 *
 * Copyright 2006-2011 Analog Devices Inc.
 * Licensed under the GPL-2, see the file COPYING in this dir
 *
 * Description:
 * Small layer to translate termios functions to Windows calls
 */

#include "headers.h"
#include "helpers.h"

#ifdef WIN32

#define FD_TO_HANDLE(fd) ((HANDLE)_get_osfhandle(fd))
#define BOOL_TO_bool(B)  (B == FALSE ? false : true)
#define bool_TO_BOOL(b)  (b == false ? FALSE : TRUE)

int tty_open(const char *filename, int flags)
{
	int ret = -1;
	HANDLE h;

	h = CreateFile(filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
	               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) {
		/* some extended device names require UNC convention ... auto-prepend
		 * this magic so users dont need to go insane.
		 */
		if (strncmp(filename, "\\\\.\\", 4)) {
			char *unc = xmalloc(strlen(filename) + 1 + 4);
			sprintf(unc, "\\\\.\\%s", filename);
			ret = tty_open(unc, flags);
			free(unc);
		}
	} else
		ret = _open_osfhandle((long)h, flags);

	return ret;
}

bool tty_init(const int fd, const size_t baud, const bool ctsrts)
{
	COMMTIMEOUTS timeouts;
	DCB state = { .DCBlength = sizeof(state) };
	if (GetCommState(FD_TO_HANDLE(fd), &state) == FALSE)
		return false;
	state.BaudRate = baud;
	state.fBinary = TRUE;
	state.fParity = FALSE;
	state.fOutxCtsFlow = bool_TO_BOOL(ctsrts);
	state.fOutxDsrFlow = FALSE;
	state.fDtrControl = DTR_CONTROL_ENABLE;
	state.fDsrSensitivity = FALSE;
	state.fOutX = FALSE;
	state.fInX = FALSE;
	state.fNull = FALSE;
	state.fRtsControl = ctsrts ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_ENABLE;
	state.fAbortOnError = FALSE;
	state.ByteSize = 8;
	state.Parity = NOPARITY;
	state.StopBits = ONESTOPBIT;
	if (GetCommTimeouts (FD_TO_HANDLE(fd), &timeouts) == FALSE) {
		printf("Failed to Get Comm Timeouts Reason: %d", GetLastError());
		return false;
	}
	timeouts.ReadIntervalTimeout = 2000;
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.ReadTotalTimeoutConstant = 10;
	timeouts.WriteTotalTimeoutMultiplier = 10;
	timeouts.WriteTotalTimeoutConstant = 2000;
	if (SetCommTimeouts(FD_TO_HANDLE(fd), &timeouts) == FALSE) {
		printf("Error setting time-outs. %d", GetLastError());
		return false;
	}
	return BOOL_TO_bool(SetCommState(FD_TO_HANDLE(fd), &state));
}

size_t tty_get_baud(const int fd)
{
	DCB state = { .DCBlength = sizeof(state) };
	if (GetCommState(FD_TO_HANDLE(fd), &state) == FALSE)
		return 0;
	return state.BaudRate;
}

int tcdrain(int fd)
{
	return (FlushFileBuffers(FD_TO_HANDLE(fd)) == FALSE) ? -1 : 0;
}

bool tty_lock(const char *tty)
{
	return true;
}

bool tty_unlock(const char *tty)
{
	return true;
}

void tty_stdin_init(void)
{
}

#endif
