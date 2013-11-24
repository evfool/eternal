// gcc eternal.c -o eternal `pkg-config gtk+-3.0 librsvg-2.0 --cflags --libs` -lm
#include <stdlib.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <librsvg/rsvg.h>
#include <cairo/cairo.h>

#define WINWIDTH        1000
#define WINHEIGHT       550

#define GREY_FLOOR 0
#define BRICK_WALL 1
#define BRICK_TOP 2
#define GREY_FLOOR_WITHOUT_COIN 3

#define WORLD_WIDTH 5
#define WORLD_HEIGHT 5
#define TILE_SIZE 32
#define ZONE_WIDTH 20
#define ZONE_HEIGHT 15
#define FPS 60

typedef struct {
  guint xtile, ytile;
  guint xpos, ypos;
  gint direction;
  guint xsize, ysize;
  gdouble progress;
  gdouble speed;
  gint nextdirection;
  gboolean human;
} Actor;

static RsvgHandle * tiles[4];
static RsvgHandle *coin;
static GRand * randgen;

static guint size = TILE_SIZE;
static guint tick_cb = 0;
static gboolean fullscreen = FALSE;
static gint64 last_tick = 0;

static GtkWidget *drawing;

static Actor playerx = {9, 2, 9*TILE_SIZE, 2*TILE_SIZE, -1, TILE_SIZE, TILE_SIZE, 0.0, 0.25, -1, TRUE};

static Actor enemy = {1, 2, 1*TILE_SIZE, 2*TILE_SIZE, 2, TILE_SIZE, TILE_SIZE, 0.0, 0.25, -1, FALSE};

static guint map[ZONE_HEIGHT][ZONE_WIDTH] = { { [0 ... ZONE_WIDTH-1] = BRICK_TOP}, 
                                              { BRICK_TOP, [1 ... ZONE_WIDTH-2] = BRICK_WALL, BRICK_TOP}, 
                                              { BRICK_TOP, [1 ... ZONE_WIDTH-2] = GREY_FLOOR, BRICK_TOP}, 
                                              { BRICK_TOP, GREY_FLOOR, [2 ... 5] = BRICK_TOP, GREY_FLOOR, [7 ... 12] = BRICK_TOP, GREY_FLOOR, [14 ... 17] = BRICK_TOP, GREY_FLOOR, BRICK_TOP}, 
                                              { BRICK_TOP, GREY_FLOOR, BRICK_TOP, [3 ... 5] = BRICK_WALL, GREY_FLOOR, [7 ... 8] = BRICK_WALL, [9 ... 10] = BRICK_TOP, [11 ... 12] = BRICK_WALL, GREY_FLOOR, [14 ... 16] = BRICK_WALL, BRICK_TOP, GREY_FLOOR, BRICK_TOP}, 
                                              { BRICK_TOP, GREY_FLOOR, BRICK_WALL, [3 ... 8] = GREY_FLOOR, [9 ... 10] = BRICK_TOP, [11 ... 16] = GREY_FLOOR, BRICK_WALL, GREY_FLOOR, BRICK_TOP}, 
                                              { BRICK_TOP, [1 ... 3] = GREY_FLOOR, [4 ... 7] = BRICK_TOP, GREY_FLOOR, [9 ... 10] = BRICK_TOP, GREY_FLOOR, [12 ... 15] = BRICK_TOP, [16 ... 18] = GREY_FLOOR, BRICK_TOP}, 
                                              { [0 ... ZONE_WIDTH-1] = BRICK_TOP, [1] = GREY_FLOOR, [8] = GREY_FLOOR, [11] = GREY_FLOOR, [18] = GREY_FLOOR}, 
                                              { BRICK_TOP, [1 ... ZONE_WIDTH-2] = BRICK_WALL, [1] = GREY_FLOOR, [8] = GREY_FLOOR, [11] = GREY_FLOOR, [18] = GREY_FLOOR, [5] = BRICK_TOP, [9] = BRICK_TOP, BRICK_TOP, [14] = BRICK_TOP, [19] = BRICK_TOP}, 
                                              { BRICK_TOP, [1 ... ZONE_WIDTH-2] = GREY_FLOOR, [5] = BRICK_TOP, [9] = BRICK_TOP, BRICK_TOP, [14] = BRICK_TOP, [19] = BRICK_TOP}, 
                                              { [0 ... ZONE_WIDTH-1] = BRICK_TOP, [1] = GREY_FLOOR, [6] = GREY_FLOOR, [13] = GREY_FLOOR, [18] = GREY_FLOOR}, 
                                              { BRICK_TOP, [1 ... ZONE_WIDTH-2] = BRICK_WALL, [1] = GREY_FLOOR, [6] = GREY_FLOOR, [13] = GREY_FLOOR, [18] = GREY_FLOOR, BRICK_TOP}, 
                                                [12 ... ZONE_HEIGHT-3] = {BRICK_TOP, [1 ... ZONE_WIDTH-2] = GREY_FLOOR, BRICK_TOP}, 
                                              { [0 ... ZONE_WIDTH-1] = BRICK_TOP},
                                              { [0 ... ZONE_WIDTH-1] = BRICK_WALL} };

static void
on_fullscreen (GtkButton *button,
               GtkWindow *window)
{
  if (!fullscreen)
    {
      gtk_window_fullscreen (window);
      fullscreen = TRUE;
    }
  else
    {
      gtk_window_unfullscreen (window);
      fullscreen = FALSE;
    }
}

//  gdouble scale_factor = ((float)MIN(width,height))/(128+SPACING/2);
//  cairo_scale(cr, scale_factor, scale_factor);
//  RsvgHandle* svg = rsvg_handle_new_from_file ("ship.svg", NULL);
//  rsvg_handle_render_cairo(svg, cr);
static void draw_map(cairo_t * cr, guint width, guint height, guint i, guint j) {
  switch (map[i][j]) {
    case BRICK_WALL:
    cairo_set_source_rgba(cr, 0.5, 0.5, 0.5, 1.0);
    break;
    case BRICK_TOP:
    cairo_set_source_rgba(cr, 0.2, 0.2, 0.2, 1.0);
    break;
    default:
    cairo_set_source_rgba(cr, 0.8, 0.8, 0.8, 1.0);
    break;
  }  
  cairo_rectangle(cr, j*width, i*height, (j+1)*width, (i+1)*height);
  cairo_fill(cr);
  if (map[i][j] == GREY_FLOOR) {
    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 1.0);
    cairo_arc (cr, j*width + width/2, i*height + height/2 + height/5, MIN(width, height)/5, 0, 2 * M_PI);
    cairo_fill(cr);
  }
  
}

static void draw_map_svg(cairo_t * cr, guint width, guint height, guint i, guint j) {
  
  cairo_save (cr);
  cairo_translate (cr, j*70, i*70);
  rsvg_handle_render_cairo(tiles[map[i][j]], cr);
  if (map[i][j] == GREY_FLOOR) {
    cairo_translate (cr, 35/2, 35/2);
    cairo_set_source_rgba(cr, 0.7, 0.7, 0.7, 0.7);
    cairo_arc(cr, 35/2, 5+35/2, 18, 0, 2*M_PI);
    cairo_fill(cr);
    rsvg_handle_render_cairo(coin, cr);
  }
  cairo_restore (cr);
}

static gboolean will_block_on_wall(Actor * player, gint xdirection, gint ydirection) {
  guint nexttile = map[player->ytile+ydirection][player->xtile+xdirection];
  //if (!player->human) {
    //printf ("Checking tile %d,%d in direction %d, %d, found %d\n", player->ytile+ydirection, player->xtile+xdirection, xdirection, ydirection, nexttile);
  //}
  return (nexttile != GREY_FLOOR && nexttile != GREY_FLOOR_WITHOUT_COIN);
  
}
static gboolean update_player_position (Actor* player) {
  if (player->direction == -1)
    return FALSE;
  gint xdirection = ((player->direction + 1)%4-2)%2;
  gint ydirection = (player->direction%4-2)%2;
  if (will_block_on_wall (player, xdirection, ydirection)) {
    player->direction = -1;
  }
  if (player->direction != -1) {
    player->progress +=  player->speed;
    player->xpos = player->xtile * player->xsize + xdirection * player->progress * player->xsize;
    player->ypos = player->ytile * player->ysize + ydirection * player->progress * player->ysize;
    if (player->progress>=1.0) {
      player->xtile = player->xtile + xdirection;
      player->ytile = player->ytile + ydirection;
      if (will_block_on_wall (player, xdirection, ydirection))
        player->direction = -1;
      xdirection = ((player->nextdirection + 1)%4-2)%2;
      ydirection = (player->nextdirection%4-2)%2;
      if (!will_block_on_wall (player, xdirection, ydirection))
        player->direction = player->nextdirection;
      player->progress = 0.0;
    }
    map[player->ytile][player->xtile] = GREY_FLOOR_WITHOUT_COIN;
    return TRUE;
  }
  return FALSE;
}

static guint randomize_next_direction (Actor *player) {
  gboolean possible[4] = {[0 ... 3] = FALSE};
  guint i;
  
  if (!will_block_on_wall(player, -1, 0) && player->direction != 2) possible[0] = TRUE;
  if (!will_block_on_wall(player, 0, -1) && player->direction != 3) possible[1] = TRUE;
  if (!will_block_on_wall(player, 1, 0) && player->direction != 0) possible[2] = TRUE;
  if (!will_block_on_wall(player, 0, 1) && player->direction != 1) possible[3] = TRUE;
  //printf ("Randomizing at %d, %d from %d, %d, %d, %d\n", player->ytile, player->xtile, possible[0], possible[1], possible[2], possible[3]);
  
  gboolean one_way = !possible[0] && !possible [1] && !possible[2] && !possible[3];
  if (one_way)
    return player->direction + 2 % 4;
  do {
    i = g_rand_int_range (randgen,0,4);
  } while (!possible[i]);
  
  return i;
}

static gboolean update_enemy_position (Actor* player) {
  if (player->direction == -1)
    return FALSE;
  gint xdirection = ((player->direction + 1)%4-2)%2;
  gint ydirection = (player->direction%4-2)%2;
  if (will_block_on_wall (player, xdirection, ydirection)) {
    player->direction = -1;
  }
  if (player->direction != -1) {
    player->progress +=  player->speed;
    player->xpos = player->xtile * player->xsize + xdirection * player->progress * player->xsize;
    player->ypos = player->ytile * player->ysize + ydirection * player->progress * player->ysize;
    if (player->progress>=1.0) {
      player->xtile = player->xtile + xdirection;
      player->ytile = player->ytile + ydirection;
      player->direction = randomize_next_direction(player);
      player->progress = 0.0;
    }
    return TRUE;
  }
  return FALSE;
}

static gboolean on_tick (gpointer user_data) {
  gint64 current = g_get_real_time ();
  gboolean changed = FALSE;
  if ((current - last_tick) < (1000/ FPS)) {
    last_tick = current;
    return G_SOURCE_CONTINUE;
  }
  changed = update_player_position(&playerx);
  if (changed) {
    gtk_widget_queue_draw_area (drawing, playerx.xpos - playerx.xsize, playerx.ypos - playerx.ysize, 3*playerx.xsize, 3*playerx.ysize);
  }
  changed = update_enemy_position(&enemy);
  if (changed) {
    gtk_widget_queue_draw_area (drawing, enemy.xpos - enemy.xsize, enemy.ypos - enemy.ysize, 3*enemy.xsize, 3*enemy.ysize);
  }
  last_tick = current;
  return G_SOURCE_CONTINUE;
}

static void draw_actor (cairo_t * cr, Actor * player) {
  if (player->human) {
    cairo_set_source_rgba (cr, 1.0, 0.0, 0.0, 1.0);
  } else {
    cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, 1.0);
  }
  cairo_arc (cr, player->xpos + player->xsize/2, player->ypos+player->ysize/2, MIN(player->xsize, player->ysize)/2, 0, 2*M_PI);
  cairo_fill (cr);
}

static void on_draw (GtkWidget *widget, cairo_t *cr, gpointer user_data) {
  GtkAllocation allocation;
  guint i,j;
  guint sizeX, sizeY;
  guint startX, startY;
  gtk_widget_get_allocation (widget, &allocation);
  sizeX = allocation.width/ZONE_WIDTH;
  sizeY = allocation.height/ZONE_HEIGHT;
  playerx.xsize = sizeX;
  playerx.ysize = sizeY;
  enemy.xsize = sizeX;
  enemy.ysize = sizeY;
  if (sizeX != size) {
    g_source_remove (tick_cb);
    size = sizeX;
    tick_cb = g_timeout_add (1000/FPS/2, (GSourceFunc)on_tick, GINT_TO_POINTER(size));
    playerx.xpos = playerx.xtile * sizeX;
    playerx.ypos = playerx.ytile * sizeY;
    enemy.xpos = enemy.xtile * sizeX;
    enemy.ypos = enemy.ytile * sizeY;
  }
  startX = (allocation.width - sizeX*ZONE_WIDTH)/2;
  startY = (allocation.height - sizeY*ZONE_HEIGHT)/2;
  cairo_translate (cr, startX, startY);
  cairo_save(cr);
  cairo_scale (cr,  ((double)sizeX)/70, ((double)sizeY)/70);
  for (i=0;i<ZONE_HEIGHT;i++){
    for (j=0;j<ZONE_WIDTH;j++) {
      draw_map_svg(cr, sizeX, sizeY, i, j);
    }
  }
  cairo_restore(cr);
  draw_actor (cr, &playerx);
  draw_actor (cr, &enemy);
}

static gboolean on_key_released (GtkWidget *widget, GdkEventKey *
event, Actor* player) {
  switch (event->keyval) {
    case GDK_KEY_Left:
    printf ("Left\n");
    player->nextdirection = 0;
    break;
    case GDK_KEY_Right:
    printf ("Right\n");
    player->nextdirection = 2;
    break;
    case GDK_KEY_Up:
    printf ("Up\n");
    player->nextdirection = 1;
    break;
    case GDK_KEY_Down:
    printf ("Down\n");
    player->nextdirection = 3;
    break;
    default:
    printf("Something else\n");
  }
  if (player->direction == -1) {
    player->direction = player->nextdirection;
  }
    
  return FALSE;
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  gtk_init (&argc, &argv);
  GError * error = NULL;
  tiles[0] = rsvg_handle_new_from_file ("castleCenter.svg", &error);
  if (error) {
    printf ("%s\n", error->message);
  }
  tiles[1] = rsvg_handle_new_from_file ("boxEmpty.svg", NULL);
  tiles[2] = rsvg_handle_new_from_file ("boxAlt.svg", NULL);
  tiles[3] = rsvg_handle_new_from_file ("castleCenter.svg", NULL);
  coin = rsvg_handle_new_from_file ("coin.svg", NULL);
  //exit(1);
  randgen = g_rand_new_with_seed (g_get_real_time());
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW (window), WINWIDTH, WINHEIGHT);
  gtk_window_set_title (GTK_WINDOW (window), "Scrolling drawingArea");
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  drawing = gtk_drawing_area_new ();
  gtk_container_add (GTK_CONTAINER (window), drawing);
  gtk_container_set_border_width (GTK_CONTAINER(window), 12);
  g_signal_connect (drawing, "draw", G_CALLBACK (on_draw), NULL);
  g_signal_connect (window, "key-release-event", G_CALLBACK (on_key_released), &playerx);
  gtk_widget_show_all (window);
  tick_cb = g_timeout_add (1000/FPS/2,(GSourceFunc)on_tick, GINT_TO_POINTER(size));
  gtk_main ();

  return 0;
}
