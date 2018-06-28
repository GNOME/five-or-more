namespace FiveOrMore
{

public class View : Gtk.DrawingArea
{
    private Game? game = null;
    private ThemeRenderer? theme = null;

    public View (Game game, ThemeRenderer theme)
    {
        this.game = game;
        this.theme = theme;

        add_events (Gdk.EventMask.BUTTON_PRESS_MASK | Gdk.EventMask.BUTTON_RELEASE_MASK);
    }

    public override bool button_press_event (Gdk.EventButton event)
    {

        if (game == null)
            return false;

        /* Ignore the 2BUTTON and 3BUTTON events. */
        if (event.type != Gdk.EventType.BUTTON_PRESS)
            return false;

        game.generate_next_pieces ();

        return true;
    }

    public override bool draw (Cairo.Context cr)
    {
        if (theme == null)
            return false;

        // theme.render_element (cr, 1, 0, 0, 0);

        return true;
    }
}

} // namespace FiveOrMore
