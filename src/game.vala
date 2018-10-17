/*
 * Color lines for GNOME
 * Copyright © 1999 Free Software Foundation
 * Authors: Robert Szokovacs <szo@appaloosacorp.hu>
 *          Szabolcs Ban <shooby@gnome.hu>
 *          Karuna Grewal <karunagrewal98@gmail.com>
 *          Ruxandra Simion <ruxandra.simion93@gmail.com>
 * Copyright © 2007 Christian Persch
 *
 * This game is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

public class Game : Object
{
    public const int N_TYPES = 7;
    public const int N_ANIMATIONS = 4;
    public const int N_MATCH = 5;

    private Settings settings;

    private int size;
    private NextPiecesGenerator next_pieces_generator;

    public Board board = null;
    public int n_rows {
        get {
            assert (board != null);
            return board.n_rows;
        }
    }

    public int n_cols {
        get {
            assert (board != null);
            return board.n_cols;
        }
    }

    public int n_next_pieces;

    private int n_cells;
    public int n_filled_cells;

    public Gee.ArrayList<Cell>? current_path = null;
    public bool animating = false;
    public Piece animating_piece;

    public int score { get; private set; }

    public signal void current_path_cell_pos_changed ();
    private int _current_path_cell_pos = -1;
    public int current_path_cell_pos
    {
        get { return _current_path_cell_pos; }
        set
        {
            _current_path_cell_pos = value;
            current_path_cell_pos_changed ();
        }
    }

    public signal void queue_changed (Gee.ArrayList<Piece> next_pieces_queue);

    private Gee.ArrayList<Piece> _next_pieces_queue;
    public Gee.ArrayList<Piece> next_pieces_queue
    {
        get { return _next_pieces_queue; }
        set
        {
            _next_pieces_queue = value;
            queue_changed (_next_pieces_queue);
        }
    }

    const GameDifficulty[] game_difficulty = {
        { -1, -1, -1, -1 },
        { 7, 7, 5, 3 },
        { 9, 9, 7, 3 },
        { 20, 15, 7, 7 }
    };

    public const KeyValue scorecats[] = {
        { "Small",  NC_("board size", "Small")  },
        { "Medium", NC_("board size", "Medium") },
        { "Large",  NC_("board size", "Large")  }
    };

    public signal void game_over ();
    public int n_categories = 3;
    public string score_current_category = null;

    public StatusMessage status_message { get; set; }

    public Game (Settings settings)
    {
        this.settings = settings;

        size = settings.get_int (FiveOrMoreApp.KEY_SIZE);
        settings.changed[FiveOrMoreApp.KEY_SIZE].connect (() => {
            size = settings.get_int (FiveOrMoreApp.KEY_SIZE);
            restart ();
        });

        init_game ();
    }

    private void init_game ()
    {
        var n_rows = game_difficulty[size].n_rows;
        var n_cols = game_difficulty[size].n_cols;
        this.n_next_pieces = game_difficulty[size].n_next_pieces;

        this.n_cells = n_rows * n_cols;
        this.n_filled_cells = 0;

        this.score = 0;
        this.score_current_category = scorecats[size - 1].key;

        this.status_message = DESCRIPTION;

        next_pieces_generator = new NextPiecesGenerator (game_difficulty[size].n_next_pieces,
                                                         game_difficulty[size].n_types);
        generate_next_pieces ();

        if (board == null)
            board = new Board (n_rows, n_cols);
        else
            board.reset (n_rows, n_cols);

        fill_board (n_rows, n_cols);

        generate_next_pieces ();
    }

    public void generate_next_pieces ()
    {
        this.next_pieces_queue = this.next_pieces_generator.yield_next_pieces ();
    }

    private void fill_board (int n_rows, int n_cols)
    {
        int row = -1, col = -1;

        for (int i = 0; i < next_pieces_queue.size; i++)
        {
            do
            {
                row = GLib.Random.int_range (0, n_rows);
                col = GLib.Random.int_range (0, n_cols);
            } while (board.get_piece (row, col) != null);

            board.set_piece (row, col, next_pieces_queue [i]);

            Gee.HashSet<Cell> inactivate =
            board.get_cell (row, col).get_all_directions (board.get_grid ());
            if (inactivate.size > 0)
            {
                n_filled_cells -= inactivate.size;
                foreach (Cell cell in inactivate)
                {
                    board.set_piece (cell.row, cell.col, null);
                }

                update_score (inactivate.size);
            }

            board.grid_changed ();
            n_filled_cells ++;

            if (check_game_over ())
            {
                status_message = GAME_OVER;
                board.grid_changed ();
                return;
            }
        }
    }

    private void update_score (int n_matched)
    {
        score += (int) (45 * Math.log (0.25 * n_matched));
    }

    private bool check_game_over ()
    {
        if (n_cells - n_filled_cells == 0)
        {
            game_over ();
            return true;
        }

        return false;
    }

    public void next_step ()
    {
        fill_board (this.n_rows, this.n_cols);
        generate_next_pieces ();
    }

    public bool make_move (int start_row, int start_col, int end_row, int end_col)
    {
        current_path = board.find_path (start_row,
                                        start_col,
                                        end_row,
                                        end_col);

        if (current_path == null || current_path.size == 0)
        {
            status_message = NO_PATH;
            return false;
        }

        current_path_cell_pos = 0;
        animating_piece = current_path.get (current_path_cell_pos).piece;
        Timeout.add (20, animate);

        return true;
    }

    public bool animate ()
    {
        animating = true;

        Cell curr_cell = current_path[current_path_cell_pos];

        if (current_path_cell_pos == 0)
            board.set_piece (curr_cell.row, curr_cell.col, null);

        if (current_path_cell_pos == current_path.size - 1)
        {
            board.set_piece (curr_cell.row, curr_cell.col, animating_piece);

            current_path = null;
            var inactivate =
                curr_cell.get_all_directions (board.get_grid ());

            if (inactivate.size > 0)
            {
                n_filled_cells -= inactivate.size;
                foreach (Cell cell in inactivate)
                {
                    board.set_piece (cell.row, cell.col, null);
                }

                update_score (inactivate.size);
            }

            if (inactivate.size < Game.N_MATCH)
                next_step ();

            board.grid_changed ();
            animating = false;

            return Source.REMOVE;
        }

        current_path_cell_pos++;

        return Source.CONTINUE;
    }

    public void restart ()
    {
        init_game ();
    }
}

public enum BoardSize
{
    UNSET,
    SMALL,
    MEDIUM,
    LARGE,
    MAX_SIZE,
}

struct GameDifficulty
{
    public int n_cols;
    public int n_rows;
    public int n_types;
    public int n_next_pieces;
}

struct KeyValue
{
  public string key;
  public string name;
}

public enum StatusMessage
{
    DESCRIPTION,
    NO_PATH,
    GAME_OVER,
    NONE,
}
