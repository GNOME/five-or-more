using Gee;

namespace FiveOrMore
{
    public class GlinesPreviewQueue
    {
        public ArrayList<GlinesPiece> pieces { get; set; }
        private int size { get; set; }
        private int types { get; set; }

        public GlinesPreviewQueue(int size, int types)
        {
            this.pieces = new ArrayList<GlinesPiece>();
            this.size = size;
            this.types = types; 
            this.refill();
        }

        public void refill()
        {
            this.pieces.clear();

            for (int i = 0; i < this.size; i++)
            {
                int id = Random.int_range(0, types);
                this.pieces.add(new GlinesPiece(id));
            }
        }
    }
}

