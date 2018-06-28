namespace FiveOrMore
{

[GtkTemplate (ui = "/org/gnome/five-or-more/ui/five-or-more-vala.ui")]
public class Window : Gtk.ApplicationWindow
{
    [GtkChild]
    private Gtk.Box preview_hbox;

    [GtkChild]
    private Gtk.Box hbox;

    private Game? game = null;
    private ThemeRenderer? theme = null;

    public Window (Gtk.Application app, Settings settings)
    {
        Object (application: app);

        game = new Game (settings);

        var theme_file = Path.build_filename (DATA_DIRECTORY, "themes", ThemeRenderer.THEME);
        theme = new ThemeRenderer (theme_file);

        NextPiecesWidget next_pieces_widget = new NextPiecesWidget (game, theme);

        game.generate_next_pieces ();
        preview_hbox.pack_start (next_pieces_widget);
        next_pieces_widget.realize ();
        next_pieces_widget.set_visible (true);

        View game_view = new View (game, theme);
        hbox.pack_start (game_view);
        game_view.set_visible (true);

    }
}

} // namespace FiveOrMore
