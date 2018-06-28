namespace FiveOrMore
{

public class NextPiecesWidget : Gtk.DrawingArea
{
    private Game? game;
    private ThemeRenderer? theme;

    private Gee.ArrayList<Piece> local_pieces_queue;
    private int widget_height = 0;
    private bool queue_resized = false;

    public NextPiecesWidget (Game game, ThemeRenderer theme)
    {
        this.game = game;
        this.theme = theme;

        game.set_next_pieces_queue.connect (queue_changed);
    }

    private Gee.ArrayList<Piece> queue_changed (Gee.ArrayList<Piece> next_pieces_queue)
    {
        if (!queue_resized)
        {
            this.set_size_request (ThemeRenderer.ELEM_WIDTH * next_pieces_queue.size, ThemeRenderer.ELEM_HEIGHT);
            queue_resized = true;
        }

        local_pieces_queue = next_pieces_queue;
        queue_draw ();

        int i;
        for (i = 0; i < next_pieces_queue.size; i++)
        {
            stderr.printf ("[DEBUG] %d\n", local_pieces_queue[i].id);
        }
        stderr.printf ("\n");

        return next_pieces_queue;
    }

    public override bool draw (Cairo.Context cr)
    {
        if (theme == null)
            return false;

        if (widget_height == 0)
        {
            widget_height = this.get_allocated_height ();
        }

        Gdk.RGBA background_color = Gdk.RGBA ();
        background_color.red = background_color.green = background_color.blue = background_color.alpha = 0;
        Gdk.cairo_set_source_rgba (cr, background_color);
        cr.paint ();

        int i;
        for (i = 0; i < local_pieces_queue.size; i++)
        {
            theme.render_element (cr,
                                  local_pieces_queue[i].id,
                                  0,
                                  i * ThemeRenderer.ELEM_WIDTH,
                                  (widget_height / 2.0f) - (ThemeRenderer.ELEM_HEIGHT / 2.0f));

        }

        cr.stroke ();

        return true;
    }
}

} // namespace FiveOrMore
