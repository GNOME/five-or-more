[GtkTemplate (ui = "/org/gnome/five-or-more/ui/five-or-more.ui")]
public class GameWindow : Gtk.ApplicationWindow
{
    [GtkChild]
    private Gtk.HeaderBar headerbar;

    [GtkChild]
    private Gtk.Box preview_hbox;

    [GtkChild]
    private Gtk.Box hbox;

    [GtkChild]
    private Games.GridFrame grid_frame;

    [GtkChild]
    private Gtk.Label scorelabel;

    private Settings? settings = null;
    private bool window_tiled;
    public bool window_maximized { get; private set; }
    public int window_width { get; private set; }
    public int window_height { get; private set; }

    private Game? game = null;
    private ThemeRenderer? theme = null;

    private Games.Scores.Context highscores;
    private string[] status = {
            _("Match five objects of the same type in a row to score!"),
            _("You can't move there!"),
            _("Game Over!"),
            null
    };

    public GameWindow (Gtk.Application app, Settings settings)
    {
        Object (application: app);

        this.settings = settings;
        game = new Game (settings);
        theme = new ThemeRenderer (settings);

        set_default_size (settings.get_int ("window-width"), settings.get_int ("window-height"));
        if (settings.get_boolean ("window-is-maximized"))
            maximize ();

        NextPiecesWidget next_pieces_widget = new NextPiecesWidget (settings, game, theme);
        preview_hbox.pack_start (next_pieces_widget);
        next_pieces_widget.realize ();
        next_pieces_widget.show ();

        grid_frame.set (game.n_cols, game.n_rows);
        /* it depends on which one changes last, so it is better to call them both */
        game.notify["n-cols"].connect ((s, p) => { grid_frame.set (game.n_cols, game.n_rows); });
        game.notify["n-rows"].connect ((s, p) => { grid_frame.set (game.n_cols, game.n_rows); });
        game.notify["score"].connect ((s, p) => { scorelabel.set_text (game.score.to_string ()); });
        game.notify["status-message"].connect ((s, p) => { set_status_message (status[game.status_message]); });
        set_status_message (status[game.status_message]);
        hbox.pack_start (grid_frame);

        View game_view = new View (settings, game, theme);
        grid_frame.add (game_view);
        game_view.show ();

        grid_frame.show ();

        var importer = new Games.Scores.DirectoryImporter ();
        highscores = new Games.Scores.Context.with_importer ("five-or-more",
                                                _("Board Size: "),
                                                this,
                                                create_category_from_key,
                                                Games.Scores.Style.POINTS_GREATER_IS_BETTER,
                                                importer);
        game.game_over.connect (score_cb);
    }

    protected override bool window_state_event (Gdk.EventWindowState event)
    {
        if ((event.changed_mask & Gdk.WindowState.MAXIMIZED) != 0)
            window_maximized = (event.new_window_state & Gdk.WindowState.MAXIMIZED) != 0;

        if ((event.changed_mask & Gdk.WindowState.TILED) != 0)
            window_tiled = (event.new_window_state & Gdk.WindowState.TILED) != 0;

        return false;
    }

    protected override void size_allocate (Gtk.Allocation allocation)
    {
        base.size_allocate (allocation);

        if (window_maximized || window_tiled)
            return;

        window_width = allocation.width;
        window_height = allocation.height;
    }

    private void score_cb ()
    {

        string name = category_name_from_key (game.score_current_category);
        var current_category = new Games.Scores.Category (game.score_current_category, name);
        highscores.add_score (game.score,
                              current_category,
                              new Cancellable ());

        show_scores ();
    }

    public void restart_game ()
    {
        game.restart ();
    }

    private void set_status_message (string? message)
    {
        headerbar.set_subtitle (message);
    }

    public void show_scores ()
    {
        highscores.run_dialog ();
    }


    private Games.Scores.Category? create_category_from_key (string key)
    {
        string? name = category_name_from_key (key);
        return new Games.Scores.Category (key, name);
    }

    private string category_name_from_key (string key)
    {
        for (int i = 0; i < game.n_categories; i++) {
            if (Game.scorecats[i].key == key)
            {
                return Game.scorecats[i].name;
            }
        }
        return "";
    }
}
