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

using Gtk;

[GtkTemplate (ui = "/org/gnome/five-or-more/ui/five-or-more.ui")]
private class GameWindow : ApplicationWindow
{
    [GtkChild]
    private HeaderBar headerbar;

    [GtkChild]
    private Box preview_hbox;

    [GtkChild]
    private Games.GridFrame grid_frame;

    private GLib.Settings? settings = null;
    private bool window_tiled;
    internal bool window_maximized { internal get; private set; }
    internal int window_width { internal get; private set; }
    internal int window_height { internal get; private set; }

    private Game? game = null;
    private ThemeRenderer? theme = null;

    private Games.Scores.Context highscores;
    private string[] status = {
            _("Match five objects of the same type in a row to score!"),
            _("You can’t move there!"),
            _("Game Over!"),
            _("Score: %d")
    };

    private const GLib.ActionEntry win_actions [] =
    {
        {"change-size", null,       "s", "'SMALL'", change_size },
        {"new-game",    new_game    },
        {"scores",      show_scores }
    };

    construct
    {
        add_action_entries (win_actions, this);
    }

    internal GameWindow (Gtk.Application app, GLib.Settings settings)
    {
        Object (application: app);

        this.settings = settings;

        var board_size_action = lookup_action ("change-size");
        BoardSize board_size = (BoardSize) settings.get_int (FiveOrMoreApp.KEY_SIZE);
        ((SimpleAction) board_size_action).set_state (new Variant.string (board_size.to_string ()));

        game = new Game ((int) board_size);
        theme = new ThemeRenderer (settings);

        set_default_size (settings.get_int ("window-width"), settings.get_int ("window-height"));
        if (settings.get_boolean ("window-is-maximized"))
            maximize ();

        NextPiecesWidget next_pieces_widget = new NextPiecesWidget (settings, game, theme);
        preview_hbox.pack_start (next_pieces_widget);
        next_pieces_widget.realize ();
        next_pieces_widget.show ();

        grid_frame.set (game.n_cols, game.n_rows);
        game.board.board_changed.connect (() => { grid_frame.set (game.n_cols, game.n_rows); });
        game.notify["score"].connect ((s, p) => { set_status_message (status[StatusMessage.NONE].printf(game.score)); });
        game.notify["status-message"].connect ((s, p) => { set_status_message (status[game.status_message].printf(game.score)); });
        set_status_message (status[game.status_message]);

        View game_view = new View (settings, game, theme);
        grid_frame.add (game_view);
        game_view.show ();

        grid_frame.show ();

        var importer = new Games.Scores.DirectoryImporter ();
        highscores = new Games.Scores.Context.with_importer ("five-or-more",
                                                             _("Board Size: "),
                                                             this,
                                                             create_category_from_key,
                                                             Games.Scores.Style.POINTS_GREATER_IS_BETTER,
                                                             importer);
        game.game_over.connect (score_cb);
    }

    protected override bool window_state_event (Gdk.EventWindowState event)
    {
        if ((event.changed_mask & Gdk.WindowState.MAXIMIZED) != 0)
            window_maximized = (event.new_window_state & Gdk.WindowState.MAXIMIZED) != 0;

        if ((event.changed_mask & Gdk.WindowState.TILED) != 0)
            window_tiled = (event.new_window_state & Gdk.WindowState.TILED) != 0;

        return false;
    }

    protected override void size_allocate (Allocation allocation)
    {
        base.size_allocate (allocation);

        if (window_maximized || window_tiled)
            return;

        window_width = allocation.width;
        window_height = allocation.height;
    }

    private void score_cb ()
    {

        string name = category_name_from_key (game.score_current_category);
        var current_category = new Games.Scores.Category (game.score_current_category, name);
        highscores.add_score.begin (game.score,
                                    current_category,
                                    new Cancellable ());

        show_scores ();
    }

    private void set_status_message (string? message)
    {
        headerbar.set_subtitle (message);
    }

    private Games.Scores.Category? create_category_from_key (string key)
    {
        string? name = category_name_from_key (key);
        return new Games.Scores.Category (key, name);
    }

    private string category_name_from_key (string key)
    {
        for (int i = 0; i < game.n_categories; i++) {
            if (Game.scorecats[i].key == key)
                return Game.scorecats[i].name;
        }
        return "";
    }

    /*\
    * * actions
    \*/

    private inline void change_size (SimpleAction action, Variant? parameter)
    {
        int size;
        action.set_state (parameter);
        switch (parameter.get_string()) {
            case "BOARD_SIZE_SMALL":    size = (int) BoardSize.SMALL;   break;
            case "BOARD_SIZE_MEDIUM":   size = (int) BoardSize.MEDIUM;  break;
            case "BOARD_SIZE_LARGE":    size = (int) BoardSize.LARGE;   break;
            default: assert_not_reached ();
        }
        settings.set_int (FiveOrMoreApp.KEY_SIZE, size);
    }

    private inline void new_game (/* SimpleAction action, Variant? parameter */)
    {
        int size = settings.get_int (FiveOrMoreApp.KEY_SIZE);
        int n_rows = Game.game_difficulty[size].n_rows;
        int n_cols = Game.game_difficulty[size].n_cols;
        if (game.score > 0 && !game.is_game_over) {
            var flags = DialogFlags.DESTROY_WITH_PARENT;
            var restart_game_dialog = new MessageDialog (this,
                                                         flags,
                                                         MessageType.WARNING,
                                                         ButtonsType.NONE,
                                                         _("Are you sure you want to start a new %u × %u game?").printf (n_rows, n_cols));
            restart_game_dialog.add_buttons (_("_Cancel"), ResponseType.CANCEL,
                                             _("_Restart"), ResponseType.OK);

            var result = restart_game_dialog.run ();
            restart_game_dialog.destroy ();
            if (result != ResponseType.OK)
                return;
        }
        game.new_game (size);
    }

    private inline void show_scores (/* SimpleAction action, Variant? parameter */)
    {
        highscores.run_dialog ();
    }
}
