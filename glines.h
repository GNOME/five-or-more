#ifndef glines_h
#define glines_h

#define STEPSIZE 4

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
                        N_("Medium"),
                        N_("Large")};

typedef struct
{
	int num;
	int score;
} scoretable;

typedef struct
{
	int color;
	int pathsearch;
	int phase;
	int active;
	int tag;
} field_props;

void draw_box(GtkWidget *widget, int x, int y);

void draw_ball(GtkWidget *widget, int x, int y);

int find_route(void);

void set_inmove(int i);

void init_preview(void);

void draw_preview(void);

void game_props_callback (void);

int init_new_balls(int num, int prev);

gint animate(gpointer gp);

#endif
