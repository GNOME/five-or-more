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

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <gnome.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnomeui/gnome-window-icon.h>
#include <gconf/gconf-client.h>
#include "games-frame.h"
#include "games-files.h"
#include "games-gridframe.h"
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

#define NCOLORS 7

GConfClient *conf_client = NULL;

GtkWidget *draw_area;
static GtkWidget *app, *appbar, *pref_dialog;
GtkWidget *preview_widgets[3];
field_props field[FIELDSIZE * FIELDSIZE];

/* The unscaled pixbuf as read from file. Cached here to save file reads. */
GdkPixbuf *raw_pixbuf = NULL;
/* The tile images with balls rendered on them. */
GdkPixmap *ball_pixmap = NULL;
/* The balls rendered to a size appropriate for the preview. */
GdkPixmap *preview_pixmaps[7] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL };
/* A pixmap of a blank tile. */
GdkPixmap *blank_pixmap = NULL;

GtkWidget *fast_moves_toggle_button = NULL;

GamesFileList * theme_file_list = NULL;

int active = -1;
int target = -1;
int inmove = 0;
int score = 0;

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
int preview[3];
int response;
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

/* predeclare the menus */

GnomeUIInfo gamemenu[];
GnomeUIInfo settingsmenu[];
GnomeUIInfo helpmenu[];
GnomeUIInfo mainmenu[];

static void 
load_image (gchar *fname,
	    GdkPixbuf **pixbuf)
{
	gchar *tmp, *fn = NULL;
	GdkPixbuf *image;

	tmp = g_build_filename ("glines", fname, NULL);
	
	fn = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP, (tmp), FALSE, NULL);
	g_free (tmp);


	/* Just think of this as an if statement, but we want to use break. */
	while (!g_file_test (fn, G_FILE_TEST_EXISTS)) {
		char * message;
		GtkWidget * w;

		/* ball.png was replaced with balls.svg. */
		if (g_utf8_collate (fname, "ball.png") == 0) {
			tmp = g_build_filename ("glines", "balls.svg", NULL);
			fn = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP, (tmp), FALSE, NULL);
			g_free (tmp);
			if (g_file_test (fn, G_FILE_TEST_EXISTS))
				break;
		}

		message = g_strdup_printf (_("Glines couldn't find image file:\n%s\n\n"
					     "Please check your Glines installation"), fn);
		w = gtk_message_dialog_new (GTK_WINDOW (app),
					    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					    GTK_MESSAGE_ERROR,
					    GTK_BUTTONS_OK,
					    message,
					    NULL);
		gtk_dialog_run (GTK_DIALOG (w));	
		g_free (message);
		exit (1);
	}

	image = gdk_pixbuf_new_from_file (fn, NULL);
	g_free( fn );


	if (*pixbuf)
		g_object_unref (*pixbuf);

	*pixbuf = image;
}

static void
refresh_pixmaps (void)
{
	static GdkGC * grid_gc = NULL;
	GdkGC * gc;
        GdkColormap *cmap;
        GdkColor color;
	GdkPixbuf *ball_pixbuf;
	gint i;

	/* Since we get called both by configure and after loading an image.
	 * it is possible the pixmaps aren't initialised. If they aren't
	 * we don't do anything. */

	if (!ball_pixmap)
		return;

	if (!grid_gc) {
		grid_gc = gdk_gc_new (draw_area->window);

		gdk_color_parse ("#525F6C", &color);
		cmap = gtk_widget_get_colormap (draw_area);
		gdk_colormap_alloc_color (cmap, &color, FALSE, TRUE);
		gdk_gc_set_foreground (grid_gc, &color);
	}

	ball_pixbuf = gdk_pixbuf_scale_simple (raw_pixbuf, 4*boxsize, 
					       7*boxsize, 
					       GDK_INTERP_BILINEAR);

	gc = gdk_gc_new (ball_pixmap);
	gdk_gc_set_foreground (gc, &backgnd.color);

	gdk_draw_rectangle (ball_pixmap, gc, TRUE, 0, 0, 4*boxsize, 7*boxsize);
	for (i=0; i<4; i++) 
		gdk_draw_line (ball_pixmap, grid_gc, i*boxsize, 0, i*boxsize,
			       7*boxsize);
	for (i=0; i<7; i++)
		gdk_draw_line (ball_pixmap, grid_gc, 0, i*boxsize, 4*boxsize,
			       i*boxsize);

	gdk_draw_pixbuf (ball_pixmap, gc,
			 ball_pixbuf, 0, 0, 0, 0, boxsize*4, boxsize*7,
			 GDK_RGB_DITHER_NORMAL, 0, 0);
	gdk_draw_rectangle (blank_pixmap, gc, TRUE, 0, 0, boxsize, boxsize);
	gdk_draw_line (blank_pixmap, grid_gc, 0, 0, 0, boxsize);
	gdk_draw_line (blank_pixmap, grid_gc, 0, 0, boxsize, 0);

	g_object_unref (ball_pixbuf);
	g_object_unref (gc);
}

static void
refresh_preview_pixmaps (void)
{
	int i;
	GdkPixbuf * scaled;
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

	scaled = gdk_pixbuf_scale_simple (raw_pixbuf, 4*preview_width, 
					  7*preview_height, 
					  GDK_INTERP_BILINEAR);

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

	for(i=0; i < FIELDSIZE * FIELDSIZE; i++)
	{
		field[i].color = 0;
		field[i].phase = 0;
		field[i].active = 0;
		field[i].pathsearch = -1;
	}
	score = 0;
	init_preview ();
	init_new_balls (5, -1);
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

	gnome_appbar_set_status (GNOME_APPBAR (appbar),
				 _("Match five balls of the same color in a row to score!"));
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

	for(i = 0; i < FIELDSIZE * FIELDSIZE; i++)
		field[i].pathsearch = -1;
}

void
init_preview (void)
{
	int i;

	for (i = 0; i < 3; i++)
	{
		preview[i] = 1 + (int) (7.0 * rand () / (RAND_MAX + 1.0));
	}
}

void
draw_preview (void)
{
	int i;

	for (i=0; i<3; i++) {
		gdk_window_set_back_pixmap (preview_widgets[i]->window,
					    preview_pixmaps[preview[i]-1],
					    FALSE);
		gdk_window_clear (preview_widgets[i]->window);
	}
}

static void
show_scores (gint pos)
{
	GtkWidget *dialog;

	dialog = gnome_scores_display (_("Glines"), "glines", NULL, pos);
	if (dialog != NULL) {
		gtk_window_set_transient_for (GTK_WINDOW (dialog),
					      GTK_WINDOW (app));
		gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
	}
}

static void
update_score_state ()
{
        gchar **names = NULL;
        gfloat *scores = NULL;
        time_t *scoretimes = NULL;
	gint top;

	top = gnome_score_get_notable ("glines", NULL, &names,
				       &scores, &scoretimes);
	gtk_widget_set_sensitive (gamemenu[2].widget, top > 0);
	g_strfreev (names);
	g_free (scores);
	g_free (scoretimes);
}

static void
game_over (void)
{
	int pos;

	gnome_appbar_set_status (GNOME_APPBAR (appbar), _("Game Over!"));
	pos = gnome_score_log (score, NULL, TRUE);
	show_scores (pos);
	update_score_state ();
	return;
}

static int
check_gameover (void)
{
	int i = 0;
	while ((i < FIELDSIZE * FIELDSIZE) && field[i].color != 0)
		i++;
	if (i == FIELDSIZE * FIELDSIZE)
	{
		game_over ();
		return -1;
	}
	return i;
}

int
init_new_balls (int num, int prev)
{
	int i, j = -1;
	gfloat num_boxes = FIELDSIZE * FIELDSIZE;
	for (i = 0; i < num;)
	{
		j = (int) (num_boxes * rand ()/(RAND_MAX + 1.0));
		if (field[j].color == 0)
		{
			field[j].color = (prev == -1) ? (1 + (int) (7.0*rand()/(RAND_MAX+1.0))):preview[prev]; 
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
	for (i = 0; i < FIELDSIZE*FIELDSIZE; i++)
	{
		if (field[i].pathsearch == num)
		{
			flag = 1;	
			if ((i/9 > 0) && (field[i - 9].pathsearch == -1) && (field[i - 9].color == 0))
				field[i - 9].pathsearch = num+1;
			if ((i/9 < FIELDSIZE - 1) && (field[i + 9].pathsearch == -1) && (field[i + 9].color == 0))
				field[i + 9].pathsearch = num+1;
			if ((i%9 > 0) && (field[i - 1].pathsearch == -1) && (field[i - 1].color == 0))
				field[i - 1].pathsearch = num+1;
			if ((i%9 < FIELDSIZE - 1) && (field[i + 1].pathsearch == -1) && (field[i + 1].color == 0))
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

	for (i = 0; i< FIELDSIZE*FIELDSIZE; i++)
		if ((i != pos) && (field[i].pathsearch == num))
			field[i].pathsearch = -1;
	if (num < 2)
		return;
	if ((pos/9 > 0) && (field[pos - 9].pathsearch == num - 1))
		fix_route(num - 1, pos - 9);	
	if ((pos%9 > 0) && (field[pos - 1].pathsearch == num - 1))
		fix_route(num - 1, pos - 1);	
	if ((pos/9 < FIELDSIZE - 1) && (field[pos + 9].pathsearch == num - 1))
		fix_route(num - 1, pos + 9);	
	if ((pos%9 < FIELDSIZE - 1) && (field[pos + 1].pathsearch == num - 1))
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

static gint
button_press_event (GtkWidget *widget, GdkEvent *event)
{
	int x, y;
	int fx, fy;

	/* XXX Ezt megkapja */

	if(inmove) return TRUE;

	/* Ignore the 2BUTTON and 3BUTTON events. */
	if (event->type != GDK_BUTTON_PRESS)
		return TRUE;
	
	gtk_widget_get_pointer (widget, &x, &y);
	fx = x / boxsize;
	fy = y / boxsize;
        gnome_appbar_set_status(GNOME_APPBAR(appbar), "");
	if(field[fx + fy*9].color == 0)
	{
		/* Clicked on an empty field */

		if((active != -1) && (inmove != 1))
		{
			/* We have an active ball, and it's not
			 * already in move. Therefore we should begin
			 * the moving sequence */

			target = fx + fy*9;
			if(find_route())
			{
				/* We found a route to the new position */

				set_inmove(1);
			}
			else
			{
				/* Can't move there! */
				gnome_appbar_set_status(GNOME_APPBAR(appbar), _("Can't move there!"));
				reset_pathsearch();
				target = -1;
			}
		}
	}
	else
	{
		/* Clicked on a ball */

		if((fx + fy*9) == active)
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

				x = active%9;
				y = active/9;
				deactivate(widget, x, y);
			}

			active = fx + fy*9;
			field[active].active = 1;
		}
	}

	return TRUE;
}

/* Redraw a part of the field */
static gint
field_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer gp)
{
	GdkWindow * window = widget->window;
	GdkGC *gc;
	guint x_start, x_end, y_start, y_end, i, j, idx;

	x_start = event->area.x / boxsize;
	x_end = (event->area.x + event->area.width - 1) / boxsize + 1;

	y_start = event->area.y / boxsize;
	y_end = (event->area.y + event->area.height - 1) / boxsize + 1;

        gc = gdk_gc_new (draw_area->window);

	for (i = y_start; i < y_end; i++) {
		for (j = x_start; j < x_end; j++) {
			int x, y, phase, color;

			idx = j + i * FIELDSIZE;
			x = idx % FIELDSIZE;
			y = idx / FIELDSIZE;

			if (field[idx].color != 0) {
				phase = field[idx].phase;
				color = field[idx].color - 1;

				phase = ABS(ABS(3-phase)-3);

				gdk_draw_drawable (window, gc, ball_pixmap,
						   phase * boxsize, 
						   color * boxsize,
						   x * boxsize, 
						   y * boxsize,
						   boxsize, boxsize);
			} else {
				gdk_draw_drawable (window, gc, blank_pixmap,
						   0, 0,
						   x * boxsize, 
						   y * boxsize,
						   boxsize, boxsize);
			}
		}
	}
        g_object_unref (gc);

	return FALSE;
}

static void
kill_tagged(GtkWidget *widget, int num)
{
	int i;

	for (i = 0; i <= num; i++)
		{
			field[field[i].pathsearch].color = 0;
			field[field[i].pathsearch].phase = 0;
			field[field[i].pathsearch].active = 0;
			draw_box (widget, field[i].pathsearch % 9,
				  field[i].pathsearch / 9);
		}
}

static int
addscore (int num)
{
	int i = 0;
	
	while ((sctab[i].num != num) && (sctab[i].num != 0))
		i++;
	if (sctab[i].num == 0)
		return 10 + (int) pow ((double)2.0, (double)(num - 5));
	return (sctab[i].score);
}
	

static int
check_goal (GtkWidget *widget, int num)
{
	int count = 0;
	int subcount = 0;
	int x = num%9;
	int y = num/9;

	field[0].pathsearch = num;

	/* Horizontal */

	x++;
	while ((x <= FIELDSIZE - 1) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		x++;
	}
	x = num % 9 - 1;
	while ((x >= 0) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		x--;
	}
	if (subcount >= 4)
		count +=subcount;
	subcount = 0;

	/* Vertical */

	x = num%9;
	y++;
	while ((y <= FIELDSIZE - 1) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		y++;
	}
	y = num / 9 - 1;
	while ((y >= 0) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		y--;
	}
	if (subcount >= 4)
		count +=subcount;
	subcount = 0;
	
	/* Diagonal ++*/
	
	x = num%9 + 1;
	y = num/9 + 1;
	while ((y <= FIELDSIZE - 1) && (x <= FIELDSIZE - 1) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		y++;
		x++;
	}
	x = num % 9 - 1;
	y = num / 9 - 1;
	while ((y >= 0) && (x >= 0) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		y--;
		x--;
	}
	if (subcount >= 4)
		count +=subcount;
	subcount = 0;

	/* Diagonal +-*/
	
	x = num % 9 + 1;
	y = num / 9 - 1;
	while ((y >= 0) && (x <= FIELDSIZE - 1) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		y--;
		x++;
	}
	x = num % 9 - 1;
	y = num / 9 + 1;
	while ((y <= FIELDSIZE - 1) && (x >= 0) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		y++;
		x--;
	}
	if (subcount >= 4)
		count +=subcount;
	subcount = 0;

	/* Finish */
	
	if (count >= 4)
	{
		char string[20];

		kill_tagged (widget, count);
		score += addscore (count+1);
		g_snprintf (string, 19, "%d", score);
		gtk_label_set_text (GTK_LABEL (scorelabel), string);

		subcount = 1;
	}
	reset_pathsearch ();
	return subcount;
}

gint
animate (gpointer gp)
{
	GtkWidget *widget = GTK_WIDGET (gp);
	int x, y;
	int newactive = 0;
	/* FIXME nem 9 hanem FIELDSIZE! */
	x = active % 9;
	y = active / 9;

	if (active == -1) return TRUE;
	if (inmove != 0)
	{
		if ((x > 0) && (field[active - 1].pathsearch == field[active].pathsearch+1))
			newactive = active - 1;
		else if ((x < 8) && (field[active + 1].pathsearch == field[active].pathsearch+1))
			newactive = active + 1;
		else if ((y > 0) && (field[active - 9].pathsearch == field[active].pathsearch+1))
			newactive = active - 9;
		else if ((y < 8) && (field[active + 9].pathsearch == field[active].pathsearch+1))
			newactive = active + 9;
		else
		{
			set_inmove (0);
		}
		draw_box (widget, x, y);
		x = newactive % 9;
		y = newactive / 9;
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
				for (x = 0; x < 3; x++)
				{
					int tmp = init_new_balls (1, x);
					draw_box (widget, tmp % FIELDSIZE,
						   tmp / FIELDSIZE);
					check_goal (widget, tmp);
					if (check_gameover () == -1)
						return FALSE;
				}
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
game_new_callback (GtkWidget *widget, void *data)
{
	reset_game ();
 	start_game ();
}

static void
game_top_ten_callback(GtkWidget *widget, gpointer data)
{
	show_scores (0);
}

static int
game_about_callback (GtkWidget *widget, void *data)
{
    static GtkWidget *about = NULL;
    GdkPixbuf *pixbuf = NULL;
    const gchar *authors[] = {
		            "Robert Szokovacs <szo@appaloosacorp.hu>",
			    "Szabolcs B\xc3\xa1n <shooby@gnome.hu>",
			    NULL
			    };
    
   gchar *documenters[] = {
                           NULL
                          };
   /* Translator credits */
   gchar *translator_credits = _("translator-credits");

   if (about != NULL) {
        gtk_window_present (GTK_WINDOW(about));
        return TRUE;
   }
   {
	   char *filename = NULL;

	   filename = gnome_program_locate_file (NULL,
			   GNOME_FILE_DOMAIN_APP_PIXMAP,  ("glines.png"),
			   TRUE, NULL);
	   if (filename != NULL)
	   {
		   pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
		   g_free (filename);
	   }
   }
   					        
	about = gnome_about_new (_("Glines"), VERSION,
			"Copyright \xc2\xa9 1997-2003 Free Software "
			"Foundation, Inc.",
			_("GNOME port of the once-popular Color Lines game"),
			(const char **)authors,
			(const char **)documenters,
			strcmp (translator_credits, "translator-credits") != 0 ? translator_credits : NULL,
		        pixbuf);
	
	if (pixbuf != NULL)
		gdk_pixbuf_unref (pixbuf);
		
	gtk_window_set_transient_for (GTK_WINDOW (about), GTK_WINDOW(app));
	g_signal_connect (G_OBJECT(about), "destroy",
		G_CALLBACK(gtk_widget_destroyed), &about);
	gtk_widget_show (about);
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
load_theme ()
{
	load_image (ball_filename, &raw_pixbuf);
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
pref_dialog_response (GtkDialog *dialog, gint response, gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (dialog));
}

static void
game_props_callback (GtkWidget *widget, void *data)
{
	GtkWidget *w, *omenu, *l, *fv;
	GtkWidget *frame;
	GtkWidget *table;

	if (! pref_dialog)
		{
			pref_dialog = gtk_dialog_new_with_buttons (_("Glines Preferences"),
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

			frame = games_frame_new (_("Themes"));
			table = gtk_table_new (2, 2, FALSE);
			gtk_container_set_border_width (GTK_CONTAINER (table), 0);
			gtk_table_set_row_spacings (GTK_TABLE (table), 6);
			gtk_table_set_col_spacings (GTK_TABLE (table), 6);
			gtk_container_add (GTK_CONTAINER (frame), table);
			gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pref_dialog)->vbox), frame, 
					    FALSE, FALSE, 0);

			l = gtk_label_new (_("Ball image"));
			gtk_misc_set_alignment (GTK_MISC (l), 0, 0.5);
			gtk_table_attach_defaults (GTK_TABLE (table), l, 0, 1, 0, 1);
	    
			omenu = fill_menu ();
			g_signal_connect (G_OBJECT (omenu), "changed",
					  G_CALLBACK (set_selection), NULL);
			gtk_table_attach_defaults (GTK_TABLE (table), omenu, 1, 2, 0, 1);


			l = gtk_label_new (_("Background color"));
			gtk_misc_set_alignment (GTK_MISC (l), 0, 0.5);
			gtk_table_attach_defaults (GTK_TABLE (table), l, 0, 1, 1, 2);
	    
			{
				w  = gtk_color_button_new ();
				gtk_color_button_set_color (GTK_COLOR_BUTTON (w), &backgnd.color);
				g_signal_connect (G_OBJECT (w), "color_set",
						  G_CALLBACK (bg_color_callback), NULL);
			}

			gtk_table_attach_defaults (GTK_TABLE (table), w, 1, 2, 1, 2);


			frame = games_frame_new (_("General"));
			fv = gtk_vbox_new (FALSE, FALSE);
			gtk_box_set_spacing (GTK_BOX (fv), 6);
			gtk_container_add (GTK_CONTAINER (frame), fv);
			gtk_box_pack_start (GTK_BOX (GTK_DIALOG (pref_dialog)->vbox), frame, 
					    FALSE, FALSE, 0);

			fast_moves_toggle_button = 
				gtk_check_button_new_with_label ( _("Use fast moves") );
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
game_quit_callback (GtkWidget *widget, void *data)
{
	if (animate_id)
		g_source_remove (animate_id);
	gtk_main_quit ();
	return FALSE;
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
	int xboxsize, yboxsize;

	if (ball_pixmap) 
		g_object_unref (ball_pixmap);

	if (blank_pixmap)
		g_object_unref (blank_pixmap);

	xboxsize = event->width/FIELDSIZE;
	yboxsize = event->height/FIELDSIZE;
	boxsize = MIN (xboxsize, yboxsize);

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

	buf = g_malloc (FIELDSIZE * FIELDSIZE * 4 + 1);
	for (i = 0; i < FIELDSIZE * FIELDSIZE; i++)
	{
		buf[i*4] = field[i].color + 'h';
		buf[i*4 + 1] = field[i].pathsearch + 'h';
		buf[i*4 + 2] = field[i].phase + 'h';
		buf[i*4 + 3] = field[i].active + 'h';
	}
	buf[FIELDSIZE * FIELDSIZE * 4] = '\0';
	gconf_client_set_string (conf_client,
                                 KEY_SAVED_FIELD, buf, NULL);
	for(i = 0; i < 3; i++)
		buf[i] = preview[i] + 'h';
	buf[3] = '\0';
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
		ball_filename = g_strdup ("pulse.png");

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
		for(i = 0; i < FIELDSIZE * FIELDSIZE; i++)
		{
			field[i].color = CLAMP (buf[i*4] - 'h', 1, NCOLORS);
			field[i].pathsearch = CLAMP (buf[i*4 + 1] - 'h', -1, FIELDSIZE*FIELDSIZE);
			field[i].phase = CLAMP (buf[i*4 + 2] - 'h', 0, 3);
			field[i].active = CLAMP (buf[i*4 + 3] - 'h', -1, 1);
		}
		g_free(buf);
	}
	buf = gconf_client_get_string (conf_client,
                                       KEY_SAVED_PREVIEW, NULL);
	if(buf)
	{
		for(i = 0; i < 3; i++)
			preview[i] = CLAMP (buf[i] - 'h', 1, NCOLORS);
		g_free(buf);
	}
}


static gint
client_die (GnomeClient *client, gpointer client_data)
{
	gtk_main_quit ();

 	return FALSE;

}

GnomeUIInfo gamemenu[] = {
	
    	GNOMEUIINFO_MENU_NEW_GAME_ITEM(game_new_callback, NULL),

	GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_MENU_SCORES_ITEM(game_top_ten_callback, NULL),

        GNOMEUIINFO_SEPARATOR,

	GNOMEUIINFO_MENU_QUIT_ITEM(game_quit_callback, NULL),

	GNOMEUIINFO_END
};

GnomeUIInfo settingsmenu[] = {
        GNOMEUIINFO_MENU_PREFERENCES_ITEM(game_props_callback, NULL),

	GNOMEUIINFO_END
};


GnomeUIInfo helpmenu[] = {
        GNOMEUIINFO_HELP("glines"),
	GNOMEUIINFO_MENU_ABOUT_ITEM(game_about_callback, NULL),
	GNOMEUIINFO_END
};

GnomeUIInfo mainmenu[] = {
  	GNOMEUIINFO_MENU_GAME_TREE(gamemenu),
	GNOMEUIINFO_MENU_SETTINGS_TREE(settingsmenu),
	GNOMEUIINFO_MENU_HELP_TREE(helpmenu),
	GNOMEUIINFO_END
};


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
  
	/* FIXME apply in the prefs dialog GUI */
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

	/* These are here because they are only loaded once. */
	width = gconf_client_get_int (conf_client,
				      KEY_WIDTH, NULL);
	width = MAX (width, MIN_WIDTH);
       
	height = gconf_client_get_int (conf_client,
				       KEY_HEIGHT, NULL);
	height = MAX (height, MIN_HEIGHT);
}

int
main (int argc, char *argv [])
{
	GtkWidget *alignment;   
	GtkWidget *gridframe, *label;
	GtkWidget *vbox, *table, *hbox;
	GtkWidget *preview_hbox;
	GnomeClient *client;
	int i;
	
	gnome_score_init ("glines");

	srand (time (NULL));
	
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gnome_program_init ("glines", VERSION,
			    LIBGNOMEUI_MODULE,
			    argc, argv,
			    GNOME_PARAM_POPT_TABLE, NULL,
			    GNOME_PARAM_APP_DATADIR, DATADIR, NULL);

        init_config (argc, argv);

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

	app = gnome_app_new ("glines", _("Lines"));
	gtk_window_set_default_size (GTK_WINDOW (app), width, height);
	gtk_widget_set_size_request (GTK_WIDGET (app), MIN_WIDTH, MIN_HEIGHT);

	g_signal_connect (G_OBJECT (app), "delete_event",
	                  G_CALLBACK (game_quit_callback), NULL);
	g_signal_connect (G_OBJECT (app), "configure_event",
	                  G_CALLBACK (window_resize_cb), NULL);

	appbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
	gnome_app_set_statusbar (GNOME_APP (app), GTK_WIDGET(appbar));  

	gnome_appbar_set_status (GNOME_APPBAR (appbar),
				 _("Welcome to Glines!"));

	gnome_app_create_menus(GNOME_APP(app), mainmenu);

	gnome_app_install_menu_hints(GNOME_APP (app), mainmenu);
  
	vbox = gtk_vbox_new (FALSE, 0);
	gnome_app_set_contents (GNOME_APP (app), vbox);

 	table = gtk_table_new (10, 1, TRUE);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), table);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_table_attach_defaults (GTK_TABLE (table), hbox, 0, 1, 0, 1);
	label = gtk_label_new (g_strdup_printf ("<span weight=\"bold\">%s</span>", _("Next Balls:")));
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 5);

	alignment = gtk_alignment_new (0, 0.5, 0.3, 1);
	gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);

	gridframe = games_grid_frame_new (3, 1);
	gtk_container_add (GTK_CONTAINER (alignment), gridframe);
	
	preview_hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (gridframe), 
			   preview_hbox);

	for (i=0; i<3; i++) {
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

	label = gtk_label_new (g_strdup_printf ("<span weight=\"bold\">%s</span>", _("Score:")));
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 5);

	draw_area = gtk_drawing_area_new ();
	g_signal_connect (G_OBJECT(draw_area), "button_press_event",
			  G_CALLBACK (button_press_event), NULL);
	g_signal_connect (G_OBJECT (draw_area), "configure_event",
			  G_CALLBACK (configure_event_callback), NULL);
	g_signal_connect (G_OBJECT (draw_area), "expose_event",
			  G_CALLBACK (field_expose_event), NULL);
	gridframe = games_grid_frame_new (FIELDSIZE, FIELDSIZE);
	gtk_container_add (GTK_CONTAINER (gridframe), draw_area);
	gtk_table_attach_defaults (GTK_TABLE (table), gridframe, 0, 1, 1, 10);

	gtk_widget_set_events (draw_area, gtk_widget_get_events(draw_area) |GDK_BUTTON_PRESS_MASK);

	update_score_state ();

	hbox = gtk_hbox_new (0,3);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, 0, 0, 0);

	load_properties ();

	gtk_widget_show_all (app);
		
	start_game ();
	
	/* Enter the event loop */
	gtk_main ();

	g_object_unref (conf_client);

	return (0);
}
