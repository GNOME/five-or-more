namespace FiveOrMore
{

public class ThemeRenderer
{
    public const string THEME = "balls.svg";
    public const int ELEM_WIDTH = 20;
    public const int ELEM_HEIGHT = 20;

    private Rsvg.Handle? theme = null;
    private float width;
    private float height;

    private Cairo.Pattern? tile_pattern = null;
    private Gdk.Rectangle? preview_rect = null;

    public ThemeRenderer (string theme_file)
    {
        try
        {
            theme = new Rsvg.Handle.from_file (theme_file);
            var dimensions = theme.get_dimensions ();
            width = dimensions.width;
            height = dimensions.height;
        }
        catch (Error e)
        {
            GLib.warning ("Unable to load theme\n");
        }

        preview_rect = Gdk.Rectangle ();
        preview_rect.x = 0;
        preview_rect.y = 0;
        preview_rect.width = ELEM_WIDTH;
        preview_rect.height = ELEM_HEIGHT;
    }

    public void render_element (Cairo.Context cr, int type, int animation, double x, double y)
    {
        if (tile_pattern == null)
        {
            var preview_surface = new Cairo.Surface.similar (cr.get_target (),
                                                             Cairo.Content.COLOR_ALPHA,
                                                             Game.N_ANIMATIONS * ELEM_HEIGHT,
                                                             Game.N_TYPES * ELEM_HEIGHT);
            var cr_preview = new Cairo.Context (preview_surface);
            tile_pattern = new Cairo.Pattern.for_surface (preview_surface);

            Cairo.Matrix matrix = Cairo.Matrix.identity ();
            matrix.scale (Game.N_ANIMATIONS * ELEM_WIDTH / width ,
                          Game.N_TYPES * ELEM_HEIGHT / height);
            cr_preview.set_matrix (matrix);
            theme.render_cairo (cr_preview);
        }

        cr.set_source (tile_pattern);

        int texture_x, texture_y;
        get_texture_pos (type, animation, out texture_x, out texture_y);

        var m = Cairo.Matrix.identity ();
        m.translate (texture_x - x, texture_y - y);
        tile_pattern.set_matrix (m);

        cr.rectangle (x, y, ELEM_WIDTH, ELEM_HEIGHT);
        cr.fill ();
    }

    private void get_texture_pos (int type, int animation, out int texture_x, out int texture_y)
    {
        texture_x = ELEM_WIDTH * animation;;
        texture_y = ELEM_HEIGHT * type;
    }
}
} // namespace FiveOrMore