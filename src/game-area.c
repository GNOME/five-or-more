/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

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

#include <config.h>

#include <stdlib.h>
#include <locale.h>
#include <math.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>

#include "games-gridframe.h"
#include "game-area.h"
#include "five-or-more-app.h"
#include "balls-preview.h"

#define MAXFIELDSIZE 30
#define KEY_SIZE              "size"
static GRand *rgen;
static int inmove = 0;
static int active = -1;
static int target = -1;

static int animate_id = 0;

static gboolean show_cursor = FALSE;
static int cursor_x = MAXFIELDSIZE / 2;
static int cursor_y = MAXFIELDSIZE / 2;
static field_props field[MAXFIELDSIZE * MAXFIELDSIZE];
static gint hfieldsize;
static gint vfieldsize;
static int boxsize;
static gint ncolors;

static gint npieces;
/* The tile images with balls rendered on them. */
static cairo_surface_t *ball_surface = NULL;
/* A cairo_surface_t of a blank tile. */
static cairo_surface_t *blank_surface = NULL;
/* Pre-rendering image data prepared from file. */
static GamesPreimage *ball_preimage = NULL;

enum {
  UNSET = 0,
  SMALL = 1,
  MEDIUM,
  LARGE,
  MAX_SIZE,
};

/* Keep these in sync with the enum above. */
static const gint field_sizes[MAX_SIZE][4] = {
  {-1, -1, -1, -1}, /* This is a dummy entry. */
  {7, 7, 5, 3},     /* SMALL */
  {9, 9, 7, 3},     /* MEDIUM */
  {20, 15, 7, 7}    /* LARGE */
};

static scoretable sctab[] =
  { {5, 10}, {6, 12}, {7, 18}, {8, 28}, {9, 42}, {10, 82}, {11, 108}, {12,
                                                                       138},
  {13, 172}, {14, 210}, {0, 0} };

static gchar *warning_message = NULL;
static GtkWidget *draw_area;

void
set_sizes (gint size)
{
  hfieldsize = field_sizes[size][0];
  vfieldsize = field_sizes[size][1];
  ncolors = field_sizes[size][2];
  npieces = field_sizes[size][3];
  gint *game_size = get_game_size();
  *game_size = size;
  GamesScoresCategory *scorecats = get_scorecats();
  games_scores_set_category (get_highscores(), scorecats[size - 1].key);

  g_settings_set_int (*(get_settings()), KEY_SIZE, size);
  GtkWidget *gridframe = get_gridframe();
  if (gridframe)
    games_grid_frame_set (GAMES_GRID_FRAME (gridframe),
                          hfieldsize, vfieldsize);
}

gint
get_hfieldsize()
{
  return hfieldsize;
}

gint
get_vfieldsize()
{
  return vfieldsize;
}

gint *
get_active_status()
{
  return &active;
}

gint *
get_target_status()
{
  return &target;
}

gint *
get_inmove_status()
{
  return &inmove;
}

gint
get_ncolors()
{
  return ncolors;
}

GRand **
get_rgen()
{
  return &rgen;
}

GamesPreimage *
get_ball_preimage()
{
  return ball_preimage;
}

gint
get_npieces()
{
  return npieces;
}

int
init_new_balls (int num, int prev)
{
  int i, j = -1;
  gfloat num_boxes = hfieldsize * vfieldsize;
  for (i = 0; i < num;) {
    j = g_rand_int_range (rgen, 0, num_boxes);
    if (field[j].color == 0) {
      field[j].color = (prev == -1) ?
        g_rand_int_range (rgen, 1, ncolors + 1) : get_preview()[prev];
      i++;
    }
  }
  return j;
}

void
reset_game (void)
{
  int i;

  for (i = 0; i < hfieldsize * vfieldsize; i++) {
    field[i].color = 0;
    field[i].phase = 0;
    field[i].active = 0;
    field[i].pathsearch = -1;
  }
  update_score(0);
  init_preview ();
  init_new_balls (npieces, -1);
}

static void
fix_route (int num, int pos)
{
  int i;

  for (i = 0; i < hfieldsize * vfieldsize; i++)
    if ((i != pos) && (field[i].pathsearch == num))
      field[i].pathsearch = -1;
  if (num < 2)
    return;
  if ((pos / hfieldsize > 0)
      && (field[pos - hfieldsize].pathsearch == num - 1))
    fix_route (num - 1, pos - hfieldsize);
  if ((pos % hfieldsize > 0) && (field[pos - 1].pathsearch == num - 1))
    fix_route (num - 1, pos - 1);
  if ((pos / hfieldsize < vfieldsize - 1)
      && (field[pos + hfieldsize].pathsearch == num - 1))
    fix_route (num - 1, pos + hfieldsize);
  if ((pos % hfieldsize < hfieldsize - 1)
      && (field[pos + 1].pathsearch == num - 1))
    fix_route (num - 1, pos + 1);
}

static int
route (int num)
{
  int i;
  int flag = 0;

  if (field[target].pathsearch == num)
    return 1;
  for (i = 0; i < hfieldsize * vfieldsize; i++) {
    if (field[i].pathsearch == num) {
      flag = 1;
      if ((i / hfieldsize > 0) && (field[i - hfieldsize].pathsearch == -1)
          && (field[i - hfieldsize].color == 0))
        field[i - hfieldsize].pathsearch = num + 1;
      if ((i / hfieldsize < vfieldsize - 1)
          && (field[i + hfieldsize].pathsearch == -1)
          && (field[i + hfieldsize].color == 0))
        field[i + hfieldsize].pathsearch = num + 1;
      if ((i % hfieldsize > 0) && (field[i - 1].pathsearch == -1)
          && (field[i - 1].color == 0))
        field[i - 1].pathsearch = num + 1;
      if ((i % hfieldsize < hfieldsize - 1) && (field[i + 1].pathsearch == -1)
          && (field[i + 1].color == 0)) {
        field[i + 1].pathsearch = num + 1;
      }
    }
  }
  if (flag == 0)
    return 0;
  return 2;
}

int
find_route (void)
{
  int i = 0;
  int j = 0;

  field[active].pathsearch = i;
  while ((j = route (i))) {
    if (j == 1) {
      fix_route (i, target);
      return 1;
    }
    i++;
  }
  return 0;
}


static void
reset_pathsearch (void)
{
  int i;

  for (i = 0; i < hfieldsize * vfieldsize; i++)
    field[i].pathsearch = -1;
}

void
draw_box (GtkWidget * widget, int x, int y)
{
  gtk_widget_queue_draw_area (widget, x * boxsize, y * boxsize,
                              boxsize, boxsize);
}


static void
deactivate (GtkWidget * widget, int x, int y)
{
  field[active].active = 0;
  field[active].phase = 0;
  draw_box (widget, x, y);
  active = -1;
}

static int
spaces_left (void)
{
  int i, j;

  j = 0;

  for (i = 0; i < hfieldsize * vfieldsize; i++) {
    if (field[i].color == 0)
      j++;
  }

  return j;
}

static int
check_gameover (void)
{
  if (spaces_left () > 0)
    return 1;
  game_over ();
  return -1;
}


static int
addscore (int num)
{
  gchar string[20];
  int i = 0;
  int retval;

  while ((sctab[i].num != num) && (sctab[i].num != 0))
    i++;
  if (sctab[i].num == 0) {
    num-=5;
    retval = sctab[num%5].score + 72 * (num/5) + (num-5)*24 ;
  } else
    retval = sctab[i].score;
  update_score(retval);

  return retval;
}


static void
tag_list (int *list, int len)
{
  int i;

  for (i = 0; i < len; i++)
    field[list[i]].tag = 1;
}

static int
find_lines (int num)
{
  int count = 1;
  int subcount = 0;
  int x = num % hfieldsize;
  int y = num / hfieldsize;
  int list[hfieldsize];

  /* Horizontal */

  x++;
  while ((x <= hfieldsize - 1)
         && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    x++;
  }
  x = num % hfieldsize - 1;
  while ((x >= 0) && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    x--;
  }
  if (subcount >= 4) {
    field[num].tag = 1;
    tag_list (list, subcount);
    count += subcount;
  }
  subcount = 0;

  /* Vertical */

  x = num % hfieldsize;
  y++;
  while ((y <= vfieldsize - 1)
         && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    y++;
  }
  y = num / hfieldsize - 1;
  while ((y >= 0) && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    y--;
  }
  if (subcount >= 4) {
    field[num].tag = 1;
    tag_list (list, subcount);
    count += subcount;
  }
  subcount = 0;

  /* Diagonal ++ */

  x = num % hfieldsize + 1;
  y = num / hfieldsize + 1;
  while ((y <= vfieldsize - 1) && (x <= hfieldsize - 1)
         && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    y++;
    x++;
  }
  x = num % hfieldsize - 1;
  y = num / hfieldsize - 1;
  while ((y >= 0) && (x >= 0)
         && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    y--;
    x--;
  }
  if (subcount >= 4) {
    field[num].tag = 1;
    tag_list (list, subcount);
    count += subcount;
  }
  subcount = 0;

  /* Diagonal +- */

  x = num % hfieldsize + 1;
  y = num / hfieldsize - 1;
  while ((y >= 0) && (x <= hfieldsize - 1)
         && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    y--;
    x++;
  }
  x = num % hfieldsize - 1;
  y = num / hfieldsize + 1;
  while ((y <= vfieldsize - 1) && (x >= 0)
         && (field[x + y * hfieldsize].color == field[num].color)) {
    list[subcount] = x + y * hfieldsize;
    subcount++;
    y++;
    x--;
  }
  if (subcount >= 4) {
    field[num].tag = 1;
    tag_list (list, subcount);
    count += subcount;
  }

  return count;
}


static void
clear_tags (void)
{
  int i;

  for (i = 0; i < hfieldsize * vfieldsize; i++)
    field[i].tag = 0;
}

static int
kill_tags (GtkWidget * widget)
{
  int i, j;

  j = 0;
  for (i = 0; i < hfieldsize * vfieldsize; i++) {
    if (field[i].tag == 1) {
      j++;
      field[i].color = 0;
      field[i].phase = 0;
      field[i].active = 0;
      draw_box (widget, i % hfieldsize, i / hfieldsize);
    }
  }

  return j;
}

static gboolean
check_goal (GtkWidget * w, int target)
{
  gboolean lines_cleared;
  int count;

  clear_tags ();

  count = find_lines (target);
  lines_cleared = count >= 5;

  if (lines_cleared) {
    kill_tags (w);
    addscore (count);
  }
  reset_pathsearch ();

  return lines_cleared;
}

gint
animate (gpointer gp)
{
  GtkWidget *widget = GTK_WIDGET (gp);
  int x, y;
  int newactive = 0;

  x = active % hfieldsize;
  y = active / hfieldsize;

  if (active == -1) {
    animate_id = 0;
    return FALSE;
  }

  if (inmove != 0) {
    if ((x > 0)
        && (field[active - 1].pathsearch == field[active].pathsearch + 1))
      newactive = active - 1;
    else if ((x < hfieldsize - 1)
             && (field[active + 1].pathsearch ==
                 field[active].pathsearch + 1))
      newactive = active + 1;
    else if ((y > 0)
             && (field[active - hfieldsize].pathsearch ==
                 field[active].pathsearch + 1))
      newactive = active - hfieldsize;
    else if ((y < vfieldsize - 1)
             && (field[active + hfieldsize].pathsearch ==
                 field[active].pathsearch + 1))
      newactive = active + hfieldsize;
    else {
      set_inmove (0);
    }
    draw_box (widget, x, y);
    x = newactive % hfieldsize;
    y = newactive / hfieldsize;
    field[newactive].phase = field[active].phase;
    field[newactive].color = field[active].color;
    field[active].phase = 0;
    field[active].color = 0;
    field[active].active = 0;
    if (newactive == target) {
      target = -1;
      set_inmove (0);
      active = -1;
      field[newactive].phase = 0;
      field[newactive].active = 0;
      draw_box (widget, x, y);
      reset_pathsearch ();
      if (!check_goal (widget, newactive)) {
        gboolean fullline;
        int balls[npieces];
        int spaces;

        clear_tags ();
        fullline = FALSE;
        spaces = spaces_left ();
        if (spaces > npieces)
          spaces = npieces;
        for (x = 0; x < spaces; x++) {
          int tmp = init_new_balls (1, x);
          draw_box (widget, tmp % hfieldsize, tmp / hfieldsize);
          balls[x] = tmp;
          fullline = (find_lines (balls[x]) >= 5) || fullline;
        }
        if (fullline)
          addscore (kill_tags (widget));
        if (check_gameover () == -1)
          return FALSE;
        init_preview ();
        draw_preview ();
      }
      return TRUE;
    }
    field[newactive].active = 1;
    active = newactive;
  }

  field[active].phase++;
  if (field[active].phase >= 4)
    field[active].phase = 0;
  draw_box (widget, x, y);

  return TRUE;
}

static void
start_animation (void)
{
  int ms;

  ms = (inmove ? get_move_timeout() : 100);
  if (animate_id == 0)
    animate_id = g_timeout_add (ms, animate, draw_area);
}

void
set_inmove (int i)
{
  if (inmove != i) {
    inmove = i;
    if (animate_id)
      g_source_remove (animate_id);
    animate_id = 0;
    start_animation ();
  }
}

static void
cell_clicked (GtkWidget * widget, int fx, int fy)
{
  int x, y;

  set_status_message (NULL);
  if (field[fx + fy * hfieldsize].color == 0) {
    /* Clicked on an empty field */

    if ((active != -1) && (inmove != 1)) {
      /* We have an active ball, and it's not
       * already in move. Therefore we should begin
       * the moving sequence */

      target = fx + fy * hfieldsize;
      if (find_route ()) {
        /* We found a route to the new position */

        set_inmove (1);
      } else {
        /* Can't move there! */
        set_status_message (_("You can’t move there!"));
        reset_pathsearch ();
        target = -1;
      }
    }
  } else {
    /* Clicked on a ball */

    if ((fx + fy * hfieldsize) == active) {
      /* It's active, so let's deactivate it! */

      deactivate (widget, fx, fy);
    } else {
      /* It's not active, so let's activate it! */

      if (active != -1) {
        /* There is an other active ball, we should deactivate it first! */

        x = active % hfieldsize;
        y = active / hfieldsize;
        deactivate (widget, x, y);
      }

      active = fx + fy * hfieldsize;
      field[active].active = 1;
      start_animation();
    }
  }

}

static gint
button_press_event (GtkWidget * widget, GdkEvent * event)
{
  int x, y;
  int fx, fy;

  if (inmove)
    return TRUE;

  /* Ignore the 2BUTTON and 3BUTTON events. */
  if (event->type != GDK_BUTTON_PRESS)
    return TRUE;

  if (show_cursor) {
    show_cursor = FALSE;
    draw_box (draw_area, cursor_x, cursor_y);//put in header
  }

  fx = event->button.x / boxsize;
  fy = event->button.y / boxsize;

  /* If we click on the outer border pixels, the previous calculation
   * will be wrong and we must correct. */
  fx = CLAMP (fx, 0, hfieldsize - 1);
  fy = CLAMP (fy, 0, vfieldsize - 1);

  cursor_x = fx;
  cursor_y = fy;

  cell_clicked (widget, fx, fy);

  return TRUE;
}

static void
move_cursor (int dx, int dy)
{
  int old_x = cursor_x;
  int old_y = cursor_y;

  if (!show_cursor) {
    show_cursor = TRUE;
    draw_box (draw_area, cursor_x, cursor_y);
  }

  cursor_x = cursor_x + dx;
  if (cursor_x < 0)
    cursor_x = 0;
  if (cursor_x >= hfieldsize)
    cursor_x = hfieldsize - 1;
  cursor_y = cursor_y + dy;
  if (cursor_y < 0)
    cursor_y = 0;
  if (cursor_y >= vfieldsize)
    cursor_y = vfieldsize - 1;

  if (cursor_x == old_x && cursor_y == old_y)
    return;

  draw_box (draw_area, old_x, old_y);
  draw_box (draw_area, cursor_x, cursor_y);
}

static gboolean
key_press_event (GtkWidget * widget, GdkEventKey * event, void *d)
{
  guint key;

  key = event->keyval;

  switch (key) {
  case GDK_KEY_Left:
  case GDK_KEY_KP_Left:
    move_cursor (-1, 0);
    break;
  case GDK_KEY_Right:
  case GDK_KEY_KP_Right:
    move_cursor (1, 0);
    break;
  case GDK_KEY_Up:
  case GDK_KEY_KP_Up:
    move_cursor (0, -1);
    break;
  case GDK_KEY_Down:
  case GDK_KEY_KP_Down:
    move_cursor (0, 1);
    break;
  case GDK_KEY_Home:
  case GDK_KEY_KP_Home:
    move_cursor (-999, 0);
    break;
  case GDK_KEY_End:
  case GDK_KEY_KP_End:
    move_cursor (999, 0);
    break;
  case GDK_KEY_Page_Up:
  case GDK_KEY_KP_Page_Up:
    move_cursor (0, -999);
    break;
  case GDK_KEY_Page_Down:
  case GDK_KEY_KP_Page_Down:
    move_cursor (0, 999);
    break;

  case GDK_KEY_space:
  case GDK_KEY_Return:
  case GDK_KEY_KP_Enter:
    if (show_cursor)
      cell_clicked (widget, cursor_x, cursor_y);
    break;
  }

  return TRUE;
}

static void
draw_grid (cairo_t *cr)
{
  GdkRGBA color;
  guint w, h;
  guint i;
  w = gtk_widget_get_allocated_width(draw_area);
  h = gtk_widget_get_allocated_height(draw_area);

  gdk_rgba_parse (&color, "#525F6C");
  gdk_cairo_set_source_rgba (cr, &color);
  cairo_set_line_width (cr, 1.0);

  for (i = boxsize; i < w; i = i + boxsize)
  {
    cairo_move_to (cr, i + 0.5, 0 + 0.5);
    cairo_line_to (cr, i + 0.5, h + 0.5);
  }

  for (i = boxsize; i < h; i = i + boxsize)
  {
    cairo_move_to (cr, 0 + 0.5, i + 0.5);
    cairo_line_to (cr, w + 0.5, i + 0.5);
  }

  cairo_rectangle (cr, 0.5, 0.5, w - 0.5, h - 0.5);
  cairo_stroke (cr);
}

static GamesPreimage *
load_image (gchar * fname)
{
  GamesPreimage *preimage;
  gchar *path;
  GError *error = NULL;

  path = g_build_filename (DATA_DIRECTORY, "themes", fname, NULL);
  if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
    warning_message = g_strdup_printf (_("Unable to locate file:\n%s\n\n"
                                         "The default theme will be loaded instead."),
                                       fname);

    path = g_build_filename (DATA_DIRECTORY, "themes", "balls.svg", NULL);
    if (!g_file_test (path, G_FILE_TEST_EXISTS)) {
      g_free (warning_message);
      warning_message = g_strdup_printf (_("Unable to locate file:\n%s\n\n"
                                           "Please check that Five or More is installed correctly."),
                                         fname);
    }
  }

  preimage = games_preimage_new_from_file (path, &error);
  g_free (path);

  if (error) {
    warning_message = g_strdup (error->message);
    g_error_free (error);
  }

  return preimage;
}

static void
show_image_warning (gchar * message)
{
  GtkWidget *dialog;
  GtkWidget *button;

  dialog = gtk_message_dialog_new (NULL,
                                   GTK_DIALOG_MODAL,
                                   GTK_MESSAGE_WARNING,
                                   GTK_BUTTONS_CLOSE,
                                   "%s",_("Could not load theme"));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            "%s", message);

  button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                  _("Preferences"), GTK_RESPONSE_ACCEPT);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

  g_signal_connect (button, "clicked", G_CALLBACK (game_props_callback),
                    NULL);

  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

void
refresh_pixmaps (void)
{
  cairo_t *cr, *cr_blank;
  GdkPixbuf *ball_pixbuf = NULL;
  background backgnd = get_backgnd();
  /* Since we get called both by configure and after loading an image.
   * it is possible the pixmaps aren't initialised. If they aren't
   * we don't do anything. */
  if (!ball_surface)
    return;

  if (!boxsize)
    return;

  if (ball_preimage) {
    ball_pixbuf = games_preimage_render (ball_preimage, 4 * boxsize,
                                         7 * boxsize);

    /* Handle rendering problems. */
    if (!ball_pixbuf) {
      g_object_unref (ball_preimage);
      ball_preimage = NULL;

      if (!warning_message) {
        warning_message = g_strdup ("The selected theme failed to render.\n\n"
                                    "Please check that Five or More is installed correctly.");
      }
    }
  }

  if (!ball_pixbuf) {
    ball_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                                  4 * boxsize, 7 * boxsize);
    gdk_pixbuf_fill (ball_pixbuf, 0x00000000);
  }

  if (warning_message)
    show_image_warning (warning_message);
  g_free (warning_message);
  warning_message = NULL;

  cr = cairo_create (ball_surface);
  gdk_cairo_set_source_rgba (cr, &backgnd.color);

  cairo_rectangle (cr, 0, 0, boxsize * 4, boxsize * 7);
  cairo_fill (cr);

  gdk_cairo_set_source_pixbuf (cr, ball_pixbuf, 0, 0);
  cairo_mask (cr, cairo_get_source (cr));
  g_object_unref (ball_pixbuf);

  cairo_destroy (cr);

  cr_blank = cairo_create (blank_surface);
  gdk_cairo_set_source_rgba (cr_blank, &backgnd.color);

  cairo_rectangle (cr_blank, 0, 0, boxsize, boxsize);
  cairo_fill (cr_blank);

  cairo_destroy (cr_blank);
}

static void
draw_all_balls (GtkWidget * widget)
{
  gdk_window_invalidate_rect (gtk_widget_get_window (widget), NULL, FALSE);
}

void
refresh_screen (void)
{
  draw_all_balls (draw_area);
  draw_preview ();
}

static int
configure_event_callback (GtkWidget * widget, GdkEventConfigure * event)
{
  if (ball_surface)
    cairo_surface_destroy(ball_surface);

  if (blank_surface)
    cairo_surface_destroy (blank_surface);

  boxsize = (event->width - 1) / hfieldsize;
  ball_surface = gdk_window_create_similar_surface (gtk_widget_get_window (draw_area),
                                                    CAIRO_CONTENT_COLOR_ALPHA,
                                                    boxsize * 4, boxsize * 7);
  blank_surface = gdk_window_create_similar_surface (gtk_widget_get_window (draw_area),
                                                     CAIRO_CONTENT_COLOR_ALPHA,
                                                     boxsize, boxsize);
  refresh_pixmaps ();

  refresh_screen ();

  return TRUE;
}

void
load_theme (void)
{
  if (ball_preimage)
    g_object_unref (ball_preimage);
  char *ball_filename = get_ball_filename();
  ball_preimage = load_image (ball_filename);

  refresh_pixmaps ();
  refresh_preview_surfaces ();
}

static gboolean
field_draw_callback (GtkWidget * widget, cairo_t *cr)
{
  guint i, j, idx;
  GdkRGBA cursorColor;
  background backgnd = get_backgnd();

  for (i = 0; i < vfieldsize; i++) {
    for (j = 0; j < hfieldsize; j++) {
      int phase, color;

      idx = j + i * hfieldsize;

      cairo_rectangle (cr, j * boxsize, i * boxsize, boxsize, boxsize);

      if (field[idx].color != 0) {
        phase = field[idx].phase;
        color = field[idx].color - 1;

        phase = ABS (ABS (3 - phase) - 3);

        cairo_set_source_surface (cr, ball_surface,
                                 (1.0 * j - phase) * boxsize,
                                 (1.0 * i - color) * boxsize);
      } else {
        cairo_set_source_surface (cr, blank_surface, 1.0 * j * boxsize, 1.0 * i * boxsize);
      }

      cairo_fill (cr);
    }
  }

  /* Cursor */
  if (show_cursor) {
    if (((backgnd.color.red + backgnd.color.green + backgnd.color.blue) / 3) > 0.5)
      gdk_rgba_parse (&cursorColor, "#000000");
    else
      gdk_rgba_parse (&cursorColor, "#FFFFFF");

    gdk_cairo_set_source_rgba (cr, &cursorColor);
    cairo_set_line_width (cr, 1.0);
    cairo_rectangle (cr,
                     cursor_x * boxsize + 1.5, cursor_y * boxsize + 1.5,
                     boxsize - 2.5, boxsize - 2.5);
    cairo_stroke (cr);
  }

  draw_grid (cr);

  return FALSE;
}

GtkWidget *
game_area_init (void)
{
  draw_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (draw_area, 300, 300);
  g_signal_connect (draw_area, "button-press-event",
                    G_CALLBACK (button_press_event), NULL);
  g_signal_connect (draw_area, "key-press-event",
                    G_CALLBACK (key_press_event), NULL);
  g_signal_connect (draw_area, "configure-event",
                    G_CALLBACK (configure_event_callback), NULL);
  g_signal_connect (draw_area, "draw",
                    G_CALLBACK (field_draw_callback), NULL);
  gtk_widget_set_events (draw_area,
                         gtk_widget_get_events (draw_area) |
                         GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK);

  gtk_widget_set_can_focus (draw_area, TRUE);
  gtk_widget_grab_focus (draw_area);
  return draw_area;
}