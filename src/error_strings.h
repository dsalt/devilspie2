
/**
 *	This file is part of devilspie2
 *	Copyright (C) 2012-2017 Andreas Rönnquist
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
extern const int max_indata_expected;
extern gchar *num_indata_expected_errors[];

extern gchar *n_or_m_indata_expected_error;
extern gchar *n_to_m_indata_expected_error;

extern gchar *at_least_four_indata_expected_error;

extern gchar *number_expected_as_indata_error;
extern gchar *boolean_expected_as_indata_error;
extern gchar *string_expected_as_indata_error;

extern gchar *number_or_string_expected_as_indata_error;
extern gchar *number_or_string_or_boolean_expected_as_indata_error;

extern gchar *integer_greater_than_zero_expected_error;
extern gchar *could_not_find_current_viewport_error;

extern gchar *setting_viewport_failed_error;

extern gchar *failed_string;

/**
 *
 */
int init_script_error_messages();
void done_script_error_messages();


#endif /*__HEADER_ERROR_STRINGS_*/
