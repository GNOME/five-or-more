/*
 * Color lines for gnome
 * (c) 1999 Free Software Foundation
 * Authors: Robert Szokovacs <szo@appaloosacorp.hu>
 *          Szabolcs Ban <shooby@gnome.hu>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <dirent.h>

#include <gnome.h>
#include <gtk/gtk.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <libgnomeui/gnome-window-icon.h>

#include "glines.h"

#undef VERSION
#define VERSION "1.9.0"

GtkWidget *bah_window = NULL;

GtkWidget *draw_area;
static GtkWidget *app, *vb, *appbar, *pref_dialog;
GtkWidget *next_draw_area; /* XXX Shouldn't be this much externls! */
field_props field[FIELDSIZE*FIELDSIZE];
GdkPixmap *box_pixmap = NULL;
GdkPixmap *ball_pixmap = NULL;
GdkBitmap *box_mask = NULL;
GdkBitmap *ball_mask = NULL;
int active = -1;
int target = -1;
int inmove = 0;
int score = 0;
int ask_me = 0;
int move_timeout = 100;
int preview[3];
int response;
char * ball_filename;
char * box_filename;
GtkWidget *scorelabel;
scoretable sctab[] = {{5, 10}, {6, 12}, {7, 18}, {8, 28}, {9, 42}, {10, 82}, {11, 108}, {12, 138}, {13, 172}, {14, 210}, {0,0}};

static void 
load_image (gchar *fname,
GdkPixmap **pixmap,
GdkPixmap **mask)
{
	char *tmp, *fn;
        GdkColor bgcolor;
	GdkPixbuf *image;
        GdkImage *tmpimage;
    
	tmp = g_strconcat ( "glines/", fname, NULL);

	fn = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP, (tmp), FALSE, NULL);
	g_free( tmp );

	if (!g_file_test (fn, G_FILE_TEST_EXISTS)) {
		char *message = g_strdup_printf (_("Glines couldn't find pixmap file:\n%s\n\n"
			"Please check your Glines installation"), fn);
		GtkWidget *w = gtk_message_dialog_new (GTK_WINDOW (app),
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

	if (*pixmap)
		gdk_drawable_unref (*pixmap);
	if (*mask)
		gdk_drawable_unref (*mask);

	gdk_pixbuf_render_pixmap_and_mask (image, pixmap, mask, 127);

	gdk_pixbuf_unref (image);
}

static void
help_cb (GtkWidget * widget, gpointer data)
{
#if 0
  GnomeHelpMenuEntry help_entry = { "glines", "user-guide/usage.html#PREFERENCES-DIALOGBOX" };

  gnome_help_display (NULL, &help_entry);
#endif
}

void
set_inmove(int i)
{
 	static int timeou = -1;
 	int ms;
 	
 	if (inmove != i) {
 	        inmove = i;
 	        ms = (inmove?move_timeout:100);
 	        if(timeou != -1) gtk_timeout_remove(timeou);
 	        timeou = gtk_timeout_add(ms, animate, (gpointer)draw_area);
 	}
}


void
reset_game(void)
{
	int i;

	for(i=0; i < 81; i++)
	{
		field[i].color = 0;
		field[i].phase = 0;
		field[i].active = 0;
		field[i].pathsearch = -1;
	}
	score = 0;
	init_preview();
	init_new_balls(5, -1);
}

void
start_game(void)
{
	char string[20];

	gnome_appbar_set_status(GNOME_APPBAR(appbar), _("Match five balls of the same color in a row to score!"));
	draw_all_balls(draw_area, -1);
	draw_preview();	
	active = -1;
	target = -1;
	inmove = -1;
	snprintf(string, 19, "%d", score);
	gtk_label_set_text(GTK_LABEL(scorelabel), string);
	set_inmove(0);
}

 
void
reset_pathsearch(void)
{
	int i;

	for(i = 0; i < FIELDSIZE*FIELDSIZE; i++)
		field[i].pathsearch = -1;
}

void
init_preview(void)
{
	int i;

	for(i = 0; i < 3; i++)
	{
		preview[i] = 1 + (int) (7.0*rand()/(RAND_MAX+1.0));
	}
}

void
draw_preview(void)
{
	int i;

	for(i = 0; i < 3; i++)
	{
		draw_ball(next_draw_area, i, 0);
	}
}

void
game_over(void)
{
	int pos;

	gnome_appbar_set_status(GNOME_APPBAR(appbar), _("Game Over!"));
	pos = gnome_score_log(score, NULL, TRUE);
	gnome_scores_display(_("Glines"), "glines", NULL, pos); 
	return;
}

int
check_gameover(void)
{
	int i = 0;
	while((i < FIELDSIZE*FIELDSIZE) && field[i].color != 0)
		i++;
	if(i == FIELDSIZE*FIELDSIZE)
	{
		game_over();
		return -1;
	}
	return i;
}

int
init_new_balls(int num, int prev)
{
	int i, j = -1;

	for(i = 0; i < num;)
	{
		j = (int) (81.0*rand()/(RAND_MAX+1.0));
		if(field[j].color == 0)
		{
			field[j].color = (prev == -1)?(1 + (int) (7.0*rand()/(RAND_MAX+1.0))):preview[prev]; 
			i++;
		}
	}
	return j;
}

void
draw_all_balls(GtkWidget *widget, int coord)
{
	int i;

	if(coord != -1)
	{
		draw_ball(widget, coord%FIELDSIZE, coord/FIELDSIZE);
		return;
	}

        /* Redraw the whole field */
        gdk_window_invalidate_rect(widget->window, NULL, FALSE);
}

void
draw_box(GtkWidget *widget, int x, int y)
{
	gtk_widget_queue_draw_area(widget, x * BOXSIZE, y * BOXSIZE,
				   BOXSIZE, BOXSIZE);
}

void
draw_ball(GtkWidget *widget, int x, int y)
{
	gtk_widget_queue_draw_area(widget, x * BOXSIZE, y * BOXSIZE,
				   BOXSIZE, BOXSIZE);
}

int
route(int num)
{
	int i;
	int flag = 0;

	if(field[target].pathsearch == num)
		return 1;
	for(i=0; i < FIELDSIZE*FIELDSIZE; i++)
	{
		if(field[i].pathsearch == num)
		{
			flag = 1;	
			if((i/9 > 0) && (field[i - 9].pathsearch == -1) && (field[i - 9].color == 0))
				field[i - 9].pathsearch = num+1;
			if((i/9 < FIELDSIZE - 1) && (field[i + 9].pathsearch == -1) && (field[i + 9].color == 0))
				field[i + 9].pathsearch = num+1;
			if((i%9 > 0) && (field[i - 1].pathsearch == -1) && (field[i - 1].color == 0))
				field[i - 1].pathsearch = num+1;
			if((i%9 < FIELDSIZE - 1) && (field[i + 1].pathsearch == -1) && (field[i + 1].color == 0))
			{
				field[i + 1].pathsearch = num+1;
			}
		}
	}
	if(flag == 0) return 0;
	return 2;
}

void
fix_route(int num, int pos)
{
	int i;

	for(i = 0; i< FIELDSIZE*FIELDSIZE; i++)
		if((i != pos) && (field[i].pathsearch == num))
			field[i].pathsearch = -1;
	if(num < 2)
		return;
	if((pos/9 > 0) && (field[pos - 9].pathsearch == num - 1))
		fix_route(num - 1, pos - 9);	
	if((pos%9 > 0) && (field[pos - 1].pathsearch == num - 1))
		fix_route(num - 1, pos - 1);	
	if((pos/9 < FIELDSIZE - 1) && (field[pos + 9].pathsearch == num - 1))
		fix_route(num - 1, pos + 9);	
	if((pos%9 < FIELDSIZE - 1) && (field[pos + 1].pathsearch == num - 1))
		fix_route(num - 1, pos + 1);	
}
	
int
find_route(void)
{
	int i = 0;
	int j = 0;

	field[active].pathsearch = i;
	while((j = route(i)))
	{
		if(j == 1)
		{
			fix_route(i, target);
			return 1;
		}
		i++;
	}
	return 0;
}
	
void
deactivate(GtkWidget *widget, int x, int y)
{
	field[active].active = 0;
	field[active].phase = 0; 
	draw_ball(widget, x, y);
	active = -1;
}

static gint
button_press_event (GtkWidget *widget, GdkEvent *event)
{
	int x, y;
	int fx, fy;

	/* XXX Ezt megkapja */

	if(inmove) return TRUE;
	gtk_widget_get_pointer (widget, &x, &y);
	fx = x / BOXSIZE;
	fy = y / BOXSIZE;
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

/* Redraw the whole preview */
static gint
preview_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer gp)
{
	GdkGC *gc;
	int i;

	for (i = 0; i < 3; i++) {
		/* Draw the box */
		gdk_draw_drawable(widget->window,
				  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
				  box_pixmap, 0, 0,
				  i * BOXSIZE, 0, BOXSIZE, BOXSIZE);

		/* Draw the ball */
		gc = widget->style->fg_gc[GTK_STATE_NORMAL];
		gdk_gc_set_clip_mask (gc, ball_mask);
		gdk_gc_set_clip_origin (gc, 5 + i * BOXSIZE,
					5 - (preview[i] - 1) * BALLSIZE);
		gdk_draw_drawable(widget->window, gc, ball_pixmap,
				  0, (preview[i] - 1) * BALLSIZE,
				  5 + i * BOXSIZE, 5, BALLSIZE, BALLSIZE);
		gdk_gc_set_clip_mask (gc, NULL);
	}

	return FALSE;
}

/* Redraw a part of the field */
static gint
field_expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer gp)
{
	GdkGC *gc;
	guint x_start, x_end, y_start, y_end, i, j, idx;

	x_start = event->area.x / BOXSIZE;
	x_end = x_start + event->area.width / BOXSIZE + 1;

	y_start = event->area.y / BOXSIZE;
	y_end = y_start + event->area.height / BOXSIZE + 1;

	for (i = y_start; i < y_end; i++) {
		for (j = x_start; j < x_end; j++) {
			idx = j + i * FIELDSIZE;

			/* Draw the box */
			gdk_draw_drawable(widget->window,
					  widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
					  box_pixmap, 0, 0,
					  j * BOXSIZE, i * BOXSIZE, BOXSIZE, BOXSIZE);

			/* Draw the ball */
			if (field[idx].color != 0) {
				int phase = field[idx].phase;
				int color = field[idx].color;
				int x = idx % FIELDSIZE;
				int y = idx / FIELDSIZE;

				phase = ABS(ABS(3-phase)-3);
				gc = widget->style->fg_gc[GTK_STATE_NORMAL];
				gdk_gc_set_clip_mask (gc, ball_mask);
				gdk_gc_set_clip_origin (gc, 5 + x * BOXSIZE - phase * BALLSIZE,
							5 + y * BOXSIZE - (color - 1) * BALLSIZE);
				gdk_draw_drawable(widget->window, gc, ball_pixmap,
						  phase * BALLSIZE,
						  (color - 1) * BALLSIZE,
						  5 + x * BOXSIZE, 5 + y * BOXSIZE,
						  BALLSIZE, BALLSIZE);
				gdk_gc_set_clip_mask (gc, NULL);
			}
		}
	}

	return FALSE;
}

void
kill_tagged(GtkWidget *widget, int num)
{
	int i;

	for(i=0; i <= num; i++)
		{
			field[field[i].pathsearch].color = 0;
			field[field[i].pathsearch].phase = 0;
			field[field[i].pathsearch].active = 0;
			draw_box(widget, field[i].pathsearch%9, field[i].pathsearch/9);
		}
}

int
addscore(int num)
{
	int i = 0;
	
	while((sctab[i].num != num) && (sctab[i].num != 0))
		i++;
	if(sctab[i].num == 0) return 10 + (int)pow((double)2.0,(double)(num - 5));
	return(sctab[i].score);
}
	

int
check_goal(GtkWidget *widget, int num, int is_score)
{
	int count = 0;
	int subcount = 0;
	int x = num%9;
	int y = num/9;

	field[0].pathsearch = num;

	/* Horizontal */

	x++;
	while((x <= FIELDSIZE - 1) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		x++;
	}
	x = num%9 - 1;
	while((x >= 0) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		x--;
	}
	if(subcount >= 4)
		count +=subcount;
	subcount = 0;

	/* Vertical */

	x = num%9;
	y++;
	while((y <= FIELDSIZE - 1) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		y++;
	}
	y = num/9 - 1;
	while((y >= 0) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		y--;
	}
	if(subcount >= 4)
		count +=subcount;
	subcount = 0;
	
	/* Diagonal ++*/
	
	x = num%9 + 1;
	y = num/9 + 1;
	while((y <= FIELDSIZE - 1) && (x <= FIELDSIZE - 1) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		y++;
		x++;
	}
	x = num%9 - 1;
	y = num/9 - 1;
	while((y >= 0) && (x >= 0) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		y--;
		x--;
	}
	if(subcount >= 4)
		count +=subcount;
	subcount = 0;

	/* Diagonal +-*/
	
	x = num%9 + 1;
	y = num/9 - 1;
	while((y >= 0) && (x <= FIELDSIZE - 1) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		y--;
		x++;
	}
	x = num%9 - 1;
	y = num/9 + 1;
	while((y <= FIELDSIZE - 1) && (x >= 0) && (field[x + y*9].color == field[num].color))
	{
		subcount++;
		field[count + subcount].pathsearch = x + y*9;
		y++;
		x--;
	}
	if(subcount >= 4)
		count +=subcount;
	subcount = 0;

	/* Finish */
	
	if(count >= 4)
	{
		kill_tagged(widget, count);
		if(is_score)
		{
			char string[20];
			score += addscore(count+1);
			snprintf(string, 19, "%d", score);
			gtk_label_set_text(GTK_LABEL(scorelabel), string);
		}

		subcount = 1;
	}
	reset_pathsearch();
	return subcount;
}

gint
animate(gpointer gp)
{
	GtkWidget *widget = GTK_WIDGET (gp);
	int x, y;
	int newactive = 0;
	/* FIXME nem 9 hanem FIELDSIZE! */
	x = active%9;
	y = active/9;

	if(active == -1) return TRUE;
	if(inmove != 0)
	{
		if((x > 0) && (field[active - 1].pathsearch == field[active].pathsearch+1))
			newactive = active - 1;
		else if((x < 8) && (field[active + 1].pathsearch == field[active].pathsearch+1))
			newactive = active + 1;
		else if((y > 0) && (field[active - 9].pathsearch == field[active].pathsearch+1))
			newactive = active - 9;
		else if((y < 8) && (field[active + 9].pathsearch == field[active].pathsearch+1))
			newactive = active + 9;
		else
		{
			set_inmove (0);
		}
		draw_box(widget, x, y);
		x = newactive%9;
		y = newactive/9;
		field[newactive].phase = field[active].phase;
		field[newactive].color = field[active].color;
		field[active].phase = 0;
		field[active].color = 0;
		field[active].active = 0;
		if(newactive == target)
		{
			target = -1;
			set_inmove (0);
			active = -1;
			field[newactive].phase = 0;
			field[newactive].active = 0;
			draw_ball(widget, x, y);
			reset_pathsearch();
			if(!check_goal(widget, newactive, 1))
			{
				for(x = 0; x < 3; x++)
				{
					int tmp = init_new_balls(1, x);
					draw_all_balls(widget, tmp);
					check_goal(widget, tmp, 0);
					if(check_gameover() == -1) return FALSE;
				}
				init_preview();
				draw_preview();
			}
			return TRUE;
		}
		field[newactive].active = 1;
		active = newactive;
	}

	field[active].phase++;
	if(field[active].phase >= 4) field[active].phase = 0;
	draw_ball(widget, x, y);

	return TRUE;
}
	
static void
game_new_callback (GtkWidget *widget, void *data)
{
	reset_game();
 	start_game();
}

static void
game_top_ten_callback(GtkWidget *widget, gpointer data)
{
        gnome_scores_display (_("Glines"), "glines", NULL, 0);
}

static void
game_maybe_quit (GtkWidget *widget, int button)
{
 	if (button == GTK_RESPONSE_YES) {
 	 	gtk_widget_destroy (app);
 	 	gtk_main_quit ();
 	}

}

static int
game_about_callback (GtkWidget *widget, void *data)
{
    GtkWidget *about;
    GdkPixbuf *pixbuf = NULL;
    const gchar *authors[] = {
		            _("Robert Szokovacs <szo@appaloosacorp.hu>"),
			    _("Szabolcs Ban <shooby@gnome.hu>"),
			    NULL
			    };
    
   gchar *documenters[] = {
                           NULL
                          };
   /* Translator credits */
   gchar *translator_credits = _("translator_credits");

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
			_("(C) 1997-2000 the Free Software Foundation"),
			_("Gnome port of the once-popular Color Lines game"),
			(const char **)authors,
			(const char **)documenters,
			strcmp (translator_credits, "translator_credits") != 0 ? translator_credits : NULL,
		        pixbuf);
	gtk_window_set_transient_for (GTK_WINDOW (about), GTK_WINDOW(app));
	gtk_widget_show (about);
	return TRUE;
}       


static void
load_theme ()
{
	load_image(box_filename, &box_pixmap, &box_mask);
	load_image(ball_filename, &ball_pixmap, &ball_mask);
}

static void
glines_cancel (GtkWidget *widget, void *data)
{
	gtk_widget_destroy (pref_dialog);
	pref_dialog = 0;
}

static void 
load_theme_cb(void)
{
  gnome_config_set_string ("/glines/Table/BallTheme",
                                     ball_filename);
  gnome_config_set_string ("/glines/Table/BoxTheme",
                                     box_filename);
  gnome_config_set_int ("/glines/Prefs/AskMe",
                                     ask_me);
  gnome_config_set_int ("/glines/Prefs/MoveTimeout",
                                     move_timeout);
  gnome_config_sync();

  load_theme();
  draw_all_balls(draw_area, -1);
  draw_preview();
  glines_cancel(0,0);
}


static void
set_selection (GtkWidget *widget, char *data)
{
	if(ball_filename)
		g_free(ball_filename);
	ball_filename = g_strdup(data);
}

static void
set_selection1 (GtkWidget *widget, char *data)
{
        if(box_filename)
		g_free(box_filename);
	box_filename = g_strdup(data);
}

static void
free_str (GtkWidget *widget, char *data)
{
	g_free (data);
}

static void
fill_menu (GtkWidget *menu, char * mtype, gboolean bg)
{
	struct dirent *e;
	char *dname = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP, ("glines"), FALSE, NULL);
	DIR *dir;
        int itemno = 0;
	
	dir = opendir (dname);

	if (!dir)
		return;
	
	while ((e = readdir (dir)) != NULL){
		GtkWidget *item;
		char *s = g_strdup (e->d_name);

		if (!strstr (e->d_name, mtype)) {
			g_free (s);
			continue;
		}
			
		item = gtk_menu_item_new_with_label (s);
		gtk_widget_show (GTK_WIDGET(item));
		gtk_menu_shell_append (GTK_MENU_SHELL(menu), GTK_WIDGET(item));
		if(bg)
		g_signal_connect (G_OBJECT(item), "activate",
				  G_CALLBACK (set_selection1), s);
		else
		g_signal_connect (G_OBJECT(item), "activate",
				  G_CALLBACK (set_selection), s);
		g_signal_connect (G_OBJECT(item), "destroy",
				  G_CALLBACK (free_str), s);

	        if (bg){
	        if (!strcmp(box_filename, s))
		  gtk_menu_set_active(GTK_MENU(menu), itemno);
		}
	        else {
	        if (!strcmp(ball_filename, s))
		  gtk_menu_set_active(GTK_MENU(menu), itemno);
		}
			  
	        itemno++;
	}
	closedir (dir);
}

static void
set_selection_def (GtkWidget *widget, gpointer *data)
{
  ask_me = GTK_TOGGLE_BUTTON (widget)->active;
}

static void
set_fast_moves (GtkWidget *widget, gpointer *data)
{
 	move_timeout = GTK_TOGGLE_BUTTON (widget)->active ? 10 : 100;
}

game_props_callback (GtkWidget *widget, void *data)
{
	GtkWidget *menu1, *omenu1, *menu, *omenu, *l, *hb, *hb1, *f, *fv, *cb;
	GtkWidget *button;

	if (pref_dialog)
		return;
	
	pref_dialog = gtk_dialog_new_with_buttons (_("Preferences"),
						   GTK_WINDOW (app),
						   GTK_DIALOG_DESTROY_WITH_PARENT,
						   GTK_STOCK_CANCEL,
						   GTK_RESPONSE_CANCEL,
						   GTK_STOCK_OK,
						   GTK_RESPONSE_OK,
						   GTK_STOCK_HELP,
						   GTK_RESPONSE_HELP,
						   NULL);
			
	g_signal_connect (G_OBJECT(pref_dialog), "delete_event",
			  G_CALLBACK (glines_cancel), NULL);

	omenu = gtk_option_menu_new ();
	menu = gtk_menu_new ();
	fill_menu (menu,".png",FALSE);
	gtk_widget_show (omenu);
	gtk_option_menu_set_menu (GTK_OPTION_MENU(omenu), menu);

	omenu1 = gtk_option_menu_new ();
	menu1 = gtk_menu_new ();
	fill_menu (menu1,".xpm",TRUE);
	gtk_option_menu_set_menu (GTK_OPTION_MENU(omenu1), menu1);

	f = gtk_frame_new (_ ("Theme"));
	gtk_container_set_border_width (GTK_CONTAINER (f), 5);

	hb = gtk_hbox_new (FALSE, FALSE);
	
	l = gtk_label_new (_("Select theme:"));
	    
	gtk_box_pack_start_defaults (GTK_BOX(hb), l);
	gtk_container_add (GTK_CONTAINER(hb), omenu);

	fv = gtk_vbox_new (0, 5);
	gtk_container_set_border_width (GTK_CONTAINER (fv), 5);
	
	gtk_box_pack_start_defaults (GTK_BOX(fv), hb);
	hb1 = gtk_hbox_new (FALSE, FALSE);
	gtk_widget_show (hb1);

	l = gtk_label_new (_("Select background:"));
	    
	gtk_box_pack_start_defaults (GTK_BOX(hb1), l);
	gtk_container_add (GTK_CONTAINER(hb1), omenu1);


	gtk_container_add (GTK_CONTAINER(fv), hb1);

	cb = gtk_check_button_new_with_label ( _("Fast moves") );
	if (move_timeout == 10) {
	        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb), TRUE);
	}
	g_signal_connect (G_OBJECT(cb), "clicked", 
			  G_CALLBACK (set_fast_moves), NULL);

	gtk_container_add (GTK_CONTAINER(fv), cb);

	cb = gtk_check_button_new_with_label ( _("Ask confirmation when quitting") );
	if (ask_me) 
	 {
	   gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (cb),
	                                   TRUE);
	 }
        g_signal_connect (G_OBJECT(cb), "clicked", 
			  G_CALLBACK (set_selection_def), NULL);

	gtk_container_add (GTK_CONTAINER(fv), cb);

	gtk_box_pack_start_defaults (GTK_BOX(GTK_DIALOG(pref_dialog)->vbox), f);
	gtk_container_add (GTK_CONTAINER (f), fv);
	
        gtk_widget_show_all (pref_dialog);
	response = gtk_dialog_run (GTK_DIALOG (pref_dialog));
	
	switch (response) {
	case GTK_RESPONSE_OK:
		load_theme_cb ();
		break;
	case GTK_RESPONSE_CANCEL:
		glines_cancel (NULL, NULL);
		break;
	case GTK_RESPONSE_HELP:
		help_cb (NULL, NULL);
		break;
	default:
		break;
	};
	
	
}

static int
game_quit_callback (GtkWidget *widget, void *data)
{
	GtkWidget *box;

	if (ask_me) {

	 box = gtk_message_dialog_new (GTK_WINDOW (app),
				       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_QUESTION,
				       GTK_BUTTONS_NONE,
				       (_("Do you really want to quit?")));
	 gtk_dialog_add_buttons (GTK_DIALOG (box),
				 GTK_STOCK_NO, GTK_RESPONSE_NO,
				 GTK_STOCK_YES, GTK_RESPONSE_YES,
				 NULL);
					
	 gtk_widget_show (box);
	 response = gtk_dialog_run (GTK_DIALOG (box));
	 
	 if (response == GTK_RESPONSE_YES)
	 {
		 gtk_widget_destroy (app);
		 gtk_main_quit ();
	 }
	 else
	 {
		 gtk_widget_hide (box);
		 return TRUE;
	 }
	}
	gtk_main_quit ();
	return FALSE;

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
	const gchar *prefix= gnome_client_get_config_prefix (client);
	gchar *argv[]= { "rm", "-r", NULL };
	gchar *buf;
	int i;

	gnome_config_push_prefix (prefix);
	gnome_config_set_int ("Glines/Score", score);

	gnome_config_set_string ("Glines/BallTheme", ball_filename);

	gnome_config_set_string ("Glines/BoxTheme", box_filename);
	
	buf = g_malloc(FIELDSIZE * FIELDSIZE * 4 + 1);
	for(i = 0; i < FIELDSIZE * FIELDSIZE; i++)
	{
		buf[i*4] = field[i].color + 'h';
		buf[i*4 + 1] = field[i].pathsearch + 'h';
		buf[i*4 + 2] = field[i].phase + 'h';
		buf[i*4 + 3] = field[i].active + 'h';
	}
	buf[FIELDSIZE * FIELDSIZE * 4] = '\0';
	gnome_config_set_string ("Glines/Field", buf);
	for(i = 0; i < 3; i++)
		buf[i] = preview[i] + 'h';
	buf[3] = '\0';
	gnome_config_set_string ("Glines/Preview", buf);
	g_free(buf);

	gnome_config_pop_prefix ();
	gnome_config_sync();

/*	argv[2]= gnome_config_get_real_path (prefix);
	gnome_client_set_discard_command (client, 3, argv);
*/
	return TRUE;
}

static void
load_properties (void)
{
  ball_filename  = gnome_config_get_string ("/glines/Table/BallTheme=pulse.png");
  box_filename  = gnome_config_get_string ("/glines/Table/BoxTheme=gray.xpm");
  ask_me = gnome_config_get_int ("/glines/Prefs/AskMe=0");
  move_timeout = gnome_config_get_int ("/glines/Prefs/MoveTimeout=100");

  load_theme();
}


static void
restart (void)
{
	gchar *buf;
	int i;

	score = gnome_config_get_int_with_default ("Glines/Score", 0);

	buf = gnome_config_get_string_with_default ("Glines/Field", NULL);
	if(buf)
	{
		for(i = 0; i < FIELDSIZE * FIELDSIZE; i++)
		{
			field[i].color = buf[i*4] - 'h';
			field[i].pathsearch = buf[i*4 + 1] - 'h';
			field[i].phase = buf[i*4 + 2] - 'h';
			field[i].active = buf[i*4 + 3] - 'h';
		}
		g_free(buf);
	}
	buf = gnome_config_get_string_with_default ("Glines/Preview", NULL);
	if(buf)
	{
		for(i = 0; i < 3; i++)
			preview[i] = buf[i] - 'h';
		g_free(buf);
	}
}


static gint
client_die (GnomeClient *client, gpointer client_data)
{
 	gtk_exit (0);

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
   
int
main (int argc, char *argv [])
{
	GtkWidget *label, *frame;   
	GtkWidget *vbox, *hbox;
	GnomeClient *client;
	
	gnome_score_init("glines");

	srand(time(NULL));
	
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	gnome_program_init ("glines", VERSION,
			      LIBGNOMEUI_MODULE,
			      argc, argv,
			      GNOME_PARAM_POPT_TABLE, NULL,
			      GNOME_PARAM_APP_DATADIR, DATADIR, NULL);
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/glines.xpm");
	client = gnome_master_client ();

	g_signal_connect (G_OBJECT (client), "save_yourself",
	                    G_CALLBACK (save_state), argv[0]);
	g_signal_connect (G_OBJECT (client), "die",
                            G_CALLBACK (client_die), NULL);

	if (GNOME_CLIENT_RESTARTED (client))
          {
	    gnome_config_push_prefix(gnome_client_get_config_prefix (client));
	    
	    restart();  
	    
	    gnome_config_pop_prefix ();
          }
	else
	{
		reset_game();
	}

	app = gnome_app_new("glines", _("Glines"));

	gtk_window_set_policy(GTK_WINDOW(app), FALSE, FALSE, TRUE);
	g_signal_connect (G_OBJECT(app), "delete_event",
	                  G_CALLBACK (game_quit_callback), NULL);

	appbar = gnome_appbar_new(FALSE, TRUE, GNOME_PREFERENCES_USER);
	gnome_app_set_statusbar(GNOME_APP (app), GTK_WIDGET(appbar));  

	gnome_appbar_set_status(GNOME_APPBAR (appbar),
	                        _("Welcome to Glines!"));

	gnome_app_create_menus(GNOME_APP(app), mainmenu);

	gnome_app_install_menu_hints(GNOME_APP (app), mainmenu);
  
 	vbox = gtk_vbox_new (FALSE, 0);

	gnome_app_set_contents (GNOME_APP (app), vbox);

	hbox = gtk_hbox_new(TRUE, 0);
	frame = gtk_frame_new(_("Next Balls"));

	draw_area = gtk_drawing_area_new();
	next_draw_area = gtk_drawing_area_new();

	gtk_widget_set_usize(draw_area, BOXSIZE*9, BOXSIZE*9);
	gtk_widget_set_usize(next_draw_area, BOXSIZE*3, BOXSIZE);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), hbox);
	gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (frame), next_draw_area);
	gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0);
	gtk_widget_show(frame);

	frame = gtk_frame_new(_("Score"));
	scorelabel = gtk_label_new("");
	gtk_container_add (GTK_CONTAINER (frame), scorelabel);
	gtk_box_pack_end(GTK_BOX(hbox), frame, 0, 0, 0);
	gtk_widget_show(scorelabel);
	gtk_widget_show(frame);

	gtk_widget_show(hbox);
	gtk_box_pack_start_defaults(GTK_BOX(vbox), draw_area);

	gtk_widget_set_events (draw_area, gtk_widget_get_events(draw_area) |GDK_BUTTON_PRESS_MASK);

	gtk_widget_realize(draw_area);

	load_properties();

	g_signal_connect (G_OBJECT(draw_area), "button_press_event",
			  G_CALLBACK (button_press_event), NULL);
	g_signal_connect (G_OBJECT (draw_area), "expose_event",
			  G_CALLBACK (field_expose_event), NULL);

	g_signal_connect (G_OBJECT (next_draw_area), "expose_event",
			  G_CALLBACK (preview_expose_event), NULL);

	gtk_widget_realize(next_draw_area);

	gtk_widget_show(draw_area);
	gtk_widget_show(next_draw_area);

	hbox = gtk_hbox_new(0,3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, 0, 0, 0);

	gtk_widget_show(hbox);

	gtk_widget_show (vbox);
	
	/*gnome_app_set_contents (GNOME_APP (app), vbox);*/

	gtk_widget_show (app);
		
	start_game();
	
	/* Enter the event loop */
	gtk_main ();

	return(0);
}

