/**
 *	This file is part of devilspie2
 *	Copyright (C) 2012-2017 Andreas Rönnquist
 *	Copyright (C) 2019-2025 Darren Salt
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
#include <glib.h>
#include <glib/gi18n.h>

#include <stdio.h>

#include "error_strings.h"

const int max_indata_expected;
gchar *num_indata_expected_errors[] = {NULL, NULL, NULL, NULL, NULL};

gchar *n_or_m_indata_expected_error = NULL;
gchar *n_to_m_indata_expected_error = NULL;

gchar *at_least_four_indata_expected_error = NULL;

gchar *number_expected_as_indata_error = NULL;
gchar *boolean_expected_as_indata_error = NULL;
gchar *string_expected_as_indata_error = NULL;

gchar *number_or_string_expected_as_indata_error = NULL;
gchar *number_or_string_or_boolean_expected_as_indata_error = NULL;

gchar *integer_greater_than_zero_expected_error = NULL;
gchar *could_not_find_current_viewport_error = NULL;

gchar *setting_viewport_failed_error = NULL;

gchar *failed_string = NULL;

/**
 *
 */
#define ALLOCATE_ERROR_STRING _("Couldn't allocate error string!")
#define INIT_ERRMSG(errvar, errtxt) \
	{ \
		errvar = g_strdup(errtxt); \
		if (!errvar) { \
			printf("%s: \"%s\"\n", ALLOCATE_ERROR_STRING, errtxt); \
			return -1; \
		} \
	}
int init_script_error_messages()
{
	INIT_ERRMSG(num_indata_expected_errors[0],              _("No parameters expected"));
	INIT_ERRMSG(num_indata_expected_errors[1],              _("One parameter expected"));
	INIT_ERRMSG(num_indata_expected_errors[2],              _("Two parameters expected"));
	INIT_ERRMSG(num_indata_expected_errors[3],              _("Three parameters expected"));
	INIT_ERRMSG(num_indata_expected_errors[4],              _("Four parameters expected"));

	INIT_ERRMSG(n_or_m_indata_expected_error,               _("%d or %d parameters expected"));
	INIT_ERRMSG(n_to_m_indata_expected_error,               _("%d to %d parameters expected"));

	INIT_ERRMSG(at_least_four_indata_expected_error,        _("At least four parameters expected"));

	INIT_ERRMSG(number_expected_as_indata_error,            _("Number expected as parameter"));
	INIT_ERRMSG(boolean_expected_as_indata_error,           _("Boolean expected as parameter"));
	INIT_ERRMSG(string_expected_as_indata_error,            _("String expected as parameter"));

	INIT_ERRMSG(number_or_string_expected_as_indata_error,  _("Number or string expected as parameter"));
	INIT_ERRMSG(number_or_string_or_boolean_expected_as_indata_error,  _("Number or string or boolean expected as parameter"));

	INIT_ERRMSG(integer_greater_than_zero_expected_error,   _("Integer greater than zero expected"));
	INIT_ERRMSG(could_not_find_current_viewport_error,      _("Could not find current viewport"));
	INIT_ERRMSG(setting_viewport_failed_error,              _("Setting viewport failed"));

	INIT_ERRMSG(failed_string,                              _("Failed!"));

	return 0;
}


/**
 *
 */
void done_script_error_messages()
{
	for (int i = 0; i <= max_indata_expected; i++) {
		g_free(num_indata_expected_errors[i]);
	}

	g_free(n_or_m_indata_expected_error);
	g_free(n_to_m_indata_expected_error);

	g_free(at_least_four_indata_expected_error);

	g_free(number_expected_as_indata_error);
	g_free(boolean_expected_as_indata_error);
	g_free(string_expected_as_indata_error);

	g_free(number_or_string_expected_as_indata_error);
	g_free(number_or_string_or_boolean_expected_as_indata_error);

	g_free(integer_greater_than_zero_expected_error);
	g_free(could_not_find_current_viewport_error);
	g_free(setting_viewport_failed_error);

	g_free(failed_string);
}
