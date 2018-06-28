namespace FiveOrMore
{
public class FiveOrMoreApp: Gtk.Application
{
    public const string KEY_SIZE = "size";
    public const string KEY_BACKGROUND_COLOR = "background-color";

    private Settings settings;

    private Gtk.ApplicationWindow window;
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
        window = new FiveOrMore.Window (this, settings);

        add_action_entries (action_entries, this);
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

    }

    private void scores_cb ()
    {

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
}

} // namespace FiveOrMore
