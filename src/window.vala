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

[GtkTemplate (ui = "/org/gnome/five-or-more/ui/five-or-more.ui")]
private class GameWindow : ApplicationWindow
{
    [GtkChild]
    private unowned HeaderBar headerbar;

    [GtkChild]
    private unowned Box preview_hbox;

    [GtkChild]
    private unowned Games.GridFrame grid_frame;

    private GLib.Settings settings = new GLib.Settings ("org.gnome.five-or-more");
    private bool window_tiled;
    private bool window_maximized;
    private int window_width;
    private int window_height;

    private Game? game = null;
    private ThemeRenderer? theme = null;

    private string[] status = {
        /* Translators: subtitle of the headerbar, at the application start */
        _("Match five objects of the same type in a row to score!"),

        /* Translators: subtitle of the headerbar, when the user clicked on a tile where the selected marble cannot move */
        _("You can’t move there!"),

        /* Translators: subtitle of the headerbar, at the end of a game */
        _("Game Over!"),

        /* Translators: subtitle of the headerbar, during a game; the %d is replaced by the score */
        _("Score: %d")
    };

    private const GLib.ActionEntry win_actions [] =
    {
        { "background",     change_background  },
        { "reset-bg",       reset_background   },

        { "change-size",    null,   "s", "'small'",     change_size     },
        { "change-theme",   null,   "s", "'balls.svg'", change_theme    },

        { "new-game",       new_game           },
        { "scores",         show_scores        }
    };

    construct
    {
        add_action_entries (win_actions, this);

        SimpleAction theme_action = (SimpleAction) lookup_action ("change-theme");
        string theme_value = settings.get_string (FiveOrMoreApp.KEY_THEME);
        if (theme_value != "balls.svg" && theme_value != "shapes.svg" && theme_value != "tango.svg") /* TODO use an enum in GSchema file? */
        {
            settings.set_string (FiveOrMoreApp.KEY_THEME, "balls.svg");
            theme_value = "balls.svg";
        }
        theme_action.set_state (new Variant.@string (theme_value));

        var board_size_action = lookup_action ("change-size");
        string board_size_string;
        int board_size = settings.get_int (FiveOrMoreApp.KEY_SIZE);
        switch (board_size)
        {
            case 1: board_size_string = "small";    break;
            case 2: board_size_string = "medium";   break;
            case 3: board_size_string = "large";    break;
            default: assert_not_reached ();
        }
        ((SimpleAction) board_size_action).set_state (board_size_string);

        game = new Game (board_size);
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

        View game_view = new View (game, theme);
        SimpleAction reset_background_action = (SimpleAction) lookup_action ("reset-bg");
        game_view.notify ["background-color"].connect (() => { reset_background_action.set_enabled (game_view.background_color != View.default_background_color); });
        settings.bind (FiveOrMoreApp.KEY_BACKGROUND_COLOR, game_view, "background-color", SettingsBindFlags.DEFAULT);
        grid_frame.add (game_view);
        game_view.show ();

        grid_frame.show ();

        init_scores_dialog ();
    }

    protected override bool window_state_event (Gdk.EventWindowState event)
    {
        base.window_state_event (event);

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

    internal inline void on_shutdown ()
    {
        settings.delay ();
        settings.set_int ("window-width", window_width);
        settings.set_int ("window-height", window_height);
        settings.set_boolean ("window-is-maximized", window_maximized);
        settings.apply ();
    }

    private void set_status_message (string? message)
    {
        headerbar.set_subtitle (message);
    }

    /*\
    * * Scores dialog
    \*/

    private Games.Scores.Context highscores;

    private inline void init_scores_dialog ()
    {
        var importer = new Games.Scores.DirectoryImporter ();
        highscores = new Games.Scores.Context.with_importer_and_icon_name ("five-or-more",
                                                                           /* Translators: text in the Scores dialog, introducing the combobox */
                                                                           _("Board Size: "),
                                                                           this,
                                                                           create_category_from_key,
                                                                           Games.Scores.Style.POINTS_GREATER_IS_BETTER,
                                                                           importer,
                                                                           "org.gnome.five-or-more");
        game.game_over.connect (score_cb);
    }

    private inline void score_cb ()
    {
        string name = category_name_from_key (game.score_current_category);
        var current_category = new Games.Scores.Category (game.score_current_category, name);
        highscores.add_score.begin (game.score,
                                    current_category,
                                    new Cancellable ());

        show_scores ();
    }

    private inline Games.Scores.Category? create_category_from_key (string key)
    {
        string? name = category_name_from_key (key);
        return new Games.Scores.Category (key, name);
    }

    private inline string category_name_from_key (string key)
    {
        for (int i = 0; i < game.n_categories; i++)
            if (Game.scorecats[i].key == key)
                return dpgettext2 (null, "board size", Game.scorecats[i].name); // C_() should work (and works if you rewrite every scorecat name here), but does not
        return "";
    }

    private inline void show_scores (/* SimpleAction action, Variant? parameter */)
    {
        highscores.run_dialog ();
    }

    /*\
    * * Appearance actions
    \*/

    private inline void change_background ()
    {
        string old_color_string = settings.get_string (FiveOrMoreApp.KEY_BACKGROUND_COLOR);
        /* Translators: title of the ColorChooser dialog that appears from the hamburger menu > "Appearance" submenu > "Background" section > "Select color" entry */
        ColorChooserDialog dialog = new ColorChooserDialog (_("Background color"), this);
        if (!dialog.rgba.parse (old_color_string))
            return;
        dialog.notify ["rgba"].connect ((dialog, param) => {
                var color = ((ColorChooserDialog) dialog).get_rgba ();
                if (!settings.set_string (FiveOrMoreApp.KEY_BACKGROUND_COLOR, color.to_string ()))
                    warning ("Failed to set color: %s", color.to_string ());
            });
        var result = dialog.run ();
        dialog.destroy ();
        if (result == ResponseType.OK)
            return;
        settings.set_string (FiveOrMoreApp.KEY_BACKGROUND_COLOR, old_color_string);
    }

    private inline void reset_background ()
    {
        settings.reset (FiveOrMoreApp.KEY_BACKGROUND_COLOR);
    }

    private inline void change_theme (SimpleAction action, Variant? parameter)
        requires (parameter != null)
    {
        action.set_state (parameter);
        settings.set_string (FiveOrMoreApp.KEY_THEME, ((!) parameter).get_string ());
    }

    /*\
    * * new game actions
    \*/

    private inline void change_size (SimpleAction action, Variant? parameter)
        requires (parameter != null)
    {
        int size;
        action.set_state (parameter);
        switch (parameter.get_string ()) {
            case "small":   size = 1;   break;
            case "medium":  size = 2;   break;
            case "large":   size = 3;   break;
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
                                                         /* Translators: text of a dialog that appears when the user starts a new game while the score is not null */
                                                         _("Are you sure you want to start a new %u × %u game?").printf (n_rows, n_cols));

            /* Translators: button of a dialog that appears when the user starts a new game while the score is not null; the other answer is "_Restart" */
            restart_game_dialog.add_buttons (_("_Cancel"), ResponseType.CANCEL,

            /* Translators: button of a dialog that appears when the user starts a new game while the score is not null; the other answer is "_Cancel" */
                                             _("_Restart"), ResponseType.OK);

            var result = restart_game_dialog.run ();
            restart_game_dialog.destroy ();
            if (result != ResponseType.OK)
                return;
        }
        game.new_game (size);
    }
}
