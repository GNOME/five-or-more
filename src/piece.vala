public class Piece
{
    public int id;

    public Piece (int id)
    {
        this.id = id;
    }

    public bool equal (Piece piece)
    {
        return this.id == piece.id;
    }
}
