public class ThemeRenderer
{
    private Settings settings;

    public const int DEFAULT_SPRITE_SIZE = 20;
    private int sprite_size = DEFAULT_SPRITE_SIZE;
    private float sprite_sheet_width;
    private float sprite_sheet_height;

    private string theme_name;
    private Rsvg.Handle? theme = null;
    public const string themes[] = {
            "balls.svg",
            "dots.png",
            "gumball.png",
            "shapes.svg",
    };

    private Cairo.Pattern? tile_pattern = null;
    private Cairo.Context cr_preview;

    public ThemeRenderer (Settings settings)
    {
        this.settings = settings;

        settings.changed[FiveOrMoreApp.KEY_THEME].connect (change_theme_cb);
        change_theme_cb ();
    }

    private void change_theme_cb ()
    {
        theme_name = settings.get_string (FiveOrMoreApp.KEY_THEME);
        var theme_file = Path.build_filename (DATA_DIRECTORY, "themes", theme_name);

        try
        {
            theme = new Rsvg.Handle.from_file (theme_file);
            var dimensions = theme.get_dimensions ();
            sprite_sheet_width = dimensions.width;
            sprite_sheet_height = dimensions.height;
        }
        catch (Error e)
        {
            GLib.warning ("Unable to load theme\n");
        }
    }

    public void render_sprite (Cairo.Context cr, int type, int animation, double x, double y, int size)
    {
        if (tile_pattern == null || sprite_size != size)
        {
            sprite_size = size;

            var preview_surface = new Cairo.Surface.similar (cr.get_target (),
                                                             Cairo.Content.COLOR_ALPHA,
                                                             Game.N_ANIMATIONS * sprite_size,
                                                             Game.N_TYPES * sprite_size);
            cr_preview = new Cairo.Context (preview_surface);

            var matrix = Cairo.Matrix.identity ();
            matrix.scale (Game.N_ANIMATIONS * sprite_size / sprite_sheet_width,
                          Game.N_TYPES * sprite_size / sprite_sheet_height);
            cr_preview.set_matrix (matrix);

            theme.render_cairo (cr_preview);

            tile_pattern = new Cairo.Pattern.for_surface (preview_surface);
        }

        cr.set_source (tile_pattern);

        var texture_x = sprite_size * animation;
        var texture_y = sprite_size * type;

        var m = Cairo.Matrix.identity ();
        m.translate (texture_x - x, texture_y - y);
        tile_pattern.set_matrix (m);

        cr.rectangle (x, y, sprite_size, sprite_size);
        cr.fill ();
    }
}
