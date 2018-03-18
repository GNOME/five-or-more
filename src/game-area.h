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

#ifndef GAME_AREA_H
#define GAME_AREA_H

#include <gtk/gtk.h>
#include "games-preimage.h"

typedef struct {
  int num;
  int score;
} scoretable;

typedef struct {
  int color;
  int pathsearch;
  int phase;
  int active;
  int tag;
} field_props;

GRand           **get_rgen          ();
GamesPreimage    *get_ball_preimage ();
gint              get_npieces       ();
void              set_inmove        (int i);
void              refresh_pixmaps   (void);
void              refresh_screen    (void);
void              load_theme        (void);
GtkWidget       * game_area_init    (void);
gint              get_ncolors       ();
void              set_sizes         (gint size);
void              reset_game        (void);
gint              get_hfieldsize    ();
gint              get_vfieldsize    ();
#endif