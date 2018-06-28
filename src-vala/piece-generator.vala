namespace FiveOrMore
{

public class NextPiecesGenerator
{
    private int n_types;
    private int n_next_pieces;
    private Gee.ArrayList<Piece> pieces;

    public NextPiecesGenerator (int n_next_pieces, int n_types)
    {
        this.pieces = new Gee.ArrayList<Piece> ();
        this.n_next_pieces = n_next_pieces;
        this.n_types = n_types;
    }

    private int yield_next_piece ()
    {
        return GLib.Random.int_range (0, this.n_types);
    }

    public Gee.ArrayList<Piece> yield_next_pieces ()
    {
        this.pieces.clear ();

        int i;
        for (i = 0; i < this.n_next_pieces; i++)
        {
            int id = yield_next_piece ();
            this.pieces.add (new Piece (id));
        }

        return this.pieces;
    }
}

} // namespace FiveOrMore
