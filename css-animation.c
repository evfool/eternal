// gcc css-animation.c -o css-animation `pkg-config gtk+-3.0 librsvg-2.0 --cflags --libs` -lm
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

#define SPRITE_HEIGHT 409
#define SPRITE_WIDTH 307

#define FPS 30

static guint tick_cb = 0;
static gboolean fullscreen = FALSE;
static gint64 last_tick = 0;
static GtkWidget *drawing;
static GtkWidget *image;

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

gboolean
on_tick(gpointer data)
{
}

gboolean
on_draw(GtkWidget *widget, cairo_t *cr, gpointer data)
{
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkStyleProvider *css_prov;
  GtkStyleContext *context;

  gtk_init (&argc, &argv);
  //exit(1);
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  css_prov = GTK_STYLE_PROVIDER (gtk_css_provider_new ());
  gtk_css_provider_load_from_data (GTK_CSS_PROVIDER (css_prov), "@keyframes run { 100% { background-position : bottom right;} }  image { animation: run 0.6s steps(4) infinite; background-repeat: no-repeat; background: url(\"spritesheet.png\") bottom left;}", -1, NULL);

  //gtk_window_set_default_size (GTK_WINDOW (window), WINWIDTH, WINHEIGHT);
  gtk_window_set_title (GTK_WINDOW (window), "CSS Animation test");
  g_signal_connect (window, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  //drawing = gtk_fixed_new ();
//  image = gtk_image_new_from_icon_name ("face-angel", GTK_ICON_SIZE_DIALOG);
  image = gtk_image_new ();
  gtk_widget_set_size_request (image, SPRITE_WIDTH, SPRITE_HEIGHT);
  gtk_widget_set_halign (image, GTK_ALIGN_START );
  gtk_widget_set_valign (image, GTK_ALIGN_START );
  context = gtk_widget_get_style_context (image);
  gtk_style_context_add_provider (context, css_prov, GTK_STYLE_PROVIDER_PRIORITY_USER);

//  gtk_container_add (GTK_CONTAINER (drawing), image);
  gtk_container_add (GTK_CONTAINER (window), image);
  gtk_container_set_border_width (GTK_CONTAINER(window), 12);
//  g_signal_connect (drawing, "draw", G_CALLBACK (on_draw), NULL);
  gtk_widget_show_all (window);
  tick_cb = g_timeout_add (1000/FPS/2,(GSourceFunc)on_tick, GINT_TO_POINTER(1));
  gtk_main ();

  return 0;
}
