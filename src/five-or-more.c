/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

/*
 * Color lines for GNOME
 * Copyright © 1999 Free Software Foundation
 * Authors: Robert Szokovacs <szo@appaloosacorp.hu>
 *          Szabolcs Ban <shooby@gnome.hu>
 *
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 * USA
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

#include "five-or-more.h"
#include "games-file-list.h"
#include "games-preimage.h"
#include "games-gridframe.h"
#include "games-scores.h"
#include "games-scores-dialog.h"

#define KEY_BACKGROUND_COLOR  "background-color"
#define KEY_BALL_THEME        "ball-theme"
#define KEY_MOVE_TIMEOUT      "move-timeout"
#define KEY_SIZE              "size"

#define KEY_SAVED_SCORE   "score"
#define KEY_SAVED_FIELD   "field"
#define KEY_SAVED_PREVIEW "preview"

#define MAXNPIECES 10
#define MAXFIELDSIZE 30
#define DEFAULT_GAME_SIZE MEDIUM
#define DEFAULT_BALL_THEME "balls.svg"

#define PREVIEW_IMAGE_WIDTH 20
#define PREVIEW_IMAGE_HEIGHT 20

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

static const GamesScoresCategory scorecats[] = {
  { "Small",  NC_("board size", "Small")  },
  { "Medium", NC_("board size", "Medium") },
  { "Large",  NC_("board size", "Large")  }
};

static GamesScores *highscores;
static GSettings *settings;
static GtkBuilder *builder;
static GtkBuilder *builder_preferences;

static gint hfieldsize;
static gint vfieldsize;
static gint ncolors;
static gint npieces;
static gint game_size = UNSET;
static gboolean pref_dialog_done = FALSE;

static GRand *rgen;

static GtkWidget *draw_area;
static GtkWidget *app, *headerbar, *pref_dialog, *gridframe, *restart_game_dialog;
static GtkWidget *size_radio_s, *size_radio_m, *size_radio_l;

static gint window_width = 0, window_height = 0;
static gboolean window_is_fullscreen = FALSE, window_is_maximized = FALSE;

static field_props field[MAXFIELDSIZE * MAXFIELDSIZE];

/* Pre-rendering image data prepared from file. */
static GamesPreimage *ball_preimage = NULL;
/* The tile images with balls rendered on them. */
static cairo_surface_t *ball_surface = NULL;

static GtkImage* preview_images[MAXNPIECES];
static GdkPixbuf* preview_pixbufs[MAXNPIECES];

/* A cairo_surface_t of a blank tile. */
static cairo_surface_t *blank_surface = NULL;

static GamesFileList *theme_file_list = NULL;

static int active = -1;
static int target = -1;
static int inmove = 0;
static guint score = 0;
static int cursor_x = MAXFIELDSIZE / 2;
static int cursor_y = MAXFIELDSIZE / 2;
static gboolean show_cursor = FALSE;

static int boxsize;

static int move_timeout = 100;
static int animate_id = 0;
static int preview[MAXNPIECES];
static char *ball_filename;
static GtkWidget *scorelabel;
static scoretable sctab[] =
  { {5, 10}, {6, 12}, {7, 18}, {8, 28}, {9, 42}, {10, 82}, {11, 108}, {12,
                                                                       138},
  {13, 172}, {14, 210}, {0, 0} };

static struct {
  GdkRGBA color;
  gchar *name;
  gint set;
} backgnd = { { 0, 0, 0, 0}, NULL, 0 };

static gchar *warning_message = NULL;

static void
set_status_message (gchar * message)
{
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (headerbar), message);
}

static void
show_image_warning (gchar * message)
{
  GtkWidget *dialog;
  GtkWidget *button;

  dialog = gtk_message_dialog_new (GTK_WINDOW (app),
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
refresh_pixmaps (void)
{
  cairo_t *cr, *cr_blank;
  GdkPixbuf *ball_pixbuf = NULL;

  /* Since we get called both by configure and after loading an image.
   * it is possible the pixmaps aren't initialised. If they aren't
   * we don't do anything. */
  if (!ball_surface)
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
refresh_preview_surfaces (void)
{
  guint i;
  GdkPixbuf *scaled = NULL;
  GtkWidget *widget = GTK_WIDGET (preview_images[0]);
  GtkStyleContext *context;
  GdkRGBA bg;
  cairo_t *cr;
  GdkRectangle preview_rect;
  cairo_surface_t *blank_preview_surface = NULL;
  /* The balls rendered to a size appropriate for the preview. */
  cairo_surface_t *preview_surface = NULL;

  context = gtk_widget_get_style_context (widget);
  gtk_style_context_get_background_color (context, GTK_STATE_FLAG_NORMAL, &bg);

  /* Like the refresh_pixmaps() function, we may be called before
   * the window is ready. */
  if (PREVIEW_IMAGE_HEIGHT == 0)
    return;

  preview_rect.x = 0;
  preview_rect.y = 0;
  preview_rect.width = PREVIEW_IMAGE_WIDTH;
  preview_rect.height = PREVIEW_IMAGE_HEIGHT;

  /* We create pixmaps for each of the ball colours and then
   * set them as the background for each widget in the preview array.
   * This code assumes that each preview window is identical. */

  if (ball_preimage)
    scaled = games_preimage_render (ball_preimage, 4 * PREVIEW_IMAGE_WIDTH,
                                    7 * PREVIEW_IMAGE_HEIGHT);

  if (!scaled) {
    scaled = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                             4 * PREVIEW_IMAGE_WIDTH, 7 * PREVIEW_IMAGE_HEIGHT);
    gdk_pixbuf_fill (scaled, 0x00000000);
  }

  for (i = 0; i < 7; i++) {
    preview_surface = gdk_window_create_similar_image_surface (gtk_widget_get_window (widget),
                                                               CAIRO_FORMAT_ARGB32,
                                                               PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT, 1);
    cr = cairo_create (preview_surface);
    gdk_cairo_set_source_rgba (cr, &bg);
    gdk_cairo_rectangle (cr, &preview_rect);
    cairo_fill (cr);

    gdk_cairo_set_source_pixbuf (cr, scaled, 0, -1.0 * PREVIEW_IMAGE_HEIGHT * i);
    cairo_mask (cr, cairo_get_source (cr));

    if (preview_pixbufs[i])
      g_object_unref (preview_pixbufs[i]);

    preview_pixbufs[i] = gdk_pixbuf_get_from_surface (preview_surface, 0, 0,
                                                      PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT);

    cairo_destroy (cr);
    cairo_surface_destroy (preview_surface);
  }

  blank_preview_surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                                             CAIRO_CONTENT_COLOR_ALPHA,
                                                             PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT);
  cr = cairo_create (blank_preview_surface);
  gdk_cairo_set_source_rgba (cr, &bg);
  gdk_cairo_rectangle (cr, &preview_rect);
  cairo_fill (cr);

  cairo_surface_destroy (blank_preview_surface);
  cairo_destroy (cr);
  g_object_unref (scaled);
}

static void
draw_all_balls (GtkWidget * widget)
{
  gdk_window_invalidate_rect (gtk_widget_get_window (widget), NULL, FALSE);
}

static void
start_animation (void)
{
  int ms;

  ms = (inmove ? move_timeout : 100);
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
reset_game (void)
{
  int i;

  for (i = 0; i < hfieldsize * vfieldsize; i++) {
    field[i].color = 0;
    field[i].phase = 0;
    field[i].active = 0;
    field[i].pathsearch = -1;
  }
  score = 0;
  init_preview ();
  init_new_balls (npieces, -1);
}

static void
refresh_screen (void)
{
  draw_all_balls (draw_area);
  draw_preview ();
}

static void
start_game (void)
{
  char string[20];

  set_status_message (_("Match five objects of the same type in a row to score!"));
  refresh_screen ();
  active = -1;
  target = -1;
  inmove = -1;
  g_snprintf (string, 19, "%d", score);
  gtk_label_set_text (GTK_LABEL (scorelabel), string);
  set_inmove (0);
}

static void
reset_pathsearch (void)
{
  int i;

  for (i = 0; i < hfieldsize * vfieldsize; i++)
    field[i].pathsearch = -1;
}

void
init_preview (void)
{
  int i;

  for (i = 0; i < npieces; i++) {
    preview[i] = g_rand_int_range (rgen, 1, ncolors + 1);
  }
}

void
draw_preview (void)
{
  guint i;

  /* This function can be called before the images are initialized */
  if (!GTK_IS_IMAGE (preview_images[0]))
    return;

  for (i = 0; i < MAXNPIECES; i++) {
    if (i < npieces)
      gtk_image_set_from_pixbuf (preview_images[i], preview_pixbufs[preview[i] - 1]);
    else
      gtk_image_clear (preview_images[i]);
  }
}

void
game_new_callback (GSimpleAction *action,
                   GVariant *parameter,
                   gpointer user_data)
{
  reset_game ();
  start_game ();
}

static void
show_scores (gint pos)
{
  static GtkWidget *dialog;

  if (dialog == NULL) {
    dialog = games_scores_dialog_new (GTK_WINDOW (app), highscores, _("Five or More Scores"));
    games_scores_dialog_set_category_description (GAMES_SCORES_DIALOG
                                                  (dialog), _("_Board size:"));
  }

  if (pos > 0) {
    games_scores_dialog_set_hilight (GAMES_SCORES_DIALOG (dialog), pos);
  }

  gtk_window_present (GTK_WINDOW (dialog));
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide (dialog);
}

static void
game_over (void)
{
  int pos;

  set_status_message (_("Game Over!"));
  if (score > 0)
    pos = games_scores_add_plain_score (highscores, score);
  show_scores (pos);
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

int
init_new_balls (int num, int prev)
{
  int i, j = -1;
  gfloat num_boxes = hfieldsize * vfieldsize;
  for (i = 0; i < num;) {
    j = g_rand_int_range (rgen, 0, num_boxes);
    if (field[j].color == 0) {
      field[j].color = (prev == -1) ?
        g_rand_int_range (rgen, 1, ncolors + 1) : preview[prev];
      i++;
    }
  }
  return j;
}

void
draw_box (GtkWidget * widget, int x, int y)
{
  gtk_widget_queue_draw_area (widget, x * boxsize, y * boxsize,
                              boxsize, boxsize);
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
deactivate (GtkWidget * widget, int x, int y)
{
  field[active].active = 0;
  field[active].phase = 0;
  draw_box (widget, x, y);
  active = -1;
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
    draw_box (draw_area, cursor_x, cursor_y);
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

static gboolean
field_draw_callback (GtkWidget * widget, cairo_t *cr)
{
  guint i, j, idx;
  GdkRGBA cursorColor;

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

  score += retval;
  g_snprintf (string, 19, "%d", score);
  gtk_label_set_text (GTK_LABEL (scorelabel), string);

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

void
game_top_ten_callback (GSimpleAction *action,
                       GVariant *parameter,
                       gpointer user_data)
{
  show_scores (0);
}

void
game_about_callback (GSimpleAction *action,
                     GVariant *parameter,
                     gpointer user_data)
{
  const gchar *authors[] = { "Robert Szokovacs <szo@appaloosacorp.hu>",
    "Szabolcs B\xc3\xa1n <shooby@gnome.hu>",
    NULL
  };

  const gchar *documenters[] = { "Tiffany Antopolski",
                                 "Lanka Rathnayaka",
    NULL
  };

  gtk_show_about_dialog (GTK_WINDOW (app),
                         "program-name", _("Five or More"),
                         "version", VERSION,
                         "comments", _("GNOME port of the once-popular Color Lines game"),
                         "copyright",
                         "Copyright © 1997–2008 Free Software Foundation, Inc.\n Copyright © 2013–2014 Michael Catanzaro",
                         "license-type", GTK_LICENSE_GPL_2_0,
                         "authors", authors,
                         "documenters", documenters,
                         "translator-credits", _("translator-credits"),
                         "logo-icon-name", "five-or-more",
                         "website", "https://wiki.gnome.org/Apps/Five%20or%20more",
                         NULL);
}

static void
set_backgnd_color (const gchar * str)
{
  if (!str)
    str = g_strdup ("#000000");

  if (str != backgnd.name) {
    g_free (backgnd.name);
    backgnd.name = g_strdup (str);
  }

  if (!gdk_rgba_parse (&backgnd.color, backgnd.name)) {
    gdk_rgba_parse (&backgnd.color, "#000000");
  }
}

static void
set_sizes (gint size)
{
  hfieldsize = field_sizes[size][0];
  vfieldsize = field_sizes[size][1];
  ncolors = field_sizes[size][2];
  npieces = field_sizes[size][3];
  game_size = size;
  games_scores_set_category (highscores, scorecats[size - 1].key);

  g_settings_set_int (settings, KEY_SIZE, size);

  if (gridframe)
    games_grid_frame_set (GAMES_GRID_FRAME (gridframe),
                          hfieldsize, vfieldsize);
}

static void
load_theme (void)
{
  if (ball_preimage)
    g_object_unref (ball_preimage);
  ball_preimage = load_image (ball_filename);

  refresh_pixmaps ();
  refresh_preview_surfaces ();
}

static void
conf_value_changed_cb (GSettings *settings, gchar *key)
{
  if (strcmp (key, KEY_BACKGROUND_COLOR) == 0) {
    gchar *color;

    color = g_settings_get_string (settings, KEY_BACKGROUND_COLOR);
    if (color != NULL) {
      set_backgnd_color (color);
      g_free (color);
    }
  } else if (strcmp (key, KEY_BALL_THEME) == 0) {
    gchar *theme_tmp = NULL;

    theme_tmp = g_settings_get_string (settings, KEY_BALL_THEME);
    if (theme_tmp) {
      if (strcmp (theme_tmp, ball_filename) != 0) {
        g_free (ball_filename);
        ball_filename = theme_tmp;
        load_theme ();
        refresh_screen ();
      } else
        g_free (theme_tmp);
    }
    /* FIXME apply in the prefs dialog GUI */
  } else if (strcmp (key, KEY_MOVE_TIMEOUT) == 0) {
    gint timeout_tmp;

    timeout_tmp = g_settings_get_int (settings, KEY_MOVE_TIMEOUT);
    timeout_tmp = CLAMP (timeout_tmp, 1, 1000);
    if (timeout_tmp != move_timeout)
      move_timeout = timeout_tmp;

  } else if (strcmp (key, KEY_SIZE) == 0) {
    gint size_tmp;
    size_tmp = g_settings_get_int (settings, KEY_SIZE);

    if (size_tmp != game_size) {
      set_sizes (size_tmp);
      reset_game ();
      start_game ();
    }
  }
}

static void
bg_color_callback (GtkWidget * widget, gpointer data)
{
  GdkRGBA c;
  char str[64];

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (widget), &c);

  g_snprintf (str, sizeof (str), "#%04x%04x%04x", (int) (c.red * 65535 + 0.5), (int) (c.green * 65535 + 0.5), (int) (c.blue * 65535 + 0.5));

  g_settings_set_string (settings, KEY_BACKGROUND_COLOR, str);

  load_theme ();
  refresh_screen ();
}

static void
size_callback (GtkWidget * widget, gpointer data)
{
  GtkWidget *size_radio, *content_area, *label;

  game_size = g_settings_get_int (settings, KEY_SIZE);
  if (pref_dialog_done && game_size != data && !restart_game_dialog) {
    GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;

    restart_game_dialog = gtk_message_dialog_new (GTK_WINDOW (pref_dialog),
						  GTK_MESSAGE_WARNING,
						  flags,
						  GTK_BUTTONS_NONE,
						  _("Are you sure you want to restart the game?"));

    gtk_dialog_add_buttons (GTK_DIALOG (restart_game_dialog),
                            _("_Cancel"), GTK_RESPONSE_CANCEL,
                            _("_Restart"), GTK_RESPONSE_OK,
                            NULL);

    gint result = gtk_dialog_run (GTK_DIALOG (restart_game_dialog));
    gtk_widget_destroy (restart_game_dialog);

    switch (result) {
    case GTK_RESPONSE_OK:
      g_settings_set_int (settings, KEY_SIZE, GPOINTER_TO_INT (data));
      break;
    case GTK_RESPONSE_CANCEL:
      switch (game_size) {
      case SMALL:
	size_radio = size_radio_s;
	break;
      case MEDIUM:
	size_radio = size_radio_m;
	break;
      case LARGE:
	size_radio = size_radio_l;
	break;
      }
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (size_radio), TRUE);
    }

    restart_game_dialog = NULL;
  }
}

static void
set_selection (GtkWidget * widget, char *data)
{
  const gchar *entry;

  entry = games_file_list_get_nth (theme_file_list,
                                   gtk_combo_box_get_active (GTK_COMBO_BOX
                                                             (widget)));
  g_settings_set_string (settings, KEY_BALL_THEME, entry);
}

static GtkWidget *
fill_menu (void)
{
  gchar *pixmap_dir;

  if (theme_file_list)
    g_object_unref (theme_file_list);

  pixmap_dir = g_build_filename (DATA_DIRECTORY, "themes", NULL);
  theme_file_list = games_file_list_new_images (pixmap_dir, NULL);
  g_free (pixmap_dir);
  games_file_list_transform_basename (theme_file_list);

  return games_file_list_create_widget (theme_file_list, ball_filename,
                                        GAMES_FILE_LIST_REMOVE_EXTENSION |
                                        GAMES_FILE_LIST_REPLACE_UNDERSCORES);
}

static void
set_fast_moves_callback (GtkWidget * widget, gpointer * data)
{
  gboolean is_on = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
  gint timeout = is_on ? 10 : 100;
  g_settings_set_int (settings, KEY_MOVE_TIMEOUT, timeout);
}

void
pref_dialog_response (GtkDialog * dialog, gint response, gpointer data)
{
  gtk_widget_hide (GTK_WIDGET (dialog));
}

void
game_props_callback (GSimpleAction *action,
                     GVariant *parameter,
                     gpointer user_data)
{
  gchar *ui_path;
  GError *error = NULL;
  GtkWidget *omenu;
  GtkWidget *grid;
  GtkWidget *color_button;
  GtkWidget *size_radio;
  GtkWidget *fast_moves_checkbutton;

  if (!pref_dialog) {
    ui_path = g_build_filename (DATA_DIRECTORY, "five-or-more-preferences.ui", NULL);
    builder_preferences = gtk_builder_new ();
    gtk_builder_add_from_file (builder_preferences, ui_path, &error);
    g_free (ui_path);

    if (error) {
      g_critical ("Unable to load the user interface file: %s", error->message);
      g_error_free (error);
      g_assert_not_reached ();
    }

    pref_dialog = GTK_WIDGET (gtk_builder_get_object (builder_preferences, "preferences_dialog"));
    g_signal_connect (pref_dialog, "response",
                      G_CALLBACK (pref_dialog_response), NULL);
    g_signal_connect (pref_dialog, "delete-event",
                      G_CALLBACK (gtk_widget_hide), NULL);

    grid = GTK_WIDGET (gtk_builder_get_object (builder_preferences, "grid1"));
    omenu = fill_menu ();
    gtk_widget_show_all (GTK_WIDGET (omenu));
    gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (omenu), 1, 0, 1, 1);
    g_signal_connect (omenu, "changed",
                      G_CALLBACK (set_selection), NULL);

    color_button = GTK_WIDGET (gtk_builder_get_object (builder_preferences, "colorbutton1"));
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (color_button), &backgnd.color);
    g_signal_connect (color_button, "color-set",
                      G_CALLBACK (bg_color_callback), NULL);

    size_radio = GTK_WIDGET (gtk_builder_get_object (builder_preferences, "radiobutton_small"));
    size_radio_s = size_radio;
    g_signal_connect (size_radio, "clicked",
                      G_CALLBACK (size_callback), GINT_TO_POINTER (1));
    if (game_size == SMALL)
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (size_radio), TRUE);

    size_radio = GTK_WIDGET (gtk_builder_get_object (builder_preferences, "radiobutton_medium"));
    size_radio_m = size_radio;
    g_signal_connect (size_radio, "clicked",
                      G_CALLBACK (size_callback), GINT_TO_POINTER (2));
    if (game_size == MEDIUM)
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (size_radio), TRUE);

    size_radio = GTK_WIDGET (gtk_builder_get_object (builder_preferences, "radiobutton_large"));
    size_radio_l = size_radio;
    g_signal_connect (size_radio, "clicked",
                      G_CALLBACK (size_callback), GINT_TO_POINTER (3));
    if (game_size == LARGE)
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (size_radio), TRUE);

    fast_moves_checkbutton = GTK_WIDGET (gtk_builder_get_object (builder_preferences, "checkbutton_fast_moves"));
    if (move_timeout == 10) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fast_moves_checkbutton), TRUE);
    }

    g_signal_connect (fast_moves_checkbutton, "clicked",
                      G_CALLBACK (set_fast_moves_callback), NULL);

    g_object_unref (G_OBJECT (builder_preferences));

    pref_dialog_done = TRUE;
    gtk_window_set_transient_for (GTK_WINDOW (pref_dialog), GTK_WINDOW (app));
  }
  gtk_dialog_run (GTK_DIALOG (pref_dialog));
}

static void cleanup ()
{
  int i = 0;

  for (i = 0; i < G_N_ELEMENTS (preview_images); i++)
    if (preview_pixbufs[i])
      g_object_unref (preview_pixbufs[i]);

  g_clear_object (&ball_preimage);
  g_object_unref (highscores);

}

void
game_quit_callback (GSimpleAction *action,
                    GVariant *parameter,
                    gpointer user_data)
{
  cleanup ();
}

void
game_help_callback (GSimpleAction *action,
                    GVariant *parameter,
                    gpointer user_data)
{
  GError *error = NULL;

  gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (app)), "help:five-or-more", gtk_get_current_event_time (), &error);
  if (error)
    g_warning ("Failed to show help: %s", error->message);
  g_clear_error (&error);
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

static void
load_properties (void)
{
  gchar *buf;

  ball_filename = g_settings_get_string (settings, KEY_BALL_THEME);

  move_timeout = g_settings_get_int (settings, KEY_MOVE_TIMEOUT);
  if (move_timeout <= 0)
    move_timeout = 100;

  buf = g_settings_get_string (settings, KEY_BACKGROUND_COLOR); /* FIXMEchpe? */
  set_backgnd_color (buf);
  g_free (buf);

  load_theme ();
}

static void
init_config (void)
{
  g_signal_connect (settings, "changed",
                    G_CALLBACK (conf_value_changed_cb), NULL);

  game_size = g_settings_get_int (settings, KEY_SIZE);
  if (game_size == UNSET)
    game_size = DEFAULT_GAME_SIZE;

  game_size = CLAMP (game_size, SMALL, MAX_SIZE - 1);
  set_sizes (game_size);
}

static gboolean
window_configure_event_cb (GtkWidget *widget, GdkEventConfigure *event)
{
  if (!window_is_maximized && !window_is_fullscreen)
  {
    window_width = event->width;
    window_height = event->height;
  }

  return FALSE;
}

static gboolean
window_state_event_cb (GtkWidget *widget, GdkEventWindowState *event)
{
  if ((event->changed_mask & GDK_WINDOW_STATE_MAXIMIZED) != 0)
    window_is_maximized = (event->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0;
  if ((event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN) != 0)
    window_is_fullscreen = (event->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0;
  return FALSE;
}

static void
startup_cb (GApplication *application)
{
  gchar *ui_path;
  GtkWidget *hbox;
  GtkWidget *preview_hbox;
  GtkWidget *new_game_button;
  guint i;
  GError *error = NULL;

  GActionEntry app_actions[] = {
    { "new-game", game_new_callback },
    { "scores", game_top_ten_callback },
    { "preferences", game_props_callback },
    { "help", game_help_callback },
    { "about", game_about_callback },
    { "quit", game_quit_callback }
  };

  g_action_map_add_action_entries (G_ACTION_MAP (application),
                                   app_actions, G_N_ELEMENTS (app_actions),
                                   application);

  gtk_application_add_accelerator (GTK_APPLICATION (application), "<Primary>N", "app.new-game", NULL);

  settings = g_settings_new ("org.gnome.five-or-more");

  highscores = games_scores_new ("five-or-more",
                                 scorecats, G_N_ELEMENTS (scorecats),
                                 "board size", NULL,
                                 0 /* default category */,
                                 GAMES_SCORES_STYLE_PLAIN_DESCENDING);

  init_config ();

  builder = gtk_builder_new ();

  ui_path = g_build_filename (DATA_DIRECTORY, "menu.ui", NULL);
  gtk_builder_add_from_file (builder, ui_path, &error);
  g_free (ui_path);

  if (error) {
    g_critical ("Unable to load the application menu file: %s", error->message);
    g_error_free (error);
  } else {
    gtk_application_set_app_menu (GTK_APPLICATION (application),
                                  G_MENU_MODEL (gtk_builder_get_object (builder, "appmenu")));
  }

  ui_path = g_build_filename (DATA_DIRECTORY, "five-or-more.ui", NULL);
  gtk_builder_add_from_file (builder, ui_path, &error);
  g_free (ui_path);

  if (error) {
    g_critical ("Unable to load the user interface file: %s", error->message);
    g_error_free (error);
    g_assert_not_reached ();
  }

  app = GTK_WIDGET (gtk_builder_get_object (builder, "glines_window"));
  gtk_window_set_icon_name (GTK_WINDOW (app), "five-or-more");
  g_signal_connect (GTK_WINDOW (app), "configure-event", G_CALLBACK (window_configure_event_cb), NULL);
  g_signal_connect (GTK_WINDOW (app), "window-state-event", G_CALLBACK (window_state_event_cb), NULL);
  gtk_window_set_default_size (GTK_WINDOW (app), g_settings_get_int (settings, "window-width"), g_settings_get_int (settings, "window-height"));
  if (g_settings_get_boolean (settings, "window-is-maximized"))
    gtk_window_maximize (GTK_WINDOW (app));
  gtk_container_set_border_width (GTK_CONTAINER (app), 20);
  gtk_application_add_window (GTK_APPLICATION (application), GTK_WINDOW (app));

  headerbar = GTK_WIDGET (gtk_builder_get_object (builder, "headerbar"));

  preview_hbox = GTK_WIDGET (gtk_builder_get_object (builder, "preview_hbox"));

  for (i = 0; i < MAXNPIECES; i++) {
    preview_images[i] = GTK_IMAGE (gtk_image_new ());
    gtk_box_pack_start (GTK_BOX (preview_hbox), GTK_WIDGET (preview_images[i]), FALSE, FALSE, 0);
    gtk_widget_realize (GTK_WIDGET (preview_images[i]));
  }

  scorelabel = GTK_WIDGET (gtk_builder_get_object (builder, "scorelabel"));

  hbox = GTK_WIDGET (gtk_builder_get_object (builder, "hbox"));
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
  gridframe = games_grid_frame_new (hfieldsize, vfieldsize);
  games_grid_frame_set_padding (GAMES_GRID_FRAME (gridframe), 1, 1);
  gtk_container_add (GTK_CONTAINER (gridframe), draw_area);
  gtk_box_pack_start (GTK_BOX (hbox), gridframe, TRUE, TRUE, 0);

  gtk_widget_set_events (draw_area,
                         gtk_widget_get_events (draw_area) |
                         GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK);

  gtk_widget_set_can_focus (draw_area, TRUE);
  gtk_widget_grab_focus (draw_area);

  new_game_button = GTK_WIDGET (gtk_builder_get_object (builder, "new_game_button"));

  load_properties ();

  gtk_builder_connect_signals (builder, NULL);

  g_object_unref (G_OBJECT (builder));
}

static void
activate_cb (GApplication *application)
{
  reset_game ();
  gtk_widget_show_all (app);
  start_game ();
}

static void
shutdown_cb (GApplication *application)
{
  g_settings_set_int (settings, "window-width", window_width);
  g_settings_set_int (settings, "window-height", window_height);
  g_settings_set_boolean (settings, "window-is-maximized", window_is_maximized);
  cleanup ();
}

int
main (int argc, char *argv[])
{
  GOptionContext *context;
  gboolean retval;
  GtkApplication *application;
  int status;
  GError *error = NULL;

  setlocale (LC_ALL, "");
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  games_scores_startup ();

  rgen = g_rand_new ();

  context = g_option_context_new (NULL);
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  if (!retval) {
    g_print ("%s", error->message);
    g_error_free (error);
    exit (1);
  }

  g_set_application_name (_("Five or More"));

  gtk_window_set_default_icon_name ("five-or-more");

  application = gtk_application_new ("org.gnome.five-or-more", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (application, "startup", G_CALLBACK (startup_cb), NULL);
  g_signal_connect (application, "activate", G_CALLBACK (activate_cb), NULL);
  g_signal_connect (application, "shutdown", G_CALLBACK (shutdown_cb), NULL);

  status = g_application_run (G_APPLICATION (application), argc, argv);

  return status;
}
