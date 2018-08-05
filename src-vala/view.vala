public class View : Gtk.DrawingArea
{
    private const int MINIMUM_BOARD_SIZE = 300;

    private Settings settings;
    private Game? game = null;
    private ThemeRenderer? theme = null;
    private Gdk.RGBA background_color;
    private Gtk.StyleContext cs;

    private Gdk.Rectangle board_rectangle;

    private int piece_size = 0;

    private int cell_x;
    private int cell_y;
    private int start_x;
    private int start_y;
    private int end_x;
    private int end_y;

    private int animation_state;
    private uint animation_id;

    public View (Settings settings, Game game, ThemeRenderer theme)
    {
        this.settings = settings;
        this.game = game;
        this.theme = theme;

        settings.changed[FiveOrMoreApp.KEY_THEME].connect (() => {
            game.board.grid_changed ();
            game.queue_changed (game.next_pieces_queue);
            queue_draw ();
        });

        background_color = Gdk.RGBA ();
        set_background_color ();
        settings.changed[FiveOrMoreApp.KEY_BACKGROUND_COLOR].connect (() => {
            set_background_color ();
            queue_draw ();
        });

        set_size_request (MINIMUM_BOARD_SIZE, MINIMUM_BOARD_SIZE);

        board_rectangle = Gdk.Rectangle ();
        board_rectangle.x = board_rectangle.y = 0;
        update_sizes (MINIMUM_BOARD_SIZE, MINIMUM_BOARD_SIZE);
        /* it depends on which one changes last, so it is better to call them both */
        game.notify["n-rows"].connect (() => {
            update_sizes (get_allocated_width (), get_allocated_height ());
            queue_draw ();
        });
        game.notify["n-cols"].connect (() => {
            update_sizes (get_allocated_width (), get_allocated_height ());
            queue_draw ();
        });

        game.board.grid_changed.connect (board_changed_cb);
        add_events (Gdk.EventMask.BUTTON_PRESS_MASK | Gdk.EventMask.BUTTON_RELEASE_MASK);

        game.current_path_cell_pos_changed.connect (current_path_cell_pos_changed_cb);

        start_x = -1;
        start_y = -1;
        end_x = -1;
        end_y = -1;

        animation_state = 0;
        animation_id = -1;
    }

    private void board_changed_cb ()
    {
        queue_draw ();
    }

    private void current_path_cell_pos_changed_cb ()
    {
        queue_draw ();
    }

    private void set_background_color ()
    {
        var color_str = settings.get_string (FiveOrMoreApp.KEY_BACKGROUND_COLOR);
        background_color.parse (color_str);

        cs = this.get_style_context ();
        var provider = new Gtk.CssProvider ();
        try
        {
            provider.load_from_data (".game-view { background-color: %s; }".printf (background_color.to_string ()));
        }
        catch (Error e)
        {
            warning ("Failed to load CSS data to provider");
            return;
        }

        cs.add_class ("game-view");
        cs.add_provider (provider, Gtk.STYLE_PROVIDER_PRIORITY_USER);
    }

    public override bool button_press_event (Gdk.EventButton event)
    {
        if (game == null || game.animating)
            return false;

        /* Ignore the 2BUTTON and 3BUTTON events. */
        if (event.type != Gdk.EventType.BUTTON_PRESS)
            return false;

        cell_x = (int)event.x / piece_size;
        cell_y = (int)event.y / piece_size;

        if (cell_x >= game.n_cols || cell_y >= game.n_rows)
            return false;

        if (game.status_message != StatusMessage.NONE)
            game.status_message = StatusMessage.NONE;

        /* if selected cell is not empty, set start */
        if (game.board.get_piece (cell_y, cell_x) != null)
        {
            if (animation_id != -1)
            {
                Source.remove (animation_id);
                animation_id = -1;
            }

            if (start_x == cell_x && start_y == cell_y)
            {
                start_x = -1;
                start_y = -1;

                animation_state = 0;
                queue_draw ();

                return true;
            }

            start_x = cell_x;
            start_y = cell_y;

            animation_id = Timeout.add (100, animate_clicked);
        }
        /* if selected cell is empty and start is set, and cell is empty, set end */
        else if (game.board.get_piece (cell_y, cell_x) == null && start_x != -1 && start_y != -1)
        {
            end_x = cell_x;
            end_y = cell_y;

            bool move = game.make_move (start_y, start_x, end_y, end_x);

            if (!move)
                return false;

            start_x = -1;
            start_y = -1;

            if (animation_id != -1)
            {
                Source.remove (animation_id);
                animation_id = -1;
            }
        }

        return true;
    }

    private bool animate_clicked ()
    {
        animation_state = (animation_state + 1) % Game.N_ANIMATIONS;
        queue_draw ();

        return Source.CONTINUE;
    }

    private void update_sizes (int width, int height)
    {
        piece_size = (width - 1) / game.n_cols;
        board_rectangle.width = piece_size * game.n_cols;
        board_rectangle.height = piece_size * game.n_rows;
    }

    public override bool configure_event (Gdk.EventConfigure event)
    {
        update_sizes (event.width, event.height);
        queue_draw ();

        return true;
    }

    private void fill_background (Cairo.Context cr)
    {
        cs.render_background (cr, board_rectangle.x, board_rectangle.y, board_rectangle.width, board_rectangle.height);
    }

    private void draw_gridlines (Cairo.Context cr)
    {
        Gdk.RGBA grid_color = cs.get_color (Gtk.StateFlags.NORMAL);
        Gdk.cairo_set_source_rgba (cr, grid_color);
        cr.set_line_width (2.0);

        for (int i = piece_size; i < board_rectangle.width; i += piece_size)
        {
            cr.move_to (i + 1, 0 + 1);
            cr.line_to (i + 1, board_rectangle.height + 1);
        }

        for (int i = piece_size; i < board_rectangle.height; i += piece_size)
        {
            cr.move_to (0 + 1, i + 1);
            cr.line_to (board_rectangle.width + 1, i + 1);
        }

        var line_rectangle = Gdk.Rectangle ();
        line_rectangle.x = line_rectangle.y = 1;
        line_rectangle.width = board_rectangle.width - 1;
        line_rectangle.height = board_rectangle.height - 1;

        Gdk.cairo_rectangle (cr, line_rectangle);
        cr.stroke ();
    }

    private void draw_shapes (Cairo.Context cr)
    {
        for (int row = 0; row < game.n_rows; row++)
        {
            for (int col = 0; col < game.n_cols; col++)
            {
                if (game.board.get_piece (row,col) != null)
                {
                    theme.render_sprite (cr,
                                         game.board.get_piece (row,col).id,
                                         (start_x == col && start_y == row) ? animation_state : 0,
                                         col * piece_size,
                                         row * piece_size,
                                         piece_size);
                }
            }
        }
    }

    private void draw_path (Cairo.Context cr)
    {
        if (game.current_path != null && game.current_path.size > 0 && game.current_path_cell_pos != -1)
        {
            Cell current_cell = game.current_path[game.current_path_cell_pos];
            theme.render_sprite (cr,
                                 game.animating_piece.id,
                                 animation_state,
                                 current_cell.col * piece_size,
                                 current_cell.row * piece_size,
                                 piece_size);
        }
    }

    public override bool draw (Cairo.Context cr)
    {
        if (theme == null)
            return false;

        fill_background (cr);
        draw_gridlines (cr);
        draw_shapes (cr);
        draw_path (cr);

        return true;
    }
}
