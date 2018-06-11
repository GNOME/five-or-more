public class Game
{
    private Settings settings;

    private Gdk.RGBA background_color;
    private int size;

    public Game (Settings settings)
    {
        this.settings = settings;

        background_color = Gdk.RGBA ();
        var color_str = settings.get_string (FiveOrMoreApp.KEY_BACKGROUND_COLOR);
        background_color.parse (color_str);
        settings.changed[FiveOrMoreApp.KEY_BACKGROUND_COLOR].connect (() => {
            color_str = settings.get_string (FiveOrMoreApp.KEY_BACKGROUND_COLOR);
            background_color.parse (color_str);
        });

        size = settings.get_int (FiveOrMoreApp.KEY_SIZE);
        settings.changed[FiveOrMoreApp.KEY_SIZE].connect (() => {
            size = settings.get_int (FiveOrMoreApp.KEY_SIZE);
        });
    }
}

enum BoardSize
{
    UNSET = 0,
    SMALL = 1,
    MEDIUM = 2,
    LARGE = 3,
    MAX_SIZE = 4,
}
