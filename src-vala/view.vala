public class View : Gtk.DrawingArea
{
    private const int MINIMUM_BOARD_SIZE = 300;

    private Settings settings;
    private Game? game = null;
    private ThemeRenderer? theme = null;
    private Gdk.RGBA background_color;

    private Gdk.Rectangle board_rectangle;

    private int piece_size = 0;

    private int cell_x;
    private int cell_y;
    private int start_x;
    private int start_y;
    private int end_x;
    private int end_y;

    public View (Settings settings, Game game, ThemeRenderer theme)
    {
        this.settings = settings;
        this.game = game;
        this.theme = theme;

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

        /* if selected cell is not empty, set start */
        if (game.board.get_piece (cell_y, cell_x) != null)
        {
            start_x = cell_x;
            start_y = cell_y;
            stderr.printf ("[DEBUG]: pointA %d %d\n", start_y, start_x);
        }
        /* if selected cell is empty and start is set, and cell is empty, set end */
        else if (game.board.get_piece (cell_y, cell_x) == null && start_x != -1 && start_y != -1)
        {
            end_x = cell_x;
            end_y = cell_y;
            stderr.printf ("[DEBUG]: pointB %d %d\n", end_y, end_x);

            bool move = game.make_move (start_y, start_x, end_y, end_x);

            if (!move)
            {
                start_x = -1;
                start_y = -1;

                return false;
            }

            foreach (Cell p in game.current_path)
            {
                stderr.printf ("[DEBUG]: Path %d %d\n", p.row, p.col);
            }

            start_x = -1;
            start_y = -1;
        }

        return true;
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

    public override bool draw (Cairo.Context cr)
    {
        if (theme == null)
            return false;

        Gdk.cairo_set_source_rgba (cr, background_color);
        Gdk.cairo_rectangle (cr, board_rectangle);
        cr.fill ();

        for (int row = 0; row < game.n_rows; row++)
        {
            for (int col = 0; col < game.n_cols; col++)
            {
                if (game.board.get_piece (row,col) != null)
                    theme.render_sprite (cr,
                                         game.board.get_piece (row,col).id,
                                         0,
                                         col * piece_size,
                                         row * piece_size,
                                         piece_size);
            }
        }

        if (game.current_path != null && game.current_path.size > 0 && game.current_path_cell_pos != -1)
        {
            Cell current_cell = game.current_path[game.current_path_cell_pos];
            theme.render_sprite (cr,
                                 game.animating_piece.id,
                                 0,
                                 current_cell.col * piece_size,
                                 current_cell.row * piece_size,
                                 piece_size);
        }

        return true;
    }
}
