/*

 rl2legend -- LegendGraphic functions

 version 0.1, 2021 August 17

 Author: Sandro Furieri a.furieri@lqt.it

 -----------------------------------------------------------------------------
 
 Version: MPL 1.1/GPL 2.0/LGPL 2.1
 
 The contents of this file are subject to the Mozilla Public License Version
 1.1 (the "License"); you may not use this file except in compliance with
 the License. You may obtain a copy of the License at
 http://www.mozilla.org/MPL/
 
Software distributed under the License is distributed on an "AS IS" basis,
WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
for the specific language governing rights and limitations under the
License.

The Original Code is the RasterLite2 library

The Initial Developer of the Original Code is Alessandro Furieri
 
Portions created by the Initial Developer are Copyright (C) 2021
the Initial Developer. All Rights Reserved.

Alternatively, the contents of this file may be used under the terms of
either the GNU General Public License Version 2 or later (the "GPL"), or
the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
in which case the provisions of the GPL or the LGPL are applicable instead
of those above. If you wish to allow use of your version of this file only
under the terms of either the GPL or the LGPL, and not to allow others to
use your version of this file under the terms of the MPL, indicate your
decision by deleting the provisions above and replace them with the notice
and other provisions required by the GPL or the LGPL. If you do not delete
the provisions above, a recipient may use your version of this file under
the terms of any one of the MPL, the GPL or the LGPL.
 
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2_private.h"

#ifdef __ANDROID__		/* Android specific */
#include <cairo.h>
#include <cairo-svg.h>
#include <cairo-pdf.h>
#include <cairo-ft.h>
#else /* any other standard platform (Win, Linux, Mac) */
#include <cairo/cairo.h>
#include <cairo/cairo-svg.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-ft.h>
#endif /* end Android conditionals */

#include "rl2paint_private.h"

static void
do_paint_monochrome_graphic (cairo_t * cairo, rl2LegendGraphicPtr aux,
			     int shift_graphic)
{
/* painting a MonoChrome Raster Graphic */
    double x0 = (double) aux->width / 10.0;
    double x1 = ((double) aux->width / 10.0) * 3.5;
    double x2 = ((double) aux->width / 10.0) * 7.0;
    double x3 = ((double) aux->width / 10.0) * 7.5;
    double x4 = ((double) aux->width / 10.0) * 5.0;
    double y0 = ((double) aux->height / 8.0) * 3.0;
    double y1 = ((double) aux->height / 8.0) * 6.0;
    double y2 = ((double) aux->height / 8.0) * 5.0;
    double y3 = ((double) aux->height / 8.0) * 2.0;
    double y4 = ((double) aux->height / 8.0) * 2.5;
    double w1 = (double) aux->width / 6.0;
    double h1 = (double) aux->height / 4.0;
    double w2 = (double) aux->width / 8.0;
    double h2 = (double) aux->height / 6.0;
    cairo_rectangle (cairo, 5, shift_graphic, aux->width, aux->height);
    cairo_set_source_rgb (cairo, 1.0, 1.0, 1.0);
    cairo_fill_preserve (cairo);
    cairo_set_source_rgb (cairo, 0.0, 0.0, 0.0);
    cairo_set_line_width (cairo, 0.5);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x0, y0, w1, h1);
    cairo_stroke (cairo);
    cairo_move_to (cairo, x0, y0);
    cairo_line_to (cairo, x0 + w1, y0 + h1);
    cairo_stroke (cairo);
    cairo_move_to (cairo, x0, y0 + h1);
    cairo_line_to (cairo, x0 + w1, y0);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x1, y1, w1, h2);
    cairo_stroke (cairo);
    cairo_move_to (cairo, x1, y1);
    cairo_line_to (cairo, x1 + w1, y1 + h2);
    cairo_stroke (cairo);
    cairo_move_to (cairo, x1, y1 + h2);
    cairo_line_to (cairo, x1 + w1, y1);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x2, y2, w2, h1);
    cairo_stroke (cairo);
    cairo_move_to (cairo, x2, y2);
    cairo_line_to (cairo, x2 + w2, y2 + h1);
    cairo_stroke (cairo);
    cairo_move_to (cairo, x2, y2 + h1);
    cairo_line_to (cairo, x2 + w2, y2);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x3, y3, w2, h2);
    cairo_stroke (cairo);
    cairo_move_to (cairo, x3, y3);
    cairo_line_to (cairo, x3 + w2, y3 + h2);
    cairo_stroke (cairo);
    cairo_move_to (cairo, x3, y3 + h2);
    cairo_line_to (cairo, x3 + w2, y3);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x4, y4, w2, h1);
    cairo_stroke (cairo);
    cairo_move_to (cairo, x4, y4);
    cairo_line_to (cairo, x4 + w2, y4 + h1);
    cairo_stroke (cairo);
    cairo_move_to (cairo, x4, y4 + h1);
    cairo_line_to (cairo, x4 + w2, y4);
    cairo_stroke (cairo);
}

static void
do_paint_grayscale_graphic (cairo_t * cairo, rl2LegendGraphicPtr aux,
			    int shift_graphic)
{
/* painting a Grayscale Raster Graphic */
    double clr_w = (double) aux->width / 3.0;
    double clr_h = (double) aux->height / 3.0;
    double x0 = 5.0;
    double x1 = x0 + clr_w;
    double x2 = x1 + clr_w;
    double y0 = shift_graphic;
    double y1 = y0 + clr_h;
    double y2 = y1 + clr_h;
    double gray = 0.0;
    double gray_incr = 1.0 / 8.0;
    cairo_rectangle (cairo, x0, y0, clr_w, clr_h);
    cairo_set_source_rgb (cairo, gray, gray, gray);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    gray += gray_incr;
    cairo_rectangle (cairo, x1, y0, clr_w, clr_h);
    cairo_set_source_rgb (cairo, gray, gray, gray);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    gray += gray_incr;
    cairo_rectangle (cairo, x2, y0, clr_w, clr_h);
    cairo_set_source_rgb (cairo, gray, gray, gray);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    gray += gray_incr;
    cairo_rectangle (cairo, x0, y1, clr_w, clr_h);
    cairo_set_source_rgb (cairo, gray, gray, gray);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    gray += gray_incr;
    cairo_rectangle (cairo, x1, y1, clr_w, clr_h);
    cairo_set_source_rgb (cairo, gray, gray, gray);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    gray += gray_incr;
    cairo_rectangle (cairo, x2, y1, clr_w, clr_h);
    cairo_set_source_rgb (cairo, gray, gray, gray);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    gray += gray_incr;
    cairo_rectangle (cairo, x0, y2, clr_w, clr_h);
    cairo_set_source_rgb (cairo, gray, gray, gray);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    gray += gray_incr;
    cairo_rectangle (cairo, x1, y2, clr_w, clr_h);
    cairo_set_source_rgb (cairo, gray, gray, gray);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    gray += gray_incr;
    cairo_rectangle (cairo, x2, y2, clr_w, clr_h);
    cairo_set_source_rgb (cairo, gray, gray, gray);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, 5, shift_graphic, aux->width, aux->height);
    cairo_set_source_rgb (cairo, 0.0, 0.0, 0.0);
    cairo_set_line_width (cairo, 0.5);
    cairo_stroke (cairo);
}

static void
do_paint_rgb_graphic (cairo_t * cairo, rl2LegendGraphicPtr aux,
		      int shift_graphic)
{
/* painting an RGB Raster Graphic */
    double clr_w = (double) aux->width / 3.0;
    double clr_h = (double) aux->height / 3.0;
    double x0 = 5.0;
    double x1 = x0 + clr_w;
    double x2 = x1 + clr_w;
    double y0 = shift_graphic;
    double y1 = y0 + clr_h;
    double y2 = y1 + clr_h;
    cairo_rectangle (cairo, x0, y0, clr_w, clr_h);
    cairo_set_source_rgb (cairo, 1.0, 0.0, 0.0);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x1, y0, clr_w, clr_h);
    cairo_set_source_rgb (cairo, 0.0, 1.0, 0.0);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x2, y0, clr_w, clr_h);
    cairo_set_source_rgb (cairo, 0.0, 0.0, 1.0);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x0, y1, clr_w, clr_h);
    cairo_set_source_rgb (cairo, 0.5, 0.75, 0.0);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x1, y1, clr_w, clr_h);
    cairo_set_source_rgb (cairo, 0.0, 0.5, 0.75);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x2, y1, clr_w, clr_h);
    cairo_set_source_rgb (cairo, 0.75, 0.0, 0.5);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x0, y2, clr_w, clr_h);
    cairo_set_source_rgb (cairo, 1.0, 1.0, 0.0);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x1, y2, clr_w, clr_h);
    cairo_set_source_rgb (cairo, 0.0, 1.0, 1.0);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, x2, y2, clr_w, clr_h);
    cairo_set_source_rgb (cairo, 1.0, 0.0, 1.0);
    cairo_fill_preserve (cairo);
    cairo_stroke (cairo);
    cairo_rectangle (cairo, 5, shift_graphic, aux->width, aux->height);
    cairo_set_source_rgb (cairo, 0.0, 0.0, 0.0);
    cairo_set_line_width (cairo, 0.5);
    cairo_stroke (cairo);
}


RL2_PRIVATE unsigned char *
rl2_paint_raster_legend_graphic (rl2LegendGraphicPtr aux)
{
/* painting a LegendGraphic for a Raster Layer */
    cairo_t *cairo;
    cairo_surface_t *surface;
    cairo_text_extents_t extents;
    int style = CAIRO_FONT_SLANT_NORMAL;
    int weight = CAIRO_FONT_WEIGHT_NORMAL;
    const char *font_name = "monospace";
    int is_font = 0;
    char *text;
    double text_width;
    double text_height;
    double legend_width;
    double legend_height;
    double red;
    double green;
    double blue;
    double next_y;
    unsigned char *rgb;
    unsigned char *p_in;
    unsigned char *p_out;
    int x;
    int y;
    int little_endian = rl2cr_endian_arch ();
    int shift_graphic;
    int shift_text;

/* creating a first initial Cairo context */
    surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 100, 100);
    if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    cairo = cairo_create (surface);
    if (cairo_status (cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* creating the Font */
    if (aux->font_italic)
	style = CAIRO_FONT_SLANT_ITALIC;
    if (aux->font_bold)
	weight = CAIRO_FONT_WEIGHT_BOLD;
    if (strcasecmp (aux->font_name, "ToyFont: serif") == 0)
	font_name = "serif";
    if (strcasecmp (aux->font_name, "ToyFont: sans-serif") == 0)
	font_name = "sans-serif";
    cairo_select_font_face (cairo, font_name, style, weight);
    cairo_set_font_size (cairo, aux->font_size);
    is_font = 1;

/* pre-measuring text extent */
    text = sqlite3_mprintf ("Layer: %s", aux->layer_name);
    cairo_text_extents (cairo, text, &extents);
    sqlite3_free (text);
    text_width = extents.width;
    text_height = extents.height;
    text = sqlite3_mprintf ("Style: %s", aux->style_name);
    cairo_text_extents (cairo, text, &extents);
    sqlite3_free (text);
    if (extents.width > text_width)
	text_width = extents.width;
    text_height += extents.height;

/* calculating the Legend Graphic size */
    legend_width = 5.0;		/* left margin */
    legend_width += aux->width;	/* Graphic width */
    legend_width += 5.0;	/* margin between Graphic and text */
    legend_width += text_width;	/*  text width */
    legend_width += 5.0;	/* right margin */
    if (aux->height > text_height)
	legend_height = aux->height;
    else
	legend_height = text_height;
    legend_height += 5.0;	/* top margin */
    legend_height += 5.0;	/* bottom margin */

/* destroying the initial Cairo context */
    cairo_destroy (cairo);
    cairo_surface_destroy (surface);

/* creating the final Cairo context */
    aux->LegendWidth = legend_width;
    aux->LegendHeight = legend_height;
    surface =
	cairo_image_surface_create (CAIRO_FORMAT_RGB24, aux->LegendWidth,
				    aux->LegendHeight);
    if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    cairo = cairo_create (surface);
    if (cairo_status (cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;
/* priming a White background */
    cairo_rectangle (cairo, 0, 0, legend_width, legend_height);
    cairo_set_source_rgb (cairo, 1.0, 1.0, 1.0);
    cairo_fill (cairo);

/* creating the Font */
    if (aux->font_italic)
	style = CAIRO_FONT_SLANT_ITALIC;
    if (aux->font_bold)
	weight = CAIRO_FONT_WEIGHT_BOLD;
    cairo_select_font_face (cairo, font_name, style, weight);
    cairo_set_font_size (cairo, aux->font_size);

/* painting the Graphic */
    shift_graphic = (legend_height - aux->height) / 2.0;
    red = (double) (aux->font_red) / 255.0;
    green = (double) (aux->font_green) / 255.0;
    blue = (double) (aux->font_blue) / 255.0;
    switch (aux->raster_type)
      {
      case RL2_PIXEL_MONOCHROME:
	  do_paint_monochrome_graphic (cairo, aux, shift_graphic);
	  break;
      case RL2_PIXEL_GRAYSCALE:
      case RL2_PIXEL_DATAGRID:
	  do_paint_grayscale_graphic (cairo, aux, shift_graphic);
	  break;
      default:
	  do_paint_rgb_graphic (cairo, aux, shift_graphic);
	  break;
      };

/* printing the Text */
    cairo_set_source_rgb (cairo, red, green, blue);
    shift_text = ((legend_height - 10.0) - text_height) / 2.0;
    text = sqlite3_mprintf ("Layer: %s", aux->layer_name);
    cairo_text_extents (cairo, text, &extents);
    next_y = extents.height + shift_text;
    cairo_move_to (cairo, 5.0 + aux->width + 5.0, next_y);
    cairo_show_text (cairo, text);
    sqlite3_free (text);
    text = sqlite3_mprintf ("Style: %s", aux->style_name);
    cairo_text_extents (cairo, text, &extents);
    next_y += 5.0 + extents.height;
    cairo_move_to (cairo, 5.0 + aux->width + 5.0, next_y);
    cairo_show_text (cairo, text);
    sqlite3_free (text);

/* preparing the RGB buffer to be returned */
    cairo_surface_flush (surface);
    rgb = malloc (aux->LegendWidth * aux->LegendHeight * 3);
    if (rgb == NULL)
	return NULL;

    p_in = cairo_image_surface_get_data (surface);
    p_out = rgb;
    for (y = 0; y < aux->LegendHeight; y++)
      {
	  for (x = 0; x < aux->LegendWidth; x++)
	    {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		if (little_endian)
		  {
		      b = *p_in++;
		      g = *p_in++;
		      r = *p_in++;
		      p_in++;	/* skipping Alpha */
		  }
		else
		  {
		      p_in++;	/* skipping Alpha */
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		  }
		*p_out++ = r;
		*p_out++ = g;
		*p_out++ = b;
	    }
      }
    cairo_font_face_destroy (cairo_get_font_face (cairo));
    cairo_destroy (cairo);
    cairo_surface_destroy (surface);
    return rgb;

  error2:
    if (is_font)
	cairo_font_face_destroy (cairo_get_font_face (cairo));
    cairo_destroy (cairo);
  error1:
    cairo_surface_destroy (surface);
    return NULL;
}

static rl2GraphicsPatternPtr
do_setup_stroke (cairo_t * cairo, rl2PrivStrokePtr stroke, sqlite3 * sqlite)
{
/* setting up a Cairo Stroke */
    rl2GraphicsPatternPtr pattern = NULL;
    RL2PrivGraphPatternPtr prv_pattern;
    double red;
    double green;
    double blue;
    double opacity;
    unsigned char norm_opacity;

    if (stroke->graphic != NULL)
      {
	  /* external Graphic stroke */
	  const char *xlink_href = NULL;
	  int recolor = 0;
	  unsigned char red;
	  unsigned char green;
	  unsigned char blue;
	  pattern = NULL;
	  if (stroke->graphic->first != NULL)
	    {
		if (stroke->graphic->first->type == RL2_EXTERNAL_GRAPHIC)
		  {
		      rl2PrivExternalGraphicPtr ext =
			  (rl2PrivExternalGraphicPtr)
			  (stroke->graphic->first->item);
		      xlink_href = ext->xlink_href;
		      if (ext->first != NULL)
			{
			    recolor = 1;
			    red = ext->first->red;
			    green = ext->first->green;
			    blue = ext->first->blue;
			}
		  }
	    }
	  if (xlink_href != NULL)
	      pattern =
		  rl2_create_pattern_from_external_graphic
		  (sqlite, xlink_href, 1);
	  if (pattern != NULL)
	    {
		if (recolor)
		  {
		      /* attempting to recolor the External Graphic resource */
		      rl2_graph_pattern_recolor (pattern, red, green, blue);
		  }
		/* transparency */
		if (stroke->opacity <= 0.0)
		    norm_opacity = 0;
		else if (stroke->opacity >= 1.0)
		    norm_opacity = 255;
		else
		  {
		      opacity = 255.0 * stroke->opacity;
		      if (opacity <= 0.0)
			  norm_opacity = 0;
		      else if (opacity >= 255.0)
			  norm_opacity = 255;
		      else
			  norm_opacity = opacity;
		  }
		if (norm_opacity < 1.0)
		    rl2_graph_pattern_transparency (pattern, norm_opacity);
		prv_pattern = (RL2PrivGraphPatternPtr) pattern;
		cairo_set_source (cairo, prv_pattern->pattern);
	    }
	  else
	    {
		/* invalid Pattern: defaulting to a Gray brush */
		cairo_set_source_rgba (cairo, 0.5, 0.5, 0.5, stroke->opacity);
	    }
      }
    else
      {
	  /* solid RGB stroke */
	  red = (double) stroke->red / 255.0;
	  green = (double) stroke->green / 255.0;
	  blue = (double) stroke->blue / 255.0;
	  cairo_set_source_rgba (cairo, red, green, blue, stroke->opacity);
      }

    /* setting Line Width and Join/Cap styles */
    cairo_set_line_width (cairo, stroke->width);
    switch (stroke->linejoin)
      {
      case RL2_STROKE_LINEJOIN_BEVEL:
	  cairo_set_line_join (cairo, CAIRO_LINE_JOIN_BEVEL);
	  break;
      case RL2_STROKE_LINEJOIN_ROUND:
	  cairo_set_line_join (cairo, CAIRO_LINE_JOIN_ROUND);
	  break;
      case RL2_STROKE_LINEJOIN_MITRE:
      default:
	  cairo_set_line_join (cairo, CAIRO_LINE_JOIN_MITER);
	  break;
      };
    switch (stroke->linecap)
      {
      case RL2_STROKE_LINECAP_SQUARE:
	  cairo_set_line_cap (cairo, CAIRO_LINE_CAP_SQUARE);
	  break;
      case RL2_STROKE_LINECAP_ROUND:
	  cairo_set_line_cap (cairo, CAIRO_LINE_CAP_ROUND);
	  break;
      case RL2_STROKE_LINECAP_BUTT:
      default:
	  cairo_set_line_cap (cairo, CAIRO_LINE_CAP_BUTT);
	  break;
      };

    /* Dash/Dotted Line */
    if (stroke->dash_count == 0 || stroke->dash_list == NULL)
	cairo_set_dash (cairo, NULL, 0, 0.0);
    else
	cairo_set_dash (cairo, stroke->dash_list,
			stroke->dash_count, stroke->dash_offset);

    return pattern;
}

static rl2GraphicsPatternPtr
do_setup_fill (cairo_t * cairo, rl2PrivFillPtr fill, sqlite3 * sqlite)
{
/* setting up a Cairo Fill */
    rl2GraphicsPatternPtr pattern = NULL;
    RL2PrivGraphPatternPtr prv_pattern;
    double red;
    double green;
    double blue;
    double opacity;
    unsigned char norm_opacity;

    if (fill->graphic != NULL)
      {
	  /* external Graphic fill */
	  const char *xlink_href = NULL;
	  int recolor = 0;
	  unsigned char red;
	  unsigned char green;
	  unsigned char blue;
	  pattern = NULL;
	  if (fill->graphic->first != NULL)
	    {
		if (fill->graphic->first->type == RL2_EXTERNAL_GRAPHIC)
		  {
		      rl2PrivExternalGraphicPtr ext =
			  (rl2PrivExternalGraphicPtr)
			  (fill->graphic->first->item);
		      xlink_href = ext->xlink_href;
		      if (ext->first != NULL)
			{
			    recolor = 1;
			    red = ext->first->red;
			    green = ext->first->green;
			    blue = ext->first->blue;
			}
		  }
	    }
	  if (xlink_href != NULL)
	      pattern =
		  rl2_create_pattern_from_external_graphic
		  (sqlite, xlink_href, 1);
	  if (pattern != NULL)
	    {
		if (recolor)
		  {
		      /* attempting to recolor the External Graphic resource */
		      rl2_graph_pattern_recolor (pattern, red, green, blue);
		  }
		/* transparency */
		if (fill->opacity <= 0.0)
		    norm_opacity = 0;
		else if (fill->opacity >= 1.0)
		    norm_opacity = 255;
		else
		  {
		      opacity = 255.0 * fill->opacity;
		      if (opacity <= 0.0)
			  norm_opacity = 0;
		      else if (opacity >= 255.0)
			  norm_opacity = 255;
		      else
			  norm_opacity = opacity;
		  }
		if (norm_opacity < 1.0)
		    rl2_graph_pattern_transparency (pattern, norm_opacity);
		prv_pattern = (RL2PrivGraphPatternPtr) pattern;
		cairo_set_source (cairo, prv_pattern->pattern);
	    }
	  else
	    {
		/* invalid Pattern: defaulting to a Gray brush */
		cairo_set_source_rgba (cairo, 0.5, 0.5, 0.5, fill->opacity);
	    }
      }
    else
      {
	  /* solid RGB fill */
	  red = (double) fill->red / 255.0;
	  green = (double) fill->green / 255.0;
	  blue = (double) fill->blue / 255.0;
	  cairo_set_source_rgba (cairo, red, green, blue, fill->opacity);
      }

    return pattern;
}

static int
do_draw_mark_symbol (cairo_t * cairo, int mark_type,
		     double size,
		     double x, double y, double angle, int fill, int stroke)
{
/* drawing a Mark Symbol */
    double size2 = size / 2.0;
    double size4 = size / 4.0;
    double size6 = size / 6.0;
    double size8 = size / 8.0;
    double size13 = size / 3.0;
    double size23 = (size / 3.0) * 2.0;
    int i;
    double rads;

    x = 20;
    y = 20;
    fprintf (stderr, "fake x=%f y=%f size=%f\n", x, y, size);
/*
    cairo_save (cairo);
    cairo_translate (cairo, x, y);
    rads = angle * .0174532925199432958;
    cairo_rotate (cairo, rads);
    */

/* setting sizes */
    if (size2 <= 0.0)
	size2 = 1.0;
    if (size4 <= 0.0)
	size4 = 1.0;
    if (size6 <= 0.0)
	size6 = 1.0;
    if (size13 <= 0.0)
	size13 = 1.0;
    if (size23 <= 0.0)
	size23 = 1.0;

/* preparing the Mark Symbol path */
    switch (mark_type)
      {
      case RL2_GRAPHIC_MARK_CIRCLE:
	  rads = 0.0;
	  for (i = 0; i < 32; i++)
	    {
		double tic = 6.28318530718 / 32.0;
		double cx = x + (size23 * sin (rads));
		double cy = y + (size23 * cos (rads));
		if (i == 0)
		    cairo_move_to (cairo, cx, cy);
		else
		    cairo_line_to (cairo, cx, cy);
		fprintf (stderr, "CIRCLE %d %f %f\n", i, cx, cy);
		rads += tic;
	    }
	  cairo_close_path (cairo);
	  break;
      case RL2_GRAPHIC_MARK_TRIANGLE:
	  rads = 0.0;
	  for (i = 0; i < 3; i++)
	    {
		double tic = 6.28318530718 / 3.0;
		double cx = x + (size23 * sin (rads));
		double cy = y + (size23 * cos (rads));
		if (i == 0)
		    cairo_move_to (cairo, cx, cy);
		else
		    cairo_line_to (cairo, cx, cy);
		rads += tic;
	    }
	  cairo_close_path (cairo);
	  break;
      case RL2_GRAPHIC_MARK_STAR:
	  rads = 3.14159265359;
	  for (i = 0; i < 10; i++)
	    {
		double tic = (i % 2) ? size4 : size23;
		double cx = x + (tic * sin (rads));
		double cy = y + (tic * cos (rads));
		if (i == 0)
		    cairo_move_to (cairo, cx, cy);
		else
		    cairo_line_to (cairo, cx, cy);
		rads += 0.628318530718;
	    }
	  cairo_close_path (cairo);
	  break;
      case RL2_GRAPHIC_MARK_CROSS:
	  cairo_move_to (cairo, x - size8, y - size2);
	  cairo_line_to (cairo, x + size8, y - size2);
	  cairo_line_to (cairo, x + size8, y - size8);
	  cairo_line_to (cairo, x + size2, y - size8);
	  cairo_line_to (cairo, x + size2, y + size8);
	  cairo_line_to (cairo, x + size8, y + size8);
	  cairo_line_to (cairo, x + size8, y + size2);
	  cairo_line_to (cairo, x - size8, y + size2);
	  cairo_line_to (cairo, x - size8, y + size8);
	  cairo_line_to (cairo, x - size2, y + size8);
	  cairo_line_to (cairo, x - size2, y - size8);
	  cairo_line_to (cairo, x - size8, y - size8);
	  cairo_close_path (cairo);
	  break;
      case RL2_GRAPHIC_MARK_X:
	  cairo_move_to (cairo, x, y - size6);
	  cairo_line_to (cairo, x - size4, y - size2);
	  cairo_line_to (cairo, x - size2, y - size2);
	  cairo_line_to (cairo, x - size8, y);
	  cairo_line_to (cairo, x - size2, y + size2);
	  cairo_line_to (cairo, x - size4, y + size2);
	  cairo_line_to (cairo, x, y + size6);
	  cairo_line_to (cairo, x + size4, y + size2);
	  cairo_line_to (cairo, x + size2, y + size2);
	  cairo_line_to (cairo, x + size8, y);
	  cairo_line_to (cairo, x + size2, y - size2);
	  cairo_line_to (cairo, x + size4, y - size2);
	  cairo_close_path (cairo);
	  break;
      case RL2_GRAPHIC_MARK_SQUARE:
      default:
	  /*
	     x = 0;
	     y = 0;
	   */
	  cairo_move_to (cairo, x - size2, y - size2);
	  fprintf (stderr, "SQUARE 1 %f %f\n", x - size2, y - size2);
	  cairo_line_to (cairo, x - size2, y + size2);
	  fprintf (stderr, "SQUARE 2 %f %f\n", x - size2, y + size2);
	  cairo_line_to (cairo, x + size2, y + size2);
	  fprintf (stderr, "SQUARE 2 %f %f\n", x + size2, y + size2);
	  cairo_line_to (cairo, x + size2, y - size2);
	  fprintf (stderr, "SQUARE 4 %f %f\n", x + size2, y - size2);
	  cairo_close_path (cairo);
	  break;
      };
    fprintf (stderr, "fill=%d stroke=%d\n", fill, stroke);

/* filling and stroking the path */
    if (fill && !stroke)
	cairo_fill (cairo);
    else if (stroke && !fill)
	cairo_stroke (cairo);
    else
      {
	  cairo_fill_preserve (cairo);
	  cairo_stroke (cairo);
      }
    //cairo_restore (cairo);

    return 1;
}

static void
do_paint_point_graphic (cairo_t * cairo, sqlite3 * sqlite,
			rl2LegendGraphicPtr aux, int shift_graphic,
			rl2PrivPointSymbolizerPtr point)
{
/* painting a POINT Vector Graphic */
    rl2PrivGraphicPtr gr;
    rl2PrivGraphicItemPtr graphic;
    double x;
    double y;

    if (point == NULL)
	return;

/* computing the center point of Graphic */
    x = 5.0 + ((double) aux->width / 2.0);
    y = (double) shift_graphic + ((double) aux->height / 2.0);

    gr = point->graphic;
    if (gr == NULL)
	return;
    graphic = point->graphic->first;
    while (graphic != NULL)
      {
	  /* looping on Graphic definitions */
	  int is_mark = 0;
	  int is_external = 0;
	  unsigned char well_known_type;
	  int fill = 0;
	  int stroke = 0;
	  int pen_cap;
	  int pen_join;
	  double opacity;
	  unsigned char norm_opacity;
	  rl2GraphicsPatternPtr pattern = NULL;
	  RL2PrivGraphPatternPtr prv_pattern;
	  rl2GraphicsPatternPtr pattern_fill = NULL;
	  rl2GraphicsPatternPtr pattern_stroke = NULL;

	  if (graphic->type == RL2_MARK_GRAPHIC)
	    {
		rl2PrivMarkPtr mark = (rl2PrivMarkPtr) (graphic->item);
		if (mark != NULL)
		  {
		      well_known_type = mark->well_known_type;
		      is_mark = 1;
		      if (mark->fill != NULL)
			{
			    if (mark->fill->graphic != NULL)
			      {
				  /* external Graphic fill */
				  const char *xlink_href = NULL;
				  int recolor = 0;
				  unsigned char red;
				  unsigned char green;
				  unsigned char blue;
				  pattern_fill = NULL;
				  if (mark->fill->graphic->first != NULL)
				    {
					if (mark->fill->graphic->first->type ==
					    RL2_EXTERNAL_GRAPHIC)
					  {
					      rl2PrivExternalGraphicPtr
						  ext =
						  (rl2PrivExternalGraphicPtr)
						  (mark->fill->graphic->
						   first->item);
					      xlink_href = ext->xlink_href;
					      if (ext->first != NULL)
						{
						    recolor = 1;
						    red = ext->first->red;
						    green = ext->first->green;
						    blue = ext->first->blue;
						}
					  }
				    }
				  if (xlink_href != NULL)
				      pattern_fill =
					  rl2_create_pattern_from_external_graphic
					  (sqlite, xlink_href, 1);
				  if (pattern_fill != NULL)
				    {
					if (recolor)
					  {
					      /* attempting to recolor the External Graphic resource */
					      rl2_graph_pattern_recolor
						  (pattern_fill,
						   red, green, blue);
					  }
					if (mark->fill->opacity <= 0.0)
					    norm_opacity = 0;
					else if (mark->fill->opacity >= 1.0)
					    norm_opacity = 255;
					else
					  {
					      opacity =
						  255.0 * mark->fill->opacity;
					      if (opacity <= 0.0)
						  norm_opacity = 0;
					      else if (opacity >= 255.0)
						  norm_opacity = 255;
					      else
						  norm_opacity = opacity;
					  }
					if (norm_opacity < 1.0)
					    rl2_graph_pattern_transparency
						(pattern_fill, norm_opacity);
					prv_pattern =
					    (RL2PrivGraphPatternPtr)
					    pattern_fill;
					cairo_set_source (cairo,
							  prv_pattern->pattern);
				    }
				  else
				    {
					/* invalid Pattern: defaulting to a Gray brush */
					cairo_set_source_rgba (cairo, 0.5, 0.5,
							       0.5,
							       mark->fill->
							       opacity);
				    }
				  fill = 1;
			      }
			    else
			      {
				  /* solid RGB fill */
				  double red = (double) mark->fill->red / 255.0;
				  double green =
				      (double) mark->fill->green / 255.0;
				  double blue =
				      (double) mark->fill->blue / 255.0;
				  cairo_set_source_rgba (cairo, red, green,
							 blue,
							 mark->fill->opacity);
				  fill = 1;
			      }
			}
		      if (mark->stroke != NULL)
			{
			    if (mark->stroke->graphic != NULL)
			      {
				  const char *xlink_href = NULL;
				  int recolor = 0;
				  unsigned char red;
				  unsigned char green;
				  unsigned char blue;
				  pattern_stroke = NULL;
				  if (mark->stroke->graphic->first != NULL)
				    {
					if (mark->stroke->graphic->first->
					    type == RL2_EXTERNAL_GRAPHIC)
					  {
					      rl2PrivExternalGraphicPtr
						  ext =
						  (rl2PrivExternalGraphicPtr)
						  (mark->stroke->graphic->
						   first->item);
					      xlink_href = ext->xlink_href;
					      if (ext->first != NULL)
						{
						    recolor = 1;
						    red = ext->first->red;
						    green = ext->first->green;
						    blue = ext->first->blue;
						}
					  }
				    }
				  if (xlink_href != NULL)
				      pattern_stroke =
					  rl2_create_pattern_from_external_graphic
					  (sqlite, xlink_href, 1);
				  if (pattern != NULL)
				    {
					if (recolor)
					  {
					      /* attempting to recolor the External Graphic resource */
					      rl2_graph_pattern_recolor
						  (pattern_stroke,
						   red, green, blue);
					  }
					if (mark->stroke->opacity <= 0.0)
					    norm_opacity = 0;
					else if (mark->stroke->opacity >= 1.0)
					    norm_opacity = 255;
					else
					  {
					      opacity =
						  255.0 * mark->stroke->opacity;
					      if (opacity <= 0.0)
						  norm_opacity = 0;
					      else if (opacity >= 255.0)
						  norm_opacity = 255;
					      else
						  norm_opacity = opacity;
					  }
					if (norm_opacity < 1.0)
					    rl2_graph_pattern_transparency
						(pattern_stroke, norm_opacity);
					if (pattern_stroke != NULL)
					  {
					      switch (mark->stroke->linecap)
						{
						case RL2_STROKE_LINECAP_ROUND:
						    pen_cap = RL2_PEN_CAP_ROUND;
						    break;
						case RL2_STROKE_LINECAP_SQUARE:
						    pen_cap =
							RL2_PEN_CAP_SQUARE;
						    break;
						default:
						    pen_cap = RL2_PEN_CAP_BUTT;
						    break;
						};
					      switch (mark->stroke->linejoin)
						{
						case RL2_STROKE_LINEJOIN_BEVEL:
						    pen_join =
							RL2_PEN_JOIN_BEVEL;
						    break;
						case RL2_STROKE_LINEJOIN_ROUND:
						    pen_join =
							RL2_PEN_JOIN_ROUND;
						    break;
						default:
						    pen_join =
							RL2_PEN_JOIN_MITER;
						    break;
						};
					      prv_pattern =
						  (RL2PrivGraphPatternPtr)
						  pattern_stroke;
					      cairo_set_source (cairo,
								prv_pattern->
								pattern);
					      cairo_set_line_width (cairo,
								    mark->
								    stroke->
								    width);
					      cairo_set_line_join (cairo,
								   pen_join);
					      cairo_set_line_cap (cairo,
								  pen_cap);
					      if (mark->stroke->dash_count == 0
						  || mark->stroke->dash_list ==
						  NULL)
						  cairo_set_dash (cairo, NULL,
								  0, 0.0);
					      else
						  cairo_set_dash (cairo,
								  mark->stroke->
								  dash_list,
								  mark->stroke->
								  dash_count,
								  mark->stroke->
								  dash_offset);
					      stroke = 1;
					  }
				    }
			      }
			    else
			      {
				  /* solid RGB stroke */
				  double red;
				  double green;
				  double blue;
				  switch (mark->stroke->linecap)
				    {
				    case RL2_STROKE_LINECAP_ROUND:
					pen_cap = RL2_PEN_CAP_ROUND;
					break;
				    case RL2_STROKE_LINECAP_SQUARE:
					pen_cap = RL2_PEN_CAP_SQUARE;
					break;
				    default:
					pen_cap = RL2_PEN_CAP_BUTT;
					break;
				    };
				  switch (mark->stroke->linejoin)
				    {
				    case RL2_STROKE_LINEJOIN_BEVEL:
					pen_join = RL2_PEN_JOIN_BEVEL;
					break;
				    case RL2_STROKE_LINEJOIN_ROUND:
					pen_join = RL2_PEN_JOIN_ROUND;
					break;
				    default:
					pen_join = RL2_PEN_JOIN_MITER;
					break;
				    };
				  red = 255.0 / (double) mark->stroke->red;
				  green = 255.0 / (double) mark->stroke->green;
				  blue = 255.0 / (double) mark->stroke->blue;
				  cairo_set_source_rgba (cairo, red, green,
							 blue,
							 mark->stroke->opacity);
				  cairo_set_line_width (cairo,
							mark->stroke->width);
				  cairo_set_line_join (cairo, pen_join);
				  cairo_set_line_cap (cairo, pen_cap);
				  if (mark->stroke->dash_count == 0
				      || mark->stroke->dash_list == NULL)
				      cairo_set_dash (cairo, NULL, 0, 0.0);
				  else
				      cairo_set_dash (cairo,
						      mark->stroke->dash_list,
						      mark->stroke->dash_count,
						      mark->stroke->
						      dash_offset);
				  stroke = 1;
			      }
			}
		  }
	    }
	  if (graphic->type == RL2_EXTERNAL_GRAPHIC)
	    {
		rl2PrivExternalGraphicPtr ext =
		    (rl2PrivExternalGraphicPtr) (graphic->item);
		const char *xlink_href = NULL;
		int recolor = 0;
		unsigned char red;
		unsigned char green;
		unsigned char blue;
		pattern = NULL;
		if (ext != NULL)
		  {
		      is_external = 1;
		      if (ext->xlink_href != NULL)
			  xlink_href = ext->xlink_href;
		      if (ext->first != NULL)
			{
			    recolor = 1;
			    red = ext->first->red;
			    green = ext->first->green;
			    blue = ext->first->blue;
			}
		      if (xlink_href != NULL)
			{
			    /* first attempt: Bitmap */
			    pattern =
				rl2_create_pattern_from_external_graphic
				(sqlite, xlink_href, 0);
			    if (pattern == NULL)
			      {
				  /* second attempt: SVG */
				  pattern =
				      rl2_create_pattern_from_external_svg
				      (sqlite, xlink_href,
				       point->graphic->size);
			      }
			}
		  }
		if (pattern != NULL)
		  {
		      if (recolor)
			{
			    /* attempting to recolor the External Graphic resource */
			    rl2_graph_pattern_recolor (pattern,
						       red, green, blue);
			}
		      if (point->graphic->opacity <= 0.0)
			  norm_opacity = 0;
		      else if (point->graphic->opacity >= 1.0)
			  norm_opacity = 255;
		      else
			{
			    opacity = 255.0 * point->graphic->opacity;
			    if (opacity <= 0.0)
				norm_opacity = 0;
			    else if (opacity >= 255.0)
				norm_opacity = 255;
			    else
				norm_opacity = opacity;
			}
		      if (norm_opacity < 1.0)
			  rl2_graph_pattern_transparency
			      (pattern, norm_opacity);
		      prv_pattern = (RL2PrivGraphPatternPtr) pattern;
		      cairo_set_source (cairo, prv_pattern->pattern);
		  }
		else
		  {
		      /* invalid Pattern: defaulting to a Gray brush */
		      cairo_set_source_rgba (cairo, 0.5, 0.5, 0.5,
					     point->graphic->opacity);
		  }
	    }


	  /* actual Point rendering */
	  if (is_mark)
	    {
		/* drawing a well-known Mark */
		do_draw_mark_symbol (cairo,
				     well_known_type,
				     gr->size, x,
				     y, point->graphic->rotation, fill, stroke);
	    }
	  if (is_external && pattern != NULL)
	    {
		/* drawing an External Graphic pattern 
		   unsigned int width;
		   unsigned int height;
		   rl2_graph_get_pattern_size (pattern,
		   &width,
		   &height);
		   double out_width = width;
		   double out_height = height;
		   rl2_graph_draw_graphic_symbol (ctx,
		   pattern,
		   out_width,
		   out_height,
		   x +
		   point_sym->
		   graphic->
		   displacement_x,
		   y -
		   point_sym->
		   graphic->
		   displacement_y,
		   point_sym->
		   graphic->
		   rotation,
		   point_sym->
		   graphic->
		   anchor_point_x,
		   point_sym->
		   graphic->
		   anchor_point_y);
		 */
	    }

	  /* releasing Patterns 
	     if (pattern != NULL)
	     rl2_graph_destroy_pattern (pattern);
	     if (pattern_fill != NULL)
	     {
	     rl2_graph_release_pattern_pen (ctx);
	     rl2_graph_destroy_pattern (pattern_fill);
	     pattern_fill = NULL;
	     }
	     if (pattern_stroke != NULL)
	     {
	     rl2_graph_release_pattern_pen (ctx);
	     rl2_graph_destroy_pattern (pattern_stroke);
	     pattern_stroke = NULL;
	     }
	   */
	  graphic = graphic->next;
      }
}

static void
do_paint_line_graphic (cairo_t * cairo, sqlite3 * sqlite,
		       rl2LegendGraphicPtr aux, int shift_graphic,
		       rl2PrivMultiLineStylePtr multi)
{
/* painting a LINESTRING Vector Graphic */
    rl2PrivLineStyleRefPtr style;
    rl2PrivLineSymbolizerPtr line;
    rl2PrivStrokePtr stroke;
    rl2GraphicsPatternPtr pattern = NULL;
    double h1;
    double h2;
    double h3;
    double h4;
    double w1;
    double w2;
    double w3;
    double w4;

    if (multi == NULL)
	return;

/* computing the reference points */
    h1 = shift_graphic + ((aux->height / 10.0) * 7.0);
    h2 = shift_graphic + ((aux->height / 10.0) * 2.0);
    h3 = shift_graphic + ((aux->height / 10.0) * 8.0);
    h4 = shift_graphic + ((aux->height / 10.0) * 3.0);
    w1 = 5.0 + ((aux->width / 10.0) * 1.0);
    w2 = 5.0 + ((aux->width / 10.0) * 6.0);
    w3 = 5.0 + ((aux->width / 10.0) * 4.0);
    w4 = 5.0 + ((aux->width / 10.0) * 9.0);

/* painting a white box */
    cairo_rectangle (cairo, 5.0, shift_graphic, aux->width, aux->height);
    cairo_set_source_rgb (cairo, 1.0, 1.0, 1.0);
    cairo_fill_preserve (cairo);
    cairo_set_source_rgb (cairo, 0.0, 0.0, 0.0);
    cairo_set_line_width (cairo, 0.5);
    cairo_stroke (cairo);

    style = multi->first;
    while (style != NULL)
      {
	  /* painting one Line Symbolizer at each time */
	  line = style->style;
	  if (line == NULL)
	      goto skip;
	  stroke = line->stroke;
	  if (stroke == NULL)
	      goto skip;

	  /* creating the Path */
	  cairo_move_to (cairo, w1, h1);
	  cairo_line_to (cairo, w2, h2);
	  cairo_line_to (cairo, w3, h3);
	  cairo_line_to (cairo, w4, h4);

	  pattern = do_setup_stroke (cairo, stroke, sqlite);

	  /* stroking the Line */
	  cairo_stroke (cairo);
	skip:
	  if (pattern != NULL)
	      rl2_graph_destroy_pattern (pattern);
	  pattern = NULL;

	  style = style->next;
      }
}

static void
do_paint_polygon_graphic (cairo_t * cairo, sqlite3 * sqlite,
			  rl2LegendGraphicPtr aux, int shift_graphic,
			  rl2PrivMultiPolygStylePtr multi)
{
/* painting a POLYGON Vector Graphic */
    rl2PrivPolygStyleRefPtr style;
    rl2PrivPolygonSymbolizerPtr polyg;
    rl2GraphicsPatternPtr pattern_fill = NULL;
    rl2GraphicsPatternPtr pattern_stroke = NULL;
    rl2PrivStrokePtr stroke;
    rl2PrivFillPtr fill;
    double h1;
    double h2;
    double h3;
    double h4;
    double w1;
    double w2;
    double w3;
    double w4;

    if (multi == NULL)
	return;

/* computing the reference points */
    w1 = 5.0 + ((aux->width / 10.0) * 1.0);
    h1 = shift_graphic + ((aux->height / 10.0) * 8.0);
    w2 = 5.0 + ((aux->width / 10.0) * 2.0);
    h2 = shift_graphic + ((aux->height / 10.0) * 1.0);
    w3 = 5.0 + ((aux->width / 10.0) * 9.0);
    h3 = shift_graphic + ((aux->height / 10.0) * 2.0);
    w4 = 5.0 + ((aux->width / 10.0) * 8.0);
    h4 = shift_graphic + ((aux->height / 10.0) * 9.0);

/* painting a white box */
    cairo_rectangle (cairo, 5.0, shift_graphic, aux->width, aux->height);
    cairo_set_source_rgb (cairo, 1.0, 1.0, 1.0);
    cairo_fill_preserve (cairo);
    cairo_set_source_rgb (cairo, 0.0, 0.0, 0.0);
    cairo_set_line_width (cairo, 0.5);
    cairo_stroke (cairo);

    style = multi->first;
    while (style != NULL)
      {
	  /* painting one Polygon Symbolizer at each time */
	  polyg = style->style;
	  if (polyg == NULL)
	      goto skip;
	  fill = polyg->fill;
	  stroke = polyg->stroke;
	  if (stroke == NULL && fill == NULL)
	      goto skip;

	  /* creating the Path */
	  cairo_move_to (cairo, w1, h1);
	  cairo_line_to (cairo, w2, h2);
	  cairo_line_to (cairo, w3, h3);
	  cairo_line_to (cairo, w4, h4);
	  cairo_close_path (cairo);

	  if (fill != NULL)
	    {
		pattern_fill = do_setup_fill (cairo, fill, sqlite);
		if (stroke != NULL)
		    cairo_fill_preserve (cairo);
		else
		    cairo_fill (cairo);
	    }

	  if (stroke != NULL)
	    {
		pattern_stroke = do_setup_stroke (cairo, stroke, sqlite);
		cairo_stroke (cairo);
	    }
	skip:
	  if (pattern_fill != NULL)
	      rl2_graph_destroy_pattern (pattern_fill);
	  if (pattern_stroke != NULL)
	      rl2_graph_destroy_pattern (pattern_stroke);
	  style = style->next;
      }
}

static void
add_multi_line_style (rl2PrivMultiLineStylePtr * multi,
		      rl2PrivLineSymbolizerPtr line)
{
/* updating a MultiLineStyle */
    rl2PrivMultiLineStylePtr ptr;
    rl2PrivLineStyleRefPtr ref;
    if (*multi == NULL)
      {
	  /* first time used: creating the container */
	  ptr = malloc (sizeof (rl2PrivMultiLineStyle));
	  ptr->first = NULL;
	  ptr->last = NULL;
      }
    else
	ptr = *multi;
/* creating the reference to the Line Style */
    ref = malloc (sizeof (rl2PrivLineStyleRef));
    ref->style = line;
    ref->next = NULL;
/* adding the reference to the MultiLineStyle */
    if (ptr->first == NULL)
	ptr->first = ref;
    if (ptr->last != NULL)
	ptr->last->next = ref;
    ptr->last = ref;
    *multi = ptr;
}

static void
destroy_multi_line_style (rl2PrivMultiLineStylePtr multi)
{
/* memory cleanup - destroying a MultiLineStyle */
    rl2PrivLineStyleRefPtr style;
    rl2PrivLineStyleRefPtr style_n;
    if (multi == NULL)
	return;
    style = multi->first;
    while (style != NULL)
      {
	  style_n = style->next;
	  free (style);
	  style = style_n;
      }
    free (multi);
}

static void
add_multi_polyg_style (rl2PrivMultiPolygStylePtr * multi,
		       rl2PrivPolygonSymbolizerPtr polyg)
{
/* updating a MultiPolygStyle */
    rl2PrivMultiPolygStylePtr ptr;
    rl2PrivPolygStyleRefPtr ref;
    if (*multi == NULL)
      {
	  /* first time used: creating the container */
	  ptr = malloc (sizeof (rl2PrivMultiPolygStyle));
	  ptr->first = NULL;
	  ptr->last = NULL;
      }
    else
	ptr = *multi;
/* creating the reference to the Polygon Style */
    ref = malloc (sizeof (rl2PrivPolygStyleRef));
    ref->style = polyg;
    ref->next = NULL;
/* adding the reference to the MultiPolygStyle */
    if (ptr->first == NULL)
	ptr->first = ref;
    if (ptr->last != NULL)
	ptr->last->next = ref;
    ptr->last = ref;
    *multi = ptr;
}

static void
destroy_multi_polyg_style (rl2PrivMultiPolygStylePtr multi)
{
/* memory cleanup - destroying a MultiPolygStyle */
    rl2PrivPolygStyleRefPtr style;
    rl2PrivPolygStyleRefPtr style_n;
    if (multi == NULL)
	return;
    style = multi->first;
    while (style != NULL)
      {
	  style_n = style->next;
	  free (style);
	  style = style_n;
      }
    free (multi);
}

RL2_PRIVATE unsigned char *
rl2_paint_vector_legend_graphic (sqlite3 * sqlite, rl2LegendGraphicPtr aux)
{
/* painting a LegendGraphic for a Vector Layer */
    cairo_t *cairo;
    cairo_surface_t *surface;
    cairo_text_extents_t extents;
    int style = CAIRO_FONT_SLANT_NORMAL;
    int weight = CAIRO_FONT_WEIGHT_NORMAL;
    const char *font_name = "monospace";
    int is_font = 0;
    char *text;
    double text_width;
    double text_height;
    double legend_width;
    double legend_height;
    double red;
    double green;
    double blue;
    double next_y;
    unsigned char *rgb;
    unsigned char *p_in;
    unsigned char *p_out;
    int x;
    int y;
    int little_endian = rl2cr_endian_arch ();
    int shift_graphic;
    int shift_text;
    rl2PrivPointSymbolizerPtr point = NULL;
    rl2PrivMultiLineStylePtr line = NULL;
    rl2PrivMultiPolygStylePtr polyg = NULL;

/* creating a first initial Cairo context */
    surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 100, 100);
    if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    cairo = cairo_create (surface);
    if (cairo_status (cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* creating the Font */
    if (aux->font_italic)
	style = CAIRO_FONT_SLANT_ITALIC;
    if (aux->font_bold)
	weight = CAIRO_FONT_WEIGHT_BOLD;
    if (strcasecmp (aux->font_name, "ToyFont: serif") == 0)
	font_name = "serif";
    if (strcasecmp (aux->font_name, "ToyFont: sans-serif") == 0)
	font_name = "sans-serif";
    cairo_select_font_face (cairo, font_name, style, weight);
    cairo_set_font_size (cairo, aux->font_size);

/* pre-measuring text extent */
    text = sqlite3_mprintf ("Layer: %s", aux->layer_name);
    cairo_text_extents (cairo, text, &extents);
    text_width = extents.width;
    text_height = extents.height;
    sqlite3_free (text);
    text = sqlite3_mprintf ("Style: %s", aux->style_name);
    cairo_text_extents (cairo, text, &extents);
    if (extents.width > text_width)
	text_width = extents.width;
    text_height += extents.height;
    sqlite3_free (text);

/* calculating the Legend Graphic size */
    legend_width = 5.0;		/* left margin */
    legend_width += aux->width;	/* Graphic width */
    legend_width += 5.0;	/* margin between Graphic and text */
    legend_width += text_width;	/*  text width */
    legend_width += 5.0;	/* right margin */
    if (aux->height > text_height)
	legend_height = aux->height;
    else
	legend_height = text_height;
    legend_height += 5.0;	/* top margin */
    legend_height += 5.0;	/* bottom margin */

/* destroying the initial Cairo context */
    cairo_destroy (cairo);
    cairo_surface_destroy (surface);

/* creating the final Cairo context */
    aux->LegendWidth = legend_width;
    aux->LegendHeight = legend_height;
    surface =
	cairo_image_surface_create (CAIRO_FORMAT_RGB24, aux->LegendWidth,
				    aux->LegendHeight);
    if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    cairo = cairo_create (surface);
    if (cairo_status (cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;
/* priming a White background */
    cairo_rectangle (cairo, 0, 0, legend_width, legend_height);
    cairo_set_source_rgb (cairo, 1.0, 1.0, 1.0);
    cairo_fill (cairo);

/* selecting the Font */
    if (aux->font_italic)
	style = CAIRO_FONT_SLANT_ITALIC;
    if (aux->font_bold)
	weight = CAIRO_FONT_WEIGHT_BOLD;
    cairo_select_font_face (cairo, font_name, style, weight);
    cairo_set_font_size (cairo, aux->font_size);
    is_font = 1;

/* clipping the Graphic rectangle */
    cairo_save (cairo);
    shift_graphic = (legend_height - aux->height) / 2.0;
    cairo_move_to (cairo, 5, shift_graphic);
    cairo_line_to (cairo, 5 + aux->width, shift_graphic);
    cairo_line_to (cairo, 5 + aux->width, shift_graphic + aux->height);
    cairo_line_to (cairo, 5, shift_graphic + aux->height);
    cairo_close_path (cairo);
    cairo_clip (cairo);

/* painting the Graphic */
    red = (double) (aux->font_red) / 255.0;
    green = (double) (aux->font_green) / 255.0;
    blue = (double) (aux->font_blue) / 255.0;
    if (aux->lyr_stl != NULL)
      {
	  rl2PrivStyleRulePtr rule;
	  rl2PrivFeatureTypeStylePtr style =
	      (rl2PrivFeatureTypeStylePtr) aux->lyr_stl;
	  if (style->first_rule == NULL && style->else_rule != NULL)
	    {
		rl2PrivVectorSymbolizerItemPtr item;
		rl2PrivVectorSymbolizerPtr symb;
		rule = style->else_rule;
		symb = rule->style;
		item = symb->first;
		while (item != NULL)
		  {
		      if (item->symbolizer_type == RL2_POINT_SYMBOLIZER
			  && item->symbolizer != NULL)
			  point = item->symbolizer;
		      if (item->symbolizer_type == RL2_LINE_SYMBOLIZER
			  && item->symbolizer != NULL)
			  add_multi_line_style (&line, item->symbolizer);
		      if (item->symbolizer_type == RL2_POLYGON_SYMBOLIZER
			  && item->symbolizer != NULL)
			  add_multi_polyg_style (&polyg, item->symbolizer);
		      item = item->next;
		  }
	    }
	  else
	    {
		rule = style->first_rule;
		while (rule != NULL)
		  {
		      if (rule->style_type == RL2_VECTOR_STYLE
			  && rule->style != NULL)
			{
			    rl2PrivVectorSymbolizerItemPtr item;
			    rl2PrivVectorSymbolizerPtr symb = rule->style;
			    item = symb->first;
			    while (item != NULL)
			      {
				  if (item->symbolizer_type ==
				      RL2_POINT_SYMBOLIZER
				      && item->symbolizer != NULL)
				      point = item->symbolizer;
				  if (item->symbolizer_type ==
				      RL2_LINE_SYMBOLIZER
				      && item->symbolizer != NULL)
				      add_multi_line_style (&line,
							    item->symbolizer);
				  if (item->symbolizer_type ==
				      RL2_POLYGON_SYMBOLIZER
				      && item->symbolizer != NULL)
				      add_multi_polyg_style (&polyg,
							     item->symbolizer);
				  item = item->next;
			      }
			}
		      rule = rule->next;
		  }
	    }
      }
    switch (aux->vector_type)
      {
      case RL2_GEOM_POINT:
	  do_paint_point_graphic (cairo, sqlite, aux, shift_graphic, point);
	  break;
      case RL2_GEOM_LINESTRING:
	  do_paint_line_graphic (cairo, sqlite, aux, shift_graphic, line);
	  break;
      case RL2_GEOM_POLYGON:
	  do_paint_polygon_graphic (cairo, sqlite, aux, shift_graphic, polyg);
	  break;
      };

    if (line != NULL)
      {
	  destroy_multi_line_style (line);
	  line = NULL;
      }
    if (polyg != NULL)
      {
	  destroy_multi_polyg_style (polyg);
	  polyg = NULL;
      }

/* restoring Cairo */
    cairo_restore (cairo);

/* printing the Text */
    cairo_set_source_rgb (cairo, red, green, blue);
    shift_text = ((legend_height - 10.0) - text_height) / 2.0;
    text = sqlite3_mprintf ("Layer: %s", aux->layer_name);
    cairo_text_extents (cairo, text, &extents);
    next_y = extents.height + shift_text;
    cairo_move_to (cairo, 5.0 + aux->width + 5.0, next_y);
    cairo_show_text (cairo, text);
    sqlite3_free (text);
    text = sqlite3_mprintf ("Style: %s", aux->style_name);
    cairo_text_extents (cairo, text, &extents);
    next_y += 5.0 + extents.height;
    cairo_move_to (cairo, 5.0 + aux->width + 5.0, next_y);
    cairo_show_text (cairo, text);
    sqlite3_free (text);

/* preparing the RGB buffer to be returned */
    cairo_surface_flush (surface);
    rgb = malloc (aux->LegendWidth * aux->LegendHeight * 3);
    if (rgb == NULL)
	return NULL;

    p_in = cairo_image_surface_get_data (surface);
    p_out = rgb;
    for (y = 0; y < aux->LegendHeight; y++)
      {
	  for (x = 0; x < aux->LegendWidth; x++)
	    {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		if (little_endian)
		  {
		      b = *p_in++;
		      g = *p_in++;
		      r = *p_in++;
		      p_in++;	/* skipping Alpha */
		  }
		else
		  {
		      p_in++;	/* skipping Alpha */
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		  }
		*p_out++ = r;
		*p_out++ = g;
		*p_out++ = b;
	    }
      }
    cairo_font_face_destroy (cairo_get_font_face (cairo));
    cairo_destroy (cairo);
    cairo_surface_destroy (surface);
    return rgb;


  error2:
    if (is_font)
	cairo_font_face_destroy (cairo_get_font_face (cairo));
    cairo_destroy (cairo);
  error1:
    cairo_surface_destroy (surface);
    if (line != NULL)
	destroy_multi_line_style (line);
    if (polyg != NULL)
	destroy_multi_polyg_style (polyg);
    return NULL;
}
