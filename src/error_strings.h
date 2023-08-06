/**
 *	This file is part of devilspie2
 *	Copyright (C) 2023 Darren Salt
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
#ifndef __HEADER_ERROR_STRINGS_
#define __HEADER_ERROR_STRINGS_

/**
 *
 */
#define DP2ERROR_WRONG_ARG_COUNT                _("%s: wrong number of arguments (expected %d)")
#define DP2ERROR_WRONG_ARG_COUNT_RANGE          _("%s: wrong number of arguments (expected %d-%d)")
#define DP2ERROR_WRONG_ARG_COUNT_MULTI          _("%s: wrong number of arguments (expected %s)")
#define DP2ERROR_WRONG_ARG_TYPE                 _("%s: argument %d: wrong type (expected %s)")
#define DP2ERROR_WRONG_ARG_TYPE_NUM_STR         _("%s: argument %d: wrong type (expected number or string)")
#define DP2ERROR_WRONG_ARG_TYPE_MULTI           _("%s: argument %d: wrong type (expected boolean, number or string)")

#define DP2ERROR_INTEGER_NOT_GT_0               _("%s: argument %d is not a positive integer")
#define DP2ERROR_MISSING_CURRENT_VIEWPORT	_("%s: could not find current viewport")
#define DP2ERROR_FAILED_SET_VIEWPORT            _("%s: setting viewport failed")

#define DP2ERROR_FAILED                         _("%s: failed!")

#define DP2_LUA_BOOLEAN_  N_("boolean")
#define DP2_LUA_NUMBER_   N_("number")
#define DP2_LUA_STRING_   N_("string")
#define DP2_LUA_FUNCTION_ N_("function")

#define DP2_LUA_BOOLEAN   _(DP2_LUA_BOOLEAN_)
#define DP2_LUA_NUMBER    _(DP2_LUA_NUMBER_)
#define DP2_LUA_STRING    _(DP2_LUA_STRING_)
#define DP2_LUA_FUNCTION  _(DP2_LUA_FUNCTION_)

#endif /*__HEADER_ERROR_STRINGS_*/
