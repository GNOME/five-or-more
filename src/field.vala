namespace FiveOrMore
{
    public class GlinesField
    {
        public GlinesField()
        {
            this.piece = null;

            this.neigbour_north = null;
            this.neigbour_south = null;
            this.neigbour_east = null;
            this.neigbour_west = null;
            this.path_search = null;
        }

        public GlinesPiece piece { get; set; }
        public bool active { get; set; }

        public GlinesField neigbour_north { get; set; }
        public GlinesField neigbour_south { get; set; }
        public GlinesField neigbour_east { get; set; }
        public GlinesField neigbour_west { get; set; }

        public GlinesField path_search { get; set; }

        public bool has_piece
        {
            get { return this.piece != null; }
        }
    }
}

