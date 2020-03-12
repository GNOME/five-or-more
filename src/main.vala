/*
 * Color lines for GNOME
 * Copyright © 1999 Free Software Foundation
 * Authors: Robert Szokovacs <szo@appaloosacorp.hu>
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

public class FiveOrMoreApp: Gtk.Application
{
    public const string KEY_SIZE = "size";
    public const string KEY_BACKGROUND_COLOR = "background-color";
    public const string KEY_THEME = "ball-theme";

    private Settings settings;

    private GameWindow window;
    private PreferencesDialog? preferences_dialog = null;

    private const GLib.ActionEntry action_entries[] =
    {
        {"new-game", new_game_cb        },
        {"change-size", null, "s", "'SMALL'", change_size_cb },
        {"scores", scores_cb            },
        {"preferences", preferences_cb  },
        {"help", help_cb                },
        {"about", about_cb              },
        {"quit", quit                   }
    };

    public static int main (string[] args)
    {
        Intl.setlocale (LocaleCategory.ALL, "");
        Intl.bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIRECTORY);
        Intl.bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        Intl.textdomain (GETTEXT_PACKAGE);

        Environment.set_application_name (_("Five or More"));
        Gtk.Window.set_default_icon_name ("org.gnome.five-or-more");

        FiveOrMoreApp app = new FiveOrMoreApp ();
        return app.run (args);
    }

    public FiveOrMoreApp ()
    {
        Object (application_id: "org.gnome.five-or-more", flags: ApplicationFlags.FLAGS_NONE);
    }

    public override void activate ()
    {
        window.present ();
    }

    protected override void startup ()
    {
        base.startup ();

        settings = new Settings ("org.gnome.five-or-more");
        window = new GameWindow (this, settings);

        add_action_entries (action_entries, this);
        set_accels_for_action ("app.new-game", {"<Primary>n"});
        set_accels_for_action ("app.quit", {"<Primary>q"});
        set_accels_for_action ("app.help", {"F1"});
        var board_size_action = lookup_action("change-size");
        BoardSize size = (BoardSize)settings.get_int (FiveOrMoreApp.KEY_SIZE);
        (board_size_action as SimpleAction).set_state (new Variant.string(size.to_string()));
    }

    private void new_game_cb ()
    {
        if (window == null)
        {
            warning ("Failed to restart game");
            return;
        }
        window.restart_game ();
    }

    private void scores_cb ()
    {
        window.show_scores ();
    }

    private void change_size_cb (SimpleAction action, Variant? parameter)
    {
        action.set_state (parameter);
        switch (parameter.get_string()) {
            case "BOARD_SIZE_SMALL":
                window.change_size (BoardSize.SMALL);
                break;
            case "BOARD_SIZE_MEDIUM":
                window.change_size (BoardSize.MEDIUM);
                break;
            case "BOARD_SIZE_LARGE":
                window.change_size (BoardSize.LARGE);
                break;
        }
    }

    private void preferences_cb ()
    {
        if (preferences_dialog != null)
        {
            preferences_dialog.present ();
            return;
        }

        preferences_dialog = new PreferencesDialog (settings);
        preferences_dialog.set_transient_for (window);

        preferences_dialog.response.connect (() => {
            preferences_dialog.destroy ();
            preferences_dialog = null;
        });

        preferences_dialog.present ();
    }

    private void help_cb ()
    {
        try
        {
            Gtk.show_uri (window.get_screen (), "help:five-or-more", Gtk.get_current_event_time ());
        }
        catch (GLib.Error e)
        {
            GLib.warning ("Unable to open help: %s", e.message);
        }
    }

    private void about_cb ()
    {
        /* Appears on the About dialog. */
        const string authors[] = {
            "Robert Szokovacs <szo@appaloosacorp.hu>",
            "Szabolcs B\xc3\xa1n <shooby@gnome.hu>",
            null
        };

        const string artists[] = {
            "Callum McKenzie",
            "Kenney.nl",
            "Robert Roth",
        };

        const string documenters[] = {
            "Tiffany Antopolski",
            "Lanka Rathnayaka",
            null
        };

        const string copyright = "Copyright © 1997–2008 Free Software Foundation, Inc.\n
            Copyright © 2013–2014 Michael Catanzaro";

        Gtk.show_about_dialog (window,
                               "program-name", _("Five or More"),
                               "logo-icon-name", "org.gnome.five-or-more",
                               "version", VERSION,
                               "comments", _("GNOME port of the once-popular Color Lines game"),
                               "copyright", copyright,
                               "license-type", Gtk.License.GPL_2_0,
                               "authors", authors,
                               "artists", artists,
                               "documenters", documenters,
                               "translator-credits", _("translator-credits"),
                               "website", "https://wiki.gnome.org/Apps/Five%20or%20more");
    }

    public override void shutdown ()
    {
        settings.set_int ("window-width", window.window_width);
        settings.set_int ("window-height", window.window_height);
        settings.set_boolean ("window-is-maximized", window.window_maximized);

        base.shutdown ();
    }
}
