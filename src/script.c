/**
 *	This file is part of devilspie2
 *	Copyright (C) 2011-2019 Andreas Rönnquist
 *	Copyright (C) 2019-2021 Darren Salt
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


#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <glib.h>

#include <gdk/gdk.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <locale.h>

#include "compat.h"
#include "intl.h"
#include "script.h"

#if (GTK_MAJOR_VERSION >= 3)
#define HAVE_GTK3
#endif

#include "script_functions.h"



/**
 *
 */
gboolean devilspie2_debug = FALSE;
gboolean devilspie2_emulate = FALSE;

lua_State *global_lua_state = NULL;

/**
 *
 */
lua_State *
init_script()
{
	lua_State *lua = luaL_newstate();
	luaL_openlibs(lua);

	register_cfunctions(lua);

	return lua;
}


/**
 *
 */
#define DP2_REGISTER(lua, name) lua_register(lua, #name, c_##name)
void
register_cfunctions(lua_State *lua)
{
	DP2_REGISTER(lua, use_utf8);

	DP2_REGISTER(lua, get_window_name);
	DP2_REGISTER(lua, get_window_has_name);

	DP2_REGISTER(lua, set_window_position);
	DP2_REGISTER(lua, set_window_position2);
	DP2_REGISTER(lua, set_window_size);
	DP2_REGISTER(lua, set_window_strut);

	DP2_REGISTER(lua, set_window_geometry);
	DP2_REGISTER(lua, set_window_geometry2);

	DP2_REGISTER(lua, get_application_name);

	DP2_REGISTER(lua, debug_print);

	DP2_REGISTER(lua, shade);
	DP2_REGISTER(lua, unshade);

	DP2_REGISTER(lua, maximize);
	lua_register(lua, "maximise", c_maximize);
	DP2_REGISTER(lua, maximize_horisontally); // deprecated
	DP2_REGISTER(lua, maximize_horizontally);
	lua_register(lua, "maximise_horizontally", c_maximize_horizontally);
	DP2_REGISTER(lua, maximize_vertically);
	lua_register(lua, "maximise_vertically", c_maximize_vertically);
	DP2_REGISTER(lua, unmaximize);
	lua_register(lua, "unmaximise", c_unmaximize);

	DP2_REGISTER(lua, minimize);
	lua_register(lua, "minimise", c_minimize);
	DP2_REGISTER(lua, unminimize);
	lua_register(lua, "unminimise", c_unminimize);

	DP2_REGISTER(lua, decorate_window);
	DP2_REGISTER(lua, undecorate_window);

	DP2_REGISTER(lua, set_window_workspace);
	DP2_REGISTER(lua, change_workspace);
	DP2_REGISTER(lua, get_workspace_count);
	DP2_REGISTER(lua, get_workspaces);

	DP2_REGISTER(lua, pin_window);
	DP2_REGISTER(lua, unpin_window);
	DP2_REGISTER(lua, stick_window);
	DP2_REGISTER(lua, unstick_window);

	DP2_REGISTER(lua, close_window);

	DP2_REGISTER(lua, set_adjust_for_decoration);

	DP2_REGISTER(lua, get_window_geometry);
	DP2_REGISTER(lua, get_window_client_geometry);
	DP2_REGISTER(lua, get_window_frame_extents);

	DP2_REGISTER(lua, set_skip_tasklist);
	DP2_REGISTER(lua, set_skip_pager);

	DP2_REGISTER(lua, get_window_is_maximized);
	lua_register(lua, "get_window_is_maximised", c_get_window_is_maximized);

	DP2_REGISTER(lua, get_window_is_maximized_vertically);
	lua_register(lua, "get_window_is_maximised_vertically", c_get_window_is_maximized_vertically);

	lua_register(lua, "get_window_is_maximized_horisontally", // deprecated
	             c_get_window_is_maximized_horisontally);
	DP2_REGISTER(lua, get_window_is_maximized_horizontally);
	lua_register(lua, "get_window_is_maximised_horizontally",
	             c_get_window_is_maximized_horizontally);
	DP2_REGISTER(lua, get_window_is_pinned);

	DP2_REGISTER(lua, get_window_is_decorated);

	DP2_REGISTER(lua, set_window_below);
	DP2_REGISTER(lua, set_window_above);
	DP2_REGISTER(lua, set_window_fullscreen);

	DP2_REGISTER(lua, make_always_on_top);
	DP2_REGISTER(lua, set_on_top);
	DP2_REGISTER(lua, set_on_bottom);

	DP2_REGISTER(lua, get_window_type);

	DP2_REGISTER(lua, get_window_property);
	DP2_REGISTER(lua, window_property_is_utf8);
	DP2_REGISTER(lua, get_window_property_full);
	DP2_REGISTER(lua, get_window_role);
	DP2_REGISTER(lua, get_window_xid);

	DP2_REGISTER(lua, get_window_class);

	DP2_REGISTER(lua, set_window_property);
	DP2_REGISTER(lua, delete_window_property);

	DP2_REGISTER(lua, set_viewport);

	DP2_REGISTER(lua, center);
	lua_register(lua, "centre", c_center);

	DP2_REGISTER(lua, set_window_opacity);
	lua_register(lua, "set_opacity", c_set_window_opacity);

	DP2_REGISTER(lua, set_window_type);

	DP2_REGISTER(lua, get_screen_geometry);

	DP2_REGISTER(lua, get_window_fullscreen);
	lua_register(lua, "get_fullscreen", c_get_window_fullscreen);

	DP2_REGISTER(lua, get_window_strut);

	// wnck_window_get_class_{instance,group}_name are only availible on wnck 3 and later
	DP2_REGISTER(lua, get_class_instance_name);
	DP2_REGISTER(lua, get_class_group_name);

	DP2_REGISTER(lua, focus);
	lua_register(lua, "focus_window", c_focus);

	DP2_REGISTER(lua, get_monitor_index);
	DP2_REGISTER(lua, get_monitor_geometry);

	DP2_REGISTER(lua, xy);
	DP2_REGISTER(lua, xywh);

	DP2_REGISTER(lua, on_geometry_changed);

	DP2_REGISTER(lua, get_process_name);

	DP2_REGISTER(lua, millisleep);
}


/**
 *
 */
static ATTR_MALLOC gchar *error_add_backtrace(lua_State *lua, const char *msg)
{
	int r, level = 0, lines = 0;
	lua_Debug state;
	const gchar *header = _("Backtrace:\n");
	gchar *backtrace = (gchar*)msg;

	while ((r = lua_getstack(lua, level, &state)) != 0)
	{
		lua_getinfo(lua, "Sln", &state);
		// only report script locations;
		// C code has name="[C]" & currentline=-1
		if (state.currentline > 0) {
			char *traced = state.name
			     ? g_strdup_printf("%s\n%s  %s:%d [%-6s] %s", backtrace, header, state.short_src, state.currentline, state.namewhat, state.name)
			     : g_strdup_printf("%s\n%s  %s:%d [%-6s]", backtrace, header, state.short_src, state.currentline, state.what);
			header = "";
			if (backtrace != msg)
				g_free(backtrace);
			backtrace = traced;
			++lines;
		}
		++level;
	}

	if (lines > 1)
		return backtrace;

	if (backtrace != msg)
		g_free(backtrace);
	return g_strdup(msg);
}

static ATTR_MALLOC gchar *error_add_location(lua_State* lua, const char *msg)
{
	lua_Debug state;

	int r = lua_getstack(lua, 0, &state);
	if (r == 0)
		return g_strdup(msg);
	lua_getinfo(lua, "Sln", &state);
	// the error handler will add a backtrace, so no need for function info
	return g_strdup_printf("%s:%d: %s", state.short_src, state.currentline, msg);
}

static gboolean timedout = FALSE;

static void timeout_script(int sig)
{
	timedout = TRUE;
}

static void check_timeout_script(lua_State *lua, lua_Debug *state)
{
	// state is invalid?
	if (!timedout)
		return;
	// don't add backtrace etc. here; just the location
	gchar *msg = error_add_location(lua, _("script timed out"));
	lua_pushstring(lua, msg);
	g_free(msg);
	lua_error(lua);
}

static int script_error(lua_State *lua)
{
	const char *msg = lua_tostring(lua, -1);
	lua_pop(lua, 1);
	// only the backtrace here, as the location's probably present already
	gchar *fullmsg = error_add_backtrace(lua, msg);
	lua_pushstring(lua, fullmsg);
	g_free(fullmsg);
	return 1;
}


/**
 *
 */
int
run_script(lua_State *lua, const char *filename)
{
#define SCRIPT_TIMEOUT_SECONDS 5

	if (!lua)
		return -1;

	lua_pushcfunction(lua, script_error);
	int errpos = lua_gettop(lua);

	int result = luaL_loadfile(lua, filename);

	if (result) {
		// We got an error, print it
		printf(_("Error: %s\n"), lua_tostring(lua, -1));
		lua_remove(lua, errpos); // unstack the error handler
		lua_pop(lua, 1);
		return -1;
	}

	// Okay, loaded the script; now run it

	struct sigaction newact, oldact;
	newact.sa_handler = timeout_script;
	sigemptyset(&newact.sa_mask);
	newact.sa_flags = 0;

	timedout = FALSE;
	lua_sethook(lua, check_timeout_script, LUA_MASKCOUNT, 1);
	sigaction(SIGALRM, &newact, &oldact);
	alarm(SCRIPT_TIMEOUT_SECONDS);

	int s = lua_pcall(lua, 0, LUA_MULTRET, errpos);

	alarm(0);
	sigaction(SIGALRM, &oldact, NULL);

	lua_remove(lua, errpos); // unstack the error handler

	if (s) {
		// no info to add here; just output the error
		printf(_("Error: %s\n"), lua_tostring(lua, -1));
		lua_pop(lua, 1); // else we leak it
	}

	return 0;
}


/**
 *
 */
void
done_script(lua_State *lua)
{
	if (lua)
		lua_close(lua);

	//lua=NULL;
}

