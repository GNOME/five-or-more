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
        {"scores", scores_cb            },
        {"preferences", preferences_cb  },
        {"help", help_cb                },
        {"about", about_cb              },
        {"quit", quit                   }
    };

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
    }

    public static int main (string[] args)
    {
        Environment.set_application_name (_("Five or More"));
        Gtk.Window.set_default_icon_name ("five-or-more");

        var app = new FiveOrMoreApp ();
        return app.run (args);
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
            null
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
                               "logo-icon-name", "five-or-more",
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
