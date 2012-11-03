using Gee;

namespace FiveOrMore
{
    public class GlinesBoard
    {
        public int cols { get; private set; }
        public int rows { get; private set; }

        public GlinesField[,] fields { get; private set; }
        public GlinesField active_field = null;

        public GlinesPreviewQueue preview_queue;

        public bool show_cursor { get; set; }
        private int _cursor_x;
        public int cursor_x
        {
            get { return this._cursor_x; }
            set { this._cursor_x = this.clamp(0, value, this.cols - 1); }
        }
        private int _cursor_Y;
        public int cursor_y
        {
            get { return this._cursor_Y; }
            set { this._cursor_Y = this.clamp(0, value, this.rows - 1); }
        }

        public bool move_in_progress { get; set; }

        public signal void changed ();
        public signal void move (ArrayList<GlinesField> path);
        public signal void info (string text);
        public signal void gameover ();

        public GlinesBoard(int cols, int rows, int preview_queue_size, int n_types)
        {
            this.cols = cols;
            this.rows = rows;

            preview_queue = new GlinesPreviewQueue(preview_queue_size, n_types);

            this.fields = new GlinesField[this.cols, this.rows];

            for (int x = 0; x < this.cols; x++)
            {
                for (int y = 0; y < this.rows; y++)
                {
                    this.fields[x, y] = new GlinesField();
                }
            }

            //Connect the neigbouring fields
            for (int x = 0; x < this.cols; x++)
            {
                for (int y = 0; y < this.rows; y++)
                {
                    var field = this.fields[x, y];
                    if (x > 0)
                        field.neigbour_west = this.fields[x - 1, y];
                    if (x < this.cols - 1)
                        field.neigbour_east = this.fields[x + 1, y];
                    if (y > 0)
                        field.neigbour_north = this.fields[x, y - 1];
                    if (y < this.rows - 1)
                        field.neigbour_south = this.fields[x, y + 1];
                }
            }

            this.move_preview_queue_to_board();
        }

        public void select_field(int x, int y)
        {
            if (this.move_in_progress)
                return;

            var clicked_field = this.fields[x, y];

            if (this.active_field == null && !clicked_field.has_piece)
            {
                info("No piece to select");
            }
            else if (this.active_field == null && clicked_field.has_piece)
            {
                //select a field
                clicked_field.active = true;
                this.active_field = clicked_field;
                info("Field selected");
            }
            else if (this.active_field == clicked_field)
            {
                //deselect the active field
                clicked_field.active = false;
                this.active_field = null;
                info("Field deselected");
            }
            else
            {
                //attempt to move the piece
                if (find_route(this.active_field, clicked_field))
                {
                    info("moving");

                    this.move_in_progress = true;

                    // Let the listener do the animation.
                    this.move(this.backtrack_route(clicked_field));

                    //actually move the piece
                    clicked_field.piece = this.active_field.piece;
                    this.active_field.piece = null;

                    this.active_field.active = false;
                    this.active_field = null;

                    move_completed();
                }
                else
                {
                    info("no route");
                }
            }

            this.changed();
        }

        private void move_completed()
        {
            //remove lines to make space for adding pieces
            this.remove_lines();

            this.move_preview_queue_to_board();

            //remove lines again. New once could be formed when adding the new pieces
            this.remove_lines();

            if (this.check_for_game_over())
            {
                gameover();
            }

            this.move_in_progress = false;
        }

        private void remove_lines()
        {
            //todo: check for lines to remove
            //      + add scoring
        }

        private void move_preview_queue_to_board()
        {
            foreach (var piece in preview_queue.pieces)
            {
                this.add_piece_at_random_location(piece);
            }

            preview_queue.refill();
        }

        private void add_piece_at_random_location(GlinesPiece piece)
        {
            var vert = new ArrayList<int>();
            var hori = new ArrayList<int>();

            for (int i = 0; i < this.cols; i++)
                vert.add(i);
            for (int i = 0; i < this.rows; i++)
                hori.add(i);

            shuffle(hori);
            shuffle(vert);

            foreach(int x in vert)
            {
                foreach(int y in hori)
                {
                    if(!fields[x,y].has_piece)
                    {
                        fields[x, y].piece = piece;
                        return;
                    }
                }
            }
        }

        private void shuffle(ArrayList<int> list)
        {
            for (int from = list.size - 1; from > 0; from--)
            {
                int to = Random.int_range(0, from);
                int temp = list[to];
                list[to] = list[from];
                list[from] = temp;
            }
        }

        private bool check_for_game_over()
        {
            for (int x = 0; x < this.cols; x++)
                for (int y = 0; y < this.rows; y++)
                    if (!this.fields[x, y].has_piece)
                        return false;

            return true;
        }

        private void reset_path_search()
        {
            for (int x = 0; x < this.cols; x++)
                for (int y = 0; y < this.rows; y++)
                    fields[x, y].path_search = null;
        }

        //After the search has been performed a route will (if possible)
        //exist from the target to the origin. Use the Pathsearch to follow
        //the path.
        public bool find_route(GlinesField from, GlinesField target)
        {
            reset_path_search();

            var check_now = new ArrayList<GlinesField> ();
            check_now.add (from);

            while (check_now.size != 0)
            {
                var check_next = new ArrayList<GlinesField>();

                foreach (var current in check_now)
                {
                    foreach (var neighbour in this.non_searched_neighbours(current))
                    {
                        if (neighbour == from)
                            continue;

                        neighbour.path_search = current;
                        check_next.add(neighbour);

                        // We have reached the target so there is no use in continuing
                        if (neighbour == target)
                            return true;
                    }
                }

                check_now = check_next;
            }

            return false;
        }

        private ArrayList<GlinesField> non_searched_neighbours(GlinesField field)
        {
            var neighbours = new ArrayList<GlinesField>();

            if (field.neigbour_north != null &&
                !field.neigbour_north.has_piece &&
                field.neigbour_north.path_search == null)
                neighbours.add(field.neigbour_north);

            if (field.neigbour_west != null &&
                !field.neigbour_west.has_piece &&
                field.neigbour_west.path_search == null)
                neighbours.add(field.neigbour_west);

            if (field.neigbour_south != null &&
                !field.neigbour_south.has_piece &&
                field.neigbour_south.path_search == null)
                neighbours.add(field.neigbour_south);

            if (field.neigbour_east != null &&
                !field.neigbour_east.has_piece &&
                field.neigbour_east.path_search == null)
                neighbours.add(field.neigbour_east);

            return neighbours;
        }

        public ArrayList<GlinesField> backtrack_route(GlinesField from)
        {
            var route = new ArrayList<GlinesField> ();
            route.add (from);

            //the routes origin may have more than one
            while (route[route.size -1 ].path_search != null)
                route.add(route[route.size -1 ].path_search);

            return route;
        }

        public void place_cursor(int x, int y)
        {
            this.cursor_x = x;
            this.cursor_y = y;

            this.changed();
        }

        public void move_cursor(int x, int y)
        {
            this.place_cursor(this.cursor_x + x, this.cursor_y + y);
        }

        private int clamp(int min, int val, int max)
        {
            if (val < min)
                return min;
            if (val > max)
                return max;
            return val;
        }
    }
}

