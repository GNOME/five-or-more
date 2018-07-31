[GtkTemplate (ui = "/org/gnome/five-or-more/ui/preferences-dialog.ui")]
public class PreferencesDialog : Gtk.Dialog
{
    private Settings settings;

    [GtkChild]
    private Gtk.ColorButton color_button;

    [GtkChild]
    private Gtk.RadioButton radiobutton_small;
    [GtkChild]
    private Gtk.RadioButton radiobutton_medium;
    [GtkChild]
    private Gtk.RadioButton radiobutton_large;

    private void color_set_cb (Gtk.ColorButton self)
    {
        var color = self.get_rgba ();
        if (!settings.set_string ("background-color", color.to_string ()))
            warning ("Failed to set color: %s", color.to_string ());
    }

    private void size_cb (Gtk.ToggleButton button, BoardSize size)
    {
        var game_size = settings.get_int ("size");

        if (game_size == size || !button.get_active ())
            return;

        var flags = Gtk.DialogFlags.DESTROY_WITH_PARENT;
        var restart_game_dialog = new Gtk.MessageDialog (this,
                                                         flags,
                                                         Gtk.MessageType.WARNING,
                                                         Gtk.ButtonsType.NONE,
                                                         _("Are you sure you want to restart the game?"),
                                                         null);
        restart_game_dialog.add_buttons (_("_Cancel"), Gtk.ResponseType.CANCEL,
                                         _("_Restart"), Gtk.ResponseType.OK,
                                         null);

        var result = restart_game_dialog.run ();
        restart_game_dialog.destroy ();
        switch (result)
        {
            case Gtk.ResponseType.OK:
                 if (!settings.set_int ("size", size))
                    warning ("Failed to set size: %d", size);
                button.set_active (true);
                break;
            case Gtk.ResponseType.CANCEL:
                Gtk.ToggleButton radiobutton;
                switch (game_size)
                {
                    case BoardSize.SMALL:
                        radiobutton = radiobutton_small;
                        break;
                    case BoardSize.MEDIUM:
                        radiobutton = radiobutton_medium;
                        break;
                    case BoardSize.LARGE:
                        radiobutton = radiobutton_large;
                        break;
                    default:
                        radiobutton = radiobutton_small;
                        break;
                }
                radiobutton.set_active (true);
                break;
        }
    }

    public PreferencesDialog (Settings settings)
    {
        this.settings = settings;

        /* Set up board color */
        var color_str = settings.get_string (FiveOrMoreApp.KEY_BACKGROUND_COLOR);
        Gdk.RGBA color = Gdk.RGBA ();
        color.parse (color_str);
        color_button.set_rgba (color);
        color_button.color_set.connect (color_set_cb);

        /* Set up size radio buttons */
        var size = settings.get_int (FiveOrMoreApp.KEY_SIZE);
        switch (size) {
            case BoardSize.SMALL:
                radiobutton_small.set_active (true);
                break;
            case BoardSize.MEDIUM:
                radiobutton_medium.set_active (true);
                break;
            case BoardSize.LARGE:
                radiobutton_large.set_active (true);
                break;
            default:
                radiobutton_medium.set_active (true);
                break;
        }

        radiobutton_small.toggled.connect ((button) => { size_cb (button, SMALL); });
        radiobutton_medium.toggled.connect ((button) => { size_cb (button, MEDIUM); });
        radiobutton_large.toggled.connect ((button) => { size_cb (button, LARGE); });
    }
}
