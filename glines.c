/*
 * Color lines for gnome
 * (c) 1999 Free Software Foundation
 * Authors: Robert Szokovacs <szo@appaloosacorp.hu>
 *          Shooby Ban <bansz@szif.hu>
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <config.h>
#include <gnome.h>
#include <gdk_imlib.h>
#include <libgnomeui/gnome-window-icon.h>

#include "glines.h"

GtkWidget *draw_area;
static GtkWidget *app, *vb, *appbar;
GtkWidget *next_draw_area; /* XXX Shouldn't be this much externls! */
GdkPixmap *backpixmap = NULL;
GdkPixmap *next_backpixmap = NULL;
field_props field[FIELDSIZE*FIELDSIZE];
GdkPixmap *box_pixmap = NULL;
GdkPixmap *ball_pixmap = NULL;
GdkBitmap *box_mask = NULL;
GdkBitmap *ball_mask = NULL;
int active = -1;
int target = -1;
int inmove = 0;
int score = 0;
int preview[3];
GtkWidget *scorelabel;
scoretable sctab[] = {{5, 10}, {6, 12}, {7, 18}, {8, 28}, {9, 42}, {10, 82}, {11, 108}, {12, 138}, {13, 172}, {14, 210}, {0,0}};

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
	static int timeou = -1;
	char string[20];

	draw_field(draw_area);
	draw_all_balls(draw_area, -1);
	gtk_widget_draw(draw_area, NULL);
	draw_preview();	
	active = -1;
	target = -1;
	inmove = 0;
	snprintf(string, 19, "%d", score);
	gtk_label_set_text(GTK_LABEL(scorelabel), string);
	if(timeou != -1) gtk_timeout_remove(timeou);
	timeou = gtk_timeout_add(100, animate, (gpointer)draw_area);
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
		draw_ball(next_draw_area, next_backpixmap, i, 0, preview[i], 0, 1);
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
		draw_ball(widget, backpixmap, coord%FIELDSIZE, coord/FIELDSIZE, field[coord].color, 0, 1);
		return;
	}

	for(i = 0; i < FIELDSIZE*FIELDSIZE; i++)
	{
		if(field[i].color != 0)
		{
			draw_ball(widget, backpixmap, i%FIELDSIZE, i/FIELDSIZE, field[i].color, 0, 1);
		}
	}
}

void
draw_box(GtkWidget *widget, GdkPixmap *bpm, int x, int y, int redraw)
{
	gdk_draw_pixmap(bpm, widget->style->fg_gc[GTK_WIDGET_STATE (widget)], box_pixmap,
			0, 0, x*BOXSIZE, y*BOXSIZE, BOXSIZE, BOXSIZE);
	if(redraw)
	{
		GdkRectangle update_rect;

		update_rect.x = x*BOXSIZE;
		update_rect.y = y*BOXSIZE;
		update_rect.width = BOXSIZE;
		update_rect.height = BOXSIZE;   
		gtk_widget_draw(widget, &update_rect);
	}
}

void
draw_ball(GtkWidget *widget, GdkPixmap *bpm, int x, int y, int color, int phase, int redraw)
{
	GdkGC *gc;

	draw_box(widget, bpm, x, y, 0);
	phase = ABS(ABS(3-phase)-3);
	gc = widget->style->fg_gc[GTK_STATE_NORMAL];
	gdk_gc_set_clip_mask (gc, ball_mask);
	gdk_gc_set_clip_origin (gc, 5 + x*BOXSIZE - phase*BALLSIZE, 5 + y*BOXSIZE - (color - 1)*BALLSIZE);
	gdk_draw_pixmap(bpm, gc, ball_pixmap,
			phase*BALLSIZE, (color - 1)*BALLSIZE, 5 + x*BOXSIZE, 5 + y*BOXSIZE, BALLSIZE, BALLSIZE);
	gdk_gc_set_clip_mask (gc, NULL);
	if(redraw == 1)
	{
		GdkRectangle update_rect;

		update_rect.x = x*BOXSIZE;
		update_rect.y = y*BOXSIZE;
		update_rect.width = BOXSIZE;
		update_rect.height = BOXSIZE;   
		gtk_widget_draw(widget, &update_rect);
	}
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
	draw_ball(widget, backpixmap, x, y, field[active].color, field[active].phase, 1);
	active = -1;
}

static gint
button_press_event (GtkWidget *widget, GdkEvent *event)
{
	int x, y;
	int fx, fy;

	/* XXX Ezt megkapja */

	gtk_widget_get_pointer (widget, &x, &y);
	fx = x / BOXSIZE;
	fy = y / BOXSIZE;
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

				inmove = 1;
			}
			else
			{
				/* Can't go there! */
				gnome_appbar_set_status(GNOME_APPBAR(appbar), _("Can't go there!"));
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

void
draw_field(GtkWidget *widget)
{
	int i, j;

	for(i=0; i<9; i++)
		for(j=0; j<9; j++)
		{
			draw_box(widget, backpixmap, i, j, 0);
		}
}

/* Redraw the screen from the backing pixmap */
static gint
expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer gp)
{
	GdkPixmap *bpm = (GdkPixmap *)gp;		
	gdk_draw_pixmap(widget->window,
			widget->style->fg_gc[GTK_WIDGET_STATE (widget)],
			bpm,
			event->area.x, event->area.y,
			event->area.x, event->area.y,
			event->area.width, event->area.height);

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
			draw_box(widget, backpixmap, field[i].pathsearch%9, field[i].pathsearch/9, 0);
		}
	gtk_widget_draw(widget, NULL);
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
			inmove = 0;
		}
		draw_box(widget, backpixmap, x, y, 1);
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
			inmove = 0;
			active = -1;
			field[newactive].phase = 0;
			field[newactive].active = 0;
			draw_ball(widget, backpixmap, x, y, field[newactive].color, field[newactive].phase, 1);
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
	draw_ball(widget, backpixmap, x, y, field[active].color, field[active].phase, 1);

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
 	if (button == 0) {
 	 	gtk_widget_destroy (app);
 	 	gtk_main_quit ();
 	}

}

static int
game_about_callback (GtkWidget *widget, void *data)
{
    GtkWidget *about;
    const gchar *authors[] = {
		            "Robert Szokovacs <szo@appaloosacorp.hu>",
			    "Szabolcs Ban <bansz@szif.hu>",
			    NULL
			    };
					        
	about = gnome_about_new (_("Glines"), VERSION,
			"(C) 1997-1998 the Free Software Foundation",
			  (const char **)authors,
		       _("Gnome port of the once-popular Color Lines game"),
			"glines.xpm");
	gnome_dialog_set_parent(GNOME_DIALOG(about), GTK_WINDOW(app));
	gtk_widget_show (about);
	return TRUE;
}       


static int
game_quit_callback (GtkWidget *widget, void *data)
{
		GtkWidget *box;

	box = gnome_message_box_new (_("Do you really want to quit?"),
	                             GNOME_MESSAGE_BOX_QUESTION,
	                             GNOME_STOCK_BUTTON_YES,
	                             GNOME_STOCK_BUTTON_NO,
	                             NULL);
	gnome_dialog_set_parent (GNOME_DIALOG(box), GTK_WINDOW(app));
	gnome_dialog_set_default (GNOME_DIALOG (box), 0);
	gtk_window_set_modal (GTK_WINDOW (box), TRUE);  
	gtk_signal_connect (GTK_OBJECT (box), "clicked",
	                   (GtkSignalFunc)game_maybe_quit, NULL);
	gtk_widget_show (box);

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
	gchar *prefix= gnome_client_get_config_prefix (client);
	gchar *argv[]= { "rm", "-r", NULL };
	gchar *buf;
	int i;

	gnome_config_push_prefix (prefix);
	gnome_config_set_int ("Glines/Score", score);
	
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

	argv[2]= gnome_config_get_real_path (prefix);
	gnome_client_set_discard_command (client, 3, argv);

	return TRUE;
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

	GNOMEUIINFO_MENU_EXIT_ITEM(game_quit_callback, NULL),

	GNOMEUIINFO_END
};

GnomeUIInfo helpmenu[] = {
	GNOMEUIINFO_MENU_ABOUT_ITEM(game_about_callback, NULL),

	GNOMEUIINFO_END
};

GnomeUIInfo mainmenu[] = {
  	GNOMEUIINFO_MENU_GAME_TREE(gamemenu),
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
	
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	gnome_init ("glines", VERSION, argc, argv);
	gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/glines.xpm");
	client = gnome_master_client ();

	gtk_signal_connect (GTK_OBJECT (client), "save_yourself",  
	                    GTK_SIGNAL_FUNC (save_state), argv[0]);
	gtk_signal_connect (GTK_OBJECT (client), "die",
                            GTK_SIGNAL_FUNC (client_die), NULL);

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
	gtk_signal_connect (GTK_OBJECT(app), "delete_event",
	                    (GtkSignalFunc)game_quit_callback, NULL);

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
	box_pixmap  = gdk_pixmap_create_from_xpm(draw_area->window, &box_mask,  NULL, gnome_unconditional_pixmap_file("glines/box.xpm"));
	ball_pixmap = gdk_pixmap_create_from_xpm(draw_area->window, &ball_mask, NULL, gnome_unconditional_pixmap_file("glines/ball.xpm"));

	backpixmap = gdk_pixmap_new(draw_area->window,
			BOXSIZE*FIELDSIZE,
			BOXSIZE*FIELDSIZE,
			-1);

	next_backpixmap = gdk_pixmap_new(draw_area->window,
			BOXSIZE*3,
			BOXSIZE,
			-1);

	gtk_signal_connect (GTK_OBJECT(draw_area), "button_press_event",
			GTK_SIGNAL_FUNC (button_press_event), NULL);                
	gtk_signal_connect (GTK_OBJECT (draw_area), "expose_event",
			GTK_SIGNAL_FUNC (expose_event), backpixmap);
	gtk_signal_connect (GTK_OBJECT (next_draw_area), "expose_event",
			GTK_SIGNAL_FUNC (expose_event), next_backpixmap);

	gtk_widget_realize(next_draw_area);

	gtk_widget_show(draw_area);
	gtk_widget_show(next_draw_area);

	hbox = gtk_hbox_new(0,3);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, 0, 0, 0);

	gtk_widget_show(hbox);

	gtk_widget_show (vbox);
	
	//gnome_app_set_contents (GNOME_APP (app), vbox);

	gtk_widget_show (app);
		
	start_game();
	
	/* Enter the event loop */
	gtk_main ();

	return(0);
}

