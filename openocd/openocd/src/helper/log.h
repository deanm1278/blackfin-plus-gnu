/***************************************************************************
 *   Copyright (C) 2005 by Dominic Rath                                    *
 *   Dominic.Rath@gmx.de                                                   *
 *                                                                         *
 *   Copyright (C) 2007,2008 Øyvind Harboe                                 *
 *   oyvind.harboe@zylin.com                                               *
 *                                                                         *
 *   Copyright (C) 2008 by Spencer Oliver                                  *
 *   spen@spen-soft.co.uk                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.           *
 ***************************************************************************/

#ifndef ERROR_H
#define ERROR_H

#include <helper/command.h>

#ifdef IS_MINGW
  #define PRIzu "Iu"
  #define PRIzd "Id"
  #define PRIzx "Ix"
  #define PRIju "I64u"
  #define PRIjd "I64d"
  #define PRIllu "I64u"
  #define PRIlld "I64d"
#else
  #define PRIzu "zu"
  #define PRIzd "zd"
  #define PRIzx "zx"
  #define PRIju "ju"
  #define PRIjd "jd"
  #define PRIllu "llu"
  #define PRIlld "lld"
#endif

/* logging priorities
 * LOG_LVL_SILENT - turn off all output. In lieu of try + catch this can be used as a
 *                  feeble ersatz.
 * LOG_LVL_USER - user messages. Could be anything from information
 *                to progress messags. These messages do not represent
 *                incorrect or unexpected behaviour, just normal execution.
 * LOG_LVL_ERROR - fatal errors, that are likely to cause program abort
 * LOG_LVL_WARNING - non-fatal errors, that may be resolved later
 * LOG_LVL_INFO - state information, etc.
 * LOG_LVL_DEBUG - debug statements, execution trace
 */
enum log_levels {
	LOG_LVL_SILENT = -3,
	LOG_LVL_OUTPUT = -2,
	LOG_LVL_USER = -1,
	LOG_LVL_ERROR = 0,
	LOG_LVL_WARNING = 1,
	LOG_LVL_INFO = 2,
	LOG_LVL_DEBUG = 3
};

void log_printf(enum log_levels level, const char *file, unsigned line,
		const char *function, const char *format, ...)
__attribute__ ((format (PRINTF_ATTRIBUTE_FORMAT, 5, 6)));
void log_printf_lf(enum log_levels level, const char *file, unsigned line,
		const char *function, const char *format, ...)
__attribute__ ((format (PRINTF_ATTRIBUTE_FORMAT, 5, 6)));

/**
 * Initialize logging module.  Call during program startup.
 */
void log_init(void);

int log_register_commands(struct command_context *cmd_ctx);

void keep_alive(void);
void kept_alive(void);

void alive_sleep(uint64_t ms);
void busy_sleep(uint64_t ms);

typedef void (*log_callback_fn)(void *priv, const char *file, unsigned line,
		const char *function, const char *string);

struct log_callback {
	log_callback_fn fn;
	void *priv;
	struct log_callback *next;
};

int log_add_callback(log_callback_fn fn, void *priv);
int log_remove_callback(log_callback_fn fn, void *priv);

char *alloc_vprintf(const char *fmt, va_list ap);
char *alloc_printf(const char *fmt, ...);

extern int debug_level;

/* Avoid fn call and building parameter list if we're not outputting the information.
 * Matters on feeble CPUs for DEBUG/INFO statements that are involved frequently */

#define LOG_LEVEL_IS(FOO)  ((debug_level) >= (FOO))

#define LOG_DEBUG(expr ...) \
	do { \
		if (debug_level >= LOG_LVL_DEBUG) \
			log_printf_lf(LOG_LVL_DEBUG, \
				__FILE__, __LINE__, __func__, \
				expr); \
	} while (0)

#define LOG_INFO(expr ...) \
	log_printf_lf(LOG_LVL_INFO, __FILE__, __LINE__, __func__, expr)

#define LOG_WARNING(expr ...) \
	log_printf_lf(LOG_LVL_WARNING, __FILE__, __LINE__, __func__, expr)

#define LOG_ERROR(expr ...) \
	log_printf_lf(LOG_LVL_ERROR, __FILE__, __LINE__, __func__, expr)

#define LOG_USER(expr ...) \
	log_printf_lf(LOG_LVL_USER, __FILE__, __LINE__, __func__, expr)

#define LOG_USER_N(expr ...) \
	log_printf(LOG_LVL_USER, __FILE__, __LINE__, __func__, expr)

#define LOG_OUTPUT(expr ...) \
	log_printf(LOG_LVL_OUTPUT, __FILE__, __LINE__, __func__, expr)

/* general failures
 * error codes < 100
 */
#define ERROR_OK						(0)
#define ERROR_NO_CONFIG_FILE			(-2)
#define ERROR_BUF_TOO_SMALL				(-3)
/* see "Error:" log entry for meaningful message to the user. The caller should
 * make no assumptions about what went wrong and try to handle the problem.
 */
#define ERROR_FAIL						(-4)
#define ERROR_WAIT						(-5)


#endif	/* LOG_H */
