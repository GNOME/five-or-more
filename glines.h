#ifndef glines_h
#define glines_h

#define STEPSIZE 4
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

void draw_box(GtkWidget *widget, int x, int y);

void draw_ball(GtkWidget *widget, int x, int y);

int find_route(void);

void set_inmove(int i);

void init_preview(void);

void draw_preview(void);

int init_new_balls(int num, int prev);

gint animate(gpointer gp);

#endif
