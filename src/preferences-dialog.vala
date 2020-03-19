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

[GtkTemplate (ui = "/org/gnome/five-or-more/ui/preferences-dialog.ui")]
public class PreferencesDialog : Gtk.Dialog
{
    private Settings settings;

    [GtkChild]
    private Gtk.ComboBoxText theme_box;

    [GtkChild]
    private Gtk.ColorButton color_button;

    private void theme_set_cb (Gtk.ComboBox self)
    {
        var combo_box_text = self as Gtk.ComboBoxText;
        var theme = combo_box_text.get_active_id ();
        foreach (var t in ThemeRenderer.themes)
        {
            if (t.split (".")[0] == theme)
            {
                if (!settings.set_string ("ball-theme", t))
                    warning ("Failed to set theme: %s", t);

                return;
            }
        }
    }

    private void color_set_cb (Gtk.ColorButton self)
    {
        var color = self.get_rgba ();
        if (!settings.set_string (FiveOrMoreApp.KEY_BACKGROUND_COLOR, color.to_string ()))
            warning ("Failed to set color: %s", color.to_string ());
    }

    public PreferencesDialog (Settings settings)
    {
        this.settings = settings;

         /* Set up theme */
        string theme = settings.get_string (FiveOrMoreApp.KEY_THEME);
        for (int id = 0; id < ThemeRenderer.themes.length; id++)
        {
            if (ThemeRenderer.themes[id] == theme)
                theme_box.set_active (id);
        }
        theme_box.changed.connect (theme_set_cb);

        /* Set up board color */
        var color_str = settings.get_string (FiveOrMoreApp.KEY_BACKGROUND_COLOR);
        Gdk.RGBA color = Gdk.RGBA ();
        color.parse (color_str);
        color_button.set_rgba (color);
        color_button.color_set.connect (color_set_cb);

    }
}
