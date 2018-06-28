namespace FiveOrMore
{

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

    private void size_cb (BoardSize size)
    {
        if (!settings.set_int ("size", size))
            warning ("Failed to set size: %d", size);
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

        radiobutton_small.toggled.connect (() => { size_cb (SMALL); });
        radiobutton_medium.toggled.connect (() => { size_cb (MEDIUM); });
        radiobutton_large.toggled.connect (() => { size_cb (LARGE); });
    }
}

} // namespace FiveOrMore
