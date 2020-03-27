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
    private MenuButton primary_menu_button;

    [GtkChild]
    private AspectFrame grid_frame;

    public GLib.Settings settings { private get; protected construct; }
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

    construct
    {
        game = new Game (settings);
        update_ratio ((BoardSize)settings.get_int ("size"));
        theme = new ThemeRenderer (settings);

        size_allocate.connect (on_size_allocate);
        map.connect (init_state_watcher);

        set_default_size (settings.get_int ("window-width"), settings.get_int ("window-height"));
        if (settings.get_boolean ("window-is-maximized"))
            maximize ();

        NextPiecesWidget next_pieces_widget = new NextPiecesWidget (settings, game, theme);
        preview_hbox.add (next_pieces_widget);

        game.notify["score"].connect ((s, p) => { set_status_message (status[StatusMessage.NONE].printf(game.score)); });
        game.notify["status-message"].connect ((s, p) => { set_status_message (status[game.status_message].printf(game.score)); });
        set_status_message (status[game.status_message]);

        View game_view = new View (settings, game, theme);
        grid_frame.add (game_view);

        var importer = new Games.Scores.DirectoryImporter ();
        highscores = new Games.Scores.Context.with_importer ("five-or-more",
                                                _("Board Size: "),
                                                this,
                                                create_category_from_key,
                                                Games.Scores.Style.POINTS_GREATER_IS_BETTER,
                                                importer);
        game.game_over.connect (score_cb);
    }

    internal GameWindow (GLib.Settings settings)
    {
        Object (settings: settings);
    }

    private void init_state_watcher ()
    {
        Gdk.Surface? nullable_surface = get_surface ();     // TODO report bug, get_surface() returns a nullable Surface
        if (nullable_surface == null || !((!) nullable_surface is Gdk.Toplevel))
            assert_not_reached ();
        surface = (Gdk.Toplevel) (!) nullable_surface;
        surface.notify ["state"].connect (on_window_state_event);
    }

    private Gdk.Toplevel surface;
    private const Gdk.SurfaceState tiled_state = Gdk.SurfaceState.TILED
                                               | Gdk.SurfaceState.TOP_TILED
                                               | Gdk.SurfaceState.BOTTOM_TILED
                                               | Gdk.SurfaceState.LEFT_TILED
                                               | Gdk.SurfaceState.RIGHT_TILED;
    private void on_window_state_event ()
    {
        Gdk.SurfaceState state = surface.get_state ();

        window_maximized =  (state & Gdk.SurfaceState.MAXIMIZED) != 0;
        window_tiled =      (state & Gdk.SurfaceState.TILED)     != 0;
    }

    private void on_size_allocate (int width, int height, int baseline)
    {
        if (window_maximized || window_tiled)
            return;

        window_width  = width;
        window_height = height;
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

    internal void restart_game ()
    {
        game.restart ();
    }

    private void set_status_message (string? message)
    {
        headerbar.set_subtitle (message);
    }

    internal void show_scores ()
    {
        highscores.run_dialog ();
    }

    internal void change_size (BoardSize size)
    {
        var game_size = settings.get_int ("size");

        if (game_size == size)
            return;

        primary_menu_button.popdown ();

        if (game.score > 0) {
            var flags = DialogFlags.DESTROY_WITH_PARENT;
            var restart_game_dialog = new MessageDialog (this,
                                                         flags,
                                                         MessageType.WARNING,
                                                         ButtonsType.NONE,
                                                         _("Are you sure you want to restart the game?"));
            restart_game_dialog.add_buttons (_("_Cancel"), ResponseType.CANCEL,
                                             _("_Restart"), ResponseType.OK);

            var result = restart_game_dialog.run ();
            restart_game_dialog.destroy ();
            switch (result)
            {
                case ResponseType.OK:
                     if (!settings.set_int (FiveOrMoreApp.KEY_SIZE, size))
                        warning ("Failed to set size: %d", size);
                    break;
                case ResponseType.CANCEL:
                    break;
            }
        } else {
            settings.set_int (FiveOrMoreApp.KEY_SIZE, size);
            update_ratio (size);
        }

    }

    private void update_ratio (BoardSize size)
    {
        if (size == BoardSize.LARGE)
            grid_frame.ratio = 4.0f/3.0f;
        else
            grid_frame.ratio = 1.0f;
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
}
