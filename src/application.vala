namespace FiveOrMore
{
    public class FiveOrMoreApp : Gtk.Application
    {
        private Gtk.ApplicationWindow window;
        private Settings settings;
        private Gtk.Builder builder;
        //private GnomeGamesSupport.Scores highscores;

        private Gtk.Dialog preferences_dialog;

        private const GLib.ActionEntry[] action_entries =
        {
            { "new-game", new_game_cb },
            { "scores", scores_cb },
            { "preferences", preferences_cb },
            { "help", help_cb },
            { "about", about_cb },
            { "quit", quit_cb }
        };

        //private const GnomeGamesSupport.ScoresCategory scorecats[] =
        //{
        //    { "Small",  NC_("board size", "Small") },
        //    { "Medium", NC_("board size", "Medium") },
        //    { "Large",  NC_("board size", "Large") }
        //};
    
        private const string[] authors = { "Thomas Andersen <phomes@gmail.com>", "Robert Szokovacs <szo@appaloosacorp.hu>", "Szabolcs B\xc3\xa1n <shooby@gnome.hu>" };
        //private const string[] documenters = { "Tiffany Antopolski", "Lanka Rathnayaka" };

        private GlinesBoard board = new GlinesBoard(10, 10, 5, 3);

        public FiveOrMoreApp ()
        {
            Object (application_id: "org.gnome.five-or-more", flags: ApplicationFlags.FLAGS_NONE);

            add_action_entries (action_entries, this);
        }

        protected override void startup ()
        {
            base.startup ();

            settings = new Settings ("org.gnome.five-or-more");

            //highscores = new GnomeGamesSupport.Scores ("glines", scorecats, "board size", null, 0, GnomeGamesSupport.ScoreStyle.PLAIN_DESCENDING);

            builder = new Gtk.Builder ();
            try
            {
                 builder.add_from_resource ("/org/gnome/five-or-more/ui/glines.ui");
                 builder.add_from_resource ("/org/gnome/five-or-more/ui/glines-preferences.ui");
            }
            catch (GLib.Error e)
            {
                GLib.warning ("Could not load UI: %s", e.message);
            }

            var menu = new Menu ();

            var section = new Menu ();
            menu.append_section (null, section);
            section.append (_("_New Game"), "app.new-game");
            section.append (_("_Scores"), "app.scores");
            section.append (_("_Preferences"), "app.preferences");

            section = new Menu ();
            menu.append_section (null, section);
            section.append (_("_Help"), "app.help");
            section.append (_("_About"), "app.about");

            section = new Menu ();
            menu.append_section (null, section);
            section.append (_("_Quit"), "app.quit");
            set_app_menu (menu);

            var box = (Gtk.Box) builder.get_object ("vbox");
            var view2d = new View2D (board);
            box.add (view2d);
            view2d.show ();

            window = (Gtk.ApplicationWindow) builder.get_object ("glines_window");
            add_window (window);
        }

        public override void activate ()
        {
            //var v = new ViewCli (board);
            //v.run();

            window.present ();
        }

        private void new_game_cb ()
        {
            stdout.printf ("new game\n");
        }

        private void scores_cb ()
        {
            stdout.printf ("FIXME: Showing scores does not currently work\n");

            //var scores_dialog = new GnomeGamesSupport.ScoresDialog (window, highscores, _("GNOME Five or More"));
            //scores_dialog.set_category_description (_("_Board size:"));
            //scores_dialog.run ();
            //scores_dialog.destroy ();
        }

        private void preferences_cb ()
        {
            stdout.printf ("preferences\n");
            if (preferences_dialog == null)
                preferences_dialog = (Gtk.Dialog) builder.get_object ("preferences_dialog");
            
            preferences_dialog.run();
            preferences_dialog.destroy ();
        }

        private void help_cb ()
        {
            try
            {
                Gtk.show_uri (window.get_screen (), "ghelp:five-or-more", Gtk.get_current_event_time ());
            }
            catch (GLib.Error e)
            {
                GLib.warning ("Unable to open help: %s", e.message);
            }
        }

        private void about_cb ()
        {
            Gtk.show_about_dialog (window,
                                   "program-name", _("Five or More"),
                                   "logo-icon-name", "five-or-more",
                                   "version", Config.VERSION,
                                   "comments", _("GNOME port of the once-popular Color Lines game.\n\nFive or More is a part of GNOME Games."),
                                   "copyright", "Copyright \xc2\xa9 1997-2012 Free Software Foundation, Inc.",
                                   "license-type", Gtk.License.GPL_3_0,
                                   "wrap-license", false,
                                   "authors", authors,
                                   //FIXME: "documenters", documenters,
                                   "translator-credits", _("translator-credits"),
                                   "website", "http://www.gnome.org/projects/gnome-games/",
                                   "website-label", _("GNOME Games web site"),
                                   null);
        }
        
        private void quit_cb ()
        {
            window.destroy ();
        }
    }
}


