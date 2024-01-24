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

private class ThemeRenderer
{
    private Settings settings;

    internal const int DEFAULT_SPRITE_SIZE = 20;
    private int sprite_size = DEFAULT_SPRITE_SIZE;
    private double sprite_sheet_width;
    private double sprite_sheet_height;
    private double theme_sprite_size;

    private string theme_name;
    private Rsvg.Handle? theme = null;
    internal const string themes[] = {
            "balls.svg",
            "shapes.svg",
            "tango.svg",
    };

    private Cairo.Pattern? tile_pattern = null;
    private Cairo.Context cr_preview;

    internal signal void theme_changed ();
    private bool is_theme_changed;

    internal ThemeRenderer (Settings settings)
    {
        this.settings = settings;

        settings.changed[FiveOrMoreApp.KEY_THEME].connect (change_theme_cb);
        change_theme_cb ();

        is_theme_changed = false;
    }

    private void change_theme_cb ()
    {
        theme_name = settings.get_string (FiveOrMoreApp.KEY_THEME);
        var theme_file = Path.build_filename (DATA_DIRECTORY, "themes", theme_name);

        try
        {
            load_theme (theme_file);
        }
        catch (Error e)
        {
            theme_name = settings.get_default_value (FiveOrMoreApp.KEY_THEME).get_string ();
            theme_file = Path.build_filename (DATA_DIRECTORY, "themes", theme_name);

            try
            {
                load_theme (theme_file);
            }
            catch
            {
                GLib.warning ("Unable to load default theme\n");
            }

            GLib.warning ("Unable to load chosen theme\n");
        }
    }

    private void load_theme (string theme_file) throws Error
    {
        theme = new Rsvg.Handle.from_file (theme_file);
        theme.get_intrinsic_size_in_pixels (out sprite_sheet_width, out sprite_sheet_height);
        theme_sprite_size = sprite_sheet_height/Game.N_TYPES;

        theme_changed ();
        is_theme_changed = true;
    }

    internal void render_sprite (Cairo.Context cr, int type, int animation, double x, double y, int size)
    {
        if (is_theme_changed || tile_pattern == null || sprite_size != size)
        {
            is_theme_changed = false;
            sprite_size = size;

            var preview_surface = new Cairo.Surface.similar (cr.get_target (),
                                                             Cairo.Content.COLOR_ALPHA,
                                                             Game.N_ANIMATIONS * sprite_size,
                                                             Game.N_TYPES * sprite_size);
            cr_preview = new Cairo.Context (preview_surface);

            var matrix = Cairo.Matrix.identity ();
            matrix.scale ((sprite_sheet_width/theme_sprite_size) * sprite_size / sprite_sheet_width,
                          Game.N_TYPES * sprite_size / sprite_sheet_height);
            cr_preview.set_matrix (matrix);

            theme.render_cairo (cr_preview);

            tile_pattern = new Cairo.Pattern.for_surface (preview_surface);
        }

        cr.set_source (tile_pattern);

        var texture_x = sprite_size * animation;
        var texture_y = sprite_size * type;

        var m = Cairo.Matrix.identity ();
        m.translate (texture_x - x, texture_y - y);
        tile_pattern.set_matrix (m);

        cr.rectangle (x, y, sprite_size, sprite_size);
        cr.fill ();
    }
}
