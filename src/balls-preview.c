/* -*- mode:C; indent-tabs-mode:t; tab-width:8; c-basic-offset:8; -*- */

/*
 * Color lines for GNOME
 * Copyright © 1999 Free Software Foundation
 * Authors: Robert Szokovacs <szo@appaloosacorp.hu>
 *          Szabolcs Ban <shooby@gnome.hu>
 *          Karuna Grewal <karunagrewal98@gmail.com>
 * Copyright © 2007 Christian Persch
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
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <stdlib.h>
#include <locale.h>
#include <math.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>

#include "games-preimage.h"
#include "balls-preview.h"
#include "game-area.h"

#define MAXNPIECES 10
#define PREVIEW_IMAGE_WIDTH 20
#define PREVIEW_IMAGE_HEIGHT 20

static GtkImage* preview_images[MAXNPIECES];
static GdkPixbuf* preview_pixbufs[MAXNPIECES];
static int preview[MAXNPIECES];

void
init_preview (void)
{
  int i;
  gint npieces = get_npieces();
  for (i = 0; i < npieces; i++) {
    preview[i] = g_rand_int_range (*get_rgen(), 1, get_ncolors() + 1);
  }
}

void
draw_preview (void)
{
  guint i;
  gint npieces = get_npieces();
  /* This function can be called before the images are initialized */
  if (!GTK_IS_IMAGE (preview_images[0]))
    return;

  for (i = 0; i < MAXNPIECES; i++) {
    if (i < npieces)
      gtk_image_set_from_pixbuf (preview_images[i], preview_pixbufs[preview[i] - 1]);
    else
      gtk_image_clear (preview_images[i]);
  }
}

void
refresh_preview_surfaces (void)
{
  guint i;
  GdkPixbuf *scaled = NULL;
  GtkWidget *widget = GTK_WIDGET (preview_images[0]);
  GtkStyleContext *context;
  GdkRGBA bg;
  cairo_t *cr;
  GdkRectangle preview_rect;
  cairo_surface_t *blank_preview_surface = NULL;
  /* The balls rendered to a size appropriate for the preview. */
  cairo_surface_t *preview_surface = NULL;

  context = gtk_widget_get_style_context (widget);
  gtk_style_context_get_background_color (context, gtk_style_context_get_state (context), &bg);

  /* Like the refresh_pixmaps() function, we may be called before
   * the window is ready. */
  if (PREVIEW_IMAGE_HEIGHT == 0)
    return;

  preview_rect.x = 0;
  preview_rect.y = 0;
  preview_rect.width = PREVIEW_IMAGE_WIDTH;
  preview_rect.height = PREVIEW_IMAGE_HEIGHT;

  /* We create pixmaps for each of the ball colours and then
   * set them as the background for each widget in the preview array.
   * This code assumes that each preview window is identical. */
  GamesPreimage *ball_preimage = get_ball_preimage();
  if (ball_preimage)
    scaled = games_preimage_render (ball_preimage, 4 * PREVIEW_IMAGE_WIDTH,
                                    7 * PREVIEW_IMAGE_HEIGHT);

  if (!scaled) {
    scaled = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
                             4 * PREVIEW_IMAGE_WIDTH, 7 * PREVIEW_IMAGE_HEIGHT);
    gdk_pixbuf_fill (scaled, 0x00000000);
  }

  for (i = 0; i < 7; i++) {
    preview_surface = gdk_window_create_similar_image_surface (gtk_widget_get_window (widget),
                                                               CAIRO_FORMAT_ARGB32,
                                                               PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT, 1);
    cr = cairo_create (preview_surface);
    gdk_cairo_set_source_rgba (cr, &bg);
    gdk_cairo_rectangle (cr, &preview_rect);
    cairo_fill (cr);

    gdk_cairo_set_source_pixbuf (cr, scaled, 0, -1.0 * PREVIEW_IMAGE_HEIGHT * i);
    cairo_mask (cr, cairo_get_source (cr));

    if (preview_pixbufs[i])
      g_object_unref (preview_pixbufs[i]);

    preview_pixbufs[i] = gdk_pixbuf_get_from_surface (preview_surface, 0, 0,
                                                      PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT);

    cairo_destroy (cr);
    cairo_surface_destroy (preview_surface);
  }

  blank_preview_surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                                             CAIRO_CONTENT_COLOR_ALPHA,
                                                             PREVIEW_IMAGE_WIDTH, PREVIEW_IMAGE_HEIGHT);
  cr = cairo_create (blank_preview_surface);
  gdk_cairo_set_source_rgba (cr, &bg);
  gdk_cairo_rectangle (cr, &preview_rect);
  cairo_fill (cr);

  cairo_surface_destroy (blank_preview_surface);
  cairo_destroy (cr);
  g_object_unref (scaled);
}

GtkImage **
get_preview_images()
{
  return preview_images;
}

int *
get_preview()
{
  return preview;
}

GdkPixbuf **
get_preview_pixbufs()
{
  return preview_pixbufs;
}