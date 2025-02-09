Copyright © 2011-2025 devilspie2 developers

This file is distributed under the same licence as the devilspie2 package
(see [COPYING](COPYING)).

# Devilspie 2

Devilspie 2 is based on the excellent program Devil's Pie by Ross Burton. 
It will read Lua scripts from a folder and run them whenever a window is
opened, and the rules in them are applied on the window. (See the
[Configuration section](#configuration) for more details.)

Unfortunately, the rules of the original Devil's Pie are not supported.

`devilspie2` will load all the Lua files in this folder in alphabetical order.

`devilspie2` accepts the following options:

|   |   |
|:--|:--|
| `-h`, `--help`         | Show help options |
| `-d`, `--debug`        | Print debug information to stdout |
| `-e`, `--emulate`      | Don't apply any rules, only emulate execution |
| `-f`, `--folder`       | Search for scripts in this folder |
| `-v`, `--version`      | Print program version then quit |
| `-w`, `--wnck-version` | Show libwnck version then quit |
| `-l`, `--lua-version`  | Show Lua version then quit |

## Configuration

Scripts are read from the scripts folder, which is customisable by using the
`--folder` option. By default, this folder is `~/.config/devilspie2/`.
(`~/.config` is the default location as defined in the
[XDG Base Directory Specification](https://specifications.freedesktop.org/basedir-spec/latest/)
and returned by the GLib function `g_get_user_config_dir`. As such, it can
also be overridden by the environment.)

This folder will be created if it doesn't already exist.

If `devilspie2` doesn't find any Lua files (`*.lua`) in the folder, it will
stop execution. (Dot-files – those with names beginning with `.` – are
ignored.)

If there is a file named `devilspie2.lua` in this folder, it is read and
executed first. You can choose to have all script functionality in this
file, `devilspie2.lua`, or you can split it up into several: in particular,
if you want actions to be taken on events other than opening a window,
you'll need to create scripts for each event type and you'll also need to
list which of those scripts are to be run on each event type. This is done
via the following variables:

* `scripts_window_close`
* `scripts_window_focus`
* `scripts_window_blur`
* `scripts_window_name_change`

It is expected that these variables are tables containing strings. The
files named in these tables are expected to be in the scripts folder and
will only be called when the respective events occur.

*All other* Lua script files in the scripts folder will be called whenever a
window is opened.

For example:

```lua
scripts_window_close = {
   "file1.lua",
   "file2.lua"
}
```

With this, both `file1.lua` and `file2.lua` will be called whenever a window
is closed.

As of v0.46, each script has 5 seconds to do its job and exit or it will be
unceremoniously interrupted.

## Scripting

The scripting language used is [Lua](https://www.lua.org/).
* FAQ:           https://www.lua.org/FAQ.html
* Documentation: https://www.lua.org/docs.html
* Tutorials:     http://lua-users.org/wiki/TutorialDirectory

Tips:

* If you're going to be testing certain window properties a lot, it's best to
  assign those property values to variables and then test against these.
* String comparison is case-sensitive.  Comparing `SomeProgram` with
  `someprogram` will not report equality.


The following commands are recognised by the Devilspie2 Lua interpreter:

### Debugging

First, a function to show some debug info:

* `debug_print(string)`
  <a name="user-content-debug-print" />

  Debug helper which prints a string to `stdout` if `devilspie2` is run with
  the `--debug` option; otherwise does nothing.

### Getters

Then there are the functions to get the properties of a window, and related
information:

* `get_window_name()`
  <a name="user-content-get-window-name" />

  Returns a string containing the name of the current window.

* `get_window_has_name()`
  <a name="user-content-get-window-has-name" />

  Returns a boolean value indicating whether the window has a name.

  *(Available from version 0.20)*

* `get_application_name()`
  <a name="user-content-get-application-name" />

  Returns the application name of the current window.

* `get_process_name()`
  <a name="user-content-get-process-name" />

  Returns the name of the process owning the current window.

  On (at least) Linux, the process name is read from `/proc/<pid>/comm`. If
  that's not possible, `ps` is launched in a shell. For this reason, you
  should avoid calling `get_process_name()` more than necessary.

  *This function is not compatible with busybox `ps`.*

  *(Available from version 0.44)*

* `get_window_geometry()`
  <a name="user-content-get-window-geometry" />

  Returns the window geometry as four numbers - x-position, y-position,
  width and height. For example, you can do something like this:

  ```lua
  x, y, width, height = get_window_geometry();
  print("X: "..x..", Y: "..y..", width: "..width..", height: "..height);
  ```

  *(Available from version 0.16)*

* `get_window_client_geometry()`
  <a name="user-content-get-window-client-geometry" />

  Returns the window geometry excluding the window manager borders as four
  numbers, x-position, y-position, width and height.

  See [`get_window_geometry`](#user-content-get-window-geometry) for an
  example on how to use this function.

  *(Available from version 0.16)*

* `get_window_frame_extents()`
  <a name="user-content-get-window-frame-extents" />

  Returns the window frame extents as four numbers: left, right, top, bottom.

  *(Available from version 0.45.)*

* `get_window_is_maximised`
  <a name="user-content-get-window-is-maximised`

  Returns `true` if the window is maximised, `false` otherwise.

  *(Available from version 0.21; -`ise` from 0.45)*

* `get_window_is_maximised_vertically()`
  <a name="user-content-get-window-is-maximised-vertically" />

  Returns `true` if the window is vertically maximised, `false` otherwise.

  *(Available from version 0.21; -`ise` from 0.45)*

* `get_window_is_maximised_horizontally()`
  <a name="user-content-get-window-is-maximised-horizontally" />

  Returns `true` if the window is horizontally maximised, `false` otherwise.

  *(Available from version 0.21; -`ise` from 0.45)*

* `get_window_is_decorated()`
  <a name="user-content-get-window-is-decorated" />

  Returns `true` if the window is decorated, `false` otherwise.

  *(Available from version 0.44.)*

* `get_window_type()`
  <a name="user-content-get-window-type" />

  Returns the type of the window. The result type is a string, and can
  be one of the following:
  * `WINDOW_TYPE_NORMAL`
  * `WINDOW_TYPE_DESKTOP`
  * `WINDOW_TYPE_DOCK`
  * `WINDOW_TYPE_DIALOG`
  * `WINDOW_TYPE_TOOLBAR`
  * `WINDOW_TYPE_MENU`
  * `WINDOW_TYPE_UTILITY`
  * `WINDOW_TYPE_SPLASHSCREEN`
  * `WINDOW_TYPE_UNRECOGNIZED` (if libwnck didn't recognise the type)
  * `WINDOW_ERROR` (if there's no window to work on)

  *(Available from version 0.21)*

* `get_class_instance_name()`
  <a name="user-content-get-class-instance-name" />

  Get the class instance name from the WM_CLASS property for the current
  window.

  *(Available from version 0.21; requires libwnck 3+)*

* `get_class_group_name()`
  <a name="user-content-get-class-group-name" />

  Get the class group name from the WM_CLASS Property for the current
  window.

  *(Available from version 0.21; requires libwnck 3+)*

* `get_window_property(string property)`
  <a name="user-content-get-window-property" />

  Returns the value of the named window property. For a list of available
  properties, see the [Freedesktop EWMH specification](http://standards.freedesktop.org/wm-spec/wm-spec-latest.html).

  From 0.45, returns `nil` if the property doesn't exist.

  *(Available from version 0.21)*

* `window_property_is_utf8(string property)`
  <a name="user-content-window-property-is-utf8" />

  Returns whether the named window property is UTF-8.
  (Always returns `true` for properties which are converted to string)

  Returns `nil` if the property doesn't exist.

  *(Available from version 0.45)*

* `get_window_property_full(string property)`
  <a name="user-content-get-window-property-full" />

  Returns a list suitable for assignment to two variables, equivalent to calling
  ```lua
  get_window_property(property), window_property_is_utf8(property)
  ```
  Returns `nil` if the property doesn't exist.

  *(Available from version 0.45)*

* `get_window_role()`
  <a name="user-content-get-window-role" />

  Returns a string describing the current window role of the matched window as
  defined by its `WM_WINDOW_ROLE` hint.

* `get_window_xid()`
  <a name="user-content-get-window-xid" />

  Returns the X window ID of the current window.

* `get_window_class()`
  <a name="user-content-get-window-class" />

  Returns a string representing the class of the current window.

* `get_workspace_count()`
  <a name="user-content-get-workspace-count" />

  Returns the number of workspaces available.

  *(Available from version 0.27)*

* `get_screen_geometry()`
  <a name="user-content-get-screen-geometry" />

  Returns the screen geometry (two numbers) for the screen of the
  current window.

  *(Available from version 0.29)*

* `get_window_fullscreen()`
  <a name="user-content-get-window-fullscreen" />

  Returns `true` if the window is fullscreen, `false` otherwise.

  *(Available from version 0.32)*

* `get_monitor_index()`
  <a name="user-content-get-monitor-index" />

  Returns the index of the monitor containing the window centre (or some
  part of the window).

  *(Available from version 0.44)*

* `get_monitor_geometry([int index])`
  <a name="user-content-get-monitor-geometry" />

  Returns x, y, width, height for the given monitor or,
  without parameters, for the window's monitor.

  If the index is out of range, nothing is returned.

  *(Available from version 0.44 without parameter)*

### Setters

The rest of the commands are used to modify the properties of the windows:

* `set_adjust_for_decoration([bool])`
  <a name="user-content-set-adjust-for-decoration" />

  Allow for situations where moving or resizing the window is done
  incorrectly, i.e.
  ```lua
  set_window_position(0,0)
  ```
  results in the window decoration being taken into account twice, i.e.
  the window (including decoration) is offset from the top left corner by
  the width of the left side decoration and the height of the title bar.

  This is currently off by default, and is sticky: if you do not explicitly
  set it in your script, its current value is retained.

  If used, it should be used at the start of the script.

  This affects the following functions:
  * [`set_window_geometry`](#user-content-set-window-geometry)
  * [`set_window_position`](#user-content-set-window-position)
  * [`set_window_size`](#user-content-set-window-size)
  * [`xy`](#user-content-xy)
  * [`xywh`](#user-content-xywh)

  *(Available from version 0.45)*

* `set_window_position(int xpos, int ypos, [int index])`
  <a name="user-content-set-window-position" />

  Set the position of a window.

  If `index` is specified then the co-ordinates are relative to a corner of
  the specified monitor (counting from 1) on the current workspace. Which
  corner is determined by the co-ordinates' signs:
  * +ve `X` ⇒ left, -ve `X` ⇒ right;
  * +ve `Y` ⇒ top,  -ve `Y` ⇒ bottom.

  **NOTE:** since `-0` would have a use here but is equal to `+0`, `~`
  (bitwise NOT) is used. To put the window 60 pixels from the right or bottom,
  use `~60` or `-61`.

  If `index` = `0` then the ‘current’ monitor (with the window's centre
  point) is used (falling back on then the first monitor showing part of the
  window then the first monitor).

  If `index` = `-1` then all monitors are treated as one large virtual
  monitor.

  *(`index` parameter available from 0.46)*

* `set_window_position2(int xpos, int ypos, [int index])`
  <a name="user-content-set-window-position2" />

  Set the position of a window. Parameters are as for
  [`set_window_position`](#user-content-set_window_position).

  This function uses `XMoveWindow` instead of `wnck_window_set_geometry`; this
  gives a slightly different result.

  *(Available from version 0.21, `index` from 0.46)*

* `set_window_property(string property, int-or-string value, [bool utf8])`
  <a name="user-content-set-window-property" />

  Set a property of a window to a string or a cardinal (32-bit integer or
  boolean).

  Optionally takes a boolean to indicate `UTF8_STRING` properties. The
  default is initially `false` and can be set via
  [`use_utf8()`](#user-content-use-utf8). Ignored for non-string values.

  *(Available from version 0.44; UTF-8 option available from version 0.45.)*

* `delete_window_property(string property)`
  <a name="user-content-delete-window-property" />

  Remove a property from a window.

  *(Available from version 0.44)*

* `set_window_size(int width, int height)`
  <a name="user-content-set-window-size" />

  Set the size of a window.

* `set_window_geometry(int xpos, int ypos, int width, int height, [int index])`
  <a name="user-content-set-window-geometry" />

  Set both size and position of a window in one command.

  The `index` parameter works exactly as for
  [`set_window_position()`](#user-content-set-window-position) and affects
  the given coordinates in the same way.

  *(`index` parameter available from 0.46)*

* `set_window_geometry2(int xpos, int ypos, int width, int height, [int index])`
  <a name="user-content-set-window-geometry2 " />

  Set the window geometry as for
  [`set_window_geometry()`](#user-content-set-window-geometry), but using
  `XMoveResizeWindow` instead of its libwnck alternative.  This may result
  in different coordinates, more like the original devilspie geometry
  function.

  *(Available from version 0.21; `index` from 0.46)*

* `shade()`
  <a name="user-content-shade" />

  “Shade” a window, showing only the title-bar.

* `unshade()`
  <a name="user-content-unshade" />

  Unshade a window; the opposite of [`shade()`](#user-content-shade).

* `maximise()`
  <a name="user-content-maximise" />

  Maximise a window.

  (-`ise` from 0.45)

* `unmaximise()`
  <a name="user-content-unmaximise" />

  Unmaximise a window.

  (-`ise` from 0.45)

* `maximise_vertically()`
  <a name="user-content-maximise-vertically" />

  Maximise the current window vertically.

  (-`ise` from 0.45)

* `maximise_horizontally()`
  <a name="user-content-maximise-horizontally" />

  Maximise the current window horizontally.

  (-`ise` from 0.45)

* `minimise()`
  <a name="user-content-minimise" />

  Minimise a window.

  (-`ise` from 0.45)

* `unminimise()`
  <a name="user-content-unminimise" />

  Unminimise a window: brings it back to screen from the minimised
  position/size.

  (-`ise` from 0.45)

* `decorate_window()`
  <a name="user-content-decorate-window" />

  Show all (relevant) window decoration.

* `undecorate_window()`
  <a name="user-content-undecorate-window" />

  Hide all window decoration.

* `close_window()`
  <a name="user-content-close-window" />

  Close the window.

  *(Available from 0.31)*

* `set_window_workspace(int-or-string workspace)`
  <a name="user-content-set-window-workspace" />

  Move a window to another workspace.
  Indicated as a 1-based number or a workspace name.

* `change_workspace(int-or-string workspace)`
  <a name="user-content-change-workspace" />

  Change the current workspace to another.
  Indicated as a 1-based number or a workspace name.

* `pin_window()`
  <a name="user-content-pin-window" />

  Ask the window manager to put the window on all workspaces.

* `unpin_window()`
  <a name="user-content-unpin-window" />

  Ask the window manager to put window only in the currently active
  workspace.

* `stick_window()`
  <a name="user-content-stick-window" />

  Ask the window manager to keep the window's position fixed on the screen,
  even when the workspace or viewport scrolls.

* `unstick_window()`
  <a name="user-content-unstick-window" />

  Ask the window manager not to have the window's position fixed on the screen
  when the workspace or viewport scrolls.

* `set_skip_tasklist(bool skip)`
  <a name="user-content-set-skip-tasklist" />

  Set this to `true` if you would like the window to skip listing in your
  tasklist, or `false` if not.

  *(Available from version 0.16)*

* `set_skip_pager(bool skip)`
  <a name="user-content-set-skip-pager" />

  Set this to `true` if you would like the window to skip listing in your
  pager, or `false` if not.

  *(Available from version 0.16)*

* `set_window_above([bool above = true])`
  <a name="user-content-set-window-above" />

  Set the current window “always on top” (moves it to the top layer, above
  most windows) or, if `above` = `false`, clears “always on top” and “always
  below” (moves it to the middle, default, layer).

  *(Available from version 0.21)*

* `set_window_below([bool below = true])`
  <a name="user-content-set-window-below" />

  Set the current window “always below” (moves it to the bottom layer,
  below most windows) or, if `below` = `false`, clears “always on top” and
  “always below” (moves it to the middle, default, layer).

  *(Available from version 0.21)*

* `set_on_top()`
  <a name="user-content-set-on-top" />

  Raise the window to the top of its layer.

  (Prior to version 0.45, this was the same as `set_window_above`.)

* `set_on_bottom()`
  <a name="user-content-set-on-bottom" />

  Lower a window to the bottom of its layer.

  *(Available from version 0.45.)*

* `set_window_fullscreen(bool fullscreen)`
  <a name="user-content-set-window-fullscreen" />

  Ask the window manager to set or clear the fullscreen state of the
  window according to `fullscreen`.

  *(Available from version 0.24)*

* `set_viewport(int viewport)`
* `set_viewport(int x, int y)`
  <a name="user-content-set-viewport" />

  **With one parameter,** move the window to the requested viewport.
  Counting starts at 1.

  *(Available from version 0.25)*

  **With two parameters,** move the window to the requested position
  within the viewport.

  *(Available from version 0.40)*

* `centre([int index = -1,] [string direction = nil])`
  <a name="user-content-centre" />

  Centre the window on one monitor or across all monitors, according to the
  following rules:

  * If `index` = `-1`, all monitors are treated as one large virtual monitor.
  * If `index` = `0`, the ‘current’ monitor (with the window's centre point)
    is used (falling back on then the first monitor showing part of the
    window then the first monitor);
  * If `index` is out of range then the first monitor is used.
  * Otherwise, the window is centred on the specified monitor.

  * If `direction` begins with `H` or `h`, the window is horizontally
    centred only.
  * If `direction` begins with `V` or `v`, the window is vertically
    centred only.
  * Otherwise it is centred along both axes.

  If centring only along one axis, the window may be moved along the other
  axis to ensure that it is on the specified monitor.

  *(Available from version 0.40; as `center` and without parameters from 0.26)*

* `set_window_opacity(float value)`
  <a name="user-content-set-window-opacity" />

  Set the window opacity to the given fractional value.
  `1.0` is completely opaque, `0.0` is completely transparent.

  *(Available from version 0.29; as `set_opacity` from 0.28)*

* `set_window_type(string type)`
  <a name="user-content-set-window-type" />

  Set the window type, according to `_NET_WM_WINDOW_TYPE`. The allowed types
  are the standard `_NET_WM` ones (formatted as a string):

  * `_NET_WM_WINDOW_TYPE_DESKTOP`
  * `_NET_WM_WINDOW_TYPE_DOCK`
  * `_NET_WM_WINDOW_TYPE_TOOLBAR`
  * `_NET_WM_WINDOW_TYPE_MENU`
  * `_NET_WM_WINDOW_TYPE_UTILITY`
  * `_NET_WM_WINDOW_TYPE_SPLASH`
  * `_NET_WM_WINDOW_TYPE_DIALOG`
  * `_NET_WM_WINDOW_TYPE_NORMAL`

  You may omit `_NET_WM_`.

  *(Available from version 0.28)*

* `focus_window()`
  <a name="user-content-focus-window" />

  Focus the current window.

  *(Available from version 0.30)*

* `set_window_strut(int left, int right, int top, int bottom, int ...)`
  <a name="user-content-set-window-strut" />

  Set the reserved area at the borders of the desktop for a docking area such
  as a taskbar or a panel. *Will handle up to 12 values.*

  Default minimum values are 0 and default maximum values are taken
  from the screen size (current or maximum, depending on whether
  `xrandr` is used).

  *(Available from version 0.32)*

* `get_window_strut()`
  <a name="user-content-get-window-strut" />

  Get the reserved area at the borders of the desktop for a docking
  area such as a taskbar or a panel.

  Returns a table (12 integers as for `_NET_WM_WINDOW_STRUT_PARTIAL`) or
  `nil`. If `_NET_WM_WINDOW_STRUT` was read then defaults are used as for
  [`set_window_strut()`](#user-content-set-window-strut).

  *(Available from version 0.45)*

* `xy([x, y])`
  <a name="user-content-xy" />

  With parameters, set the position of a window.

  Without, returns the position of a window.

* `xywh([int x, int y, int w, int h])`
  <a name="user-content-xywh" />

  With parameters, set the position and size of a window.

  When no parameters are passed, returns the position and size of a window.

### Utilities

* `use_utf8([bool])`
  <a name="user-content-use-utf8" />

  Controls whether string-setting functions assume UTF-8 by default.
  If no value is supplied, the setting is left unchanged.
  Returns the previous value.

  This is initially `false`.

  *(Available from version 0.45)*

* `millisleep(int time)`
  <a name="user-content-millisleep" />

  Sleep for a number of milliseconds, between 1 and 1000 (1 second).

  This is a convenience function so that you don't have to use `os.execute`
  (to run `sleep`) or (from LuaPosix `posix.time`) `nanosleep`.

  *(Available from version 0.46)*

### Function aliases

* [`get_window_is_maximized`](#user-content-get_window_is_maximised)
  <a name="get_window_is_maximized" />
* [`get_window_is_maximized_vertically`](#user-content-get_window_is_maximised_vertically)
  <a name="get_window_is_maximized_vertically" />
* [`get_window_is_maximized_horizontally`](#user-content-get_window_is_maximised_horizontally)
  <a name="get_window_is_maximized_horizontally" />
* [`get_fullscreen`](#user-content-get-window-fullscreen`)
  <a name="get_fullscreen" />
* [`maximize`](#user-content-maximise)
  <a name="maximize" />
* [`unmaximize`](#user-content-unmaximise)
  <a name="unmaximize" />
* [`maximize_vertically`](#user-content-maximise-vertically)
  <a name="maximize_vertically" />
* [`maximize_horizontally`](#user-content-maximise_horizontally)
  <a name="maximize_horizontally" />
* [`minimize`](#user-content-minimise)
  <a name="minimize" />
* [`unminimize`](#user-content-unminimise)
  <a name="unminimize" />
* [`center`](#user-content-centre)
  <a name="center" />
* [`set_opacity`](#user-content-set-window-opacity`)
  <a name="set_opacity" />
* [`focus`](#user-content-focus-window`)
  <a name="focus" />


### Simple script examples

**Showing debug output and resizing and maximisation of specific windows:**

```lua
-- the debug_print command only prints to stdout
-- if devilspie2 is run using the '--debug' option
debug_print("Window name: " .. get_window_name());
debug_print("Application name: " .. get_application_name())

-- I want my Xfce4 terminal to the right on the second screen (1080p) of my
-- two-monitor setup.
-- Note that this rule will only work with the window's initial title.
if (get_window_name() == "Terminal") then
   set_window_position(1300, 200, 2)
   set_window_size(600, 800)
end

-- Make Firefox always start maximised.
if (get_application_name() == "Firefox") then
   maximise() -- maximize() for compatibility with <0.45
end
```

**Showing handling of conflict between `devilspie2` and other programs (in this case,
`emacs`):**

This example uses [millisleep](#user-content-millisleep) to enforce a short
delay.

```lua
-- Make Emacs (emacs or emacs-gtk) always start maximised.
win_class = get_class_instance_name()

debug_print("Window class: " .. win_class)

if win_class == "emacs" or win_class == "Emacs" then
  -- Emacs applies default window size etc. after a brief delay,
  -- potentially overriding devilspie2.
  --
  -- A brief pause (here, of 0.1s) ensures that devilspie2's actions on the
  -- window take effect after Emacs completes its initialisation. A shorter
  -- pause may work, or a longer one may be needed. Experiment! Could be
  -- that 'millisleep(10)' (0.01s) works well on one PC…?
  --
  -- If you prefer, you can have Emacs maximise its window (as in this
  -- example) via one of its configuration files - early-init.el, which is
  -- normally faster (and avoids a possible visual effect), or init.el –
  -- using this LISP statement:
  --   (push '(fullscreen .  maximized) default-frame-alist)
  --
  -- 'millisleep' is new to 0.46. The 0.1s delay for older versions:
  --   option 1:
  --     os.execute("sleep 0.1")
  --   option 2 (needs luaposix):
  --     nanosleep = require "posix.time".nanosleep
  --     nanosleep{tv_nsec=100e6}
  millisleep(100)
  maximise() -- maximize() for compatibility with <0.45
end
```

## Translations

`devilspie2` is translatable using `gettext` - see
[README.translators.md](README.translators.md) for more information.

## Authors

See [AUTHORS](AUTHORS).

## Contact

* Homepage: http://www.nongnu.org/devilspie2
* Contact / Mailing list: devilspie2-discuss@nongnu.org,
  https://lists.nongnu.org/mailman/listinfo/devilspie2-discuss
* See also: https://github.com/dsalt/devilspie

* IRC: irc://irc.libera.chat/#devilspie2

