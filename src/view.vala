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

using Gtk;

private class View : DrawingArea
{
    private const int MINIMUM_BOARD_SIZE = 256;

    private Game? game = null;
    private ThemeRenderer? theme = null;
    private StyleContext cs;
    private CssProvider provider;

    private Gdk.Rectangle board_rectangle;

    private int piece_size = 0;

    private int cell_x;
    private int cell_y;
    private int start_x;
    private int start_y;
    private int end_x;
    private int end_y;

    private bool show_cursor;
    private int keyboard_cursor_x;
    private int keyboard_cursor_y;

    private int animation_state;
    private uint animation_id;

    private EventControllerKey key_controller;          // for keeping in memory
    private GestureMultiPress click_controller;         // for keeping in memory

    internal const string default_background_color = "rgb(117,144,174)";
    private string _background_color = default_background_color;
    internal string background_color
    {
        internal get { return _background_color; }
        internal set
        {
            Gdk.RGBA color = Gdk.RGBA ();
            if (!color.parse (value))
                _background_color = default_background_color;
            else
                _background_color = color.to_string ();

            try
            {
                provider.load_from_data (".game-view { background-color: %s; }".printf (_background_color));
            }
            catch (Error e)
            {
                warning ("Failed to load CSS data to provider");
                return;
            }
            queue_draw ();
        }
    }

    internal View (Game game, ThemeRenderer theme)
    {
        this.game = game;
        this.theme = theme;

        init_keyboard ();
        init_mouse ();

        cs = get_style_context ();
        provider = new CssProvider ();
        cs.add_class ("game-view");
        cs.add_provider (provider, STYLE_PROVIDER_PRIORITY_USER);

        set_size_request (MINIMUM_BOARD_SIZE, MINIMUM_BOARD_SIZE);

        board_rectangle = Gdk.Rectangle ();
        board_rectangle.x = board_rectangle.y = 0;
        update_sizes (MINIMUM_BOARD_SIZE, MINIMUM_BOARD_SIZE);
        game.board.board_changed.connect (() => {
            start_x = -1;
            start_y = -1;
            end_x = -1;
            end_y = -1;

            show_cursor = false;
            keyboard_cursor_x = -1;
            keyboard_cursor_y = -1;
            update_sizes (get_allocated_width (), get_allocated_height ());
            queue_draw ();
        });

        game.board.grid_changed.connect (queue_draw);
        add_events (Gdk.EventMask.BUTTON_PRESS_MASK | Gdk.EventMask.BUTTON_RELEASE_MASK);
        add_events (Gdk.EventMask.KEY_PRESS_MASK | Gdk.EventMask.KEY_RELEASE_MASK);

        set_can_focus (true);
        grab_focus ();

        game.current_path_cell_pos_changed.connect (queue_draw);

        theme.theme_changed.connect (theme_changed_cb);

        start_x = -1;
        start_y = -1;
        end_x = -1;
        end_y = -1;

        show_cursor = false;
        keyboard_cursor_x = -1;
        keyboard_cursor_y = -1;

        animation_state = 0;
        animation_id = -1;
    }

    private void theme_changed_cb ()
    {
        game.board.grid_changed ();
        game.queue_changed (game.next_pieces_queue);
        queue_draw ();
    }

    private void move_keyboard_cursor (int x, int y)
    {
        int prev_x = keyboard_cursor_x;
        int prev_y = keyboard_cursor_y;

        if (!show_cursor)
        {
            show_cursor = true;
            queue_draw_area (keyboard_cursor_x * piece_size, keyboard_cursor_y * piece_size, piece_size, piece_size);
        }

        keyboard_cursor_x += x;
        if (keyboard_cursor_x < 0)
            keyboard_cursor_x = 0;
        if (keyboard_cursor_x >= game.n_cols)
            keyboard_cursor_x = game.n_cols - 1;

        keyboard_cursor_y += y;
        if (keyboard_cursor_y < 0)
            keyboard_cursor_y = 0;
        if (keyboard_cursor_y >= game.n_rows)
            keyboard_cursor_y = game.n_rows - 1;

        if (keyboard_cursor_x == prev_x && keyboard_cursor_y == prev_y)
            return;

        queue_draw_area (prev_x * piece_size, prev_y * piece_size, piece_size, piece_size);
        queue_draw_area (keyboard_cursor_x * piece_size, keyboard_cursor_y * piece_size, piece_size, piece_size);
    }

    private void init_keyboard ()
    {
        key_controller = new Gtk.EventControllerKey (this);
        key_controller.key_pressed.connect (on_key_pressed);
    }

    private inline bool on_key_pressed (Gtk.EventControllerKey _key_controller, uint keyval, uint keycode, Gdk.ModifierType state)
    {
        switch (keyval)
        {
            case (Gdk.Key.Left):
                /* fall-thru */
            case (Gdk.Key.KP_Left):
                move_keyboard_cursor (-1, 0);
                break;
            case (Gdk.Key.Right):
                /* fall-thru */
            case (Gdk.Key.KP_Right):
                move_keyboard_cursor (1, 0);
                break;
            case (Gdk.Key.Up):
                /* fall-thru */
            case (Gdk.Key.KP_Up):
                move_keyboard_cursor (0, -1);
                break;
            case (Gdk.Key.Down):
                /* fall-thru */
            case (Gdk.Key.KP_Down):
                move_keyboard_cursor (0, 1);
                break;
            case (Gdk.Key.Home):
                /* fall-thru */
            case (Gdk.Key.KP_Home):
                move_keyboard_cursor (-999, 0);
                break;
            case (Gdk.Key.End):
                /* fall-thru */
            case (Gdk.Key.KP_End):
                move_keyboard_cursor (999, 0);
                break;
            case (Gdk.Key.Page_Up):
                /* fall-thru */
            case (Gdk.Key.KP_Page_Up):
                move_keyboard_cursor (0, -999);
                break;
            case (Gdk.Key.Page_Down):
                /* fall-thru */
            case (Gdk.Key.KP_Page_Down):
                move_keyboard_cursor (0, 999);
                break;
            case (Gdk.Key.space):
                /* fall-thru */
            case (Gdk.Key.KP_Space):
                /* fall-thru */
            case (Gdk.Key.Return):
                /* fall-thru */
            case (Gdk.Key.KP_Enter):
                if (show_cursor)
                    cell_clicked (keyboard_cursor_x, keyboard_cursor_y);
                break;
            default:
                return false;
        }

        return true;
    }

    private void cell_clicked (int cell_x, int cell_y)
    {
        if (cell_x >= game.n_cols || cell_y >= game.n_rows)
            return;

        keyboard_cursor_x = cell_x;
        keyboard_cursor_y = cell_y;

        /* if selected cell is not empty, set start */
        if (game.board.get_piece (cell_y, cell_x) != null)
        {
            if (animation_id != -1)
            {
                Source.remove (animation_id);
                animation_id = -1;
            }

            if (start_x == cell_x && start_y == cell_y)
            {
                start_x = -1;
                start_y = -1;

                animation_state = 0;
                queue_draw ();

                return;
            }

            start_x = cell_x;
            start_y = cell_y;

            animation_id = Timeout.add (100, animate_clicked);
        }
        /* if selected cell is empty and start is set, and cell is empty, set end */
        else if (game.board.get_piece (cell_y, cell_x) == null && start_x != -1 && start_y != -1)
        {
            if (game.status_message != StatusMessage.NONE)
                game.status_message = StatusMessage.NONE;

            end_x = cell_x;
            end_y = cell_y;

            bool move = game.make_move (start_y, start_x, end_y, end_x);

            if (!move)
                return;

            start_x = -1;
            start_y = -1;

            if (animation_id != -1)
            {
                Source.remove (animation_id);
                animation_id = -1;
            }
        }
    }

    private void init_mouse ()
    {
        click_controller = new GestureMultiPress (this);    // only reacts to Gdk.BUTTON_PRIMARY
        click_controller.pressed.connect (on_click);
    }

    private inline void on_click (GestureMultiPress _click_controller, int n_press, double event_x, double event_y)
    {
        if (game == null || game.animating)
            return;

        if (show_cursor)
        {
            show_cursor = false;
            queue_draw_area (keyboard_cursor_x * piece_size, keyboard_cursor_y * piece_size, piece_size, piece_size);
        }

        cell_x = (int)event_x / piece_size;
        cell_y = (int)event_y / piece_size;

        cell_clicked (cell_x, cell_y);
    }

    private bool animate_clicked ()
    {
        animation_state = (animation_state + 1) % Game.N_ANIMATIONS;
        queue_draw ();

        return Source.CONTINUE;
    }

    private void update_sizes (int width, int height)
    {
        piece_size = (width - 1) / game.n_cols;
        board_rectangle.width = piece_size * game.n_cols;
        board_rectangle.height = piece_size * game.n_rows;
    }

    protected override bool configure_event (Gdk.EventConfigure event)
    {
        update_sizes (event.width, event.height);
        queue_draw ();

        return true;
    }

    private void fill_background (Cairo.Context cr)
    {
        cs.render_background (cr, board_rectangle.x, board_rectangle.y, board_rectangle.width, board_rectangle.height);
    }

    private void draw_gridlines (Cairo.Context cr)
    {
        Gdk.RGBA grid_color = cs.get_color (cs.get_state ());
        Gdk.cairo_set_source_rgba (cr, grid_color);
        cr.set_line_width (1.0);

        for (int i = piece_size; i < board_rectangle.width; i += piece_size)
        {
            cr.move_to (i + 0.5, 0 + 0.5);
            cr.line_to (i + 0.5, board_rectangle.height + 0.5);
        }

        for (int i = piece_size; i < board_rectangle.height; i += piece_size)
        {
            cr.move_to (0 + 0.5, i + 0.5);
            cr.line_to (board_rectangle.width + 0.5, i + 0.5);
        }

        var border = Gdk.Rectangle ();
        border.x = border.y = 1;
        border.width = board_rectangle.width;
        border.height = board_rectangle.height;

        Gdk.cairo_rectangle (cr, border);
        cr.stroke ();
    }

    private void draw_cursor_box (Cairo.Context cr)
    {
        if (show_cursor)
        {
            Gdk.RGBA grid_color = cs.get_color (cs.get_state ());
            Gdk.cairo_set_source_rgba (cr, grid_color);
            cr.set_line_width (2.0);

            var cursor_rectangle = Gdk.Rectangle ();
            cursor_rectangle.x = keyboard_cursor_x * piece_size + 1;
            cursor_rectangle.y = keyboard_cursor_y * piece_size + 1;
            cursor_rectangle.width = piece_size - 1;
            cursor_rectangle.height = piece_size - 1;
            Gdk.cairo_rectangle (cr, cursor_rectangle);
            cr.stroke ();
        }
    }

    private void draw_shapes (Cairo.Context cr)
    {
        for (int row = 0; row < game.n_rows; row++)
        {
            for (int col = 0; col < game.n_cols; col++)
            {
                if (game.board.get_piece (row,col) != null)
                {
                    theme.render_sprite (cr,
                                         game.board.get_piece (row,col).id,
                                         (start_x == col && start_y == row) ? animation_state : 0,
                                         col * piece_size,
                                         row * piece_size,
                                         piece_size);
                }
            }
        }
    }

    private void draw_path (Cairo.Context cr)
    {
        if (game.current_path != null && game.current_path.size > 0 && game.current_path_cell_pos != -1)
        {
            Cell current_cell = game.current_path[game.current_path_cell_pos];
            theme.render_sprite (cr,
                                 game.animating_piece.id,
                                 animation_state,
                                 current_cell.col * piece_size,
                                 current_cell.row * piece_size,
                                 piece_size);
        }
    }

    protected override bool draw (Cairo.Context cr)
    {
        if (theme == null)
            return false;

        fill_background (cr);
        draw_gridlines (cr);
        draw_shapes (cr);
        draw_cursor_box (cr);
        draw_path (cr);

        return true;
    }
}
