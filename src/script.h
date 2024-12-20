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
#ifndef __HEADER_SCRIPT_
#define __HEADER_SCRIPT_

#include <lua.h>
#include <glib.h>

/**
 *
 */


/**
 *
 */
lua_State *init_script(gchar *script_folder);
void configureLuaPaths(lua_State *lua, gchar * script_folder);

void register_cfunctions(lua_State *lua);
int run_script(lua_State *lua, const char *filename);
void done_script(lua_State *lua);


extern gboolean devilspie2_debug;
extern gboolean devilspie2_emulate;

extern lua_State *global_lua_state;

#endif /*__HEADER_SCRIPT_*/
