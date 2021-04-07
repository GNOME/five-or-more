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

private class NextPiecesGenerator
{
    private int n_types;
    private int n_next_pieces;
    private Gee.ArrayList<Piece> pieces;

    internal NextPiecesGenerator (int n_next_pieces, int n_types)
    {
        this.pieces = new Gee.ArrayList<Piece> ();
        this.n_next_pieces = n_next_pieces;
        this.n_types = n_types;
    }

    private int yield_next_piece ()
    {
        return GLib.Random.int_range (0, this.n_types);
    }

    internal Gee.ArrayList<Piece> yield_next_pieces ()
    {
        this.pieces.clear ();

        for (int i = 0; i < this.n_next_pieces; i++)
        {
            int id = yield_next_piece ();
            this.pieces.add (new Piece (id));
        }

        return this.pieces;
    }
}
