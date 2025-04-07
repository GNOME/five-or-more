/*
 * Color lines for GNOME
 * Copyright © 1999 Free Software Foundation
 * Authors: Robert Szokovacs <szo@szo.hu>
 *          Szabolcs Ban <shooby@gnome.hu>
 *          Karuna Grewal <karunagrewal98@gmail.com>
 *          Ruxandra Simion <ruxandra.simion93@gmail.com>
 * Copyright © 2007 Christian Persch
 *
 * This game is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

private class FiveOrMoreApp: Gtk.Application
{
    /* Translators: name of the application, as displayed in the About dialog, some window managers, etc. */
    private const string PROGRAM_NAME = _("Five or More");

    internal const string KEY_SIZE = "size";
    internal const string KEY_BACKGROUND_COLOR = "background-color";
    internal const string KEY_THEME = "ball-theme";

    private GameWindow window;

    private const GLib.ActionEntry action_entries[] =
    {
        { "help",           help_cb         },
        { "about",          about_cb        },
        { "quit",           quit            }
    };

    private static int main (string[] args)
    {
        Intl.setlocale (LocaleCategory.ALL, "");
        Intl.bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIRECTORY);
        Intl.bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        Intl.textdomain (GETTEXT_PACKAGE);

        Environment.set_application_name (PROGRAM_NAME);
        Environment.set_prgname ("org.gnome.five-or-more");
        Gtk.Window.set_default_icon_name ("org.gnome.five-or-more");

        FiveOrMoreApp app = new FiveOrMoreApp ();
        return app.run (args);
    }

    private FiveOrMoreApp ()
    {
        Object (application_id: "org.gnome.five-or-more", flags: ApplicationFlags.FLAGS_NONE);
    }

    protected override void startup ()
    {
        base.startup ();

        add_action_entries (action_entries, this);
        set_accels_for_action ("win.new-game",  { "<Primary>n"  });
        set_accels_for_action ("app.quit",      { "<Primary>q"  });
        set_accels_for_action ("app.help",      {          "F1" });
    }

    protected override void activate ()
    {
        if (window == null)
        {
            window = new GameWindow ();
            add_window (window);
        }

        window.present ();
    }

    private void help_cb ()
    {
        try
        {
            Gtk.show_uri_on_window (window, "help:five-or-more", Gtk.get_current_event_time ());
        }
        catch (GLib.Error e)
        {
            GLib.warning ("Unable to open help: %s", e.message);
        }
    }

    private void about_cb ()
    {
        string [] authors = {
            /* Translators: About dialog text, name and email of one of the authors */
            _("Robert Szokovacs <szo@szo.hu>"),

            /* Translators: About dialog text, name and email of one of the authors */
            _("Szabolcs B\xc3\xa1n <shooby@gnome.hu>")
        };

        string [] artists = {
            /* Translators: About dialog text, name of one of the artists */
            _("Callum McKenzie"),

            /* Translators: About dialog text, name of a website that provided some artworks */
            _("kenney.nl"),

            /* Translators: About dialog text, name of one of the artists */
            _("Robert Roth")
        };

        string [] documenters = {
            /* Translators: About dialog text, name of one of the documenters */
            _("Tiffany Antopolski"),

            /* Translators: About dialog text, name of one of the documenters */
            _("Lanka Rathnayaka")
        };

        /* Translators: About dialog text, first line of the copyright notice */
        string copyright = _("Copyright © 1997–2008 Free Software Foundation, Inc.") + "\n"

        /* Translators: About dialog text, second line of the copyright notice */
                         + _("Copyright © 2013–2014 Michael Catanzaro");

        Gtk.show_about_dialog (window,
                               "program-name", PROGRAM_NAME,
                               "logo-icon-name", "org.gnome.five-or-more",
                               "version", VERSION,
                               /* Translators: About dialog text, describing the application */
                               "comments", _("GNOME port of the once-popular Color Lines game"),
                               "copyright", copyright,
                               "license-type", Gtk.License.GPL_2_0,
                               "authors", authors,
                               "artists", artists,
                               "documenters", documenters,
                               /* Translators: about dialog text; this string should be replaced by a text crediting yourselves and your translation team, or should be left empty. Do not translate literally! */
                               "translator-credits", _("translator-credits"),
                               "website", "https://wiki.gnome.org/Apps/Five%20or%20more");
    }

    protected override void shutdown ()
    {
        if (window != null)
            window.on_shutdown ();

        base.shutdown ();
    }
}
