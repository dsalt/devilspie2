Copyright © 2011-2024 devilspie2 developers

This file is distributed under the same licence as the devilspie2 package
(see [COPYING](COPYING)).

# Information for translators

**Note:** some translation work may be done via
https://gitlocalize.com/repo/9900. If you use that service then much of the
text below does not apply.

## Starting fresh

Get a copy of the [devilspie2 source repository](https://github.com/dsalt/devilspie2).
If a `devilspie2.pot` isn't available in the `po` folder (it should be!),
create one:

```sh
cd po
make update-pot
```

When you have made sure you have `devilspie2.pot`, generate a `po` file for
your language using `msginit`:

```sh
msginit -i devilspie2.pot -l LOCALE
```

This copies the `devilspie2.pot` to `LOCALE.po` (it replaces `LOCALE` with
the correct abbreviation for your language & country, e.g. 'sv' for generic
Swedish or `de_CH` for Swiss German) and fills it with information that it
gets from your system.

Then start translating all strings in the generated `po` file using your
favourite text editor, or a “gettext catalogues (.po files) editor” like
[poedit](http://www.poedit.net/).


## Checking your translation for errors and completeness

```sh
msgfmt -c --statistics sv.po -o /dev/null
```

This checks the file (in this example `sv.po`) for errors, outputs the
result statistics to `stdout`, and sends other output to `/dev/null`. (“Other
output” is of no use if just checking for errors.)


## Building Devil's Pie 2 with your translation included

Edit `po/Makefile` to include your translation under the `LANGUAGES`
variable listing.

Then build devilspie2 as usual with `make` and install using `make install`
as root.


## Updating your translation

When it is time to update your translation, you update your language file
from the content of `devilspie2.pot` (in this example the language is
Swedish, and the target file is called `sv.po`):

```sh
msgmerge --update sv.po devilspie2.pot
```

There will normally be an updated devilspie2.pot in the git repository
before each release. If you would like to generate it yourself (which you
should if you're randomly checking for changed strings), just get the latest
git version of `devilspie2`, and then do the following:

```sh
cd po
make -B update-po
```

and then update your po file as described above.

Open your file and locate the strings that need updating. To get the
translation to me you can fork `devilspie2` on github, do the translation,
commit your changes then send a pull request. Please use English in your
commit messages so that I can understand what you have done.
