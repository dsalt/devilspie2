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
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>

#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include <libwnck/libwnck.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <locale.h>

#include <limits.h>

#include "intl.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <X11/extensions/Xrandr.h>

#if (GTK_MAJOR_VERSION >= 3)
#define HAVE_GTK3
#endif


#include "compat.h"

#include "script_functions.h"

#include "script.h"

#include "xutils.h"

#include "error_strings.h"


/**
 *
 */
WnckWindow *current_window = NULL;
static gboolean adjusting_for_decoration = FALSE;

static Bool current_time_cb(Display *display, XEvent *xevent, XPointer arg)
{
	Window wnd = GPOINTER_TO_UINT(arg);

	if (xevent->type == PropertyNotify &&
	    xevent->xproperty.window == wnd &&
	    xevent->xproperty.atom == my_wnck_atom_get("WM_NAME"))
		return True;

	return False;
}

/**
 * Get current X11 timestamp.
 *
 * Unfortunately, gtk_get_current_event_time() does not work here
 * because we cannot assume we are inside an event.
 *
 * Getting this timestamp is tricky. According to a comment in the ICCCM
 * specification (https://tronche.com/gui/x/icccm/sec-2.html#s-2.1):
 *
 *     A zero-length append to a property is a way to obtain a
 *     timestamp for this purpose; the timestamp is in the
 *     corresponding PropertyNotify event.
 *
 * So here we are zero-length appending to the "WM_NAME" property.
 */
static guint32 current_time(void)
{
	WnckWindow *window;
	gulong wnd;
	Display *dpy;
	Atom prop;
	XEvent xevent;

	window = get_current_window();
	if (window == NULL)
		return GDK_CURRENT_TIME;

	dpy = gdk_x11_get_default_xdisplay();
	wnd = wnck_window_get_xid(window);
	prop = my_wnck_atom_get("WM_NAME");
	XChangeProperty(dpy, wnd, prop, XA_STRING, 8, PropModeAppend, NULL, 0);

	/* Wait for the event to succeed */
	XIfEvent(dpy, &xevent, current_time_cb, GUINT_TO_POINTER(wnd));
	return xevent.xproperty.time;
}


/***********************************
 * Error-checking functions & macros
 */

static const char *const dp2_type_to_string[] = {
	[LUA_TBOOLEAN] = DP2_LUA_BOOLEAN_,
	[LUA_TNUMBER]  = DP2_LUA_NUMBER_,
	[LUA_TSTRING]  = DP2_LUA_STRING_
};

#define DP2_C_FUNC_TO_LUA (__func__ + 2)

/* This macro inserts the function name after the format parameter.
 */
#define dp2_lua_error(lua, errmsg, ...) \
	luaL_error(lua, errmsg, DP2_C_FUNC_TO_LUA, ##__VA_ARGS__)

/*
 * Check that types match. Reports the first error encountered.
 * Returns 0 if all match.
 */
static int dp2_check_types(lua_State *lua, const char *func, size_t count, int check)
{
	for (size_t i = 1; i <= count; ++i) {
		int type = lua_type(lua, i);
		if (type != check) {
			luaL_error(lua, DP2ERROR_WRONG_ARG_TYPE, func, i, _(dp2_type_to_string[check])); \
			return 1;
		}
	}

	return 0;
}

static int dp2_check_type(lua_State *lua, const char *func, size_t argi, int check)
{
	int type = lua_type(lua, argi);
	if (type != check) {
		luaL_error(lua, DP2ERROR_WRONG_ARG_TYPE, func, argi, _(dp2_type_to_string[check])); \
		return 1;
	}

	return 0;
}

/*
 * Get & check the expected argument count (fixed count).
 * On mismatch, will report and return from the invoking function.
 * Assumes lua_State *lua
 */
#define DP2_REQUIRE_ARG_COUNT(count) \
	do { \
		if (lua_gettop(lua) != count) { \
			dp2_lua_error(lua, DP2ERROR_WRONG_ARG_COUNT, count); \
			return 0; \
		} \
	} while (0)

/* As above, but range of no. of args (is written to var) */
#define DP2_REQUIRE_ARG_COUNT_RANGE(var, count_min, count_max) \
	do { \
		var = lua_gettop(lua); \
		if (var < count_min || var > count_max) { \
			dp2_lua_error(lua, DP2ERROR_WRONG_ARG_COUNT_RANGE, count_min, count_max); \
			return 0; \
		} \
	} while (0)

/* As above, but various nos. of args (is written to var) */
#define DP2_REQUIRE_ARG_COUNT_MULTI(var, ...) \
	do { \
		const int args__[] = { __VA_ARGS__ }; \
		int i__; \
		var = lua_gettop(lua); \
		for (i__ = 0; i__ < sizeof(args__) / sizeof(args__[0]); ++i__) \
			if (var == args__[i__]) break; \
		if (i__ >= sizeof(args__) / sizeof(args__[0])) { \
			dp2_lua_error(lua, DP2ERROR_WRONG_ARG_COUNT_MULTI, #__VA_ARGS__); \
			return 0; \
		} \
	} while (0)

#define DP2_REPORT_ERROR_ARG_COUNT_MULTI(...) \
	dp2_lua_error(lua, DP2ERROR_WRONG_ARG_COUNT_MULTI, #__VA_ARGS__); \

/*
 * Macros to check that types match.
 * On mismatch, Will report and return from the invoking function.
 * Assumes lua_State *lua
 *
 * Checks all arguments
 */
#define DP2_REQUIRE_ARGS_TYPE(argc, check) \
	do { \
		if (dp2_check_types(lua, DP2_C_FUNC_TO_LUA, argc, LUA_T##check)) return 0; \
	} while (0)
/* Checks one argument */
#define DP2_REQUIRE_ARG_TYPE(argi, check) \
	do { \
		if (dp2_check_type(lua, DP2_C_FUNC_TO_LUA, argi, LUA_T##check)) return 0; \
	} while (0)

/***************
 * Lua functions
 *
 * Implementation function names are of the form "c_<lua function name>".
 */

int c_set_adjust_for_decoration(lua_State *lua)
{
	gboolean v = TRUE;
	int top;
	DP2_REQUIRE_ARG_COUNT_RANGE(top, 0, 1);

	if (top) {
		DP2_REQUIRE_ARG_TYPE(1, BOOLEAN);
		int value = lua_toboolean(lua, 1);
		v = (gboolean)(value);
	}

	adjusting_for_decoration = v;
	return 0;
}


/**
 * returns the window name
 */
int c_get_window_name(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();
	char *test = window ? (char*)wnck_window_get_name(window) : "";

	lua_pushstring(lua, test);

	// one item returned (the window name as a string)
	return 1;
}


/**
 * c_get_window_name always returns a string, even if a window hasn't
 * got a name - use this function to determine if a window really has
 * a name or not.
 * returns a boolean true or false
 */
int c_get_window_has_name(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();
	gboolean has_name = window ? wnck_window_has_name(window) : FALSE;

	lua_pushboolean(lua, has_name);

	return 1;
}


/**
 * Set the Window Geometry
 * 	set_window_geometry(x,y,xsize,ysize);
 */
int c_set_window_geometry(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(4);
	DP2_REQUIRE_ARGS_TYPE(4, NUMBER);

	int x = lua_tonumber(lua, 1);
	int y = lua_tonumber(lua, 2);
	int xsize = lua_tonumber(lua, 3);
	int ysize = lua_tonumber(lua, 4);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();
		set_window_geometry(window, x, y, xsize, ysize, adjusting_for_decoration);
	}

	return 0;
}


/**
 * Set the Window Geometry2
 * 	set_window_geometry2(x,y,xsize,ysize);
 */
int c_set_window_geometry2(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(4);
	DP2_REQUIRE_ARGS_TYPE(4, NUMBER);

	int x = lua_tonumber(lua, 1);
	int y = lua_tonumber(lua, 2);
	int xsize = lua_tonumber(lua, 3);
	int ysize = lua_tonumber(lua, 4);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window) {
			XMoveResizeWindow(gdk_x11_get_default_xdisplay(),
			                  wnck_window_get_xid(window),
			                  x, y,
			                  xsize, ysize);
		}
	}

	return 0;
}


/**
 * Set the position of the window
 */
int c_set_window_position(lua_State *lua)
{
	int top;

	DP2_REQUIRE_ARG_COUNT_RANGE(top, 2, 3);
	DP2_REQUIRE_ARGS_TYPE(top, NUMBER);

	int x = lua_tonumber(lua, 1);
	int y = lua_tonumber(lua, 2);
	int monitor = MONITOR_NONE;

	if (top == 3) {
		monitor = lua_tonumber (lua, 3) - 1;
		if (monitor < MONITOR_ALL || monitor >= get_monitor_count())
			monitor = 0; // FIXME: primary monitor; show warning?
	}

	if (!devilspie2_emulate) {

		WnckWindow *window = get_current_window();

		if (window) {
			if (monitor != MONITOR_NONE) {
				/* +ve x: relative to left
				 * -ve x: relative to right (bitwise NOT)
				 * +ve y: relative to top
				 * -ve y: relative to bottom (bitwise NOT)
				 */
				GdkRectangle bounds, geom;

				wnck_window_get_geometry(window, &geom.x, &geom.y, &geom.width, &geom.height);

				monitor = get_monitor_or_workspace_geometry(monitor, window, &bounds);
				if (monitor == MONITOR_NONE)	{
					lua_pushboolean(lua, FALSE);
					return 1;
				}

				if (x < 0)
					x = bounds.x + bounds.width - ~x - geom.width;
				else
					x += bounds.x;
				if (y < 0)
					y = bounds.y + bounds.height - ~y - geom.height;
				else
					y += bounds.y;
			}

			if (adjusting_for_decoration)
				adjust_for_decoration(window, &x, &y, NULL, NULL);
			wnck_window_set_geometry(window,
			                         WNCK_WINDOW_GRAVITY_CURRENT,
			                         WNCK_WINDOW_CHANGE_X + WNCK_WINDOW_CHANGE_Y,
			                         x, y, -1, -1);
		}
	}

	lua_pushboolean(lua, TRUE);
	return 1;
}


/**
 *
 */
int c_set_window_position2(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(2);
	DP2_REQUIRE_ARGS_TYPE(2, NUMBER);

	int x = lua_tonumber(lua,1);
	int y = lua_tonumber(lua,2);

	if (!devilspie2_emulate) {

		WnckWindow *window = get_current_window();

		if (window) {
			XMoveWindow(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
			            wnck_window_get_xid(window),
			            x, y);
		}
	}

	return 0;
}


/**
 * Sets the size of the window
 */
int c_set_window_size(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(2);
	DP2_REQUIRE_ARGS_TYPE(2, NUMBER);

	int x = lua_tonumber(lua,1);
	int y = lua_tonumber(lua,2);

	if (!devilspie2_emulate) {

		WnckWindow *window = get_current_window();

		if (window) {

			devilspie2_error_trap_push();
			if (adjusting_for_decoration)
				adjust_for_decoration (window, NULL, NULL, &x, &y);
			wnck_window_set_geometry(window,
			                         WNCK_WINDOW_GRAVITY_CURRENT,
			                         WNCK_WINDOW_CHANGE_WIDTH + WNCK_WINDOW_CHANGE_HEIGHT,
			                         -1, -1, x, y);

			if (devilspie2_error_trap_pop()) {
				g_printerr(DP2ERROR_FAILED, DP2_C_FUNC_TO_LUA);
			}
		}
	}

	return 0;
}


#define NUM_STRUTS 12
static gulong *get_default_struts(Display *dpy)
{
	int screen = DefaultScreen(dpy);
	int width, height;

	static gulong struts[NUM_STRUTS];
	memset (struts, 0, sizeof(struts));
#ifdef HAVE_XRANDR
	// If we have xrandr (we probably do), get the maximum screen size
	int x; // throwaway
	XRRGetScreenSizeRange (dpy, RootWindow(dpy, screen),
	                       &x, &x, &width, &height);
#else
	// Otherwise, fall back to the current size
	width = DisplayWidth(dpy, screen);
	height = DisplayHeight(dpy, screen);
#endif
	struts[5] = struts[7] = height;
	struts[9] = struts[11] = width;

	return struts;
}

/**
 * Sets the window strut
 */
int c_set_window_strut(lua_State *lua)
{
	int top;
	DP2_REQUIRE_ARG_COUNT_RANGE(top, 4, 12);
	DP2_REQUIRE_ARGS_TYPE(top, NUMBER);

	if (!devilspie2_emulate) {
		Display *dpy = gdk_x11_get_default_xdisplay();

		gulong *struts = get_default_struts(dpy);
		for (int i = 0; i < top; i++) {
			struts[i] = lua_tonumber(lua, i + 1);
		}

		WnckWindow *window = get_current_window();

		if (window) {
			XChangeProperty(dpy,
			                wnck_window_get_xid(window),
			                XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False), XA_CARDINAL,
			                32,
			                PropModeReplace,
			                (unsigned char*)struts,
			                NUM_STRUTS);
			XSync(dpy, False);
		}
	}

	return 0;
}

/**
 * Gets the window strut
 */
int c_get_window_strut(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();

	if (!window)
		return 0;

	Display *dpy = gdk_x11_get_default_xdisplay();
	gulong *struts = NULL;
	int len = 0;

	gboolean ret = my_wnck_get_cardinal_list (wnck_window_get_xid(window),
	                                          XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False),
	                                          &struts, &len);
	/* if that fails, try reading the older, deprecated property */
	if (!ret)
		ret = my_wnck_get_cardinal_list (wnck_window_get_xid(window),
		                                 XInternAtom(dpy, "_NET_WM_STRUT", False),
		                                 &struts, &len);

	if (len) {
		int i;
		if (len > NUM_STRUTS)
			len = NUM_STRUTS;

		lua_createtable(lua, NUM_STRUTS, 0);
		for (i = 0; i < len; ++i) {
			lua_pushinteger(lua, struts[i]);
			lua_rawseti(lua, -2, i + 1);
		}
		g_free(struts);

		// pad out with default values if necessary
		if (len < NUM_STRUTS) {
			struts = get_default_struts(dpy);
			for (; i < NUM_STRUTS; ++i) {
				lua_pushinteger(lua, struts[i]);
				lua_rawseti(lua, -2, i + 1);
			}
		}
		return 1;
	}
	return 0;
}

/**
 * Sets the window on top of all others and locks it "always on top"
 */
int c_make_always_on_top(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window) {
			wnck_window_make_above(window);
		}
	}

	return 0;
}


/**
 * sets the window on top of all the others
 */
int c_set_on_top(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window)
			XRaiseWindow(gdk_x11_get_default_xdisplay(), wnck_window_get_xid(window));
	}

	return 0;
}


/**
 * sets the window below all the others
 */
int c_set_on_bottom(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window)
			XLowerWindow(gdk_x11_get_default_xdisplay(), wnck_window_get_xid(window));
	}

	return 0;
}


/**
 * returns the application name
 */
int c_get_application_name(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	const char *application_name = "";

	WnckWindow *window = get_current_window();

	if (window) {

		WnckApplication *application=
		    wnck_window_get_application(get_current_window());
		if (application)
			application_name = wnck_application_get_name(application);
	}

	// one item returned - the application name as a string.
	lua_pushstring(lua, application_name);

	return 1;
}


/**
 *	lua_Bprint from http://www.lua.org/source/5.1/lbaselib.c.html
 * but with the change that fputs is only called if devilspie2_debug is
 * set to TRUE
 */
int c_debug_print(lua_State *lua)
{
	int n = lua_gettop(lua);  /* number of arguments */
	lua_getglobal(lua, "tostring");

	for (int i = 1; i <= n; i++) {
		lua_pushvalue(lua, -1);  /* function to be called */
		lua_pushvalue(lua, i);   /* value to print */
		lua_call(lua, 1, 1);

		const char *s = lua_tostring(lua, -1);  /* get result */
		if (s == NULL)
			return luaL_error(lua, "'tostring' must return a string to 'print'");
		if (i > 1) {
			if (devilspie2_debug) fputs("\t", stdout);
		}
		if (devilspie2_debug) fputs(s, stdout);
		lua_pop(lua, 1);  /* pop result */
	}
	if (devilspie2_debug) {
		fputs("\n", stdout);
		fflush(stdout);
	}

	return 0;
}


/**
 * "Shades" the window
 */
int c_shade(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window) {

			wnck_window_shade(window);
		}
	}

	return 0;
}


/**
 * "Unshades" the window
 */
int c_unshade(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window) {

			wnck_window_unshade(window);
		}
	}

	return 0;
}


/**
 * Minimizes the window
 */
int c_minimize(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window) {

			wnck_window_minimize(window);
		}
	}

	return 0;
}


/**
 * unminimizes window
 */
int c_unminimize(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window) {
			wnck_window_unminimize (window, current_time());
		}
	}

	return 0;
}


/**
 * sets the window that the scripts are affecting
 */
void set_current_window(WnckWindow *window)
{
	current_window=window;
}


/**
 * gets the window that the scripts are affecting
 */
WnckWindow *get_current_window()
{
	return current_window;
}


/**
 * Decorates a window
 */
int c_undecorate_window(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	gboolean result = TRUE;

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window) {
			if (!devilspie2_emulate) {

				gulong xid = wnck_window_get_xid(window);

				if (!undecorate_window(xid)) {
					result=FALSE;
				}
			}
		}
	}

	lua_pushboolean(lua,result);

	return 1;
}


/**
 * undecorates a window
 */
int c_decorate_window(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	gboolean result = TRUE;

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window) {

			if (!devilspie2_emulate) {

				gulong xid = wnck_window_get_xid(window);

				if (!decorate_window(xid)) {
					result=FALSE;
				}
			}
		}
	}

	lua_pushboolean(lua,result);

	return 1;
}


/**
 * Checks if a window is decorated
 */
int c_get_window_is_decorated(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	gboolean result = TRUE;

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window) {
			result = get_decorated(wnck_window_get_xid(window));
		}
	}

	lua_pushboolean(lua,result);

	return 1;
}


/**
 * Move a window to a specific workspace
 */
int c_set_window_workspace(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(1);
	DP2_REQUIRE_ARG_TYPE(1, NUMBER);

	int number = lua_tonumber(lua, 1);

	WnckWindow *window = get_current_window();

	if (window) {
		WnckScreen *screen = wnck_window_get_screen(window);
		WnckWorkspace *workspace = wnck_screen_get_workspace(screen, number-1);

		if (!workspace) {
			g_warning(_("Workspace number %d does not exist!"), number);
		}
		if (!devilspie2_emulate) {
			wnck_window_move_to_workspace(window, workspace);
		}
	}

	lua_pushboolean(lua,TRUE);

	return 1;
}




/**
 * Makes a workspace the active one
 */
int c_change_workspace(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(1);
	DP2_REQUIRE_ARG_TYPE(1, NUMBER);

	int number = lua_tonumber(lua, 1);

	WnckWindow *window = get_current_window();
	if (window) {
		WnckScreen *screen = wnck_window_get_screen(window);
		WnckWorkspace *workspace = wnck_screen_get_workspace(screen, number-1);

		if (!workspace) {
			g_warning(_("Workspace number %d does not exist!"), number);
		}

		gint64 timestamp = g_get_real_time();
		if (!devilspie2_emulate) {
			wnck_workspace_activate(workspace, timestamp / 1000000);
		}
	}

	lua_pushboolean(lua, TRUE);

	return 1;
}


/**
 * Return workspace count
 */
int c_get_workspace_count(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	int count = 0;

	WnckWindow *window = get_current_window();

	if (window) {
		WnckScreen *screen = wnck_window_get_screen(window);
		count = wnck_screen_get_workspace_count(screen);
	}

	lua_pushinteger(lua, count);

	return 1;
}


/**
 * Unmaximize window
 */
int c_unmaximize(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();
		if (window) {
			wnck_window_unmaximize(window);
		}
	}

	return 0;
}


/**
 * Maximize Window
 */
int c_maximize(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();
		if (window) {
			wnck_window_maximize(window);
		}
	}
	return 0;
}


/**
 * Maximize Window Vertically
 */
int c_maximize_vertically(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();
		if (window) {
			wnck_window_maximize_vertically(window);
		}
	}

	return 0;
}


/**
 * Maximize the window horizontally
 */
int c_maximize_horizontally(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();
		if (window) {
			wnck_window_maximize_horizontally(window);
		}
	}

	return 0;
}

int c_maximize_horisontally(lua_State *lua)
{
	fprintf(stderr, "warning: deprecated function %s called\n", __func__ + 2);
	return c_maximize_horizontally(lua);
}


/**
 * Pins the window
 */
int c_pin_window(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();
		if (window) {
			wnck_window_pin(window);
		}
	}

	return 0;
}



/**
 * Unpin the window
 */
int c_unpin_window(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();
		if (window) {
			wnck_window_unpin(window);
		}
	}

	return 0;
}


/**
 * Sticks the window
 */
int c_stick_window(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();
		if (window) {
			wnck_window_stick(window);
		}
	}

	return 0;
}


/**
 * Unstick the window
 */
int c_unstick_window(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();
		if (window) {
			wnck_window_unstick(window);
		}
	}

	return 0;
}


/**
 * return the geometry of the current window to the Lua script
 */
int c_get_window_geometry(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	int x = 0, y = 0, width = 0, height = 0;

	WnckWindow *window = get_current_window();
	if (window)
	{
		wnck_window_get_geometry(window, &x, &y, &width, &height);
	}

	lua_pushinteger(lua, x);
	lua_pushinteger(lua, y);
	lua_pushinteger(lua, width);
	lua_pushinteger(lua, height);

	return 4;
}


/**
 * return the client geometry of the current window to the Lua script
 * this is excluding the window manager frame
 */
int c_get_window_client_geometry(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	int x = 0, y = 0, width = 0, height = 0;

	WnckWindow *window = get_current_window();
	if (window)
	{
		wnck_window_get_client_window_geometry(window, &x, &y, &width, &height);
	}

	lua_pushinteger(lua, x);
	lua_pushinteger(lua, y);
	lua_pushinteger(lua, width);
	lua_pushinteger(lua, height);

	return 4;
}


/**
 *
 */
int c_set_skip_tasklist(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(1);
	DP2_REQUIRE_ARG_TYPE(1, BOOLEAN);

	int value = lua_toboolean(lua, 1);

	gboolean skip_tasklist = (gboolean)(value);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();
		if (window) {
			wnck_window_set_skip_tasklist(window, skip_tasklist);
		}
	}

	return 0;
}


/**
 *
 */
int c_set_skip_pager(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(1);
	DP2_REQUIRE_ARG_TYPE(1, BOOLEAN);

	int value = lua_toboolean(lua, 1);

	gboolean skip_pager = (gboolean)(value);

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();
		if (window) {
			wnck_window_set_skip_pager(window, skip_pager);
		}
	}

	return 0;
}


/**
 *
 */
int c_get_window_is_maximized(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();
	gboolean is_maximized = window ? wnck_window_is_maximized(window) : FALSE;

	lua_pushboolean(lua, is_maximized);

	return 1;
}

/**
 *
 */
int c_get_window_is_maximized_vertically(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();
	gboolean is_vertically_maximized = window ? wnck_window_is_maximized_vertically(window) : FALSE;

	lua_pushboolean(lua, is_vertically_maximized);

	return 1;
}


/**
 *
 */
int c_get_window_is_maximized_horizontally(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();
	gboolean is_horizontally_maximized = window ? wnck_window_is_maximized_horizontally(window) : FALSE;

	lua_pushboolean(lua, is_horizontally_maximized);

	return 1;
}

int c_get_window_is_maximized_horisontally(lua_State *lua)
{
	fprintf(stderr, "warning: deprecated function %s called\n", __func__ + 2);
	return c_get_window_is_maximized_horizontally(lua);
}


/**
 *
 */
int c_get_window_is_pinned(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();
	gboolean is_pinned = window ? wnck_window_is_pinned(window) : FALSE;

	lua_pushboolean(lua, is_pinned);

	return 1;
}


/**
 *
 */
int c_set_window_above(lua_State *lua)
{
	int top;
	gboolean set_above;

	DP2_REQUIRE_ARG_COUNT_RANGE(top, 0, 1);

	if (top == 1) {
		DP2_REQUIRE_ARG_TYPE(top, BOOLEAN);

		int value = lua_toboolean(lua, 1);
		set_above = (gboolean)(value);
	}
	else {
		set_above = TRUE;
	}

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window) {
			if (set_above)
				wnck_window_make_above(window);
			else
				wnck_window_unmake_above(window);
		}
	}

	return 0;
}


/**
 *
 */
int c_set_window_below(lua_State *lua)
{
	int top;
	gboolean set_below;

	DP2_REQUIRE_ARG_COUNT_RANGE(top, 0, 1);

	if (top == 1) {
		DP2_REQUIRE_ARG_TYPE(top, BOOLEAN);

		int value = lua_toboolean(lua, 1);
		set_below = (gboolean)(value);
	}
	else {
		set_below = TRUE;
	}

	if (!devilspie2_emulate) {
		WnckWindow *window = get_current_window();

		if (window) {
			if (set_below)
				wnck_window_make_below(window);
			else
				wnck_window_unmake_below(window);
		}
	}

	return 0;
}


/**
 *
 */
int c_get_window_type(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();
	const char *window_type_string;

	if (window) {
		WnckWindowType window_type = wnck_window_get_window_type(window);

		switch (window_type) {
		case WNCK_WINDOW_NORMAL:
			window_type_string = "WINDOW_TYPE_NORMAL";
			break;
		case WNCK_WINDOW_DESKTOP:
			window_type_string = "WINDOW_TYPE_DESKTOP";
			break;
		case WNCK_WINDOW_DOCK:
			window_type_string = "WINDOW_TYPE_DOCK";
			break;
		case WNCK_WINDOW_DIALOG:
			window_type_string = "WINDOW_TYPE_DIALOG";
			break;
		case WNCK_WINDOW_TOOLBAR:
			window_type_string = "WINDOW_TYPE_TOOLBAR";
			break;
		case WNCK_WINDOW_MENU:
			window_type_string = "WINDOW_TYPE_MENU";
			break;
		case WNCK_WINDOW_UTILITY:
			window_type_string = "WINDOW_TYPE_UTILITY";
			break;
		case WNCK_WINDOW_SPLASHSCREEN:
			window_type_string = "WINDOW_TYPE_SPLASHSCREEN";
			break;
		default:
			window_type_string = "WINDOW_TYPE_UNRECOGNIZED";
		};
	} else {
		window_type_string = "WINDOW_ERROR";
	}

	lua_pushstring(lua, window_type_string);

	return 1;
}


/**
 * c_get_class_instance_name
 * Only available on libwnck 3+
 */
int c_get_class_instance_name(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

#ifdef HAVE_GTK3
	WnckWindow *window = get_current_window();
	const char *class_instance_name = window ? wnck_window_get_class_instance_name(window) : "";

	// one item returned - the window class instance name as a string.
	lua_pushstring(lua, class_instance_name);
#else
	lua_pushnil(lua);
#endif
	return 1;
}

/**
 * c_get_class_group_name
 * Only available on libwnck 3+
 */
int c_get_class_group_name(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

#ifdef HAVE_GTK3
	WnckWindow *window = get_current_window();
	const char *class_group_name = window ? wnck_window_get_class_group_name(window) : "";

	// one item returned - the window class instance name as a string.
	lua_pushstring(lua, class_group_name);
#else
	lua_pushnil(lua);
#endif
	return 1;
}


/**
 *
 */
int c_get_window_property(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(1);
	DP2_REQUIRE_ARG_TYPE(1, STRING);

	const gchar *value = lua_tostring(lua, 1);
	WnckWindow *window = get_current_window();

	if (window) {
		char *result = my_wnck_get_string_property_latin1(wnck_window_get_xid(window), my_wnck_atom_get(value));

		lua_pushstring(lua, result ? result : "");
		g_free (result);
	} else {
		lua_pushnil(lua);
	}

	return 1;
}


/**
 *
 */
int c_set_window_property(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(2);
	DP2_REQUIRE_ARG_TYPE(1, STRING);

	WnckWindow *window = get_current_window();

	const gchar *property = lua_tostring(lua, 1);

	switch (lua_type(lua, 2)) {
	case LUA_TSTRING:
		if (!devilspie2_emulate)
			my_wnck_set_string_property_latin1(wnck_window_get_xid(window), my_wnck_atom_get(property),
			                                   lua_tostring(lua, 2));
		break;

	case LUA_TNUMBER:
		if (!devilspie2_emulate)
			my_wnck_set_cardinal_property(wnck_window_get_xid(window), my_wnck_atom_get(property),
			                              (int32_t) lua_tonumber(lua, 2));
		break;

	case LUA_TBOOLEAN:
		if (!devilspie2_emulate)
			my_wnck_set_cardinal_property(wnck_window_get_xid(window), my_wnck_atom_get(property),
			                              (int32_t) lua_toboolean(lua, 2));
		break;

	default:
		luaL_error(lua, DP2ERROR_WRONG_ARG_TYPE_MULTI, DP2_C_FUNC_TO_LUA, 2);
	}

	return 0;
}


/**
 *
 */
int c_delete_window_property(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(1);
	DP2_REQUIRE_ARG_TYPE(1, STRING);

	WnckWindow *window = get_current_window();
	const gchar *property = lua_tostring(lua, 1);

	if (!devilspie2_emulate)
		my_wnck_delete_property(wnck_window_get_xid(window), my_wnck_atom_get(property));

	return 0;
}
/**
 *
 */
int c_get_window_role(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();

	if (window) {
		char *result = my_wnck_get_string_property_latin1(wnck_window_get_xid(window), my_wnck_atom_get("WM_WINDOW_ROLE"));

		lua_pushstring(lua, result ? result : "");
		g_free (result);
	} else {
		lua_pushstring(lua, "");
	}

	return 1;
}


/**
 *
 */
int c_get_window_xid(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();
	gulong result = window ? wnck_window_get_xid(window) : 0;

	lua_pushinteger(lua, result);

	return 1;
}


/**
 *
 */
int c_get_window_class(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();
	const char *result = "";

	if (window) {
		WnckClassGroup *class_group = wnck_window_get_class_group(window);
		if (class_group) {
#ifdef WNCK_MAJOR_VERSION
#if WNCK_CHECK_VERSION(3,2,0)
			result = (char*)wnck_class_group_get_id(class_group);
#else
			result = (char*)wnck_class_group_get_res_class (class_group);
#endif
#else
			result = (char*)wnck_class_group_get_res_class (class_group);
#endif
		}
	}

	lua_pushstring(lua, result);

	return 1;
}


/**
 *
 */
int c_set_window_fullscreen(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(1);
	DP2_REQUIRE_ARG_TYPE(1, BOOLEAN);

	WnckWindow *window = get_current_window();

	gboolean fullscreen = lua_toboolean(lua, 1);

	if (window) {
		if (!devilspie2_emulate) {
			wnck_window_set_fullscreen(window, fullscreen);
		}
	}


	return 0;
}


/**
 *
 */
int c_set_viewport(lua_State *lua)
{
	int top;
	int width, height;
	int viewport_start_x, viewport_start_y;
	int win_x, win_y;

	DP2_REQUIRE_ARG_COUNT_RANGE(top, 1, 2);
	DP2_REQUIRE_ARGS_TYPE(top, NUMBER);

	if (top == 1) {
		WnckScreen *screen;
		int x;

		int num = lua_tonumber(lua,1);

		if (num <= 0) {
			g_error(DP2ERROR_INTEGER_NOT_GT_0, DP2_C_FUNC_TO_LUA, 1);
			lua_pushboolean(lua, FALSE);
			return 1;
		}

		WnckWindow *window = get_current_window();

		if (!window) {
			lua_pushboolean(lua, FALSE);
			return 1;
		}

		screen = wnck_window_get_screen(window);

		wnck_window_get_geometry(window, &win_x, &win_y, &width, &height);

		gulong xid = wnck_window_get_xid(window);

		//viewport_start = devilspie2_get_viewport_start(xid);
		if (devilspie2_get_viewport_start(xid, &viewport_start_x, &viewport_start_y) != 0) {
			g_printerr(DP2ERROR_MISSING_CURRENT_VIEWPORT, DP2_C_FUNC_TO_LUA);
			lua_pushboolean(lua, FALSE);
			return 1;
		}

		x = ((num - 1) * wnck_screen_get_width(screen)) - viewport_start_x + win_x;

		if (!devilspie2_emulate) {
			devilspie2_error_trap_push();
			XMoveResizeWindow(gdk_x11_get_default_xdisplay(),
			                  wnck_window_get_xid(window),
			                  x, win_y, width, height);

			if (devilspie2_error_trap_pop()) {
				g_printerr(DP2ERROR_FAILED_SET_VIEWPORT, DP2_C_FUNC_TO_LUA);
				lua_pushboolean(lua, FALSE);
				return 1;
			}
		}

		lua_pushboolean(lua, TRUE);
		return 1;

	} else if (top == 2) {
		int new_xpos = lua_tonumber(lua, 1);
		int new_ypos = lua_tonumber(lua, 2);

		WnckWindow *window = get_current_window();

		if (!window) {
			lua_pushboolean(lua, FALSE);
			return 1;
		}

		wnck_window_get_geometry(window, &win_x, &win_y, &width, &height);

		gulong xid = wnck_window_get_xid(window);

		//viewport_start = devilspie2_get_viewport_start(xid);
		if (devilspie2_get_viewport_start(xid, &viewport_start_x, &viewport_start_y) != 0) {
			g_printerr(DP2ERROR_MISSING_CURRENT_VIEWPORT, DP2_C_FUNC_TO_LUA);
			lua_pushboolean(lua, FALSE);
			return 1;
		}

		if (!devilspie2_emulate) {
			devilspie2_error_trap_push();
			XMoveResizeWindow(gdk_x11_get_default_xdisplay(),
			                  wnck_window_get_xid(window),
			                  new_xpos, new_ypos, width, height);

			if (devilspie2_error_trap_pop()) {
				g_printerr(DP2ERROR_FAILED_SET_VIEWPORT, DP2_C_FUNC_TO_LUA);
				lua_pushboolean(lua, FALSE);
				return 1;
			}
		}

		lua_pushboolean(lua, TRUE);
		return 1;
	}

	return 0;
}


/**
 *
 */
int c_center(lua_State *lua)
{
	int top;
	GdkRectangle desktop_r, window_r;

	DP2_REQUIRE_ARG_COUNT_RANGE(top, 0, 2);

	WnckWindow *window = get_current_window();

	if (!window) {
		lua_pushboolean(lua, FALSE);
		return 1;
	}

	wnck_window_get_geometry(window, &window_r.x, &window_r.y, &window_r.width, &window_r.height);

	int monitor_no = MONITOR_ALL;
	enum { CENTRE_NONE, CENTRE_H, CENTRE_V, CENTRE_HV } centre = CENTRE_HV;

	for (int i = 1; i <= top; ++i) {
		int type = lua_type(lua, i);
		gchar *indata;

		switch (type) {
		case LUA_TNUMBER:
			monitor_no = lua_tonumber(lua, i) - 1;
			if (monitor_no < MONITOR_ALL || monitor_no >= get_monitor_count())
				monitor_no = MONITOR_WINDOW; // FIXME: primary monitor; show warning?
			break;
		case LUA_TSTRING:
			indata = (gchar*)lua_tostring(lua, i);
			switch (*indata & 0xDF) {
			case 'H':
				centre = CENTRE_H;
				break;
			case 'V':
				centre = CENTRE_V;
				break;
			default:
				centre = CENTRE_HV;
			}
			break;
		default:
			luaL_error(lua, DP2ERROR_WRONG_ARG_TYPE_NUM_STR, DP2_C_FUNC_TO_LUA, i);
			return 0;
		}
	}

	monitor_no = get_monitor_or_workspace_geometry(monitor_no, window, &desktop_r);
	if (monitor_no == MONITOR_NONE)	{
		lua_pushboolean(lua, FALSE);
		return 1;
	}

	if (centre & 1)
		window_r.x = desktop_r.x + (desktop_r.width - window_r.width) / 2;
	else if (window_r.x < desktop_r.x)
		window_r.x = desktop_r.x;
	else if (window_r.x + window_r.width >= desktop_r.x + desktop_r.width)
		window_r.x = desktop_r.x + desktop_r.width - window_r.width;

	if (centre & 2)
		window_r.y = desktop_r.y + (desktop_r.height - window_r.height) / 2;
	else if (window_r.y < desktop_r.y)
		window_r.y = desktop_r.y;
	else if (window_r.y + window_r.height >= desktop_r.y + desktop_r.height)
		window_r.y = desktop_r.y + desktop_r.height - window_r.height;

	if (!devilspie2_emulate) {
		devilspie2_error_trap_push();
		XMoveWindow (gdk_x11_get_default_xdisplay(),
		             wnck_window_get_xid(window),
		             window_r.x, window_r.y);

		if (devilspie2_error_trap_pop()) {
			g_printerr(DP2ERROR_FAILED, DP2_C_FUNC_TO_LUA);
			lua_pushboolean(lua, FALSE);
			return 1;
		}
	}

	lua_pushboolean(lua, TRUE);

	return 1;
}


/**
 *
 */
int c_set_window_opacity(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(1);
	DP2_REQUIRE_ARG_TYPE(1, NUMBER);

	double value = (double)lua_tonumber(lua, 1);
	WnckWindow *window = get_current_window();

	if (!devilspie2_emulate && window) {
		gulong xid = wnck_window_get_xid(window);
		my_window_set_opacity(xid, value);
	}

	return 0;
}


/**
 *
 */
int c_set_window_type(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(1);
	DP2_REQUIRE_ARG_TYPE(1, STRING);

	gchar *indata = (gchar*)lua_tostring(lua, 1);

	WnckWindow *window = get_current_window();

	if (!devilspie2_emulate && window) {
		gulong xid = wnck_window_get_xid(window);
		my_window_set_window_type(xid, indata);
	}

	return 0;
}


/**
 * return the geometry of the screen  to the Lua script
 */
int c_get_screen_geometry(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	int width = -1, height = -1;
	WnckWindow *window = get_current_window();

	if (window) {
		WnckScreen *screen;
		screen = wnck_window_get_screen(window);
		width  = wnck_screen_get_width(screen);
		height = wnck_screen_get_height(screen);
	}

	lua_pushinteger(lua, width);
	lua_pushinteger(lua, height);

	return 2;
}


/**
 *
 */
int c_focus(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();

	if (!devilspie2_emulate && window) {
		wnck_window_activate(window, current_time());
	}

	return 0;
}


/**
 *
 */
int c_close_window(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();
	if (!devilspie2_emulate && window) {
		wnck_window_close(window, current_time());
	}

	return 0;
}


/**
 *
 */
int c_get_window_fullscreen(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	gboolean result = FALSE;

	WnckWindow *window = get_current_window();
	if (window) {
		result = wnck_window_is_fullscreen(window);
	}

	lua_pushboolean(lua, result);

	return 1;
}


/**
 *
 */
int c_get_monitor_index(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();
	if (window) {
		int index = get_monitor_index_geometry(window, NULL, NULL);
		if (index < 0)
			index = -1; // invalid? assume single monitor
		lua_pushinteger(lua, index + 1);
	}

	return 1;
}


/**
 *
 */
int c_get_monitor_geometry(lua_State *lua)
{
	int top;
	GdkRectangle geom;

	DP2_REQUIRE_ARG_COUNT_RANGE(top, 0, 1);

	if (top == 0) {
		WnckWindow *window = get_current_window();
		if (!window)
			return 1; // =nil

		get_monitor_index_geometry(window, NULL, &geom);

	} else if (top == 1) {
		DP2_REQUIRE_ARG_TYPE(1, NUMBER);

		int index = lua_tonumber(lua, 1) - 1;
		int actual = get_monitor_geometry(index, &geom);

		if (actual != index)
			return 0;
	}

	lua_pushinteger(lua, geom.x);
	lua_pushinteger(lua, geom.y);
	lua_pushinteger(lua, geom.width);
	lua_pushinteger(lua, geom.height);

	return 4;
}


/**
 *
 */
int c_xy(lua_State *lua)
{
	int top = lua_gettop(lua);

	if (top == 0) {
		// return the xy coordinates of the window

		WnckWindow *window = get_current_window();
		if (window != NULL) {

			int x, y, width, height;

			wnck_window_get_geometry(window, &x, &y, &width, &height);

			lua_pushinteger(lua, x);
			lua_pushinteger(lua, y);

			return 2;
		}

	} else if (top == 2) {
		DP2_REQUIRE_ARGS_TYPE(top, NUMBER);

		// set the coordinates of the window
		int x = lua_tonumber(lua, 1);
		int y = lua_tonumber(lua, 2);

		if (!devilspie2_emulate) {

			WnckWindow *window = get_current_window();

			if (window) {
				if (adjusting_for_decoration)
					adjust_for_decoration (window, &x, &y, NULL, NULL);
				wnck_window_set_geometry(window,
				                         WNCK_WINDOW_GRAVITY_CURRENT,
				                         WNCK_WINDOW_CHANGE_X + WNCK_WINDOW_CHANGE_Y,
				                         x, y, -1, -1);
			}
		}
	} else {
		DP2_REPORT_ERROR_ARG_COUNT_MULTI(0, 2);
	}
	return 0;
}


/**
 *
 */
int c_xywh(lua_State *lua)
{
	int top = lua_gettop(lua);

	if (top == 0) {
		// Return the xywh settings of the window

		WnckWindow *window = get_current_window();
		if (window != NULL) {

			int x, y, width, height;

			wnck_window_get_geometry(window, &x, &y, &width, &height);

			lua_pushinteger(lua, x);
			lua_pushinteger(lua, y);
			lua_pushinteger(lua, width);
			lua_pushinteger(lua, height);

			return 4;
		}

	} else if (top == 4) {
		// Set the xywh settings in the window
		DP2_REQUIRE_ARGS_TYPE(4, NUMBER);

		int x = lua_tonumber(lua, 1);
		int y = lua_tonumber(lua, 2);
		int xsize = lua_tonumber(lua, 3);
		int ysize = lua_tonumber(lua, 4);

		if (!devilspie2_emulate) {
			WnckWindow *window = get_current_window();
			set_window_geometry(window, x, y, xsize, ysize, adjusting_for_decoration);
		}

		return 0;
	} else {
		DP2_REPORT_ERROR_ARG_COUNT_MULTI(0, 4);
	}

	return 0;
}


struct lua_callback {
	lua_State *lua;
	int ref;
};

static void on_geometry_changed(WnckWindow *window, struct lua_callback *callback)
{
	if (callback == NULL)
		return;

	WnckWindow *old_window = get_current_window();
	set_current_window(window);

	lua_rawgeti(callback->lua, LUA_REGISTRYINDEX, callback->ref);
	lua_pcall(callback->lua, 0, 0, 0);

	set_current_window(old_window);
}

static void on_geometry_changed_disconnect(gpointer data, GClosure *closure G_GNUC_UNUSED)
{
	g_free(data);
}

/**
 *
 */
int c_on_geometry_changed(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(1);
	DP2_REQUIRE_ARG_TYPE(1, FUNCTION);

	struct lua_callback *cb = g_malloc(sizeof(struct lua_callback));
	cb->lua = lua;
	cb->ref = luaL_ref(lua, LUA_REGISTRYINDEX);

	WnckWindow *window = get_current_window();

	if (window) {
		g_signal_connect_data(window, "geometry-changed", G_CALLBACK(on_geometry_changed), (gpointer)cb, (GClosureNotify)(on_geometry_changed_disconnect), 0);
	}

	return 0;
}

/**
 * returns the process binary name
 */
static ATTR_MALLOC gchar *c_get_process_name_INT_proc(lua_State *, pid_t);
static ATTR_MALLOC gchar *c_get_process_name_INT_ps(lua_State *, pid_t);

int c_get_process_name(lua_State *lua)
{
	DP2_REQUIRE_ARG_COUNT(0);

	WnckWindow *window = get_current_window();

	if (window) {
		pid_t pid = wnck_window_get_pid(window);

		if (pid != 0) {
			gchar *cmdname = c_get_process_name_INT_proc(lua, pid);
			if (!cmdname)
				cmdname = c_get_process_name_INT_ps(lua, pid);

			/* chop off any trailing LF */
			gchar *lf = cmdname + strlen(cmdname) - 1;
			if (lf >= cmdname && *lf == '\n')
				*lf = 0;

			lua_pushstring(lua, cmdname ? cmdname : "");
			g_free(cmdname);
			return 1;
		}
	}

	lua_pushstring(lua, "");
	return 1;
}

static gchar *c_get_process_name_INT_proc(lua_State *lua, pid_t pid)
{
	/* 16 is fine for cmdname on Linux. Could be longer elsewhere, though. */
	char cmd[1024], cmdname[1024];
	FILE *cmdfp;

	cmdname[0] = 0;

	snprintf(cmd, sizeof(cmd), "/proc/%lu/comm", (unsigned long)pid);
	cmdfp = fopen(cmd, "r");
	if (cmdfp == NULL) {
		if (errno != ENOENT && errno != EACCES) {
			luaL_error(lua, "get_process_name: Failed to open \"%s\" (%d).", cmd, errno);
		}
		return NULL;
	}

	if (fgets(cmdname, sizeof(cmdname), cmdfp) == NULL) {
		luaL_error(lua, "get_process_name: Failed to read from \"%s\".", cmd);
		fclose(cmdfp);
		return NULL;
	}

	fclose(cmdfp);
	return g_strdup(cmdname);
}

static gchar *c_get_process_name_INT_ps(lua_State *lua, pid_t pid)
{
	char cmd[1024], cmdname[1024];
	FILE *cmdfp;

	/* I'd like to use "ps ho comm c %lu" here.
	 * Seems that FreeBSD ps outputs headers regardless.
	 * (Tested using procps 'ps' with PS_PERSONALITY=bsd)
	 */
	snprintf(cmd, sizeof(cmd), "ps o comm c %lu | tail -n 1", (unsigned long)pid);
	cmdfp = popen(cmd, "r");
	if (cmdfp == NULL) {
		luaL_error(lua, "get_process_name: Failed to run command \"%s\".", cmd);
		return 0;
	}

	if (fgets(cmdname, sizeof(cmdname), cmdfp) == NULL) {
		luaL_error(lua, "get_process_name: Failed to read output from command \"%s\".", cmd);
		pclose(cmdfp);
		return 0;
	}

	pclose(cmdfp);
	return g_strdup(cmdname);
}


/*
 * Devilspie:

 * Focus the current window.

ESExpResult *func_focus(ESExp *f, int argc, ESExpResult **argv, Context *c) {
  wnck_window_activate (c->window, current_time());
  if (debug) g_printerr (_("Focusing\n"));
  return e_sexp_result_new_bool (f, TRUE);
}

*/
