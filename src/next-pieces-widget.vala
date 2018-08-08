public class NextPiecesWidget : Gtk.DrawingArea
{
    private Settings settings;
    private Game? game;
    private ThemeRenderer? theme;

    private Gee.ArrayList<Piece> local_pieces_queue;
    private int widget_height = -1;

    public NextPiecesWidget (Settings settings, Game game, ThemeRenderer theme)
    {
        this.settings = settings;
        this.game = game;
        this.theme = theme;

        set_queue_size ();
        settings.changed[FiveOrMoreApp.KEY_SIZE].connect (() => {
            set_queue_size ();
            queue_draw ();
        });

        local_pieces_queue = game.next_pieces_queue;
        queue_changed_cb (local_pieces_queue);

        game.queue_changed.connect (queue_changed_cb);
    }

    private void set_queue_size ()
    {
        set_size_request (ThemeRenderer.DEFAULT_SPRITE_SIZE * game.n_next_pieces, ThemeRenderer.DEFAULT_SPRITE_SIZE);
    }

    private void queue_changed_cb (Gee.ArrayList<Piece> next_pieces_queue)
    {
        local_pieces_queue = next_pieces_queue;
        queue_draw ();
    }

    public override bool draw (Cairo.Context cr)
    {
        if (theme == null)
            return false;

        if (widget_height == -1)
        {
            widget_height = this.get_allocated_height ();
        }

        Gdk.RGBA background_color = Gdk.RGBA ();
        background_color.red = background_color.green = background_color.blue = background_color.alpha = 0;
        Gdk.cairo_set_source_rgba (cr, background_color);
        cr.paint ();

        for (int i = 0; i < local_pieces_queue.size; i++)
        {
            theme.render_sprite (cr,
                                 local_pieces_queue[i].id,
                                 0,
                                 i * ThemeRenderer.DEFAULT_SPRITE_SIZE,
                                 (widget_height / 2) - (ThemeRenderer.DEFAULT_SPRITE_SIZE / 2),
                                 ThemeRenderer.DEFAULT_SPRITE_SIZE);

        }

        cr.stroke ();

        return true;
    }
}
