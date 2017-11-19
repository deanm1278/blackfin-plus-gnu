/***************************************************************************
 *   Copyright (C) 2010 Øyvind Harboe                                      *
 *   oyvind.harboe@zylin.com                                               *
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tcl_server.h"
#include <target/target.h>

#define TCL_SERVER_VERSION		"TCL Server 0.1"
#define TCL_MAX_LINE			(4096)

struct tcl_connection {
	int tc_linedrop;
	int tc_lineoffset;
	char tc_line[TCL_MAX_LINE];
	int tc_outerror;/* flag an output error */
	enum target_state tc_laststate;
	bool tc_notify;
};

static char *tcl_port;

/* handlers */
static int tcl_new_connection(struct connection *connection);
static int tcl_input(struct connection *connection);
static int tcl_output(struct connection *connection, const void *buf, ssize_t len);
static int tcl_closed(struct connection *connection);

static int tcl_target_callback_event_handler(struct target *target,
		enum target_event event, void *priv)
{
	struct connection *connection = priv;
	struct tcl_connection *tclc;
	char buf[256];

	tclc = connection->priv;

	if (tclc->tc_notify) {
		snprintf(buf, sizeof(buf), "type target_event event %s\r\n\x1a", target_event_name(event));
		tcl_output(connection, buf, strlen(buf));
	}

	if (tclc->tc_laststate != target->state) {
		tclc->tc_laststate = target->state;
		if (tclc->tc_notify) {
			snprintf(buf, sizeof(buf), "type target_state state %s\r\n\x1a", target_state_name(target));
			tcl_output(connection, buf, strlen(buf));
		}
	}

	return ERROR_OK;
}

static int tcl_target_callback_reset_handler(struct target *target,
		enum target_reset_mode reset_mode, void *priv)
{
	struct connection *connection = priv;
	struct tcl_connection *tclc;
	char buf[256];

	tclc = connection->priv;

	if (tclc->tc_notify) {
		snprintf(buf, sizeof(buf), "type target_reset mode %s\r\n\x1a", target_reset_mode_name(reset_mode));
		tcl_output(connection, buf, strlen(buf));
	}

	return ERROR_OK;
}

/* write data out to a socket.
 *
 * this is a blocking write, so the return value must equal the length, if
 * that is not the case then flag the connection with an output error.
 */
int tcl_output(struct connection *connection, const void *data, ssize_t len)
{
	ssize_t wlen;
	struct tcl_connection *tclc;

	tclc = connection->priv;
	if (tclc->tc_outerror)
		return ERROR_SERVER_REMOTE_CLOSED;

	wlen = connection_write(connection, data, len);

	if (wlen == len)
		return ERROR_OK;

	LOG_ERROR("error during write: %d != %d", (int)wlen, (int)len);
	tclc->tc_outerror = 1;
	return ERROR_SERVER_REMOTE_CLOSED;
}

/* connections */
static int tcl_new_connection(struct connection *connection)
{
	struct tcl_connection *tclc;

	tclc = malloc(sizeof(struct tcl_connection));
	if (tclc == NULL)
		return ERROR_CONNECTION_REJECTED;

	memset(tclc, 0, sizeof(struct tcl_connection));
	connection->priv = tclc;

	struct target *target = get_target_by_num(connection->cmd_ctx->current_target);
	if (target != NULL)
		tclc->tc_laststate = target->state;

	/* store the connection object on cmd_ctx so we can access it from command handlers */
	connection->cmd_ctx->output_handler_priv = connection;

	target_register_event_callback(tcl_target_callback_event_handler, connection);
	target_register_reset_callback(tcl_target_callback_reset_handler, connection);

	return ERROR_OK;
}

static int tcl_input(struct connection *connection)
{
	Jim_Interp *interp = (Jim_Interp *)connection->cmd_ctx->interp;
	int retval;
	int i;
	ssize_t rlen;
	const char *result;
	int reslen;
	struct tcl_connection *tclc;
	unsigned char in[256];

	rlen = connection_read(connection, &in, sizeof(in));
	if (rlen <= 0) {
		if (rlen < 0)
			LOG_ERROR("error during read: %s", strerror(errno));
		return ERROR_SERVER_REMOTE_CLOSED;
	}

	tclc = connection->priv;
	if (tclc == NULL)
		return ERROR_CONNECTION_REJECTED;

	/* push as much data into the line as possible */
	for (i = 0; i < rlen; i++) {
		/* buffer the data */
		tclc->tc_line[tclc->tc_lineoffset] = in[i];
		if (tclc->tc_lineoffset < TCL_MAX_LINE)
			tclc->tc_lineoffset++;
		else
			tclc->tc_linedrop = 1;

		/* ctrl-z is end of command. When testing from telnet, just
		 * press ctrl-z a couple of times first to put telnet into the
		 * mode where it will send 0x1a in response to pressing ctrl-z
		 */
		if (in[i] != '\x1a')
			continue;

		/* process the line */
		if (tclc->tc_linedrop) {
#define ESTR "line too long\n"
			retval = tcl_output(connection, ESTR, sizeof(ESTR));
			if (retval != ERROR_OK)
				return retval;
#undef ESTR
		} else {
			tclc->tc_line[tclc->tc_lineoffset-1] = '\0';
			retval = command_run_line(connection->cmd_ctx, tclc->tc_line);
			result = Jim_GetString(Jim_GetResult(interp), &reslen);
			retval = tcl_output(connection, result, reslen);
			if (retval != ERROR_OK)
				return retval;
			/* Always output ctrl-d as end of line to allow multiline results */
			tcl_output(connection, "\x1a", 1);
		}

		tclc->tc_lineoffset = 0;
		tclc->tc_linedrop = 0;
	}

	return ERROR_OK;
}

static int tcl_closed(struct connection *connection)
{
	/* cleanup connection context */
	if (connection->priv) {
		free(connection->priv);
		connection->priv = NULL;
	}

	target_unregister_event_callback(tcl_target_callback_event_handler, connection);
	target_unregister_reset_callback(tcl_target_callback_reset_handler, connection);

	return ERROR_OK;
}

int tcl_init(void)
{
	if (strcmp(tcl_port, "disabled") == 0) {
		LOG_INFO("tcl server disabled");
		return ERROR_OK;
	}

	return add_service("tcl", tcl_port, 1,
		&tcl_new_connection, &tcl_input,
		&tcl_closed, NULL);
}

COMMAND_HANDLER(handle_tcl_port_command)
{
	return CALL_COMMAND_HANDLER(server_pipe_command, &tcl_port);
}

COMMAND_HANDLER(handle_tcl_notifications_command)
{
	struct connection *connection = NULL;
	struct tcl_connection *tclc = NULL;

	if (CMD_CTX->output_handler_priv != NULL)
		connection = CMD_CTX->output_handler_priv;

	if (connection != NULL && !strcmp(connection->service->name, "tcl")) {
		tclc = connection->priv;
		return CALL_COMMAND_HANDLER(handle_command_parse_bool, &tclc->tc_notify, "Target Notification output is");
	} else {
		LOG_ERROR("%s: can only be called from the tcl server", CMD_NAME);
		return ERROR_COMMAND_SYNTAX_ERROR;
	}
}

static const struct command_registration tcl_command_handlers[] = {
	{
		.name = "tcl_port",
		.handler = handle_tcl_port_command,
		.mode = COMMAND_ANY,
		.help = "Specify port on which to listen "
			"for incoming Tcl syntax.  "
			"Read help on 'gdb_port'.",
		.usage = "[port_num]",
	},
	{
		.name = "tcl_notifications",
		.handler = handle_tcl_notifications_command,
		.mode = COMMAND_EXEC,
		.help = "Target Notification output",
		.usage = "[on|off]",
	},
	COMMAND_REGISTRATION_DONE
};

int tcl_register_commands(struct command_context *cmd_ctx)
{
	tcl_port = strdup("6666");
	return register_commands(cmd_ctx, NULL, tcl_command_handlers);
}
