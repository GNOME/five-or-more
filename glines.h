#ifndef glines_h
#define glines_h

#define STEPSIZE 4
#define BOXSIZE 42
#define BALLSIZE 32
#define FIELDSIZE 9

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
} field_props;

void draw_box(GtkWidget *widget, GdkPixmap *bpm, int x, int y, int redraw);

void draw_ball(GtkWidget *widget, GdkPixmap *bpm, int x, int y, int color, int phase, int redraw);

int find_route(void);

static gint button_press_event (GtkWidget *widget, GdkEvent *event);

void draw_field(GtkWidget *widget, gint redraw);

static gint expose_event (GtkWidget *widget, GdkEventExpose *event, gpointer gp);

void set_inmove(int i);

void init_preview(void);

void draw_preview(void);

void draw_all_balls(GtkWidget *widget, int coord);

int init_new_balls(int num, int prev);

gint animate(gpointer gp);

#endif
