/**
 *	This file is part of devilspie2
 *	Copyright (C) 2011-2019 Andreas Rönnquist
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

#ifndef __HEADER_SCRIPT_FUNCTIONS_
#define __HEADER_SCRIPT_FUNCTIONS_

/**
 *
 */
#include "lua.h"
#define WNCK_I_KNOW_THIS_IS_UNSTABLE
#include "libwnck/libwnck.h"

int c_use_utf8(lua_State *lua);

int c_get_window_name(lua_State *lua);
int c_get_window_has_name(lua_State *lua);

int c_set_window_position(lua_State *lua);
int c_set_window_position2(lua_State *lua);

int c_set_window_geometry(lua_State *lua);
int c_set_window_geometry2(lua_State *lua);

int c_set_window_size(lua_State *lua);

int c_set_window_strut(lua_State *lua);
int c_get_window_strut(lua_State *lua);

int c_get_application_name(lua_State *lua);

int c_debug_print(lua_State *lua);

int c_shade(lua_State *lua);
int c_unshade(lua_State *lua);

int c_minimize(lua_State *lua);
int c_unminimize(lua_State *lua);

int c_decorate_window(lua_State *lua);
int c_undecorate_window(lua_State *lua);
int c_get_window_is_decorated(lua_State *lua);

int c_get_window_workspace(lua_State *lua);
int c_set_window_workspace(lua_State *lua);
int c_change_workspace(lua_State *lua);
int c_get_workspace_count(lua_State *lua);

int c_unmaximize(lua_State *lua);
int c_maximize(lua_State *lua);
int c_maximize_vertically(lua_State *lua);
int c_maximize_horisontally(lua_State *lua); // deprecated
int c_maximize_horizontally(lua_State *lua);

int c_pin_window(lua_State *lua);
int c_unpin_window(lua_State *lua);
int c_stick_window(lua_State *lua);
int c_unstick_window(lua_State *lua);

int c_close_window(lua_State *lua);

void set_current_window(WnckWindow *window);
WnckWindow *get_current_window();

int c_set_adjust_for_decoration(lua_State *lua);

int c_get_window_geometry(lua_State *lua);
int c_get_window_client_geometry(lua_State *lua);
int c_get_window_frame_extents(lua_State *lua);

int c_set_skip_tasklist(lua_State *lua);
int c_set_skip_pager(lua_State *lua);

int c_get_window_is_maximized(lua_State *lua);
int c_get_window_is_maximized_vertically(lua_State *lua);
int c_get_window_is_maximized_horisontally(lua_State *lua); // deprecated
int c_get_window_is_maximized_horizontally(lua_State *lua);
int c_get_window_is_pinned(lua_State *lua);

int c_set_window_fullscreen(lua_State *lua);

int c_set_window_above(lua_State *lua);
int c_set_window_below(lua_State *lua);

int c_make_always_on_top(lua_State *lua);
int c_set_on_top(lua_State *lua);
int c_set_on_bottom(lua_State *lua);

int c_get_window_type(lua_State *lua);

// these two require GTK 3 or later
int c_get_class_instance_name(lua_State *lua);
int c_get_class_group_name(lua_State *lua);

int c_get_window_property(lua_State *lua);
int c_window_property_is_utf8(lua_State *lua);
int c_get_window_property_full(lua_State *lua);
int c_get_window_role(lua_State *lua);

int c_get_window_xid(lua_State *lua);

int c_get_window_class(lua_State *lua);

int c_set_window_property(lua_State *lua);
int c_delete_window_property(lua_State *lua);

int c_set_viewport(lua_State *lua);

int c_center(lua_State *lua);

int c_set_window_opacity(lua_State *lua);
int c_set_window_type(lua_State *lua);


int c_get_screen_geometry(lua_State *lua);

int c_focus(lua_State *lua);

int c_get_window_fullscreen(lua_State *lua);

int c_get_monitor_index(lua_State *lua);
int c_get_monitor_geometry(lua_State *lua);

int c_xy(lua_State *lua);
int c_xywh(lua_State *lua);

int c_on_geometry_changed(lua_State *lua);

int c_get_process_name(lua_State *lua);

int c_millisleep(lua_State *lua);

#endif /*__HEADER_SCRIPT_FUNCTIONS_*/
