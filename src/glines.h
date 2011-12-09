#ifndef glines_h
#define glines_h

#define STEPSIZE 4

typedef struct {
  int num;
  int score;
} scoretable;

typedef struct {
  int color;
  int pathsearch;
  int phase;
  int active;
  int tag;
} field_props;

void draw_box (GtkWidget * widget, int x, int y);

void draw_ball (GtkWidget * widget, int x, int y);

int find_route (void);

void set_inmove (int i);

void init_preview (void);

void draw_preview (void);

void game_props_callback (void);

int init_new_balls (int num, int prev);

gint animate (gpointer gp);


void game_new_callback (void);
void game_top_ten_callback (GtkAction * action, gpointer data);
int game_quit_callback (GtkAction * action, gpointer data);
void game_props_callback (void);
void game_help_callback (GtkAction * action, gpointer data);
void game_about_callback (GtkAction * action, gpointer * data);

void pref_dialog_response (GtkDialog * dialog, gint response, gpointer data);

#endif
