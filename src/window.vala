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

[GtkTemplate (ui = "/org/gnome/five-or-more/ui/five-or-more.ui")]
public class GameWindow : Gtk.ApplicationWindow
{
    [GtkChild]
    private Gtk.HeaderBar headerbar;

    [GtkChild]
    private Gtk.Box preview_hbox;

    [GtkChild]
    private Gtk.MenuButton primary_menu_button;

    [GtkChild]
    private Games.GridFrame grid_frame;

    private Settings? settings = null;
    private bool window_tiled;
    public bool window_maximized { get; private set; }
    public int window_width { get; private set; }
    public int window_height { get; private set; }

    private Game? game = null;
    private ThemeRenderer? theme = null;

    private Games.Scores.Context highscores;
    private string[] status = {
            _("Match five objects of the same type in a row to score!"),
            _("You can’t move there!"),
            _("Game Over!"),
            _("Score: %d")
    };

    public GameWindow (Gtk.Application app, Settings settings)
    {
        Object (application: app);

        this.settings = settings;
        game = new Game (settings);
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

    protected override void size_allocate (Gtk.Allocation allocation)
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
        highscores.add_score (game.score,
                              current_category,
                              new Cancellable ());

        show_scores ();
    }

    public void restart_game ()
    {
        game.restart ();
    }

    private void set_status_message (string? message)
    {
        headerbar.set_subtitle (message);
    }

    public void show_scores ()
    {
        highscores.run_dialog ();
    }

    public void change_size (BoardSize size)
    {
        var game_size = settings.get_int ("size");

        if (game_size == size)
            return;

        primary_menu_button.set_active (false);

        if (game.score > 0) {
            var flags = Gtk.DialogFlags.DESTROY_WITH_PARENT;
            var restart_game_dialog = new Gtk.MessageDialog (this,
                                                             flags,
                                                             Gtk.MessageType.WARNING,
                                                             Gtk.ButtonsType.NONE,
                                                             _("Are you sure you want to restart the game?"),
                                                             null);
            restart_game_dialog.add_buttons (_("_Cancel"), Gtk.ResponseType.CANCEL,
                                             _("_Restart"), Gtk.ResponseType.OK,
                                             null);

            var result = restart_game_dialog.run ();
            restart_game_dialog.destroy ();
            switch (result)
            {
                case Gtk.ResponseType.OK:
                     if (!settings.set_int (FiveOrMoreApp.KEY_SIZE, size))
                        warning ("Failed to set size: %d", size);
                    break;
                case Gtk.ResponseType.CANCEL:
                    break;
            }
        } else {
            settings.set_int (FiveOrMoreApp.KEY_SIZE, size);
        }

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
