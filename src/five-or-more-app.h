/*
 * Color lines for GNOME
 * Copyright © 1999 Free Software Foundation
 * Authors: Robert Szokovacs <szo@appaloosacorp.hu>
 *          Szabolcs Ban <shooby@gnome.hu>
 *          Karuna Grewal <karunagrewal98@gmail.com>
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

#ifndef FIVE_OR_MORE_APP_H
#define FIVE_OR_MORE_APP_H

#include <gtk/gtk.h>

typedef struct _background background;
struct _background{
  GdkRGBA color;
  gchar *name;
  gint set;
} ;

GtkWidget                   *get_gridframe              ();
char                        *get_ball_filename          ();
background                   get_backgnd                ();
void                         set_status_message         (gchar * message);
void                         game_over                  (void);
void                         set_application_callbacks  (GtkApplication *application);
gint                        *get_game_size              ();
GSettings                  **get_settings               ();
int                          get_move_timeout           ();
void                         game_props_callback        (GSimpleAction *action,
                                                         GVariant *parameter,
                                                         gpointer user_data);
void                         update_score               (guint value);
#endif
