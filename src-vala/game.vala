namespace FiveOrMore
{

public class Game : Object
{
    public const int N_TYPES = 7;
    public const int N_ANIMATIONS = 4;

    private Settings settings;

    private Gdk.RGBA background_color;
    private int size;
    private NextPiecesGenerator next_pieces_generator;

    public signal Gee.ArrayList<Piece> set_next_pieces_queue (Gee.ArrayList<Piece> next_pieces_queue);

    private Gee.ArrayList<Piece> _next_pieces_queue;
    public Gee.ArrayList<Piece> next_pieces_queue
    {
        get { return _next_pieces_queue; }
        set { _next_pieces_queue = set_next_pieces_queue (value); }
    }

    const GameDifficulty[] game_difficulty = {
        { -1, -1, -1, -1 },
        { 7, 7, 5, 3 },
        { 9, 9, 7, 3 },
        { 20, 15, 7, 7 }
    };

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

        next_pieces_generator = new NextPiecesGenerator (game_difficulty[size].n_next_pieces,
                                                         game_difficulty[size].n_types);
        next_pieces_queue = null;
    }

    public void generate_next_pieces ()
    {
        next_pieces_queue = next_pieces_generator.yield_next_pieces ();
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

struct GameDifficulty
{
    public int n_cols;
    public int n_rows;
    public int n_types;
    public int n_next_pieces;
}

} // namespace FiveOrMore
