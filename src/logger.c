/**
 *	This file is part of devilspie2
 *	Copyright (C) 2024 Darren Salt
 *
 *	devilspie2 is free software: you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License as published
 *	by the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	devilspie2 is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with devilspie2.
 *	If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

#include <glib.h>

#include "intl.h"
#include "logger.h"
#include "script.h" // for devilspie2_debug
#include <lauxlib.h>

#ifdef EWOULDBLOCK
# define ERRNO_IS_EAGAIN (errno == EAGAIN || errno == EWOULDBLOCK)
#else
# define ERRNO_IS_EAGAIN (errno == EAGAIN)
#endif

static int fifo_read = -1, fifo_write = -1;
static char *fifo_name = NULL;

typedef struct logbuffer_t {
	struct logbuffer_t *next;
	ssize_t length;
	char *text, *write;
} logbuffer_t;

static logbuffer_t *logbuffer = NULL, *logbuffer_latest = NULL;


static void logger_drop(void)
{
	logbuffer_t *old = logbuffer;
	if (!old)
		return;
	if (logbuffer_latest == old)
		logbuffer_latest = NULL;
	logbuffer = old->next;
	free(old);
}

static inline void logger_drop_all(void)
{
	while (logbuffer)
		logger_drop();
}


#define LOGGER_DONE 0
#define LOGGER_MORE 1
#define LOGGER_FULL 2

static int logger_write(void)
{
	if (!logbuffer)
		return LOGGER_DONE;

	ssize_t bytes = write(fifo_write, logbuffer->write, logbuffer->length);

	if (bytes < 0) {
		if (errno == EPIPE) {
			// nobody listening
			logger_drop_all();
			return LOGGER_DONE;
		} else if (ERRNO_IS_EAGAIN) {
			return LOGGER_FULL;
		} else {
			// something unhandled
			perror("logger write backlog");
			logger_drop_all();
			return LOGGER_DONE;
		}
	}

	logbuffer->write += bytes;
	logbuffer->length -= bytes;
	if (logbuffer->length > 0)
		return LOGGER_MORE;

	logger_drop();
	return logbuffer ? LOGGER_MORE: LOGGER_DONE;
}


/**
 *
 */
static int logger_lua_print(lua_State *lua)
{
	int top = lua_gettop(lua);
	for (int i = 1; i <= top; ++i) {
		ssize_t length;
		const char *str;
#if LUA_VERSION_NUM < 503
		lua_pushvalue(lua, i);
		str = lua_tolstring(lua, 1, (size_t *)&length);
#else
		str = luaL_tolstring(lua, i, (size_t *)&length);
#endif
		if (length < 0) // if true, something's broken; expect a crash
			length = SSIZE_MAX;
		if (i > 1)
			logger_print_always("\t");
		// we don't handle NULs well, so just drop them
		while (length > 0) {
			logger_print_always(str);
			ssize_t l = strlen(str) + 1;
			length -= l;
			str += l;
		}
		lua_pop(lua, 1);
	}
	logger_print_always("\n");
	return 0;
}


/**
 *
 */
char *logger_get_fifo_name(void)
{
	char *name = getenv("XDG_RUNTIME_DIR");
	if (!name) {
		fputs(_("logger: XDG_RUNTIME_DIR is not set; falling back on TMPDIR.\n"), stderr);
		name = getenv("TMPDIR");
	}
	if (!name) {
		fputs(_("logger: TMPDIR is not set; falling back on /tmp.\n"), stderr);
		name = "/tmp";
	}

	return g_strdup_printf("%s/devilspie2-%s", name, getenv("DISPLAY")); // X11
}


/**
 * returns 0 on success, -1 otherwise
 */
int logger_create(lua_State *lua)
{
	fifo_name = getenv("XDG_RUNTIME_DIR");
	if (!fifo_name) {
		puts(_("logger: XDG_RUNTIME_DIR is not set; falling back on TMPDIR."));
		fifo_name = getenv("TMPDIR");
	}
	if (!fifo_name) {
		puts(_("logger: TMPDIR is not set; falling back on /tmp."));
		fifo_name = "/tmp";
	}

	fifo_name = logger_get_fifo_name();


	if (unlink(fifo_name) < 0 && errno != ENOENT) {
		perror("logger unlink");
		return -1;
	}

	if (mkfifo(fifo_name, 0640) < 0) {
		perror("logger mkfifo");
		return -1;
	}

	fifo_read = open(fifo_name, O_RDONLY | O_NONBLOCK);
	if (fifo_read < 0)
	{
		perror("logger open (r)");
		logger_shutdown();
		return -1;
	}

	fifo_write = open(fifo_name, O_WRONLY | O_NONBLOCK);
	if (-1 == fifo_write)
	{
		perror ("logger open (w)");
		logger_shutdown();
		return -1;
	}

	close(fifo_read);
	fifo_read = -1;

	static const struct luaL_Reg print[] = {
		{ "print", logger_lua_print },
		{ NULL, NULL }
	};
	lua_getglobal(lua, "_G");
#if LUA_VERSION_NUM < 502
	luaL_register(lua, NULL, print); // Lua 5.1 and older
#else
	luaL_setfuncs(lua, print, 0); // Lua 5.2 and newer
#endif
	lua_pop(lua, 1);

	atexit(logger_shutdown);
	printf(_("logger initialised; FIFO is at %s\n"), fifo_name);

	return 0;
}


static gboolean logger_flush(void);

static gboolean logger_send(logbuffer_t *text, gboolean always_print)
{
	if (text) {
		// first, enqueue
		logbuffer_latest = text;
		if (!logbuffer)
			logbuffer = text;

		// write to stdout (if debug option given)
		if (devilspie2_debug || always_print) {
			ssize_t bytes = write(STDOUT_FILENO, text->text, text->length);
			if (bytes < 0)
				perror ("logger stdout");
		}
	}
	if (fifo_write < 0)
		return LOGGER_DONE;

	// write to log if there's a reader
	int ret;
	while ((ret = logger_write()) == LOGGER_MORE)
		/**/;
	if (ret == LOGGER_FULL)
		g_timeout_add(1, (GSourceFunc) logger_flush, NULL);
	return ret == LOGGER_MORE;
}

static gboolean logger_flush(void)
{
	return logger_send(NULL, FALSE) == LOGGER_MORE;
}


/**
 *
 */
void logger_printf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	logbuffer_t *next = malloc(sizeof(logbuffer_t));
	next->text = next->write = g_strdup_vprintf(format, ap);
	next->length = strlen(next->text);
	next->next = NULL;

	logger_send(next, FALSE);
}


/**
 *
 */
void logger_print(const char *text)
{
	logbuffer_t *next = malloc(sizeof(logbuffer_t));
	next->text = next->write = g_strdup(text);
	next->length = strlen(next->text);
	next->next = NULL;

	logger_send(next, FALSE);
}


/**
 *
 */
void logger_print_always(const char *text)
{
	logbuffer_t *next = malloc(sizeof(logbuffer_t));
	next->text = next->write = g_strdup(text);
	next->length = strlen(next->text);
	next->next = NULL;

	logger_send(next, TRUE);
}


/**
 *
 */
void logger_err_printf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	logbuffer_t *next = malloc(sizeof(logbuffer_t));
	next->text = next->write = g_strdup_vprintf(format, ap);
	next->length = strlen(next->text);
	next->next = NULL;

	logger_send(next, TRUE);
}


/**
 *
 */
void logger_err_print(const char *text)
{
	logbuffer_t *next = malloc(sizeof(logbuffer_t));
	next->text = next->write = g_strdup(text);
	next->length = strlen(next->text);
	next->next = NULL;

	logger_send(next, TRUE);
}


/**
 *
 */
void logger_shutdown(void)
{
	// send what we can
	logger_flush();

	if (fifo_read >= 0 && close(fifo_read))
		perror("logger close (r)");
	fifo_read = -1;

	if (fifo_write >= 0 && close(fifo_write))
		perror("logger close (w)");
	fifo_write = -1;

	if (fifo_name && unlink(fifo_name) < 0)
		perror("logger unlink");
	g_free(fifo_name);
	fifo_name = NULL;
}
