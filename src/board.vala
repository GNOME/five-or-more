/*
 * Color lines for GNOME
 * Copyright © 1999 Free Software Foundation
 * Authors: Robert Szokovacs <szo@szo.hu>
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

private class Board
{
    private Cell[,] grid = null;

    internal int n_rows {
        internal get {
            assert (grid != null);
            return grid.length[0];
        }
    }

    internal int n_cols {
        internal get {
            assert (grid != null);
            return grid.length[1];
        }
    }

    internal signal void grid_changed ();
    internal signal void board_changed ();

    private Cell src;
    private Cell dst;

    private  Gee.ArrayList<Cell> open = null;
    internal Gee.ArrayList<Cell> closed = null;
    internal Gee.ArrayList<Cell>? path = null;

    internal Board (int n_rows, int n_cols)
    {
        grid = new Cell[n_rows, n_cols];
        for (int col = 0; col < n_cols; col++)
        {
            for (int row = 0; row < n_rows; row++)
            {
                grid[row, col] = new Cell (row, col, null, null);
            }
        }
    }

    internal void reset (int n_rows, int n_cols)
    {
        grid = new Cell[n_rows, n_cols];

        for (int col = 0; col < n_cols; col++)
        {
            for (int row = 0; row < n_rows; row++)
            {
                if (grid[row, col] != null)
                {
                    grid[row, col].parent = null;
                    grid[row, col].piece = null;
                    grid[row, col].cost = int.MAX;
                }
                else
                    grid[row, col] = new Cell (row, col, null, null);
            }
        }

        board_changed ();
    }

    internal Cell[,]? get_grid ()
    {
        return this.grid;
    }

    internal void set_piece (int row, int col, Piece? piece)
    {
        grid[row, col].piece = piece;
    }

    internal Piece? get_piece (int row, int col)
    {
        return grid[row, col].piece;
    }

    internal Cell? get_cell (int row, int col)
    {
        return grid[row, col];
    }

    internal Gee.ArrayList<Cell> find_path (int start_row,
                                            int start_col,
                                            int end_row,
                                            int end_col)
    {
        reset_path_search ();

        src = grid[start_row, start_col];
        dst = grid[end_row, end_col];

        closed = new Gee.ArrayList<Cell> (cell_equal);

        open = new Gee.ArrayList<Cell> (cell_equal);
        open.add (src);

        Cell? current_cell = null;
        int current_cost = 0;
        Gee.ArrayList<Cell>? neighbours;

        do
        {
            current_cost = closed.size;

            current_cell = best_candidate (open, current_cost, dst);
            closed.add (current_cell);
            open.remove (current_cell);

            if (closed.contains (dst))
            {
                for (Cell? p = dst; p != null; p = p.parent)
                {
                    path.insert (0, p);
                }
                return path;
            }

            neighbours = current_cell.get_neighbours (grid);
            foreach (Cell neighbour in neighbours)
            {
                // if this adjacent square is already in the closed list ignore it
                if (closed.contains (neighbour))
                {
                    continue;
                }

                // if its not in the open list add it
                if (!open.contains (neighbour))
                {
                    neighbour.parent = current_cell;
                    open.add (neighbour);
                }

                // if its already in the open list and using the current score makes it lower,
                // update the parent because it means it is a better path
                else
                {
                    if (total_cost (neighbour, dst, current_cost) < neighbour.cost)
                    {
                        neighbour.parent = current_cell;
                        neighbour.cost = total_cost (neighbour, dst, current_cost);
                    }
                }
            }
        } while (open.size != 0);

        return path;
    }

    private bool cell_equal (Cell? a, Cell? b)
    {
        return (a != null && b != null) ? a.equal (b) : false;
    }

    private void reset_path_search ()
    {
        if (path != null)
            path.clear ();
        else
            path = new Gee.ArrayList<Cell> ();

        foreach (Cell cell in grid)
        {
            cell.parent = null;
            cell.cost = int.MAX;
        }
    }

    // f = g + h, where f is the cost of the road
    // g is the movement cost from the start cell to the current square
    // h is the estimated movement cost from the current square to the destination cell
    private Cell? best_candidate (Gee.ArrayList<Cell> neighbours, int current_cost, Cell end)
    {
        int lowest_f = int.MAX;
        Cell? best_candidate = null;

        foreach (Cell neighbour in neighbours)
        {
            neighbour.cost = total_cost (neighbour, end, current_cost);

            if (neighbour.cost < lowest_f)
            {
                lowest_f = neighbour.cost;
                best_candidate = neighbour;
            }
        }

        return best_candidate;
    }

    // for h it is used the Manhattan distance
    // the sum of the absolute values of the differences of the coordinates.
    // if start = (start_x, start_y) and end = (end_x, end_y)
    // => h = |start_x - end_x| + |start_y - end_y|
    private int manhattan (int start_x, int start_y, int end_x, int end_y)
    {
        return (start_x - end_x).abs () + (start_y - end_y).abs ();
    }

    private int total_cost (Cell start, Cell end, int current_cost)
    {
        int f, g, h;

        g = current_cost + 1;
        h = manhattan (start.row, start.col, end.row, end.col);
        f = g + h;

        return f;
    }
}

private class Cell
{
    internal int row;
    internal int col;
    internal Cell? parent;
    internal Piece? piece;
    internal int cost;

    internal Cell (int row, int col, Cell? parent, Piece? piece)
    {
        this.row = row;
        this.col = col;
        this.parent = parent;
        this.piece = piece;
        this.cost = int.MAX;
    }

    internal bool equal (Cell cell)
    {
        return this.row == cell.row && this.col == cell.col;
    }

    private Cell get_neighbour (Cell[,] board, Direction dir)
    {
        Cell? neighbour = null;
        int row = -1, col = -1;

        switch (dir)
        {
            case Direction.RIGHT:
                row = this.row;
                col = this.col + 1;
                break;
            case Direction.LEFT:
                row = this.row;
                col = this.col - 1;
                break;
            case Direction.UP:
                row = this.row - 1;
                col = this.col;
                break;
            case Direction.DOWN:
                row = this.row + 1;
                col = this.col;
                break;
            case Direction.UPPER_LEFT:
                row = this.row - 1;
                col = this.col - 1;
                break;
            case Direction.LOWER_RIGHT:
                row = this.row + 1;
                col = this.col + 1;
                break;
            case Direction.UPPER_RIGHT:
                row = this.row - 1;
                col = this.col + 1;
                break;
            case Direction.LOWER_LEFT:
                row = this.row + 1;
                col = this.col - 1;
                break;
        }

        if (row >= 0 && row < board.length[0] &&
            col >= 0 && col < board.length[1])
            neighbour = board[row, col];

        return neighbour;
    }

    internal Gee.ArrayList<Cell> get_neighbours (Cell[,] board)
    {
        Gee.ArrayList<Cell> neighbours = new Gee.ArrayList<Cell> ();
        Cell? right = null, left = null, up = null, down = null;

        right = this.get_neighbour (board, Direction.RIGHT);
        if (right != null && right.piece == null)
            neighbours.add (right);

        left = get_neighbour (board, Direction.LEFT);
        if (left != null && left.piece == null)
            neighbours.add (left);

        up = get_neighbour (board, Direction.UP);
        if (up != null && up.piece == null)
            neighbours.add (up);

        down = get_neighbour (board, Direction.DOWN);
        if (down != null && down.piece == null)
            neighbours.add (down);

        return neighbours;
    }

    private void get_direction (Cell[,] board, Direction dir, ref Gee.ArrayList<Cell>? list)
    {
        if (list == null)
            list = new Gee.ArrayList<Cell> ();

        for (Cell? cell = this;
            cell != null && cell.piece != null && cell.piece.equal (this.piece);
            cell = cell.get_neighbour (board, dir))
        {
            if (!list.contains (cell))
                list.add (cell);
        }
    }

    private Gee.ArrayList<Cell> get_horizontal (Cell[,] board)
    {
        Gee.ArrayList<Cell>? list = null;

        get_direction (board, Direction.LEFT, ref list);
        get_direction (board, Direction.RIGHT, ref list);

        return list;
    }

    private Gee.ArrayList<Cell> get_vertical (Cell[,] board)
    {
        Gee.ArrayList<Cell>? list = null;

        get_direction (board, Direction.UP, ref list);
        get_direction (board, Direction.DOWN, ref list);

        return list;
    }

    private Gee.ArrayList<Cell> get_first_diagonal (Cell[,] board)
    {
        Gee.ArrayList<Cell>? list = null;

        get_direction (board, Direction.UPPER_LEFT, ref list);
        get_direction (board, Direction.LOWER_RIGHT, ref list);

        return list;
    }

    private Gee.ArrayList<Cell> get_second_diagonal (Cell[,] board)
    {
        Gee.ArrayList<Cell>? list = null;

        get_direction (board, Direction.UPPER_RIGHT, ref list);
        get_direction (board, Direction.LOWER_LEFT, ref list);

        return list;
    }

    internal Gee.HashSet<Cell> get_all_directions (Cell[,] board)
    {
        Gee.ArrayList<Cell>? list;
        Gee.HashSet<Cell>? inactivate = new Gee.HashSet<Cell> ();

        list = get_horizontal (board);
        if (list.size >= Game.N_MATCH)
        {
            foreach (var l in list)
                inactivate.add (l);
        }

        list = get_vertical (board);
        if (list.size >= Game.N_MATCH)
        {
            foreach (var l in list)
                inactivate.add (l);
        }

        list = get_first_diagonal (board);
        if (list.size >= Game.N_MATCH)
        {
            foreach (var l in list)
                inactivate.add (l);
        }

        list = get_second_diagonal (board);
        if (list.size >= Game.N_MATCH)
        {
            foreach (var l in list)
                inactivate.add (l);
        }

        return inactivate;
    }
}

private enum Direction
{
    RIGHT,
    LEFT,
    UP,
    DOWN,
    UPPER_RIGHT,
    LOWER_LEFT,
    UPPER_LEFT,
    LOWER_RIGHT,
}
