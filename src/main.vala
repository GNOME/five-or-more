private static bool show_version;

private static const OptionEntry[] options =
{
    { "version", 'v', 0, OptionArg.NONE, ref show_version,
    /* Help string for command line --version flag */
    N_("Show release version"), null},
    { null }
};

public static int main (string[] args)
{
    Gtk.init (ref args);

    var c = new OptionContext (/* Arguments and description for --help text */
                                   _("[FILE] - Play Glines"));
    c.add_main_entries (options, Config.GETTEXT_PACKAGE);
    c.add_group (Gtk.get_option_group (true));
    try
    {
        c.parse (ref args);
    }
    catch (Error e)
    {
        stderr.printf ("%s\n", e.message);
        stderr.printf (/* Text printed out when an unknown command-line argument provided */
                       _("Run '%s --help' to see a full list of available command line options."), args[0]);
        stderr.printf ("\n");
        return Posix.EXIT_FAILURE;
    }
    if (show_version)
    {
        /* Note, not translated so can be easily parsed */
        stderr.printf ("glines %s\n", Config.VERSION);
        return Posix.EXIT_SUCCESS;
    }

    var app = new FiveOrMore.FiveOrMoreApp ();
    return app.run ();
}

