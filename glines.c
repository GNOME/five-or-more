/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

/*
 * Color lines for GNOME
 * (c) 1999 Free Software Foundation
 * Authors: Robert Szokovacs <szo@appaloosacorp.hu>
 *          Szabolcs Ban <shooby@gnome.hu>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <gnome.h>
#include <glib/gi18n.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnomeui/gnome-window-icon.h>
#include <gconf/gconf-client.h>
#include <games-scores.h>
#include <games-scores-dialog.h>
#include <games-frame.h>
#include <games-files.h>
#include <games-gridframe.h>
#include <games-preimage.h>
#include <games-stock.h>
#include "glines.h"

#define KEY_DIR "/apps/glines"
#define KEY_BACKGROUND_COLOR "/apps/glines/preferences/background_color"
#define KEY_BALL_THEME "/apps/glines/preferences/ball_theme"
#define KEY_MOVE_TIMEOUT "/apps/glines/preferences/move_timeout"
#define KEY_SAVED_SCORE "/apps/glines/saved/score"
#define KEY_SAVED_FIELD "/apps/glines/saved/field"
#define KEY_SAVED_PREVIEW "/apps/glines/saved/preview"
#define KEY_WIDTH "/apps/glines/saved/width"
#define KEY_HEIGHT "/apps/glines/saved/height"
#define KEY_SIZE "/apps/glines/preferences/size"

#define MAXNPIECES 10
#define MAXFIELDSIZE 30
#define DEFAULT_GAME_SIZE MEDIUM

enum {
	UNSET = 0,
	SMALL = 1,
	MEDIUM,
	LARGE,
	MAX_SIZE,
};

/* Keep these in sync with the enum above. */
const
gint field_sizes[MAX_SIZE][4] = {{-1, -1, -1, -1}, /* This is a dummy entry. */
                                 { 7,  7, 5, 3},   /* SMALL */
                                 {9,  9, 7, 3},   /* MEDIUM */
                                 {20, 15, 7, 7}};  /* LARGE */

const
gchar *scorenames[]  = {N_("Small"),
                        N_("glines|Medium"),
                        N_("Large")};

static const GamesScoresCategory scorecats[] = {{"Small", N_("Small")},
                                                {"Medium", N_("glines|Medium")},
                                                {"Large", N_("Large")},
                                                GAMES_SCORES_LAST_CATEGORY};

static const GamesScoresDescription scoredesc = {scorecats,
                                                 "Small",
                                                 "glines",
                                                 GAMES_SCORES_STYLE_PLAIN_DESCENDING};

GamesScores *highscores;



gint hfieldsize;
gint vfieldsize;
gint ncolors;
gint npieces;
gint game_size = UNSET;
gboolean pref_dialog_done = FALSE;

GRand *rgen;

GConfClient *conf_client = NULL;

GtkWidget *draw_area;
GtkWidget *app, *statusbar, *pref_dialog, *gridframe;
GtkWidget *preview_widgets[MAXNPIECES];
GtkWidget *menubar;
GtkWidget *scoreitem;
GtkAction *fullscreen_action;
GtkAction *leavefullscreen_action;

/* These keep track of what we put in the main table so we
 * can reshuffle them when we change the field size. */
GtkWidget *table;
GtkWidget *top_pane;
GtkWidget *bottom_pane;

field_props field[MAXFIELDSIZE * MAXFIELDSIZE];

/* Pre-rendering image data prepared from file. */
GamesPreimage *ball_preimage = NULL;
/* The tile images with balls rendered on them. */
GdkPixmap *ball_pixmap = NULL;
/* The balls rendered to a size appropriate for the preview. */
GdkPixmap *preview_pixmaps[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
/* A pixmap of a blank tile. */
GdkPixmap *blank_pixmap = NULL;
GdkPixmap *blank_preview_pixmap = NULL;

GtkWidget *fast_moves_toggle_button = NULL;

GamesFileList * theme_file_list = NULL;

int active = -1;
int target = -1;
int inmove = 0;
guint score = 0;
int cursor_x = MAXFIELDSIZE / 2;
int cursor_y = MAXFIELDSIZE / 2;
gboolean show_cursor = FALSE;

int boxsize;

/* The width and height of the main window. */
#define MIN_WIDTH  190
#define MIN_HEIGHT 240
int width;
int height;

int preview_height = 0;
int preview_width = 0;

int move_timeout = 100;
int animate_id = 0;
int preview[MAXNPIECES];
char * ball_filename;
GtkWidget *scorelabel;
scoretable sctab[] = {{5, 10}, {6, 12}, {7, 18}, {8, 28}, {9, 42}, {10, 82}, {11, 108}, {12, 138}, {13, 172}, {14, 210}, {0,0}};

static struct {
  GdkColor color ;
  gchar *name ;
  gint set;
} backgnd = {
	{0, 0, 0, 0}, NULL, 0
};

gchar *warning_message = NULL;

static void
set_statusbar_message (gchar *message)
{
	guint context_id;
	context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (statusbar),
					       "message");
	gtk_statusbar_pop (GTK_STATUSBAR (statusbar), context_id);
	gtk_statusbar_push (GTK_STATUSBAR (statusbar), context_id,
			    message);
}

static void
show_image_warning (gchar *message)
{
	GtkWidget *dialog;
	GtkWidget *button;

        dialog = gtk_message_dialog_new (GTK_WINDOW (app),
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_WARNING,
                                         GTK_BUTTONS_CLOSE,
                                         _("Could not load theme"));

        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), message);

        button = gtk_dialog_add_button (GTK_DIALOG (dialog),
                                        _("Preferences"), GTK_RESPONSE_ACCEPT);

        gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);

        g_signal_connect (button, "clicked", G_CALLBACK (game_props_callback), NULL);

        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
}

static void 
load_image (gchar *fname,
	    GamesPreimage **preimage)
{
	gchar *tmp, *fn = NULL;
	GamesPreimage *tmp_preimage;

	tmp = g_build_filename ("glines", fname, NULL);
	
	fn = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP, (tmp), FALSE, NULL);
	g_free (tmp);


	if (!g_file_test (fn, G_FILE_TEST_EXISTS)) {
		warning_message = g_strdup_printf (_("Unable to locate file:\n%s\n\n"
						     "The default theme will be loaded instead."), fn);

		tmp = g_build_filename ("glines", "balls.svg", NULL);
		fn = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP, (tmp), FALSE, NULL);
		g_free (tmp);

		if (!g_file_test (fn, G_FILE_TEST_EXISTS)) {
			g_free (warning_message);
			warning_message = g_strdup_printf (_("Unable to locate file:\n%s\n\n"
				         		     "Please check that Five or More is installed correctly."), fn);
		}
	}

	tmp_preimage = games_preimage_new_from_file (fn, NULL);
	g_free( fn );


	if (*preimage)
		g_object_unref (*preimage);

	*preimage = tmp_preimage;
}

/* The main table has to be layed out differently depending on the
   size of the playing area we choose. Otherwise the preview window
   gets too large. */
static void relay_table (void)
{
	gint height;
	GValue value = { 0 };

	height = 1 + vfieldsize;
	g_value_init (&value, G_TYPE_INT);
	g_value_set_int (&value, height);
	
	gtk_container_child_set_property (GTK_CONTAINER (table),
					  bottom_pane,
					  "bottom_attach",
					  &value);

	gtk_table_resize (GTK_TABLE (table), vfieldsize + 1, 1);
}

static void
refresh_pixmaps (void)
{
	GdkGC * gc;
	GdkPixbuf *ball_pixbuf = NULL;

	/* Since we get called both by configure and after loading an image.
	 * it is possible the pixmaps aren't initialised. If they aren't
	 * we don't do anything. */
	if (!ball_pixmap)
		return;

	if (ball_preimage) {
		ball_pixbuf = games_preimage_render (ball_preimage, 4*boxsize, 
						     7*boxsize, NULL);

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
					      4*boxsize, 7*boxsize);
		gdk_pixbuf_fill (ball_pixbuf, 0x00000000);
	}

	if (warning_message)
		show_image_warning (warning_message);
	g_free (warning_message);
	warning_message = NULL;


	gc = gdk_gc_new (ball_pixmap);
	gdk_gc_set_foreground (gc, &backgnd.color);

	gdk_draw_rectangle (ball_pixmap, gc, TRUE, 0, 0, 4*boxsize, 7*boxsize);

	gdk_draw_pixbuf (ball_pixmap, gc,
			 ball_pixbuf, 0, 0, 0, 0, boxsize*4, boxsize*7,
			 GDK_RGB_DITHER_NORMAL, 0, 0);

	gdk_draw_rectangle (blank_pixmap, gc, TRUE, 0, 0, boxsize, boxsize);

	g_object_unref (ball_pixbuf);
	g_object_unref (gc);
}

static void
refresh_preview_pixmaps (void)
{
	int i;
	GdkPixbuf * scaled = NULL;
	GtkWidget * widget = preview_widgets[0];

	/* Like the refresh_pixmaps() function, we may be called before
	 * the window is ready. */
	if (preview_height == 0)
		return;

	/* We create pixmaps for each of the ball colours and then
	 * set them as the background for each widget in the preview array.
	 * This code assumes that each preview window is identical. */

	if (preview_pixmaps[0])
		for (i=0; i<7; i++)
			g_object_unref (preview_pixmaps[i]);

	if (ball_preimage)
		scaled = games_preimage_render (ball_preimage, 4*preview_width, 
						7*preview_height, NULL);

	if (!scaled) {
		scaled = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
					 4*preview_width, 7*preview_height);
		gdk_pixbuf_fill (scaled, 0x00000000);
	}


	for (i=0; i<7; i++) {
		preview_pixmaps[i] = gdk_pixmap_new (widget->window,
						     preview_width, 
						     preview_height, -1);
		gdk_draw_rectangle (preview_pixmaps[i], 
				    widget->style->bg_gc[GTK_STATE_NORMAL],
				    TRUE, 0, 0, preview_width, preview_height);

		gdk_draw_pixbuf (preview_pixmaps[i], widget->style->white_gc, 
				 scaled, 0, i*preview_height, 0, 0, 
				 preview_width, preview_height,
				 GDK_RGB_DITHER_NORMAL, 0, 0);
	}

	if (blank_preview_pixmap)
		g_object_unref (blank_preview_pixmap);
	blank_preview_pixmap = gdk_pixmap_new (widget->window, preview_width, preview_height, -1);
	gdk_draw_rectangle (blank_preview_pixmap, widget->style->bg_gc[GTK_STATE_NORMAL],
			    TRUE, 0, 0, preview_width, preview_height);

	g_object_unref (scaled);

}

static void
draw_all_balls (GtkWidget *widget)
{
        gdk_window_invalidate_rect (widget->window, NULL, FALSE);
}

void
set_inmove (int i)
{
 	int ms;
 	
 	if (inmove != i) {
 	        inmove = i;
 	        ms = (inmove?move_timeout:100);
		if (animate_id)
			g_source_remove (animate_id);
		animate_id = g_timeout_add (ms, animate, draw_area);
 	}
}

static void
reset_game (void)
{
	int i;

	for(i=0; i < hfieldsize * vfieldsize; i++)
	{
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
 
	set_statusbar_message (_("Match five objects of the same type in a row to score!"));
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

	for(i = 0; i < hfieldsize * vfieldsize; i++)
		field[i].pathsearch = -1;
}

void
init_preview (void)
{
	int i;

	for (i = 0; i < npieces; i++)
	{
		preview[i] = g_rand_int_range (rgen, 1, ncolors+1);
	}
}

void
draw_preview (void)
{
	int i;

	for (i=0; i < MAXNPIECES; i++) {

		if (i < npieces)
			gdk_window_set_back_pixmap (preview_widgets[i]->window,
						    preview_pixmaps[preview[i]-1],
						    FALSE);
		else
			gdk_window_set_back_pixmap (preview_widgets[i]->window,
						    blank_preview_pixmap,
						    FALSE);

		gdk_window_clear (preview_widgets[i]->window);
	}
	
}

static void
game_new_callback (void)
{
	reset_game ();
 	start_game ();
}

static void
show_scores (gint pos, gboolean new_game)
{
	static GtkWidget *dialog;

	if (dialog == NULL) {
		dialog = games_scores_dialog_new (highscores, 
						  _("GNOME Five or More"));
		games_scores_dialog_set_category_description (GAMES_SCORES_DIALOG (dialog),
							      _("Board Size:"));	
	}

	if (pos > 0) {
		games_scores_dialog_set_hilight (GAMES_SCORES_DIALOG (dialog),
						 pos);
	}

	gtk_window_present (GTK_WINDOW (dialog));
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_hide (dialog);
}

static void
update_score_state ()
{
	GList *top;

	top = games_scores_get (highscores);
	gtk_widget_set_sensitive (scoreitem, top != NULL);
}

static void
game_over (void)
{
	int pos;
	GamesScoreValue hiscore;

	set_statusbar_message (_("Game Over!"));
	hiscore.plain = score;
	pos = games_scores_add_score (highscores, hiscore);
	show_scores (pos, TRUE);
	update_score_state ();
	return;
}

static int spaces_left (void)
{
	int i,j;

	j = 0;

	for (i=0; i<hfieldsize*vfieldsize; i++) {
		if (field[i].color == 0)
			j++;
	}

	return j;
}

static int
check_gameover (void)
{
	if (spaces_left ()> 0)
		return 1;

	game_over ();

	return -1;
}

int
init_new_balls (int num, int prev)
{
	int i, j = -1;
	gfloat num_boxes = hfieldsize * vfieldsize;
	for (i = 0; i < num;)
	{
		j = g_rand_int_range (rgen, 0, num_boxes);
		if (field[j].color == 0)
		{
			field[j].color = (prev == -1) ? 
				g_rand_int_range (rgen, 1, ncolors+1) :
				preview[prev]; 
			i++;
		}
	}
	return j;
}

void
draw_box (GtkWidget *widget, int x, int y)
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
	for (i = 0; i < hfieldsize*vfieldsize; i++)
	{
		if (field[i].pathsearch == num)
		{
			flag = 1;	
			if ((i/hfieldsize > 0) && (field[i - hfieldsize].pathsearch == -1) && (field[i - hfieldsize].color == 0))
				field[i - hfieldsize].pathsearch = num+1;
			if ((i/hfieldsize < vfieldsize - 1) && (field[i + hfieldsize].pathsearch == -1) && (field[i + hfieldsize].color == 0))
				field[i + hfieldsize].pathsearch = num+1;
			if ((i%hfieldsize > 0) && (field[i - 1].pathsearch == -1) && (field[i - 1].color == 0))
				field[i - 1].pathsearch = num+1;
			if ((i%hfieldsize < hfieldsize - 1) && (field[i + 1].pathsearch == -1) && (field[i + 1].color == 0))
			{
				field[i + 1].pathsearch = num+1;
			}
		}
	}
	if (flag == 0) return 0;
	return 2;
}

static void
fix_route (int num, int pos)
{
	int i;

	for (i = 0; i< hfieldsize*vfieldsize; i++)
		if ((i != pos) && (field[i].pathsearch == num))
			field[i].pathsearch = -1;
	if (num < 2)
		return;
	if ((pos/hfieldsize > 0) && (field[pos - hfieldsize].pathsearch == num - 1))
		fix_route(num - 1, pos - hfieldsize);	
	if ((pos%hfieldsize > 0) && (field[pos - 1].pathsearch == num - 1))
		fix_route(num - 1, pos - 1);	
	if ((pos/hfieldsize < vfieldsize - 1) && (field[pos + hfieldsize].pathsearch == num - 1))
		fix_route(num - 1, pos + hfieldsize);	
	if ((pos%hfieldsize < hfieldsize - 1) && (field[pos + 1].pathsearch == num - 1))
		fix_route(num - 1, pos + 1);	
}
	
int
find_route (void)
{
	int i = 0;
	int j = 0;

	field[active].pathsearch = i;
	while ((j = route (i)))
	{
		if (j == 1)
		{
			fix_route (i, target);
			return 1;
		}
		i++;
	}
	return 0;
}
	
static void
deactivate (GtkWidget *widget, int x, int y)
{
	field[active].active = 0;
	field[active].phase = 0; 
	draw_box (widget, x, y);
	active = -1;
}

static void cell_clicked (GtkWidget *widget, int fx, int fy)
{
	int x, y;

	set_statusbar_message ("");
	if(field[fx + fy*hfieldsize].color == 0)
	{
		/* Clicked on an empty field */

		if((active != -1) && (inmove != 1))
		{
			/* We have an active ball, and it's not
			 * already in move. Therefore we should begin
			 * the moving sequence */

			target = fx + fy*hfieldsize;
			if(find_route())
			{
				/* We found a route to the new position */

				set_inmove(1);
			}
			else
			{
				/* Can't move there! */
				set_statusbar_message (_("You can't move there!"));
				reset_pathsearch();
				target = -1;
			}
		}
	}
	else
	{
		/* Clicked on a ball */

		if((fx + fy*hfieldsize) == active)
		{
			/* It's active, so let's deactivate it! */

			deactivate(widget, fx, fy);
		}
		else
		{
			/* It's not active, so let's activate it! */

			if(active != -1) 
			{
				/* There is an other active ball, we should deactivate it first! */

				x = active%hfieldsize;
				y = active/hfieldsize;
				deactivate(widget, x, y);
			}

			active = fx + fy*hfieldsize;
			field[active].active = 1;
		}
	}

}

static gint
button_press_event (GtkWidget *widget, GdkEvent *event)
{
	int x, y;
	int fx, fy;

	if(inmove) return TRUE;

	/* Ignore the 2BUTTON and 3BUTTON events. */
	if (event->type != GDK_BUTTON_PRESS)
		return TRUE;
	
	if (show_cursor)
	{
		show_cursor = FALSE;
		draw_box (draw_area, cursor_x, cursor_y);
	}

	gtk_widget_get_pointer (widget, &x, &y);
	fx = x / boxsize;
	fy = y / boxsize;
	
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

	if (!show_cursor)
	{
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
key_press_event (GtkWidget *widget, GdkEventKey *event, void *d)
{
	guint key;

	key = event->keyval;

	switch (key)
	{
		case GDK_Left:
		case GDK_KP_Left:
			move_cursor (-1, 0);
			break;
		case GDK_Right:
		case GDK_KP_Right:
			move_cursor (1, 0);
			break;
		case GDK_Up:
		case GDK_KP_Up:
			move_cursor (0, -1);
			break;
		case GDK_Down:
		case GDK_KP_Down:
			move_cursor (0, 1);
			break;
		case GDK_Home:
		case GDK_KP_Home:
			move_cursor (-999, 0);
			break;
		case GDK_End:
		case GDK_KP_End:
			move_cursor (999, 0);
			break;
		case GDK_Page_Up:
		case GDK_KP_Page_Up:
			move_cursor (0, -999);
			break;
		case GDK_Page_Down:
		case GDK_KP_Page_Down:
			move_cursor (0, 999);
			break;

		case GDK_space:
		case GDK_Return:
		case GDK_KP_Enter:
			if (show_cursor)
				cell_clicked (widget, cursor_x, cursor_y);
			break;
	}

	return TRUE;
}

static void
draw_grid (void)
{
	static GdkGC *grid_gc;
	guint w, h;
	guint i;

	w = draw_area->allocation.width;
	h = draw_area->allocation.height;

	if (!grid_gc) {
		GdkColormap *cmap;
		GdkColor color;

		grid_gc = gdk_gc_new (draw_area->window);

		gdk_color_parse ("#525F6C", &color);
		cmap = gtk_widget_get_colormap (draw_area);
		gdk_colormap_alloc_color (cmap, &color, FALSE, TRUE);
		gdk_gc_set_foreground (grid_gc, &color);
	}

	for (i = boxsize; i < w; i = i + boxsize)
		gdk_draw_line (draw_area->window, grid_gc, i, 0, i, h);

	for (i = boxsize; i < h; i = i + boxsize)
		gdk_draw_line (draw_area->window, grid_gc, 0, i, w, i);

	gdk_draw_rectangle (draw_area->window, grid_gc, FALSE, 0, 0, w - 1, h - 1);
}

/* Redraw a part of the field */
static gint
field_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer gp)
{
	GdkWindow *window = widget->window;
	GdkGC *gc;
	guint x_start, x_end, y_start, y_end, i, j, idx;

	x_start = event->area.x / boxsize;
	x_end = (event->area.x + event->area.width - 1) / boxsize + 1;

	y_start = event->area.y / boxsize;
	y_end = (event->area.y + event->area.height - 1) / boxsize + 1;

        gc = gdk_gc_new (draw_area->window);

	for (i = y_start; i < y_end; i++) {
		for (j = x_start; j < x_end; j++) {
			int phase, color;

			idx = j + i * hfieldsize;

			if (field[idx].color != 0) {
				phase = field[idx].phase;
				color = field[idx].color - 1;

				phase = ABS(ABS(3-phase)-3);

				gdk_draw_drawable (window, gc, ball_pixmap,
						   phase * boxsize, 
						   color * boxsize,
						   j * boxsize, 
						   i * boxsize,
						   boxsize, boxsize);
			} else {
				gdk_draw_drawable (window, gc, blank_pixmap,
						   0, 0,
						   j * boxsize, 
						   i * boxsize,
						   boxsize, boxsize);
			}
		}
	}

	if (show_cursor && cursor_x >= x_start && cursor_x <= x_end
		&& cursor_y >= y_start && cursor_y <= y_end)
	{
		GdkColormap *cmap;
		GdkColor color;

		if ( ((backgnd.color.red + backgnd.color.green + backgnd.color.blue) / 3) > (G_MAXUINT16 / 2))
			gdk_color_parse ("#000000", &color);
		else
			gdk_color_parse ("#FFFFFF", &color);

		cmap = gtk_widget_get_colormap (widget);
		gdk_colormap_alloc_color (cmap, &color, FALSE, TRUE);
		gdk_gc_set_foreground (gc, &color);

		gdk_draw_rectangle (window, gc, FALSE,
			cursor_x * boxsize + 1, cursor_y * boxsize + 1,
			boxsize - 2, boxsize - 2);
	}

        g_object_unref (gc);

	draw_grid ();

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
	if (sctab[i].num == 0)
		retval = 10 + (int) pow ((double)2.0, (double)(num - 5));
	else
		retval = sctab[i].score;

	score += retval;
	g_snprintf (string, 19, "%d", score);
	gtk_label_set_text (GTK_LABEL (scorelabel), string);		

	return retval;
}
	

static void tag_list (int * list, int len)
{
	int i;

	for (i=0; i<len; i++)
		field[list[i]].tag = 1;
}

static int
find_lines (int num)
{
	int count = 1;
	int subcount = 0;
	int x = num%hfieldsize;
	int y = num/hfieldsize;
	int list[hfieldsize];

	/* Horizontal */

	x++;
	while ((x <= hfieldsize - 1) && (field[x + y*hfieldsize].color == field[num].color))
	{
		list[subcount] = x + y*hfieldsize;
		subcount++;
		x++;
	}
	x = num % hfieldsize - 1;
	while ((x >= 0) && (field[x + y*hfieldsize].color == field[num].color))
	{
		list[subcount] = x + y*hfieldsize;
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

	x = num%hfieldsize;
	y++;
	while ((y <= vfieldsize - 1) && (field[x + y*hfieldsize].color == field[num].color))
	{
		list[subcount] = x + y*hfieldsize;
		subcount++;
		y++;
	}
	y = num / hfieldsize - 1;
	while ((y >= 0) && (field[x + y*hfieldsize].color == field[num].color))
	{
		list[subcount] = x + y*hfieldsize;
		subcount++;
		y--;
	}
	if (subcount >= 4) {
		field[num].tag = 1;
		tag_list (list, subcount);
		count += subcount;
	}
	subcount = 0;
	
	/* Diagonal ++*/
	
	x = num%hfieldsize + 1;
	y = num/hfieldsize + 1;
	while ((y <= vfieldsize - 1) && (x <= hfieldsize - 1) && (field[x + y*hfieldsize].color == field[num].color))
	{
		list[subcount] = x + y*hfieldsize;
		subcount++;
		y++;
		x++;
	}
	x = num % hfieldsize - 1;
	y = num / hfieldsize - 1;
	while ((y >= 0) && (x >= 0) && (field[x + y*hfieldsize].color == field[num].color))
	{
		list[subcount] = x + y*hfieldsize;
		subcount++;
		y--;
		x--;
	}
	if (subcount >= 4) {
		field[num].tag = 1;
		tag_list (list, subcount);
		count +=subcount;
	}
	subcount = 0;

	/* Diagonal +-*/
	
	x = num % hfieldsize + 1;
	y = num / hfieldsize - 1;
	while ((y >= 0) && (x <= hfieldsize - 1) && (field[x + y*hfieldsize].color == field[num].color))
	{
		list[subcount] = x + y*hfieldsize;
		subcount++;
		y--;
		x++;
	}
	x = num % hfieldsize - 1;
	y = num / hfieldsize + 1;
	while ((y <= vfieldsize - 1) && (x >= 0) && (field[x + y*hfieldsize].color == field[num].color))
	{
		list[subcount] = x + y*hfieldsize;
		subcount++;
		y++;
		x--;
	}
	if (subcount >= 4) {
		field[num].tag = 1;
		tag_list (list, subcount);
		count +=subcount;
	}

	return count;
}

static void clear_tags (void)
{
	int i;

	for (i=0; i<hfieldsize*vfieldsize; i++)
		field[i].tag = 0;
}

static int kill_tags (GtkWidget * widget)
{
	int i,j;

	j = 0;
	for (i = 0; i <hfieldsize*vfieldsize; i++) {
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

static gboolean check_goal (GtkWidget * w, int target)
{
	gboolean lines_cleared;
	int count;

	clear_tags ();

	count = find_lines (target);
	lines_cleared = count >= 5;

	if (lines_cleared)
	{
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

	if (active == -1) return TRUE;
	if (inmove != 0)
	{
		if ((x > 0) && (field[active - 1].pathsearch == field[active].pathsearch+1))
			newactive = active - 1;
		else if ((x < hfieldsize - 1) && (field[active + 1].pathsearch == field[active].pathsearch+1))
			newactive = active + 1;
		else if ((y > 0) && (field[active - hfieldsize].pathsearch == field[active].pathsearch+1))
			newactive = active - hfieldsize;
		else if ((y < vfieldsize - 1) && (field[active + hfieldsize].pathsearch == field[active].pathsearch+1))
			newactive = active + hfieldsize;
		else
		{
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
		if (newactive == target)
		{
			target = -1;
			set_inmove (0);
			active = -1;
			field[newactive].phase = 0;
			field[newactive].active = 0;
			draw_box (widget, x, y);
			reset_pathsearch ();
			if (!check_goal (widget, newactive))
			{
				gboolean fullline;
				int balls[npieces];
				int spaces;

				clear_tags ();
				fullline = FALSE;
				spaces = spaces_left ();
				if (spaces > npieces)
					spaces = npieces;
				for (x = 0; x < spaces; x++)
				{
					int tmp = init_new_balls (1, x);
					draw_box (widget, tmp % hfieldsize,
						  tmp / hfieldsize);
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
	if(field[active].phase >= 4)
		field[active].phase = 0;
	draw_box (widget, x, y);

	return TRUE;
}

static void
game_top_ten_callback (GtkAction *action, gpointer data)
{
	show_scores (0, FALSE);
}

static int
game_about_callback (GtkAction *action, gpointer *data)
{
   const gchar *authors[] = {"Robert Szokovacs <szo@appaloosacorp.hu>",
			     "Szabolcs B\xc3\xa1n <shooby@gnome.hu>",
			     NULL};

   gtk_show_about_dialog (GTK_WINDOW(app),
			  "name", _("Five or More"),
			  "version", VERSION,
			  "comments", _("GNOME port of the once-popular Color Lines game"),
			  "copyright", "Copyright \xc2\xa9 1997-2004 Free Software Foundation, Inc.",
			  "authors", authors,
			  "translator_credits", _("translator-credits"),
			  NULL);
   return TRUE;
}       

static void
set_backgnd_color (gchar *str)
{
	GdkColormap *colormap;
	GtkStyle *widget_style, *temp_style;

	if (!str)
		str = g_strdup ("#000000");

	if (str != backgnd.name) {
		g_free (backgnd.name) ;
		backgnd.name = g_strdup (str) ;
	}

	if (!gdk_color_parse (backgnd.name, &backgnd.color)) {
		gdk_color_parse ("#000000", &backgnd.color);
	}

	colormap = gtk_widget_get_colormap (draw_area);
	gdk_colormap_alloc_color (colormap, &backgnd.color, FALSE, TRUE);
	widget_style = gtk_widget_get_style (draw_area);
	temp_style = gtk_style_copy (widget_style);
	temp_style->bg[0] = backgnd.color;
	temp_style->bg[1] = backgnd.color;
	temp_style->bg[2] = backgnd.color;
	temp_style->bg[3] = backgnd.color;
	temp_style->bg[4] = backgnd.color;
	gtk_widget_set_style (draw_area, temp_style);
}

static void
bg_color_changed_cb (GConfClient *client,
		      guint cnxn_id,
		      GConfEntry *entry,
		      gpointer user_data)
{
	gchar *color;

	color = gconf_client_get_string (client,
					 KEY_BACKGROUND_COLOR, NULL);
	if (color != NULL)
		set_backgnd_color (color);
}

static void
bg_color_callback (GtkWidget *widget, gpointer data)
{
	static char *tmp = "";
	GdkColor c;

	gtk_color_button_get_color (GTK_COLOR_BUTTON (widget), &c);

	tmp = g_strdup_printf ("#%04x%04x%04x", c.red, c.green, c.blue);

	gconf_client_set_string (conf_client,
				 KEY_BACKGROUND_COLOR, tmp, NULL);
}

static void
set_sizes (gint size)
{
	hfieldsize = field_sizes[size][0];
	vfieldsize = field_sizes[size][1];
	ncolors    = field_sizes[size][2];
	npieces    = field_sizes[size][3];
	game_size  = size;
	games_scores_set_category (highscores, scorecats[size - 1].key);

	gconf_client_set_int (conf_client, KEY_SIZE, size, NULL);

	if (gridframe)
		games_grid_frame_set (GAMES_GRID_FRAME (gridframe),
				      hfieldsize, vfieldsize);
}

static void
size_callback (GtkWidget *widget, gpointer data)
{
	if (pref_dialog_done)
		gconf_client_set_int (conf_client, KEY_SIZE, GPOINTER_TO_INT (data), NULL);
}

static void
size_changed_cb (GConfClient *client,
		 guint cnxn_id,
		 GConfEntry *entry,
		 gpointer user_data)
{
	gint size_tmp;
	size_tmp = gconf_client_get_int (conf_client, KEY_SIZE, NULL);

	if (size_tmp != game_size) {
		set_sizes (size_tmp);
		relay_table ();
		reset_game ();
		start_game ();
	}
}

static void
load_theme ()
{
	load_image (ball_filename, &ball_preimage);
	refresh_pixmaps ();
	refresh_preview_pixmaps ();
}

static void
set_selection (GtkWidget *widget, char *data)
{
	gchar *entry;

	entry = games_file_list_get_nth (theme_file_list,
			    gtk_combo_box_get_active (GTK_COMBO_BOX (widget)));
	gconf_client_set_string (conf_client,
				 KEY_BALL_THEME,
				 entry, NULL);
}

static GtkWidget * 
fill_menu (void)
{
	gchar *dname = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP,
						  ("glines"), FALSE, NULL);

	if (theme_file_list)
		g_object_unref (theme_file_list);

	theme_file_list = games_file_list_new_images (dname, NULL);
	g_free (dname);
	games_file_list_transform_basename (theme_file_list);

	return games_file_list_create_widget (theme_file_list, ball_filename,
					      GAMES_FILE_LIST_REMOVE_EXTENSION |
					      GAMES_FILE_LIST_REPLACE_UNDERSCORES);
}

static void
set_fast_moves_callback (GtkWidget *widget, gpointer *data)
{
	gboolean is_on = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	gint timeout = is_on ? 10 : 100;
	gconf_client_set_int (conf_client,
			      KEY_MOVE_TIMEOUT,
			      timeout, NULL);
}

static void
set_fullscreen_actions (gboolean is_fullscreen)
{
	gtk_action_set_sensitive (leavefullscreen_action, is_fullscreen);
	gtk_action_set_visible (leavefullscreen_action, is_fullscreen);

	gtk_action_set_sensitive (fullscreen_action, !is_fullscreen);
	gtk_action_set_visible (fullscreen_action, !is_fullscreen);
}

static void
fullscreen_callback (GtkAction *action)
{
	if (action == fullscreen_action)
		gtk_window_fullscreen (GTK_WINDOW (app));
	else
		gtk_window_unfullscreen (GTK_WINDOW (app));
}

static void
window_state_callback (GtkWidget *widget, GdkEventWindowState *event)
{
	if (!(event->changed_mask & GDK_WINDOW_STATE_FULLSCREEN))
		return;

	set_fullscreen_actions (event->new_window_state & 
				GDK_WINDOW_STATE_FULLSCREEN);
}

static void
pref_dialog_response (GtkDialog *dialog, gint response, gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (dialog));
}

void
game_props_callback (void)
{
	GtkWidget *w, *omenu, *l, *fv;
	GtkWidget *frame;
	GtkWidget *table;
	GtkWidget *vbox;
	GtkWidget *button;

	if (! pref_dialog)
		{
			pref_dialog = gtk_dialog_new_with_buttons (_("Five or More Preferences"),
								   GTK_WINDOW (app),
								   GTK_DIALOG_DESTROY_WITH_PARENT,
								   GTK_STOCK_CLOSE,
								   GTK_RESPONSE_CLOSE,
								   NULL);
			gtk_dialog_set_has_separator (GTK_DIALOG (pref_dialog), FALSE);
			g_signal_connect (G_OBJECT (pref_dialog), "response",
					  G_CALLBACK (pref_dialog_response), NULL);
			g_signal_connect (G_OBJECT (pref_dialog), "delete_event",
					  G_CALLBACK (gtk_widget_hide), NULL);

			gtk_window_set_resizable (GTK_WINDOW (pref_dialog), FALSE);
			gtk_container_set_border_width (GTK_CONTAINER (pref_dialog), 5);
			gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (pref_dialog)->vbox), 2);

			vbox = gtk_vbox_new (FALSE, 18);
			gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
			gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pref_dialog)->vbox), vbox, 
					    FALSE, FALSE, 0);

			frame = games_frame_new (_("Themes"));
			table = gtk_table_new (2, 2, FALSE);
			gtk_container_set_border_width (GTK_CONTAINER (table), 0);
			gtk_table_set_row_spacings (GTK_TABLE (table), 6);
			gtk_table_set_col_spacings (GTK_TABLE (table), 6);
			gtk_container_add (GTK_CONTAINER (frame), table);
			gtk_box_pack_start (GTK_BOX (vbox), frame, 
					    FALSE, FALSE, 0);

			l = gtk_label_new_with_mnemonic (_("_Image:"));
			gtk_misc_set_alignment (GTK_MISC (l), 0, 0.5);
			gtk_table_attach_defaults (GTK_TABLE (table), l, 0, 1, 0, 1);

			omenu = fill_menu ();
			g_signal_connect (G_OBJECT (omenu), "changed",
					  G_CALLBACK (set_selection), NULL);
			gtk_table_attach_defaults (GTK_TABLE (table), omenu, 1, 2, 0, 1);
			gtk_label_set_mnemonic_widget (GTK_LABEL (l), omenu);


			l = gtk_label_new_with_mnemonic (_("B_ackground color:"));
			gtk_misc_set_alignment (GTK_MISC (l), 0, 0.5);
			gtk_table_attach_defaults (GTK_TABLE (table), l, 0, 1, 1, 2);

			{
				w  = gtk_color_button_new ();
				gtk_color_button_set_color (GTK_COLOR_BUTTON (w), &backgnd.color);
				g_signal_connect (G_OBJECT (w), "color_set",
						  G_CALLBACK (bg_color_callback), NULL);
			}

			gtk_table_attach_defaults (GTK_TABLE (table), w, 1, 2, 1, 2);
			gtk_label_set_mnemonic_widget (GTK_LABEL (l), w);


			frame = games_frame_new (_("Board Size"));
			fv = gtk_vbox_new (FALSE, FALSE);
			gtk_box_set_spacing (GTK_BOX (fv), 6);
			gtk_container_add (GTK_CONTAINER (frame), fv);
			gtk_box_pack_start (GTK_BOX (vbox), frame, 
					    FALSE, FALSE, 0);
			
			button = gtk_radio_button_new_with_mnemonic (NULL, _("_Small") );
			if (game_size == SMALL)
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
			g_signal_connect (G_OBJECT (button), "clicked", 
					  G_CALLBACK (size_callback), (gpointer) SMALL);

			gtk_container_add (GTK_CONTAINER (fv), button);

			button = gtk_radio_button_new_with_mnemonic
				(gtk_radio_button_get_group (GTK_RADIO_BUTTON(button)),
				 _("_Medium") );
			
			if (game_size == MEDIUM)
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
			g_signal_connect (G_OBJECT (button), "clicked", 
					  G_CALLBACK (size_callback), (gpointer) MEDIUM);
			gtk_container_add (GTK_CONTAINER (fv), button);

			button = gtk_radio_button_new_with_mnemonic
				(gtk_radio_button_get_group (GTK_RADIO_BUTTON(button)),
				 _("_Large") );
			if (game_size == LARGE)
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);
			g_signal_connect (G_OBJECT (button), "clicked", 
					  G_CALLBACK (size_callback), (gpointer) LARGE);
			gtk_container_add (GTK_CONTAINER (fv), button);


			frame = games_frame_new (_("General"));
			fv = gtk_vbox_new (FALSE, FALSE);
			gtk_box_set_spacing (GTK_BOX (fv), 6);
			gtk_container_add (GTK_CONTAINER (frame), fv);
			gtk_box_pack_start (GTK_BOX (vbox), frame,
					    FALSE, FALSE, 0);
			
			fast_moves_toggle_button = 
				gtk_check_button_new_with_mnemonic ( _("_Use fast moves") );
			if (move_timeout == 10) 
				{
					gtk_toggle_button_set_active 
						(GTK_TOGGLE_BUTTON (fast_moves_toggle_button),
						 TRUE);
				}
			g_signal_connect (G_OBJECT (fast_moves_toggle_button), "clicked", 
					  G_CALLBACK (set_fast_moves_callback), NULL);

			gtk_container_add (GTK_CONTAINER (fv), fast_moves_toggle_button);

			gtk_widget_show_all (pref_dialog);

			pref_dialog_done = TRUE;
		}
	
	gtk_window_present (GTK_WINDOW (pref_dialog));
}

static int
window_resize_cb (GtkWidget * widget, GdkEventConfigure * event, void * data)
{
	gconf_client_set_int (conf_client, KEY_WIDTH, event->width, NULL);
	gconf_client_set_int (conf_client, KEY_HEIGHT, event->height, NULL);

	return FALSE;
}

static int
game_quit_callback (GtkAction *action, gpointer data)
{
	if (animate_id)
		g_source_remove (animate_id);
	gtk_main_quit ();
	return FALSE;
}

static void
game_help_callback (GtkAction *action, gpointer data)
{
	gnome_help_display ("glines.xml", NULL, NULL);
}

static int
preview_configure_cb (GtkWidget * widget, GdkEventConfigure * event)
{
	preview_width = event->width;
	preview_height = event->height;

	refresh_preview_pixmaps ();

	draw_preview ();

	return TRUE;
}

static int
configure_event_callback (GtkWidget *widget, GdkEventConfigure *event)
{
	if (ball_pixmap) 
		g_object_unref (ball_pixmap);

	if (blank_pixmap)
		g_object_unref (blank_pixmap);

	boxsize = (event->width - 1)/hfieldsize;
	ball_pixmap = gdk_pixmap_new (draw_area->window, boxsize*4, boxsize*7,
				      -1);
	blank_pixmap = gdk_pixmap_new (draw_area->window, boxsize, boxsize, -1);
	refresh_pixmaps ();

	refresh_screen ();

	return TRUE;
}

static int
save_state (GnomeClient *client,
	    gint phase,
	    GnomeSaveStyle save_style,
	    gint shutdown,
	    GnomeInteractStyle interact_style,
	    gint fast,
	    gpointer client_data)
{
	gchar *buf;
	int i;

	gconf_client_set_int (conf_client,
                              KEY_SAVED_SCORE, score, NULL);

	buf = g_malloc (hfieldsize * vfieldsize * 4 + 1);
	for (i = 0; i < hfieldsize * vfieldsize; i++)
	{
		buf[i*4] = field[i].color + 'h';
		buf[i*4 + 1] = field[i].pathsearch + 'h';
		buf[i*4 + 2] = field[i].phase + 'h';
		buf[i*4 + 3] = field[i].active + 'h';
	}
	buf[hfieldsize * vfieldsize * 4] = '\0';
	gconf_client_set_string (conf_client,
                                 KEY_SAVED_FIELD, buf, NULL);
	for(i = 0; i < npieces; i++)
		buf[i] = preview[i] + 'h';
	buf[npieces] = '\0';
	gconf_client_set_string (conf_client,
                                 KEY_SAVED_PREVIEW, buf, NULL);
	g_free(buf);

	return TRUE;
}

static void
load_properties (void)
{
	gchar * buf;
	
	ball_filename = gconf_client_get_string (conf_client,
						 KEY_BALL_THEME,
						 NULL);
	if (ball_filename == NULL)
		ball_filename = g_strdup ("balls.svg");

	move_timeout = gconf_client_get_int (conf_client,
					     KEY_MOVE_TIMEOUT,
					     NULL);
	if (move_timeout <= 0)
		move_timeout = 100;

	buf = gconf_client_get_string (conf_client,
				       KEY_BACKGROUND_COLOR,
				       NULL);
	if (buf == NULL)
		buf = g_strdup ("#000000");
	set_backgnd_color (buf);
	g_free (buf);

	load_theme ();
}

static void
restart (void)
{
	gchar *buf;
	int i;

	/* This isn't really a good idea, but if we're going to
	 * restore the game the score has to be stored somewhere
	 * and without some sort of restricted-access storage the
	 * user will always be able to change it. */
	score = gconf_client_get_int (conf_client,
                                      KEY_SAVED_SCORE, NULL);
	if (score < 0)
		score = 0;
	
	buf = gconf_client_get_string (conf_client,
                                       KEY_SAVED_FIELD, NULL);
	if(buf)
	{
		for(i = 0; i < hfieldsize * vfieldsize; i++)
		{
			field[i].color = CLAMP (buf[i*4] - 'h', 1, ncolors);
			field[i].pathsearch = CLAMP (buf[i*4 + 1] - 'h', -1, hfieldsize*vfieldsize);
			field[i].phase = CLAMP (buf[i*4 + 2] - 'h', 0, 3);
			field[i].active = CLAMP (buf[i*4 + 3] - 'h', -1, 1);
		}
		g_free(buf);
	}
	buf = gconf_client_get_string (conf_client,
                                       KEY_SAVED_PREVIEW, NULL);
	if(buf)
	{
		for(i = 0; i < npieces; i++)
			preview[i] = CLAMP (buf[i] - 'h', 1, ncolors);
		g_free(buf);
	}
}


static gint
client_die (GnomeClient *client, gpointer client_data)
{
	gtk_main_quit ();

 	return FALSE;

}


const GtkActionEntry actions[] = {
	{ "GameMenu", NULL, N_("_Game") },
	{ "SettingsMenu", NULL, N_("_Settings") }, 
	{ "HelpMenu", NULL, N_("_Help") },
	{ "NewGame", GAMES_STOCK_NEW_GAME, NULL, NULL, NULL, G_CALLBACK (game_new_callback) },
	{ "Scores", GAMES_STOCK_SCORES, NULL, NULL, NULL, G_CALLBACK (game_top_ten_callback) },
	{ "Quit", GTK_STOCK_QUIT, NULL, NULL, NULL, G_CALLBACK (game_quit_callback) },
	{ "Preferences", GTK_STOCK_PREFERENCES, NULL, NULL, NULL, G_CALLBACK (game_props_callback) },
	{ "Contents", GAMES_STOCK_CONTENTS, NULL, NULL, NULL, G_CALLBACK (game_help_callback) },
	{ "About", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK (game_about_callback) },
	{ "Fullscreen", GAMES_STOCK_FULLSCREEN, NULL, NULL, NULL, G_CALLBACK (fullscreen_callback) },
	{ "LeaveFullscreen", GAMES_STOCK_LEAVE_FULLSCREEN, NULL, NULL, NULL, G_CALLBACK (fullscreen_callback) }
};

const char ui_description[] = 
"<ui>"
"  <menubar name='MainMenu'>"
"    <menu action='GameMenu'>"
"      <menuitem action='NewGame'/>"
"      <separator/>"
"      <menuitem action='Scores'/>"
"      <separator/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='SettingsMenu'>"
"      <menuitem action='Fullscreen'/>"
"      <menuitem action='LeaveFullscreen'/>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='Contents'/>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"
"</ui>";

static void create_menus (GtkUIManager *ui_manager)
{
	GtkAccelGroup *accel_group;
	GtkActionGroup *action_group;

	action_group = gtk_action_group_new ("MenuActions");
	gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (action_group, actions, 
				      G_N_ELEMENTS (actions), NULL);

	fullscreen_action = gtk_action_group_get_action (action_group, "Fullscreen");
	leavefullscreen_action = gtk_action_group_get_action (action_group, "LeaveFullscreen");
	set_fullscreen_actions (FALSE);
	
	gtk_ui_manager_insert_action_group (ui_manager, action_group, 0);
	gtk_ui_manager_add_ui_from_string (ui_manager, ui_description, -1, NULL);
	accel_group = gtk_ui_manager_get_accel_group (ui_manager);
	gtk_window_add_accel_group (GTK_WINDOW (app), accel_group);

	scoreitem = gtk_ui_manager_get_widget (ui_manager, 
					       "/MainMenu/GameMenu/Scores");
	menubar = gtk_ui_manager_get_widget (ui_manager, "/MainMenu");
}

#ifndef GNOME_CLIENT_RESTARTED
#define GNOME_CLIENT_RESTARTED(client) \
(GNOME_CLIENT_CONNECTED (client) && \
 (gnome_client_get_previous_id (client) != NULL) && \
 (strcmp (gnome_client_get_id (client), \
  gnome_client_get_previous_id (client)) == 0))
#endif /* GNOME_CLIENT_RESTARTED */

static void
ball_theme_changed_cb (GConfClient *client,
                       guint cnxn_id,
                       GConfEntry *entry,
                       gpointer user_data)
{
	gchar *theme_tmp = NULL;

	theme_tmp = gconf_client_get_string (client,
					     KEY_BALL_THEME, NULL);

	if (theme_tmp) {
		if (strcmp (theme_tmp, ball_filename) != 0)
			{
				g_free (ball_filename);
				ball_filename = theme_tmp;
				load_theme ();
				refresh_screen ();
			} 
		else
			g_free (theme_tmp);
	}
	
	/* FIXME apply in the prefs dialog GUI */
}

static void
move_timeout_changed_cb (GConfClient *client,
			 guint cnxn_id,
			 GConfEntry *entry,
			 gpointer user_data)
{
	gint timeout_tmp;

	timeout_tmp = gconf_client_get_int (client,
					    KEY_MOVE_TIMEOUT, NULL);
	timeout_tmp = CLAMP (timeout_tmp, 1, 1000);
	if (timeout_tmp != move_timeout)
		move_timeout = timeout_tmp;
  
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (fast_moves_toggle_button),
				      (move_timeout == 10));

}

static void
init_config (int argc, char ** argv)
{
	conf_client = gconf_client_get_default ();

        gconf_init (argc, argv, NULL);

	gconf_client_add_dir (conf_client,
			      KEY_DIR,
			      GCONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);
	gconf_client_notify_add (conf_client,
				 KEY_BALL_THEME,
				 ball_theme_changed_cb,
				 NULL, NULL, NULL);
	gconf_client_notify_add (conf_client,
				 KEY_MOVE_TIMEOUT,
				 move_timeout_changed_cb,
				 NULL, NULL, NULL);
	gconf_client_notify_add (conf_client,
				 KEY_BACKGROUND_COLOR,
				 bg_color_changed_cb,
				 NULL, NULL, NULL);
	gconf_client_notify_add (conf_client,
				 KEY_SIZE,
				 size_changed_cb,
				 NULL, NULL, NULL);


	/* These are here because they are only loaded once. */
	width = gconf_client_get_int (conf_client,
				      KEY_WIDTH, NULL);
	width = MAX (width, MIN_WIDTH);
       
	height = gconf_client_get_int (conf_client,
				       KEY_HEIGHT, NULL);
	height = MAX (height, MIN_HEIGHT);

	game_size = gconf_client_get_int (conf_client,
					  KEY_SIZE, NULL);
	if (game_size == UNSET)
		game_size = DEFAULT_GAME_SIZE;
		
	game_size = CLAMP (game_size, SMALL, MAX_SIZE - 1);
	set_sizes (game_size);
}

int
main (int argc, char *argv [])
{
	char *label_text;
	GtkWidget *label;
	GtkWidget *vbox, *hbox;
	GtkWidget *preview_hbox;
	GtkUIManager *ui_manager;
	GnomeClient *client;
	int i;

	setgid_io_init ();

	rgen = g_rand_new ();
	
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gnome_program_init ("glines", VERSION,
			    LIBGNOMEUI_MODULE,
			    argc, argv,
			    GNOME_PARAM_POPT_TABLE, NULL,
			    GNOME_PARAM_APP_DATADIR, DATADIR, NULL);

	highscores = games_scores_new (&scoredesc);

        init_config (argc, argv);
	games_stock_init ();
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/glines.png");
	client = gnome_master_client ();

	g_signal_connect (G_OBJECT (client), "save_yourself",
			  G_CALLBACK (save_state), argv[0]);
	g_signal_connect (G_OBJECT (client), "die",
			  G_CALLBACK (client_die), NULL);

	if (GNOME_CLIENT_RESTARTED (client))
		restart ();  
	else
		reset_game ();

	app = gnome_app_new ("glines", _("Five or More"));
	gtk_window_set_default_size (GTK_WINDOW (app), width, height);
	gtk_widget_set_size_request (GTK_WIDGET (app), MIN_WIDTH, MIN_HEIGHT);

	g_signal_connect (G_OBJECT (app), "delete_event",
	                  G_CALLBACK (game_quit_callback), NULL);
	g_signal_connect (G_OBJECT (app), "configure_event",
	                  G_CALLBACK (window_resize_cb), NULL);
	g_signal_connect (G_OBJECT (app), "window_state_event",
			  G_CALLBACK (window_state_callback), NULL);

	statusbar = gtk_statusbar_new ();
	ui_manager = gtk_ui_manager_new ();
	
	games_stock_prepare_for_statusbar_tooltips (ui_manager, statusbar);
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (statusbar), FALSE);

	vbox = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (app), vbox);

	create_menus (ui_manager);
	gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

 	table = gtk_table_new (2, 1, TRUE);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), table);
	gtk_box_pack_start (GTK_BOX (vbox), statusbar, FALSE, FALSE, 0);
	hbox = gtk_hbox_new(FALSE, 0);

	gtk_table_attach_defaults (GTK_TABLE (table), hbox, 0, 1, 0, 1);
	top_pane = hbox;
	label_text = g_strdup_printf ("<span weight=\"bold\">%s</span>", _("Next:"));
	label = gtk_label_new (label_text);
	g_free (label_text);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

	gridframe = games_grid_frame_new (MAXNPIECES, 1);
	games_grid_frame_set_alignment (GAMES_GRID_FRAME(gridframe), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox), gridframe, TRUE, TRUE, 0);

	preview_hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (gridframe), 
			   preview_hbox);

	for (i=0; i<MAXNPIECES; i++) {
		preview_widgets[i] = gtk_drawing_area_new ();
		gtk_box_pack_start (GTK_BOX (preview_hbox),
				    preview_widgets[i], TRUE, TRUE, 0);
		/* So we have all the windows at configure time since
		 * we only hook into one of the configure events. */
		/* Yes, this is an evil hack. */
		gtk_widget_realize (preview_widgets[i]);
	}
	g_signal_connect (G_OBJECT (preview_widgets[0]), "configure_event",
			  G_CALLBACK (preview_configure_cb), NULL);

	scorelabel = gtk_label_new ("");

	gtk_box_pack_end (GTK_BOX (hbox), scorelabel, FALSE, FALSE, 5);

	label_text = g_strdup_printf ("<span weight=\"bold\">%s</span>", _("Score:"));
	label = gtk_label_new (label_text);
	g_free (label_text);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 5);

	draw_area = gtk_drawing_area_new ();
	g_signal_connect (G_OBJECT(draw_area), "button_press_event",
			  G_CALLBACK (button_press_event), NULL);
	g_signal_connect (G_OBJECT(draw_area), "key_press_event",
			  G_CALLBACK (key_press_event), NULL);
	g_signal_connect (G_OBJECT (draw_area), "configure_event",
			  G_CALLBACK (configure_event_callback), NULL);
	g_signal_connect (G_OBJECT (draw_area), "expose_event",
			  G_CALLBACK (field_expose_event), NULL);
	gridframe = games_grid_frame_new (hfieldsize, vfieldsize);
	games_grid_frame_set_padding (GAMES_GRID_FRAME(gridframe), 1, 1);
	gtk_container_add (GTK_CONTAINER (gridframe), draw_area);
	gtk_table_attach_defaults (GTK_TABLE (table), gridframe, 0, 1, 1, 2);
	bottom_pane = gridframe;

	relay_table ();

	gtk_widget_set_events (draw_area, gtk_widget_get_events(draw_area) |GDK_BUTTON_PRESS_MASK|GDK_KEY_PRESS_MASK);

	GTK_WIDGET_SET_FLAGS (draw_area, GTK_CAN_FOCUS);
	gtk_widget_grab_focus (draw_area);

	update_score_state ();

	load_properties ();

	gtk_widget_show_all (app);
		
	start_game ();
	
	/* Enter the event loop */
	gtk_main ();

	g_object_unref (conf_client);

	gnome_accelerators_sync();

	return (0);
}
