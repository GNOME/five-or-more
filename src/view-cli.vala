using Gee;

namespace FiveOrMore
{
    class ViewCli
    {
        private GlinesBoard board;

        private bool gameover = false;
        private string message = "";
        private char[] id_to_char_symbol = { 'a', 'b', 'c', 'd', 'e' };

        public ViewCli(GlinesBoard board)
        {
            this.board = board;
            board.show_cursor = true;
            board.place_cursor(5,5);

            board.changed.connect ((o) => this.print(null));
            board.move.connect ((o, path) => this.animate_move(path));
            board.info.connect ((o, info) => message = info);
            board.gameover.connect ((o) => this.gameover = true);

            this.print(null);
        }

        public void run()
        {
            while (!gameover)
            {
                var c = stdin.getc();

                if (c == 49) //southwest
                    board.move_cursor(-1, 1);

                if (c == 50) //south
                    board.move_cursor(0, 1);

                if (c == 51) //southeast
                    board.move_cursor(1, 1);

                if (c == 52) //west
                    board.move_cursor(-1, 0);

                if (c == 54) //east
                    board.move_cursor(1, 0);

                if (c == 55) //northwest
                    board.move_cursor(-1, -1);

                if (c == 56) //north
                    board.move_cursor(0, -1);

                if (c == 57) //northeast
                    board.move_cursor(1, -1);

                if (c == 53) //click
                    board.select_field(board.cursor_x, board.cursor_y);
            }

            stdout.printf("Game over.\n");
        }

        private void print(ArrayList<GlinesField>? path)
        {
            var sb = new StringBuilder();

            sb.append("Preview: ");
            foreach (var preview in board.preview_queue.pieces)
                sb.append_c(id_to_char_symbol[preview.id]);
            sb.append("\n\n");

            for (int n = 0; n < board.cols + 2; n++)
                sb.append("#");
            sb.append("\n");

            for (int y = 0; y < board.rows; y++)
            {
                sb.append("#");
                for (int x = 0; x < board.cols; x++)
                {
                    var field = board.fields[x,y];

                    bool is_cursor = board.cursor_x == x && board.cursor_y == y;

                    if (field.active)
                    {
                        sb.append("@");
                        continue;
                    }

                    if (!field.has_piece)
                    {
                        if (is_cursor) sb.append("£");
                        else
                        {
                            if (path != null && field.path_search != null && path.contains(field))
                            {
                                if (field.path_search == field.neigbour_north)
                                    sb.append("↓");
                                if (field.path_search == field.neigbour_south)
                                    sb.append("↑");
                                if (field.path_search == field.neigbour_west)
                                    sb.append("→");
                                if (field.path_search == field.neigbour_east)
                                    sb.append("←");
                            }
                            else
                                sb.append(" ");
                        }
                    }
                    else
                    {
                        char symbol = id_to_char_symbol[field.piece.id];

                        if (is_cursor)
                            sb.append_c(symbol.toupper());
                        else
                            sb.append_c(symbol);
                    }
                }

                sb.append("#\n");
            }

            for (int n = 0; n < board.cols + 2; n++)
                sb.append("#");

            sb.append("\n\n");
            sb.append(message);
            sb.append("\n");

            stdout.printf(sb.str);
        }

        private void animate_move(ArrayList<GlinesField> path)
        {
            //"animate" the move:
            this.message = "'Animating' the move...";
            this.print(path);
            this.message = "";
            stdin.getc();
        }
    }
}

