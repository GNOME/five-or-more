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

#include "games-file-list.h"
#include "games-preimage.h"
#include "games-gridframe.h"
#include "games-scores.h"
#include "games-scores-dialog.h"
#include "game-area.h"
#include "five-or-more-app.h"
#include "balls-preview.h"

#define DEFAULT_GAME_SIZE MEDIUM
#define KEY_SIZE              "size"
#define MAXNPIECES 10
#define KEY_MOVE_TIMEOUT      "move-timeout"
#define KEY_BACKGROUND_COLOR  "background-color"
#define KEY_BALL_THEME        "ball-theme"

static const GamesScoresCategory scorecats[] = {
  { "Small",  NC_("board size", "Small")  },
  { "Medium", NC_("board size", "Medium") },
  { "Large",  NC_("board size", "Large")  }
};

enum {
  UNSET = 0,
  SMALL = 1,
  MEDIUM,
  LARGE,
  MAX_SIZE,
};

static background backgnd = { { 0, 0, 0, 0}, NULL, 0 };
static GtkWidget *gridframe;
static GtkWidget *app, *headerbar, *restart_game_dialog, *pref_dialog;

static char *ball_filename;
static gint game_size = UNSET;
static gboolean pref_dialog_done = FALSE;
static GamesFileList *theme_file_list = NULL;
static GtkWidget *size_radio_s, *size_radio_m, *size_radio_l;

static int move_timeout = 100;
static gboolean window_is_fullscreen = FALSE, window_is_maximized = FALSE;
static gint window_width = 0, window_height = 0;
static GamesScores *highscores;
static GSettings *settings;
static GtkBuilder *builder;
static GtkBuilder *builder_preferences;

static guint score = 0;
static GtkWidget *scorelabel;

/*getter funcs*/

GSettings **
get_settings ()
{
  return &settings;
}

const GamesScoresCategory *
get_scorecats ()
{
  return scorecats;
}

gint *
get_game_size ()
{
  return &game_size;
}

GtkWidget *
get_gridframe ()
{
  return gridframe;
}

char *
get_ball_filename ()
{
  return ball_filename;
}

background
get_backgnd ()
{
  return backgnd;
}

int
get_move_timeout ()
{
  return move_timeout;
}

GamesScores *
get_highscores()
{
  return highscores;
}

void
update_score (guint value)
{
  char string[20];
  if(value)
  score += value;
  else
  score = value;
  g_snprintf (string, 19, "%d", score);
  gtk_label_set_text (GTK_LABEL (scorelabel), string);
}

void
set_status_message (gchar * message)
{
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (headerbar), message);
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

void
game_over (void)
{
  int pos;

  set_status_message (_("Game Over!"));
  if (score > 0)
    pos = games_scores_add_plain_score (highscores, score);
  show_scores (pos);
}

static void
start_game (void)
{
  set_status_message (_("Match five objects of the same type in a row to score!"));
  refresh_screen ();
  update_score(0);
  set_inmove (0);
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
window_size_allocate_cb (GtkWidget *widget, GdkRectangle *allocation)
{
  if (!window_is_maximized && !window_is_fullscreen)
    gtk_window_get_size (GTK_WINDOW (widget), &window_width, &window_height);

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


void
game_new_callback (GSimpleAction *action,
                   GVariant *parameter,
                   gpointer user_data)
{
  reset_game ();
  start_game ();
}

void
game_top_ten_callback (GSimpleAction *action,
                       GVariant *parameter,
                       gpointer user_data)
{
  show_scores (0);
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



void
pref_dialog_response (GtkDialog * dialog, gint response, gpointer data)
{
  gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
bg_color_callback (GtkWidget * widget, gpointer data)
{
  GdkRGBA c;
  char str[64];

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (widget), &c);

  g_snprintf (str, sizeof (str), "#%04x%04x%04x", (int) (c.red * 65535 + 0.5), (int) (c.green * 65535 + 0.5), (int) (c.blue * 65535 + 0.5));

  g_settings_set_string (settings, KEY_BACKGROUND_COLOR, str);
  //area,h
  load_theme ();
  //area.h
  refresh_screen ();
}

static void
size_callback (GtkWidget * widget, gpointer data)
{
  GtkWidget *size_radio, *content_area, *label;

  game_size = g_settings_get_int (settings, KEY_SIZE);
  if (pref_dialog_done && game_size != GPOINTER_TO_INT (data) && !restart_game_dialog) {
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

void
game_quit_callback (GSimpleAction *action,
                    GVariant *parameter,
                    gpointer user_data)
{
  g_application_quit (G_APPLICATION (user_data));
}

void
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
  g_signal_connect (GTK_WINDOW (app), "size-allocate", G_CALLBACK (window_size_allocate_cb), NULL);
  g_signal_connect (GTK_WINDOW (app), "window-state-event", G_CALLBACK (window_state_event_cb), NULL);
  gtk_window_set_default_size (GTK_WINDOW (app), g_settings_get_int (settings, "window-width"), g_settings_get_int (settings, "window-height"));
  if (g_settings_get_boolean (settings, "window-is-maximized"))
    gtk_window_maximize (GTK_WINDOW (app));
  gtk_container_set_border_width (GTK_CONTAINER (app), 20);
  gtk_application_add_window (GTK_APPLICATION (application), GTK_WINDOW (app));

  headerbar = GTK_WIDGET (gtk_builder_get_object (builder, "headerbar"));

  GtkImage **preview_images = get_preview_images();
  preview_hbox = GTK_WIDGET (gtk_builder_get_object (builder, "preview_hbox"));
  for (i = 0; i < MAXNPIECES; i++) {
    //preview images getter req this is defiend in src or here
    preview_images[i] = GTK_IMAGE (gtk_image_new ());
    gtk_box_pack_start (GTK_BOX (preview_hbox), GTK_WIDGET (preview_images[i]), FALSE, FALSE, 0);
    gtk_widget_realize (GTK_WIDGET (preview_images[i]));
  }

  scorelabel = GTK_WIDGET (gtk_builder_get_object (builder, "scorelabel"));

  hbox = GTK_WIDGET (gtk_builder_get_object (builder, "hbox"));

  GtkWidget *draw_area = game_area_init ();

  gridframe = games_grid_frame_new (get_hfieldsize(), get_vfieldsize());
  games_grid_frame_set_padding (GAMES_GRID_FRAME (gridframe), 1, 1);
  gtk_container_add (GTK_CONTAINER (gridframe), draw_area);
  gtk_box_pack_start (GTK_BOX (hbox), gridframe, TRUE, TRUE, 0);

  new_game_button = GTK_WIDGET (gtk_builder_get_object (builder, "new_game_button"));

  load_properties ();

  gtk_builder_connect_signals (builder, NULL);

  g_object_unref (G_OBJECT (builder));
}


void
activate_cb (GApplication *application)
{
  reset_game ();
  gtk_widget_show_all (app);
  start_game ();
}

void
shutdown_cb (GApplication *application)
{
  int i = 0;
  GtkImage **preview_images = get_preview_images();
  GdkPixbuf **preview_pixbufs = get_preview_pixbufs();
  for (i = 0; i < G_N_ELEMENTS (preview_images); i++)
    if (preview_pixbufs[i])
      g_object_unref (preview_pixbufs[i]);

  GamesPreimage *ball_preimage = get_ball_preimage();
  g_clear_object (&ball_preimage);
  g_object_unref (highscores);

  g_settings_set_int (settings, "window-width", window_width);
  g_settings_set_int (settings, "window-height", window_height);
  g_settings_set_boolean (settings, "window-is-maximized", window_is_maximized);
}

void
set_application_callbacks(GtkApplication *application)
{
  g_signal_connect (application, "startup", G_CALLBACK (startup_cb), NULL);
  g_signal_connect (application, "activate", G_CALLBACK (activate_cb), NULL);
  g_signal_connect (application, "shutdown", G_CALLBACK (shutdown_cb), NULL);
}