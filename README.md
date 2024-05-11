```
#
# Copyright © 2011-2017 Andreas Rönnquist.
# Copyright © 2019-2021 Darren Salt.
# This file is distributed under the same license
# as the devilspie2 package, see COPYING file.
#
```

# Devilspie 2

Devilspie 2 is based on the excellent program Devil's Pie by Ross Burton.
It takes a folder as indata and checks that folder for Lua scripts. These
scripts are run each time a window is opened or closed, and the rules in them
are applied on the window.

Unfortunately the rules of the original Devils Pie are not supported.

If you don't give `devilspie2` any folder with --folder, it will read Lua scripts
from the folder that it gets from the `GLib` function `g_get_user_config_dir()` with
devilspie2/ added to the end - `g_get_user_config_dir()` returns the config
directory as defined in the XDG Base Directory Specification. In most cases
would be the `~/.config/devilspie2/` folder and this folder will be created if
it doesn't already exist. This folder is changeable with the `--folder` option.
If `devilspie2` doesn't find any Lua files in the folder, it will stop execution.

Devilspie2 will load all the Lua files in this folder in alphabetical order.

`devilspie2` accepts the following options:

```
-h, --help          Show help options

-d, --debug         Print debug information to stdout
-e, --emulate       Don't apply any rules, but only emulate execution
-f, --folder        From which folder should we load our Lua scripts
-v, --version       Print program version and quit
-w, --wnck-version  Show version of libwnck and quit
```

## Config

A config is read from the folder where we read all scripts, and is customizable
by the `--folder` option. By default this folder is `~/.config/devilspie2/`.

If there is a file named `devilspie2.lua` in this folder, it is read and it is
searched for a variable (a Lua table of strings) named 
`scripts_window_close`, `scripts_window_focus` or `scripts_window_blur`.
The filenames in the strings in this table will be called when windows are
closed, focused or blurred respectively. If these variables aren't present in
the file, it will be called as a `devilspie2` script file like any other.

For example:

```lua
scripts_window_close = {
   "file1.lua",
   "file2.lua"
}
```

This would make the files `file1.lua` and `file2.lua` interpreted when windows are
closing.


## Scripting

You can choose to have all script functionality in one file, or you can split
it up into several, `devilspie2` runs on all Lua files in your requested
`devilspie2` folder, by default `~/.config/devilspie2` - that is all files with a
`.lua` extension.

The scripting language used is Lua; https://see www.lua.org/

  FAQ:           https://www.lua.org/FAQ.html
  Documentation: https://www.lua.org/docs.html
  Tutorials:     http://lua-users.org/wiki/TutorialDirectory


The following commands are recognised by the Devilspie2 Lua interpreter:

First, a function to show some debug info


`debug_print(string)`

    Debug helper that prints a string to stdout. The string is only printed to
    stdout if `devilspie2` is run with the `--debug` option, otherwise nothing will
    be printed.




Then, there are the functions to get the properties of a window, and related
information:


`get_window_name()`

    Returns a string containing the name of the current window.


`get_window_has_name()`

    Return true or false depending on if the window has a name or not
**(Available from version 0.20)**


`get_application_name()`

    Returns the application name of the current window.


`get_process_name()`

    Returns the name of the process owning the current window.

    On (at least) Linux, the process name is read from /proc/<pid>/comm. If
    that's not possible, 'ps' is launched in a shell. For this reason, you
    should avoid calling get_process_name() more than necessary.

    This function is not compatible with busybox ps.

**(Available from version 0.44)**


`get_window_geometry()`

    Returns the window geometry as four numbers - x-position, y-position,
    width and height. (from version 0.16) - for example you can do something
    like this:

    x, y, width, height = get_window_geometry();
    print("X: "..x..", Y: "..y..", width: "..width..", height: "..height);


`get_window_client_geometry()`

    Returns the window geometry excluding the window manager borders as four
    numbers, x-position, y-position, width and height. (from version 0.16) -
    see `get_window_geometry()` for an example on how to use this function.


```
get_window_is_maximized()
get_window_is_maximised()
```

    Returns true if the window is maximised, false otherwise. 
**(Available from version 0.21; -ise from 0.45)**


```
get_window_is_maximized_vertically()
get_window_is_maximised_vertically()
```

    Returns true if the window is vertically maximised, false otherwise.
**(Available from version 0.21; -ise from 0.45)**


```
get_window_is_maximized_horizontally()
get_window_is_maximised_horizontally()
```

    Returns true if the window is horizontally maximised, false otherwise.
**(Available from version 0.21; -ise from 0.45)**


`get_window_is_decorated()`

    Returns true if the window is decorated, false otherwise.
    
**(Available from version 0.44)**


`get_window_type()`

    Returns the type of the window - The result type is a string, and can
    be one of the following:

   "WINDOW_TYPE_NORMAL"
   "WINDOW_TYPE_DESKTOP"
   "WINDOW_TYPE_DOCK"
   "WINDOW_TYPE_DIALOG"
   "WINDOW_TYPE_TOOLBAR"
   "WINDOW_TYPE_MENU"
   "WINDOW_TYPE_UTILITY"
   "WINDOW_TYPE_SPLASHSCREEN"

   or `WINDOW_TYPE_UNRECOGNIZED` if libwnck didn't recognise the type.

   If, for whatever reason, the function didn't have a window to work on,  
   `WINDOW_ERROR` will be returned.

**(Available from version 0.21)**


`get_class_instance_name()`

   Gets the class instance name from the `WM_CLASS` property for the current
   window. Only available on libwnck 3+, and from `devilspie2` version 0.21.


`get_class_group_name()`

   Gets the class group name from the `WM_CLASS` property for the current
   window. Only available on libwnck 3+, and from `devilspie2` version 0.45.


`get_window_property(property)`

   Returns the window property described in the property string. For a list of
   available properties, you should see the page
   http://standards.freedesktop.org/wm-spec/wm-spec-latest.html

   From 0.45, returns nil if the property doesn't exist.

**(Available from version 0.21)**


`get_window_role()`

    Returns a string describing the current window role of the matched window as
    defined by it's WM_WINDOW_ROLE hint.


`get_window_xid()`

    Returns the X window id of the current window.


`get_window_class()`

    Returns a string representing the class of the current window.


`get_workspace_count()`

    Return the number of workspaces available (available from version 0.27)


`get_screen_geometry()`

    Returns the screen geometry (two numbers) for the screen of the
    current window (available from version 0.29)


`get_window_fullscreen()`
`get_fullscreen()`

    Returns true if the window is fullscreen, false otherwise
    (available from version 0.32)


`get_monitor_index()`

    Returns the index of the monitor containing the window centre (or some
    part of the window).

**(Available from version 0.44)**


`get_monitor_geometry()`

    Returns x, y, width, height for the window's monitor.
    
**(Available from version 0.44)**


`get_monitor_geometry(index)`

    Returns x, y, width, height for the given monitor.
    If the index is out of range, nothing is returned.


The rest of the functions are used to modify the properties of the windows:


`set_adjust_for_decoration([bool])`

    Allow for situations where moving or resizing the window is done
    incorrectly, i.e.
        `set_window_position(0,0)`
    results in the window decoration being taken into account twice, i.e. 
    the window (including decoration) is offset from the top left corner by
    the width of the left side decoration and the height of the title bar.

    This is currently off by default, and is sticky: if you do not explicitly
    set it in your script, its current value is retained.

    If used, it should be used at the start of the script.

    This affects the following functions:

      set_window_geometry()
      set_window_position()
      set_window_size()
      xy()
      xywh()

**(Available from version 0.45)**


`set_window_position(xpos, ypos, [index])`

    Set the position of a window.

    If index is specified then the co-ordinates are relative to a corner of
    the specified monitor (counting from 1) on the current workspace. Which
    corner is determined by the co-ordinates' signs:
        `+ve X ⇒ left, -ve X ⇒ right;`
        `+ve Y ⇒ top,  -ve Y ⇒ bottom.`

    NOTE: since -0 would have a use here but is equal to +0, ~ (bitwise NOT)
    is used. To put the window 60px from the right or bottom, use ~60 or -61.

    If index = 0 then the ‘current’ monitor (with the window's centre point)
    is used (falling back on then the first monitor showing part of the
    window then the first monitor).

    If index = -1 then all monitors are treated as one large virtual monitor.


`set_window_position2(xpos, ypos)`

    Set the position of a window - unlike `set_window_position()`, this
    function uses `XMoveWindow` instead of `wnck_window_set_geometry()` which
    gives a slightly different result.

**(Available from version 0.21)**


`set_window_property(property, value)`

    Set a property of a window to a string or a cardinal (32-bit integer or
    boolean).
    
**(Available from version 0.44)**


`delete_window_property(property)`

    Remove a property from a window.
   
**(Available from version 0.44)**


`set_window_size (xsize, ysize)`

    Sets the size of a window - takes `xsize` and `ysize` as parameters.


`set_window_geometry (xpos, ypos, xsize, ysize)`

    Sets both size and position of a window in one command. Takes four
    parameters, `xpos`, `ypos`, `xsize` and `ysize`.


`set_window_geometry2 (xpos, ypos, xsize, ysize)`

    Sets the window geometry just as `set_window_geometry()`, using
    `XMoveResizeWindow()` instead of its `libwnck` alternative. This results in
    different coordinates than the `set_window_geometry()` function, and results
    are more similar to the results of the original `devilspie` geometry function.
    (available from version 0.21)


`shade()`

    "Shades" a window, showing only the title-bar.


`unshade()`

    Un-shades a window - the opposite of "shade"


`maximize(), maximise()`

    Maximises a window
    (-ise from 0.45)


`unmaximize(), unmaximise()`
    
    Un-maximises a window
    (-ise from 0.45)


`maximize_vertically(), maximise_vertically()`

    Maximises the current window vertically.
    (-ise from 0.45)


`maximize_horizontally(), maximise_horizontally()`

    Maximises the current window horizontally.
    (-ise from 0.45)


`minimize(), minimise()`

    Minimises a window.
    (-ise from 0.45)


`unminimize(), unminimise()`

    Un-minimises a window, that is bringing it back to screen from the minimised
    position/size.
    (-ise from 0.45)


`decorate_window()`

    Shows all window decoration.


`undecorate_window()`

    Removes all window decorations.


`close_window()`

    Closes the window. (Available from 0.31)


`set_window_workspace(number)`

    Moves a window to another workspace. The number variable starts counting at
    1.


`change_workspace(number)`

    Changes the current workspace to another. The number variable starts counting
    at 1.


`pin_window()`

    Asks the window manager to put the window on all workspaces.


`unpin_window()`

    Asks the window manager to put window only in the currently active workspace.


`stick_window()`

    Asks the window manager to keep the window's position fixed on the screen,
    even when the workspace or viewport scrolls.


`unstick_window()`

    Asks the window manager to not have window's position fixed on the screen
    when the workspace or viewport scrolls.


`set_skip_tasklist(skip)`

    Set this to true if you would like the window to skip listing in your
    tasklist. Takes a boolean(true or false) as value. (from version 0.16)


`set_skip_pager(skip)`

    Set this to true if you would like the window to skip listing in your pager.
    Takes a boolean(true or false) as value. (from version 0.16)


`set_window_above([above])`

    Set the current window above all normal windows. (available from version
    0.21)
    Optionally takes a boolean(true or false) as value.

    `make_always_on_top()` is the same as `set_window_above(true)`.


`set_on_top()`

    sets a window on top of the others.
    Unlike `set_window_above()`, it doesn't lock the window in this position.
    As of version 0.45, the window's layer (above, between, below) is
    maintained.


`set_window_below([below])`

    Set the current window below all normal windows. 
    Optionally takes a boolean(true or false) as value.

**(available from version 0.21)**


`set_on_bottom()`

    Sets a window below the others.
    Unlike `set_window_below()`, it doesn't lock the window in this position.

**(Available from version 0.45.)**


`set_window_fullscreen(fullscreen)`

    Asks the window manager to set the fullscreen state of the window according
    to the fullscreen boolean. (available from version 0.24)


`set_viewport(viewport)`

    Using one indata, moves the window to the requested viewport - Counting
    starts at number 1. (Available from version 0.25)
    Also see the alternative usage of this function below.

`set_viewport(x, y)`

    If you are using two indata to the set_viewport function, you can decide
    where in the viewport the window will be placed.

**(Available from version 0.40)**

`center([index,] [direction]), centre([index,] [direction])`

    With no parameters, centres the current window on the current workspace.
    May place the window across multiple monitors.

    If index is specified, centres the current window on the specified
    monitor (counting from 1) on the current workspace.

    If index = 0 then the ‘current’ monitor (with the window's centre point)
    is used (falling back on then the first monitor showing part of the
    window then the first monitor).

    If index = -1 then all monitors are treated as one large virtual monitor.

    If index is out of range then the first monitor is used.

    If direction begins with 'H' or 'h', the window is horizontally centred only.

    If direction begins with 'V' or 'v', the window is vertically centred only.

    If centring only along one axis, the window may be moved along the other
    axis to ensure that it is on the specified monitor.

    The defaults are index = -1 (all monitors), direction = '' (both axes).


**(Available without parameters from version 0.26)**

**(Parameters and ‘centre’ available from version 0.44)**


`set_opacity(value), set_window_opacity(value)`

    Sets the window opacity, takes a float value, 1.0 = completely opaque,
    0.0, completely see-through. Both set_opacity and set_window_opacity
    will do the same thing.
    (Available from version 0.28, set_window_opacity from 0.29)


`set_window_type(type)`

	Sets the window type, according to `_NET_WM_WINDOW_TYPE`. The allowed types
	are the standard `_NET_WM` ones (formatted as a string):

	"_NET_WM_WINDOW_TYPE_DESKTOP"
	"_NET_WM_WINDOW_TYPE_DOCK"
	"_NET_WM_WINDOW_TYPE_TOOLBAR"
	"_NET_WM_WINDOW_TYPE_MENU"
	"_NET_WM_WINDOW_TYPE_UTILITY"
	"_NET_WM_WINDOW_TYPE_SPLASH"
	"_NET_WM_WINDOW_TYPE_DIALOG"
	"_NET_WM_WINDOW_TYPE_NORMAL"

	or shorter versions of the same values

	"WINDOW_TYPE_DESKTOP"
	"WINDOW_TYPE_DOCK"
	"WINDOW_TYPE_TOOLBAR"
	"WINDOW_TYPE_MENU"
	"WINDOW_TYPE_UTILITY"
	"WINDOW_TYPE_SPLASH"
	"WINDOW_TYPE_DIALOG"
	"WINDOW_TYPE_NORMAL"
	
**(Available from version 0.28)**

`focus(), focus_window()`

	Focuses the current window. (Function is available from version 0.30)


`set_window_strut(left, right, top, bottom, ...)`

	Set the reserved area at the borders of the desktop for a docking area such
	as a taskbar or a panel. Will handle up to 12 values.

	Default minimum values are 0 and default maximum values are taken
	from the screen size (current or maximum, depending on whether
	xrandr is used).

	(Available from version 0.32)

`get_window_strut()`

	Get the reserved area at the borders of the desktop for a docking
	area such as a taskbar or a panel.

	Returns a table (12 integers as for _NET_WM_WINDOW_STRUT_PARTIAL) or
	nil. If _NET_WM_WINDOW_STRUT was read then defaults are	used as for
	set_window_strut().

	(Available from version 0.45)

`xy(x, y), xy()`

    Set the position of a window, or if you don't give any input, get the
    position of a window.


`xywh(x, y, w, h), xywh()`

    Set the position and size of a window, or if you don't give any input, get
    the position and size of a window.


Please note that strings comparisons are case sensitive, comparing
"SomeProgram" with "someprogram" will not report equality.


## Simple script example

```lua

-- the debug_print command does only print anything to stdout
-- if devilspie2 is run using the --debug option
debug_print("Window Name: " .. get_window_name());
debug_print("Application name: " .. get_application_name())

-- I want my Xfce4-terminal to the right on the second screen of my two-monitor
-- setup. (String comparison are case sensitive, please note this when
-- creating rule scripts.)
if (get_window_name() == "Terminal") then
   -- x,y, xsize, ysize
   set_window_geometry(1600, 300, 900, 700);
end

-- Make Firefox always start maximised.
if (get_application_name() == "Firefox") then
   maximise();
end

```

## Translations

`devilspie2` is translatable using `gettext` - see README.translators for more
information.


## Authors

See [AUTHORS](./AUTHORS)


## Contact

Author: Darren Salt
Homepage: http://www.nongnu.org/devilspie2
Contact, Mailing list: devilspie2-discuss@nongnu.org,
        https://lists.nongnu.org/mailman/listinfo/devilspie2-discuss
See also: https://github.com/dsalt/devilspie

IRC: #devilspie2 on irc.libera.chat
