/*

 rl2paint -- Cairo graphics functions

 version 0.1, 2013 September 29

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
 
Portions created by the Initial Developer are Copyright (C) 2013
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

#if defined(_WIN32) && !defined(__MINGW32__)
#include <windows.h>
#include <process.h>
#else
#include <unistd.h>
#include <pthread.h>
#endif

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2/rl2svg.h"
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

struct aux_utf8_text
{
/* an internal struct wrapping UTF8 text strings */
    char *buf;
    char **chars;
    int n_bytes;
    int n_chars;
    int is_reverse;
};

static struct aux_utf8_text *
create_aux_utf8_text (const char *text)
{
/* creating and initializing an internal UTF8 text string */
    struct aux_utf8_text *ptr;
    int len;
    int i;
    const char *c = text;

    if (text == 0)
	return NULL;
    len = strlen (text);
    if (len == 0)
	return NULL;

/* initializing an empty struct */
    ptr = malloc (sizeof (struct aux_utf8_text));
    ptr->buf = malloc (len * 5);
    ptr->chars = malloc (sizeof (char *) * len);
    ptr->n_bytes = len;
    ptr->n_chars = 0;
    ptr->is_reverse = 0;
    for (i = 0; i < ptr->n_bytes; i++)
      {
	  *(ptr->chars + i) = ptr->buf + (i * 5);
	  memset (ptr->buf + (i * 5), '\0', 5);
      }

    i = 0;
    while (*c != '\0')
      {
	  /* extracting UTF8 chars */
	  unsigned char byte = *c;
	  char *utf8char = *(ptr->chars + i);
	  int incr;
	  if ((byte | 0x7f) == 0x7f)
	    {
		/* found a single byte UTF8 char */
		*utf8char = *c;
		incr = 1;
	    }
	  else if ((byte & 0xc0) == 0xc0)
	    {
		/* found a two bytes UTF9 char */
		memcpy (utf8char, c, 2);
		incr = 2;
	    }
	  else if ((byte & 0xe0) == 0xe0)
	    {
		/* found a three bytes UTF8 char */
		memcpy (utf8char, c, 3);
		incr = 3;
	    }
	  else if ((byte & 0xf0) == 0xf0)
	    {
		/* found a four bytes UTF8 char */
		memcpy (utf8char, c, 4);
		incr = 4;
	    }
	  else
	    {
		/* invalid UTF8 char */
		*utf8char = '?';
		incr = 1;
	    }
	  ptr->n_chars += 1;
	  i++;
	  c += incr;
      }

    return ptr;
}

void
destroy_aux_utf8_text (struct aux_utf8_text *ptr)
{
/* memory cleanup: freeing an internal UTF8 text string */
    if (ptr == NULL)
	return;
    if (ptr->buf != NULL)
	free (ptr->buf);
    if (ptr->chars != NULL)
	free (ptr->chars);
    free (ptr);
}

static void
reverse_aux_utf8_text (struct aux_utf8_text *ptr)
{
/* inverting the order of chars for an internal UTF8 text string */
    int i;
    char *buf;
    if (ptr == NULL)
	return;

    buf = malloc (ptr->n_chars * 5);
    for (i = 0; i < ptr->n_chars; i++)
      {
	  int ir = ptr->n_chars - (i + 1);
	  char *in = *(ptr->chars + i);
	  char *out = buf + (ir * 5);
	  memcpy (out, in, 5);
      }
    for (i = 0; i < ptr->n_chars; i++)
      {
	  char *in = buf + (i * 5);
	  char *out = *(ptr->chars + i);
	  memcpy (out, in, 5);
      }
    ptr->is_reverse = 1;
    free (buf);
}

static unsigned char
unpremultiply (unsigned char c, unsigned char a)
{
/* Cairo has premultiplied alphas */
    double x = ((double) c * 255.0) / (double) a;
    if (a == 0)
	return 0;
    return (unsigned char) x;
}

RL2_DECLARE int
rl2_graph_context_get_dimensions (rl2GraphicsContextPtr handle, int *width,
				  int *height)
{
/* retrieving Width and Height from a Graphics Context */
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) handle;
    if (ctx == NULL)
	return RL2_ERROR;
    *width = cairo_image_surface_get_width (ctx->surface);
    *height = cairo_image_surface_get_height (ctx->surface);
    return RL2_OK;
}

static void
do_initialize_context (RL2GraphContextPtr ctx)
{
/* common initialization tasks */

/* setting up a default Black Pen */
    ctx->current_pen.is_solid_color = 1;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = 0.0;
    ctx->current_pen.green = 0.0;
    ctx->current_pen.blue = 0.0;
    ctx->current_pen.alpha = 1.0;
    ctx->current_pen.width = 1.0;
    ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
    ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_count = 0;
    ctx->current_pen.dash_offset = 0.0;
    ctx->current_pen.pattern = NULL;

/* setting up a default Black Brush */
    ctx->current_brush.is_solid_color = 1;
    ctx->current_brush.is_linear_gradient = 0;
    ctx->current_brush.is_pattern = 0;
    ctx->current_brush.red = 0.0;
    ctx->current_brush.green = 0.0;
    ctx->current_brush.blue = 0.0;
    ctx->current_brush.alpha = 1.0;
    ctx->current_brush.pattern = NULL;

/* setting up default Font options */
    ctx->font_red = 0.0;
    ctx->font_green = 0.0;
    ctx->font_blue = 0.0;
    ctx->font_alpha = 1.0;
    ctx->with_font_halo = 0;
    ctx->halo_radius = 0.0;
    ctx->halo_red = 1.0;
    ctx->halo_green = 1.0;
    ctx->halo_blue = 1.0;
    ctx->halo_alpha = 1.0;
}

RL2_DECLARE rl2GraphicsContextPtr
rl2_graph_create_context (const void *priv_data, int width, int height)
{
/* creating a generic Graphics Context */
    struct rl2_private_data *cache = (struct rl2_private_data *) priv_data;
    RL2GraphContextPtr ctx;

    ctx = malloc (sizeof (RL2GraphContext));
    if (!ctx)
	return NULL;

    ctx->type = RL2_SURFACE_IMG;
    ctx->clip_surface = NULL;
    ctx->clip_cairo = NULL;
    ctx->surface =
	cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
    if (cairo_surface_status (ctx->surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    ctx->cairo = cairo_create (ctx->surface);
    if (cairo_status (ctx->cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

    do_initialize_context (ctx);
    ctx->labeling = &(cache->labeling);
    if (ctx->labeling != NULL)
	do_cleanup_advanced_labeling (ctx->labeling);

/* priming a transparent background */
    cairo_rectangle (ctx->cairo, 0, 0, width, height);
    cairo_set_source_rgba (ctx->cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (ctx->cairo);
    return (rl2GraphicsContextPtr) ctx;

  error2:
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error1:
    cairo_surface_destroy (ctx->surface);
    return NULL;
}

RL2_PRIVATE int
rl2cr_endian_arch ()
{
/* checking if target CPU is a little-endian one */
    union cvt
    {
	unsigned char byte[4];
	int int_value;
    } convert;
    convert.int_value = 1;
    if (convert.byte[0] == 0)
	return 0;
    return 1;
}

static void
adjust_for_endianness (unsigned char *rgbaArray, int width, int height)
{
/* Adjusting from RGBA to ARGB respecting platform endianness */
    int x;
    int y;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
    unsigned char *p_in = rgbaArray;
    unsigned char *p_out = rgbaArray;
    int little_endian = rl2cr_endian_arch ();

    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		red = *p_in++;
		green = *p_in++;
		blue = *p_in++;
		alpha = *p_in++;
		if (little_endian)
		  {
		      *p_out++ = blue;
		      *p_out++ = green;
		      *p_out++ = red;
		      *p_out++ = alpha;
		  }
		else
		  {
		      *p_out++ = alpha;
		      *p_out++ = red;
		      *p_out++ = green;
		      *p_out++ = blue;
		  }
	    }
      }
}

RL2_DECLARE rl2GraphicsContextPtr
rl2_graph_create_context_rgba (const void *priv_data, int width, int height,
			       unsigned char *rgbaArray)
{
/* creating a generic Graphics Context */
    struct rl2_private_data *cache = (struct rl2_private_data *) priv_data;
    RL2GraphContextPtr ctx;

    if (rgbaArray == NULL)
	return NULL;

    adjust_for_endianness (rgbaArray, width, height);

    ctx = malloc (sizeof (RL2GraphContext));
    if (!ctx)
	return NULL;

    ctx->type = RL2_SURFACE_IMG;
    ctx->clip_surface = NULL;
    ctx->clip_cairo = NULL;
    ctx->surface =
	cairo_image_surface_create_for_data (rgbaArray, CAIRO_FORMAT_ARGB32,
					     width, height, width * 4);
    if (cairo_surface_status (ctx->surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    ctx->cairo = cairo_create (ctx->surface);
    if (cairo_status (ctx->cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

    do_initialize_context (ctx);
    ctx->labeling = &(cache->labeling);
    return (rl2GraphicsContextPtr) ctx;
  error2:
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error1:
    cairo_surface_destroy (ctx->surface);
    return NULL;
}

static void
destroy_context (RL2GraphContextPtr ctx)
{
/* memory cleanup - destroying a Graphics Context */
    if (ctx == NULL)
	return;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    cairo_destroy (ctx->cairo);
    cairo_surface_finish (ctx->surface);
    cairo_surface_destroy (ctx->surface);
    free (ctx);
}

static void
destroy_svg_context (RL2GraphContextPtr ctx)
{
/* freeing an SVG Graphics Context */
    if (ctx == NULL)
	return;
    cairo_surface_show_page (ctx->surface);
    cairo_destroy (ctx->cairo);
    cairo_surface_finish (ctx->surface);
    cairo_surface_destroy (ctx->surface);
    free (ctx);
}

static void
destroy_pdf_context (RL2GraphContextPtr ctx)
{
/* freeing a PDF Graphics Context */
    if (ctx == NULL)
	return;
    cairo_surface_finish (ctx->clip_surface);
    cairo_surface_destroy (ctx->clip_surface);
    cairo_destroy (ctx->clip_cairo);
    cairo_surface_show_page (ctx->surface);
    cairo_destroy (ctx->cairo);
    cairo_surface_finish (ctx->surface);
    cairo_surface_destroy (ctx->surface);
    free (ctx);
}

RL2_DECLARE void
rl2_graph_destroy_context (rl2GraphicsContextPtr context)
{
/* memory cleanup - destroying a Graphics Context */
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return;
    if (ctx->type == RL2_SURFACE_SVG)
	destroy_svg_context (ctx);
    else if (ctx->type == RL2_SURFACE_PDF)
	destroy_pdf_context (ctx);
    else
	destroy_context (ctx);
}

RL2_DECLARE rl2GraphicsContextPtr
rl2_graph_create_svg_context (const void *priv_data, const char *path,
			      int width, int height)
{
/* creating an SVG Graphics Context */
    struct rl2_private_data *cache = (struct rl2_private_data *) priv_data;
    RL2GraphContextPtr ctx;

    ctx = malloc (sizeof (RL2GraphContext));
    if (!ctx)
	return NULL;

    ctx->type = RL2_SURFACE_SVG;
    ctx->clip_surface = NULL;
    ctx->clip_cairo = NULL;
    ctx->surface =
	cairo_svg_surface_create (path, (double) width, (double) height);

    if (cairo_surface_status (ctx->surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    ctx->cairo = cairo_create (ctx->surface);
    if (cairo_status (ctx->cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* setting up a default Black Pen */
    ctx->current_pen.is_solid_color = 1;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = 0.0;
    ctx->current_pen.green = 0.0;
    ctx->current_pen.blue = 0.0;
    ctx->current_pen.alpha = 1.0;
    ctx->current_pen.width = 1.0;
    ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
    ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_count = 0;
    ctx->current_pen.dash_offset = 0.0;
    ctx->current_pen.pattern = NULL;

/* setting up a default Black Brush */
    ctx->current_brush.is_solid_color = 1;
    ctx->current_brush.is_linear_gradient = 0;
    ctx->current_brush.is_pattern = 0;
    ctx->current_brush.red = 0.0;
    ctx->current_brush.green = 0.0;
    ctx->current_brush.blue = 0.0;
    ctx->current_brush.alpha = 1.0;
    ctx->current_brush.pattern = NULL;

/* priming a transparent background */
    cairo_rectangle (ctx->cairo, 0, 0, width, height);
    cairo_set_source_rgba (ctx->cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (ctx->cairo);

/* setting up default Font options */
    ctx->font_red = 0.0;
    ctx->font_green = 0.0;
    ctx->font_blue = 0.0;
    ctx->font_alpha = 1.0;
    ctx->with_font_halo = 0;
    ctx->halo_radius = 0.0;
    ctx->halo_red = 1.0;
    ctx->halo_green = 1.0;
    ctx->halo_blue = 1.0;
    ctx->halo_alpha = 1.0;
    ctx->labeling = &(cache->labeling);
    return (rl2GraphicsContextPtr) ctx;
  error2:
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error1:
    cairo_surface_destroy (ctx->surface);
    return NULL;
}

RL2_DECLARE rl2GraphicsContextPtr
rl2_graph_create_pdf_context (const void *priv_data, const char *path, int dpi,
			      double page_width, double page_height,
			      double margin_width, double margin_height)
{
/* creating a PDF Graphics Context */
    struct rl2_private_data *cache = (struct rl2_private_data *) priv_data;
    RL2GraphContextPtr ctx;
    double page2_width = page_width * dpi;
    double page2_height = page_height * dpi;
    double horz_margin_sz = margin_width * dpi;
    double vert_margin_sz = margin_height * dpi;
    double img_width = (page_width - (margin_width * 2.0)) * dpi;
    double img_height = (page_height - (margin_height * 2.0)) * dpi;

    ctx = malloc (sizeof (RL2GraphContext));
    if (ctx == NULL)
	return NULL;

    ctx->type = RL2_SURFACE_PDF;
    ctx->clip_surface = NULL;
    ctx->clip_cairo = NULL;
    ctx->surface = cairo_pdf_surface_create (path, page2_width, page2_height);
    if (cairo_surface_status (ctx->surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    ctx->cairo = cairo_create (ctx->surface);
    if (cairo_status (ctx->cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* priming a transparent background */
    cairo_rectangle (ctx->cairo, 0, 0, page2_width, page2_height);
    cairo_set_source_rgba (ctx->cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (ctx->cairo);

/* clipped surface respecting free margins */
    ctx->clip_surface =
	cairo_surface_create_for_rectangle (ctx->surface, horz_margin_sz,
					    vert_margin_sz, img_width,
					    img_height);
    if (cairo_surface_status (ctx->clip_surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error3;
    ctx->clip_cairo = cairo_create (ctx->clip_surface);
    if (cairo_status (ctx->clip_cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error4;

/* setting up a default Black Pen */
    ctx->current_pen.is_solid_color = 1;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = 0.0;
    ctx->current_pen.green = 0.0;
    ctx->current_pen.blue = 0.0;
    ctx->current_pen.alpha = 1.0;
    ctx->current_pen.width = 1.0;
    ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
    ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_count = 0;
    ctx->current_pen.dash_offset = 0.0;
    ctx->current_pen.pattern = NULL;

/* setting up a default Black Brush */
    ctx->current_brush.is_solid_color = 1;
    ctx->current_brush.is_linear_gradient = 0;
    ctx->current_brush.is_pattern = 0;
    ctx->current_brush.red = 0.0;
    ctx->current_brush.green = 0.0;
    ctx->current_brush.blue = 0.0;
    ctx->current_brush.alpha = 1.0;
    ctx->current_brush.pattern = NULL;

/* scaling accordingly to DPI resolution */
    //cairo_scale (ctx->clip_cairo, scale, scale);

/* setting up default Font options */
    ctx->font_red = 0.0;
    ctx->font_green = 0.0;
    ctx->font_blue = 0.0;
    ctx->font_alpha = 1.0;
    ctx->with_font_halo = 0;
    ctx->halo_radius = 0.0;
    ctx->halo_red = 1.0;
    ctx->halo_green = 1.0;
    ctx->halo_blue = 1.0;
    ctx->halo_alpha = 1.0;
    ctx->labeling = &(cache->labeling);
    return (rl2GraphicsContextPtr) ctx;
  error4:
    cairo_destroy (ctx->clip_cairo);
    cairo_surface_destroy (ctx->clip_surface);
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error3:
    cairo_surface_destroy (ctx->clip_surface);
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error2:
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error1:
    cairo_surface_destroy (ctx->surface);
    return NULL;
}

static cairo_status_t
pdf_write_func (void *ptr, const unsigned char *data, unsigned int length)
{
/* writing into the in-memory PDF target */
    rl2PrivMemPdfPtr mem = (rl2PrivMemPdfPtr) ptr;
    if (mem == NULL)
	return CAIRO_STATUS_WRITE_ERROR;

    if (mem->write_offset + (int) length < mem->size)
      {
	  /* inserting into the current buffer */
	  memcpy (mem->buffer + mem->write_offset, data, length);
	  mem->write_offset += length;
      }
    else
      {
	  /* expanding the current buffer */
	  int new_sz = mem->size + length + (64 * 1024);
	  unsigned char *save = mem->buffer;
	  mem->buffer = realloc (mem->buffer, new_sz);
	  if (mem->buffer == NULL)
	    {
		free (save);
		return CAIRO_STATUS_WRITE_ERROR;
	    }
	  mem->size = new_sz;
	  memcpy (mem->buffer + mem->write_offset, data, length);
	  mem->write_offset += length;
      }
    return CAIRO_STATUS_SUCCESS;
}

RL2_DECLARE rl2GraphicsContextPtr
rl2_graph_create_mem_pdf_context (const void *priv_data, rl2MemPdfPtr mem_pdf,
				  int dpi, double page_width,
				  double page_height, double margin_width,
				  double margin_height)
{
/* creating an in-memory PDF Graphics Context */
    struct rl2_private_data *cache = (struct rl2_private_data *) priv_data;
    RL2GraphContextPtr ctx;
    double scale = 72.0 / (double) dpi;
    double page2_width = page_width * 72.0;
    double page2_height = page_height * 72.0;
    double horz_margin_sz = margin_width * 72.0;
    double vert_margin_sz = margin_height * 72.0;
    double img_width = (page_width - (margin_width * 2.0)) * 72.0;
    double img_height = (page_height - (margin_height * 2.0)) * 72.0;

    ctx = malloc (sizeof (RL2GraphContext));
    if (ctx == NULL)
	return NULL;

    ctx->type = RL2_SURFACE_PDF;
    ctx->clip_surface = NULL;
    ctx->clip_cairo = NULL;
    ctx->surface =
	cairo_pdf_surface_create_for_stream (pdf_write_func, mem_pdf,
					     page2_width, page2_height);
    if (cairo_surface_status (ctx->surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    ctx->cairo = cairo_create (ctx->surface);
    if (cairo_status (ctx->cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* priming a transparent background */
    cairo_rectangle (ctx->cairo, 0, 0, page2_width, page2_height);
    cairo_set_source_rgba (ctx->cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (ctx->cairo);

/* clipped surface respecting free margins */
    ctx->clip_surface =
	cairo_surface_create_for_rectangle (ctx->surface, horz_margin_sz,
					    vert_margin_sz, img_width,
					    img_height);
    if (cairo_surface_status (ctx->clip_surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error3;
    ctx->clip_cairo = cairo_create (ctx->clip_surface);
    if (cairo_status (ctx->clip_cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error4;

/* setting up a default Black Pen */
    ctx->current_pen.is_solid_color = 1;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = 0.0;
    ctx->current_pen.green = 0.0;
    ctx->current_pen.blue = 0.0;
    ctx->current_pen.alpha = 1.0;
    ctx->current_pen.width = 1.0;
    ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
    ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_count = 0;
    ctx->current_pen.dash_offset = 0.0;
    ctx->current_pen.pattern = NULL;

/* setting up a default Black Brush */
    ctx->current_brush.is_solid_color = 1;
    ctx->current_brush.is_linear_gradient = 0;
    ctx->current_brush.is_pattern = 0;
    ctx->current_brush.red = 0.0;
    ctx->current_brush.green = 0.0;
    ctx->current_brush.blue = 0.0;
    ctx->current_brush.alpha = 1.0;
    ctx->current_brush.pattern = NULL;

/* scaling accordingly to DPI resolution */
    cairo_scale (ctx->clip_cairo, scale, scale);

/* setting up default Font options */
    ctx->font_red = 0.0;
    ctx->font_green = 0.0;
    ctx->font_blue = 0.0;
    ctx->font_alpha = 1.0;
    ctx->with_font_halo = 0;
    ctx->halo_radius = 0.0;
    ctx->halo_red = 1.0;
    ctx->halo_green = 1.0;
    ctx->halo_blue = 1.0;
    ctx->halo_alpha = 1.0;
    ctx->labeling = &(cache->labeling);
    return (rl2GraphicsContextPtr) ctx;
  error4:
    cairo_destroy (ctx->clip_cairo);
    cairo_surface_destroy (ctx->clip_surface);
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error3:
    cairo_surface_destroy (ctx->clip_surface);
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error2:
    cairo_destroy (ctx->cairo);
    cairo_surface_destroy (ctx->surface);
    return NULL;
  error1:
    cairo_surface_destroy (ctx->surface);
    return NULL;
}

RL2_DECLARE int
rl2_graph_set_solid_pen (rl2GraphicsContextPtr context, unsigned char red,
			 unsigned char green, unsigned char blue,
			 unsigned char alpha, double width, int line_cap,
			 int line_join)
{
/* creating a Color Pen - solid style */
    double d_red = (double) red / 255.0;
    double d_green = (double) green / 255.0;
    double d_blue = (double) blue / 255.0;
    double d_alpha = (double) alpha / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;

    ctx->current_pen.width = width;
    ctx->current_pen.is_solid_color = 1;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = d_red;
    ctx->current_pen.green = d_green;
    ctx->current_pen.blue = d_blue;
    ctx->current_pen.alpha = d_alpha;
    switch (line_cap)
      {
      case RL2_PEN_CAP_ROUND:
      case RL2_PEN_CAP_SQUARE:
	  ctx->current_pen.line_cap = line_cap;
	  break;
      default:
	  ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
	  break;

      };
    switch (line_join)
      {
      case RL2_PEN_JOIN_ROUND:
      case RL2_PEN_JOIN_BEVEL:
	  ctx->current_pen.line_join = line_join;
	  break;
      default:
	  ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
	  break;

      };
    ctx->current_pen.dash_count = 0;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_offset = 0.0;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_dashed_pen (rl2GraphicsContextPtr context, unsigned char red,
			  unsigned char green, unsigned char blue,
			  unsigned char alpha, double width, int line_cap,
			  int line_join, int dash_count, double dash_list[],
			  double dash_offset)
{
/* creating a Color Pen - dashed style */
    int d;
    double d_red = (double) red / 255.0;
    double d_green = (double) green / 255.0;
    double d_blue = (double) blue / 255.0;
    double d_alpha = (double) alpha / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (dash_count <= 0 || dash_list == NULL)
	return 0;

    ctx->current_pen.width = width;
    ctx->current_pen.is_solid_color = 1;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = d_red;
    ctx->current_pen.green = d_green;
    ctx->current_pen.blue = d_blue;
    ctx->current_pen.alpha = d_alpha;
    switch (line_cap)
      {
      case RL2_PEN_CAP_ROUND:
      case RL2_PEN_CAP_SQUARE:
	  ctx->current_pen.line_cap = line_cap;
	  break;
      default:
	  ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
	  break;

      };
    switch (line_join)
      {
      case RL2_PEN_JOIN_ROUND:
      case RL2_PEN_JOIN_BEVEL:
	  ctx->current_pen.line_join = line_join;
	  break;
      default:
	  ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
	  break;

      };
    ctx->current_pen.dash_count = dash_count;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    ctx->current_pen.dash_array = malloc (sizeof (double) * dash_count);
    for (d = 0; d < dash_count; d++)
	*(ctx->current_pen.dash_array + d) = *(dash_list + d);
    ctx->current_pen.dash_offset = dash_offset;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_linear_gradient_solid_pen (rl2GraphicsContextPtr context,
					 double x, double y, double width,
					 double height, unsigned char red1,
					 unsigned char green1,
					 unsigned char blue1,
					 unsigned char alpha1,
					 unsigned char red2,
					 unsigned char green2,
					 unsigned char blue2,
					 unsigned char alpha2,
					 double pen_width, int line_cap,
					 int line_join)
{
/* setting up a Linear Gradient Pen - solid style */
    double d_red = (double) red1 / 255.0;
    double d_green = (double) green1 / 255.0;
    double d_blue = (double) blue1 / 255.0;
    double d_alpha = (double) alpha1 / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;

    ctx->current_pen.width = pen_width;
    switch (line_cap)
      {
      case RL2_PEN_CAP_ROUND:
      case RL2_PEN_CAP_SQUARE:
	  ctx->current_pen.line_cap = line_cap;
	  break;
      default:
	  ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
	  break;

      };
    switch (line_join)
      {
      case RL2_PEN_JOIN_ROUND:
      case RL2_PEN_JOIN_BEVEL:
	  ctx->current_pen.line_join = line_join;
	  break;
      default:
	  ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
	  break;

      };
    ctx->current_pen.is_solid_color = 0;
    ctx->current_pen.is_linear_gradient = 1;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = d_red;
    ctx->current_pen.green = d_green;
    ctx->current_pen.blue = d_blue;
    ctx->current_pen.alpha = d_alpha;
    ctx->current_pen.x0 = x;
    ctx->current_pen.y0 = y;
    ctx->current_pen.x1 = x + width;
    ctx->current_pen.y1 = y + height;
    d_red = (double) red2 / 255.0;
    d_green = (double) green2 / 255.0;
    d_blue = (double) blue2 / 255.0;
    d_alpha = (double) alpha2 / 255.0;
    ctx->current_pen.red2 = d_red;
    ctx->current_pen.green2 = d_green;
    ctx->current_pen.blue2 = d_blue;
    ctx->current_pen.alpha2 = d_alpha;
    ctx->current_pen.dash_count = 0;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_offset = 0.0;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_linear_gradient_dashed_pen (rl2GraphicsContextPtr context,
					  double x, double y, double width,
					  double height, unsigned char red1,
					  unsigned char green1,
					  unsigned char blue1,
					  unsigned char alpha1,
					  unsigned char red2,
					  unsigned char green2,
					  unsigned char blue2,
					  unsigned char alpha2,
					  double pen_width, int line_cap,
					  int line_join, int dash_count,
					  double dash_list[],
					  double dash_offset)
{
/* setting up a Linear Gradient Pen - dashed style */
    int d;
    double d_red = (double) red1 / 255.0;
    double d_green = (double) green1 / 255.0;
    double d_blue = (double) blue1 / 255.0;
    double d_alpha = (double) alpha1 / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (dash_count <= 0 || dash_list == NULL)
	return 0;

    ctx->current_pen.width = pen_width;
    switch (line_cap)
      {
      case RL2_PEN_CAP_ROUND:
      case RL2_PEN_CAP_SQUARE:
	  ctx->current_pen.line_cap = line_cap;
	  break;
      default:
	  ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
	  break;

      };
    switch (line_join)
      {
      case RL2_PEN_JOIN_ROUND:
      case RL2_PEN_JOIN_BEVEL:
	  ctx->current_pen.line_join = line_join;
	  break;
      default:
	  ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
	  break;

      };
    ctx->current_pen.is_solid_color = 0;
    ctx->current_pen.is_linear_gradient = 1;
    ctx->current_pen.is_pattern = 0;
    ctx->current_pen.red = d_red;
    ctx->current_pen.green = d_green;
    ctx->current_pen.blue = d_blue;
    ctx->current_pen.alpha = d_alpha;
    ctx->current_pen.x0 = x;
    ctx->current_pen.y0 = y;
    ctx->current_pen.x1 = x + width;
    ctx->current_pen.y1 = y + height;
    d_red = (double) red2 / 255.0;
    d_green = (double) green2 / 255.0;
    d_blue = (double) blue2 / 255.0;
    d_alpha = (double) alpha2 / 255.0;
    ctx->current_pen.red2 = d_red;
    ctx->current_pen.green2 = d_green;
    ctx->current_pen.blue2 = d_blue;
    ctx->current_pen.alpha2 = d_alpha;
    ctx->current_pen.dash_count = dash_count;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    ctx->current_pen.dash_array = malloc (sizeof (double) * dash_count);
    for (d = 0; d < dash_count; d++)
	*(ctx->current_pen.dash_array + d) = *(dash_list + d);
    ctx->current_pen.dash_offset = dash_offset;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_pattern_solid_pen (rl2GraphicsContextPtr context,
				 rl2GraphicsPatternPtr brush,
				 double width, int line_cap, int line_join)
{
/* setting up a Pattern Pen - solid style */
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) brush;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (pattern == NULL)
	return 0;

    ctx->current_pen.width = width;
    switch (line_cap)
      {
      case RL2_PEN_CAP_ROUND:
      case RL2_PEN_CAP_SQUARE:
	  ctx->current_pen.line_cap = line_cap;
	  break;
      default:
	  ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
	  break;

      };
    switch (line_join)
      {
      case RL2_PEN_JOIN_ROUND:
      case RL2_PEN_JOIN_BEVEL:
	  ctx->current_pen.line_join = line_join;
	  break;
      default:
	  ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
	  break;

      };
    ctx->current_pen.is_solid_color = 0;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 1;
    ctx->current_pen.pattern = pattern->pattern;
    ctx->current_pen.dash_count = 0;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    ctx->current_pen.dash_array = NULL;
    ctx->current_pen.dash_offset = 0.0;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_pattern_dashed_pen (rl2GraphicsContextPtr context,
				  rl2GraphicsPatternPtr brush,
				  double width, int line_cap, int line_join,
				  int dash_count, double dash_list[],
				  double dash_offset)
{
/* setting up a Pattern Pen - dashed style */
    int d;
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) brush;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (pattern == NULL)
	return 0;
    if (dash_count <= 0 || dash_list == NULL)
	return 0;

    ctx->current_pen.width = width;
    switch (line_cap)
      {
      case RL2_PEN_CAP_ROUND:
      case RL2_PEN_CAP_SQUARE:
	  ctx->current_pen.line_cap = line_cap;
	  break;
      default:
	  ctx->current_pen.line_cap = RL2_PEN_CAP_BUTT;
	  break;

      };
    switch (line_join)
      {
      case RL2_PEN_JOIN_ROUND:
      case RL2_PEN_JOIN_BEVEL:
	  ctx->current_pen.line_join = line_join;
	  break;
      default:
	  ctx->current_pen.line_join = RL2_PEN_JOIN_MITER;
	  break;

      };
    ctx->current_pen.is_solid_color = 0;
    ctx->current_pen.is_linear_gradient = 0;
    ctx->current_pen.is_pattern = 1;
    ctx->current_pen.pattern = pattern->pattern;
    ctx->current_pen.dash_count = dash_count;
    if (ctx->current_pen.dash_array != NULL)
	free (ctx->current_pen.dash_array);
    ctx->current_pen.dash_array = malloc (sizeof (double) * dash_count);
    for (d = 0; d < dash_count; d++)
	*(ctx->current_pen.dash_array + d) = *(dash_list + d);
    ctx->current_pen.dash_offset = dash_offset;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_brush (rl2GraphicsContextPtr context, unsigned char red,
		     unsigned char green, unsigned char blue,
		     unsigned char alpha)
{
/* setting up a Color Brush */
    double d_red = (double) red / 255.0;
    double d_green = (double) green / 255.0;
    double d_blue = (double) blue / 255.0;
    double d_alpha = (double) alpha / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;

    ctx->current_brush.is_solid_color = 1;
    ctx->current_brush.is_linear_gradient = 0;
    ctx->current_brush.is_pattern = 0;
    ctx->current_brush.red = d_red;
    ctx->current_brush.green = d_green;
    ctx->current_brush.blue = d_blue;
    ctx->current_brush.alpha = d_alpha;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_linear_gradient_brush (rl2GraphicsContextPtr context, double x,
				     double y, double width, double height,
				     unsigned char red1, unsigned char green1,
				     unsigned char blue1,
				     unsigned char alpha1, unsigned char red2,
				     unsigned char green2,
				     unsigned char blue2, unsigned char alpha2)
{
/* setting up a Linear Gradient Brush */
    double d_red = (double) red1 / 255.0;
    double d_green = (double) green1 / 255.0;
    double d_blue = (double) blue1 / 255.0;
    double d_alpha = (double) alpha1 / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;

    ctx->current_brush.is_solid_color = 0;
    ctx->current_brush.is_linear_gradient = 1;
    ctx->current_brush.is_pattern = 0;
    ctx->current_brush.red = d_red;
    ctx->current_brush.green = d_green;
    ctx->current_brush.blue = d_blue;
    ctx->current_brush.alpha = d_alpha;
    ctx->current_brush.x0 = x;
    ctx->current_brush.y0 = y;
    ctx->current_brush.x1 = x + width;
    ctx->current_brush.y1 = y + height;
    d_red = (double) red2 / 255.0;
    d_green = (double) green2 / 255.0;
    d_blue = (double) blue2 / 255.0;
    d_alpha = (double) alpha2 / 255.0;
    ctx->current_brush.red2 = d_red;
    ctx->current_brush.green2 = d_green;
    ctx->current_brush.blue2 = d_blue;
    ctx->current_brush.alpha2 = d_alpha;
    return 1;
}

RL2_DECLARE int
rl2_graph_set_pattern_brush (rl2GraphicsContextPtr context,
			     rl2GraphicsPatternPtr brush)
{
/* setting up a Pattern Brush */
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) brush;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (pattern == NULL)
	return 0;

    ctx->current_brush.is_solid_color = 0;
    ctx->current_brush.is_linear_gradient = 0;
    ctx->current_brush.is_pattern = 1;

    ctx->current_brush.pattern = pattern->pattern;
    return 1;
}

static void
rl2_priv_graph_destroy_font (RL2GraphFontPtr fnt)
{
/* destroying a font (not yet cached) */
    if (fnt == NULL)
	return;

    if (fnt->tt_font != NULL)
      {
	  if (fnt->tt_font->facename != NULL)
	      free (fnt->tt_font->facename);
	  if (fnt->tt_font->FTface != NULL)
	      FT_Done_Face ((FT_Face) (fnt->tt_font->FTface));
	  if (fnt->tt_font->ttf_data != NULL)
	      free (fnt->tt_font->ttf_data);
      }
    free (fnt);
}

RL2_DECLARE int
rl2_graph_set_font (rl2GraphicsContextPtr context, rl2GraphicsFontPtr font)
{
/* setting up the current font */
    cairo_t *cairo;
    int style = CAIRO_FONT_SLANT_NORMAL;
    int weight = CAIRO_FONT_WEIGHT_NORMAL;
    double size;
    RL2GraphFontPtr fnt = (RL2GraphFontPtr) font;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (fnt == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    size = fnt->size;
    ctx->font_red = fnt->font_red;
    ctx->font_green = fnt->font_green;
    ctx->font_blue = fnt->font_blue;
    ctx->font_alpha = fnt->font_alpha;
    ctx->with_font_halo = fnt->with_halo;
    if (fnt->with_halo)
      {
	  ctx->halo_radius = fnt->halo_radius;
	  ctx->halo_red = fnt->halo_red;
	  ctx->halo_green = fnt->halo_green;
	  ctx->halo_blue = fnt->halo_blue;
	  ctx->halo_alpha = fnt->halo_alpha;
	  size += fnt->halo_radius;
      }
    if (fnt->toy_font)
      {
	  /* using a CAIRO built-in "toy" font */
	  if (fnt->style == RL2_FONTSTYLE_ITALIC)
	      style = CAIRO_FONT_SLANT_ITALIC;
	  if (fnt->style == RL2_FONTSTYLE_OBLIQUE)
	      style = CAIRO_FONT_SLANT_OBLIQUE;
	  if (fnt->weight == RL2_FONTWEIGHT_BOLD)
	      weight = CAIRO_FONT_WEIGHT_BOLD;
	  cairo_select_font_face (cairo, fnt->facename, style, weight);
	  cairo_set_font_size (cairo, size);
	  fnt->cairo_font = cairo_get_font_face (cairo);
      }
    else
      {
	  /* using a TrueType font */
	  cairo_font_options_t *font_options = cairo_font_options_create ();
	  cairo_matrix_t ctm;
	  cairo_matrix_t font_matrix;
	  cairo_get_matrix (cairo, &ctm);
	  cairo_matrix_init (&font_matrix, size, 0.0, 0.0, size, 0.0, 0.0);
	  fnt->cairo_scaled_font =
	      cairo_scaled_font_create (fnt->cairo_font, &font_matrix, &ctm,
					font_options);
	  cairo_font_options_destroy (font_options);
	  cairo_set_scaled_font (cairo, fnt->cairo_scaled_font);
      }

    return 1;
}

RL2_DECLARE rl2GraphicsPatternPtr
rl2_graph_create_pattern (unsigned char *rgbaArray, int width, int height,
			  int extend)
{
/* creating a pattern brush */
    RL2PrivGraphPatternPtr pattern;

    if (rgbaArray == NULL)
	return NULL;

    adjust_for_endianness (rgbaArray, width, height);
    pattern = malloc (sizeof (RL2PrivGraphPattern));
    if (pattern == NULL)
	return NULL;
    pattern->width = width;
    pattern->height = height;
    pattern->rgba = rgbaArray;
    pattern->bitmap =
	cairo_image_surface_create_for_data (rgbaArray, CAIRO_FORMAT_ARGB32,
					     width, height, width * 4);
    pattern->pattern = cairo_pattern_create_for_surface (pattern->bitmap);
    if (extend)
	cairo_pattern_set_extend (pattern->pattern, CAIRO_EXTEND_REPEAT);
    else
	cairo_pattern_set_extend (pattern->pattern, CAIRO_EXTEND_NONE);
    return (rl2GraphicsPatternPtr) pattern;
}

RL2_DECLARE rl2GraphicsPatternPtr
rl2_create_pattern_from_external_graphic (sqlite3 * handle,
					  const char *xlink_href, int extend)
{
/* creating a pattern brush from an External Graphic resource */
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    rl2RasterPtr raster = NULL;
    unsigned char *rgbaArray = NULL;
    int rgbaSize;
    unsigned int width;
    unsigned int height;
    if (xlink_href == NULL)
	return NULL;

/* preparing the SQL query statement */
    sql =
	"SELECT resource, GetMimeType(resource) FROM SE_external_graphics "
	"WHERE Lower(xlink_href) = Lower(?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xlink_href, strlen (xlink_href), SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      const char *mime =
			  (const char *) sqlite3_column_text (stmt, 1);
		      if (strcmp (mime, "image/jpeg") == 0)
			{
			    if (raster != NULL)
				rl2_destroy_raster (raster);
			    raster = rl2_raster_from_jpeg (blob, blob_sz);
			}
		      if (strcmp (mime, "image/png") == 0)
			{
			    if (raster != NULL)
				rl2_destroy_raster (raster);
			    raster = rl2_raster_from_png (blob, blob_sz, 1);
			}
		      if (strcmp (mime, "image/gif") == 0)
			{
			    if (raster != NULL)
				rl2_destroy_raster (raster);
			    raster = rl2_raster_from_gif (blob, blob_sz);
			}
		  }
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (raster == NULL)
	goto error;

/* retieving the raster RGBA map */
    if (rl2_get_raster_size (raster, &width, &height) == RL2_OK)
      {
	  if (rl2_raster_data_to_RGBA (raster, &rgbaArray, &rgbaSize) != RL2_OK)
	      rgbaArray = NULL;
      }
    rl2_destroy_raster (raster);
    raster = NULL;
    if (rgbaArray == NULL)
	goto error;
    return rl2_graph_create_pattern (rgbaArray, width, height, extend);

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    return NULL;
}

RL2_DECLARE rl2GraphicsPatternPtr
rl2_create_pattern_from_external_svg (sqlite3 * handle,
				      const char *xlink_href, double size)
{
/* creating a pattern brush from an External SVG resource */
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    rl2RasterPtr raster = NULL;
    unsigned char *rgbaArray = NULL;
    int rgbaSize;
    unsigned int width;
    unsigned int height;
    if (xlink_href == NULL)
	return NULL;
    if (size <= 0.0)
	return NULL;

/* preparing the SQL query statement */
    sql =
	"SELECT XB_GetPayload(resource) FROM SE_external_graphics "
	"WHERE Lower(xlink_href) = Lower(?) AND "
	"GetMimeType(resource) = 'image/svg+xml'";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, xlink_href, strlen (xlink_href), SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      rl2SvgPtr svg_handle = rl2_create_svg (blob, blob_sz);
		      if (svg_handle != NULL)
			{
			    double svgWidth;
			    double svgHeight;
			    if (rl2_get_svg_size
				(svg_handle, &svgWidth, &svgHeight) == RL2_OK)
			      {
				  double sz;
				  double w = svgWidth;
				  double h = svgHeight;
				  if (w < size && h < size)
				    {
					while (w < size && h < size)
					  {
					      /* rescaling */
					      w *= 1.0001;
					      h *= 1.0001;
					  }
				    }
				  else
				    {
					while (w > size || h > size)
					  {
					      /* rescaling */
					      w *= 0.9;
					      h *= 0.9;
					  }
				    }
				  sz = w;
				  if (h > sz)
				      sz = h;
				  if (raster != NULL)
				      rl2_destroy_raster (raster);
				  raster =
				      rl2_raster_from_svg (svg_handle, size);
			      }
			    rl2_destroy_svg (svg_handle);
			}
		  }
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (raster == NULL)
	goto error;

/* retieving the raster RGBA map */
    if (rl2_get_raster_size (raster, &width, &height) == RL2_OK)
      {
	  if (rl2_raster_data_to_RGBA (raster, &rgbaArray, &rgbaSize) != RL2_OK)
	      rgbaArray = NULL;
      }
    rl2_destroy_raster (raster);
    raster = NULL;
    if (rgbaArray == NULL)
	goto error;
    return rl2_graph_create_pattern (rgbaArray, width, height, 0);

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (raster != NULL)
	rl2_destroy_raster (raster);
    return NULL;
}

RL2_DECLARE void
rl2_graph_destroy_pattern (rl2GraphicsPatternPtr brush)
{
/* destroying a pattern brush */
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) brush;

    if (pattern == NULL)
	return;

    cairo_pattern_destroy (pattern->pattern);
    cairo_surface_destroy (pattern->bitmap);
    if (pattern->rgba != NULL)
	free (pattern->rgba);
    free (pattern);
}

RL2_DECLARE int
rl2_graph_get_pattern_size (rl2GraphicsPatternPtr ptr,
			    unsigned int *width, unsigned int *height)
{
/* will return the Pattern dimensions */
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) ptr;

    if (pattern == NULL)
	return RL2_ERROR;
    *width = pattern->width;
    *height = pattern->height;
    return RL2_OK;
}

static void
aux_pattern_get_pixel (int x, int y, int width, unsigned char *bitmap,
		       unsigned char *red, unsigned char *green,
		       unsigned char *blue, unsigned char *alpha)
{
/* get pixel */
    unsigned char *p_in = bitmap + (y * width * 4) + (x * 4);
    int little_endian = rl2cr_endian_arch ();
    if (little_endian)
      {
	  *blue = *p_in++;
	  *green = *p_in++;
	  *red = *p_in++;
	  *alpha = *p_in++;
      }
    else
      {
	  *alpha = *p_in++;
	  *red = *p_in++;
	  *green = *p_in++;
	  *blue = *p_in++;
      }
}

static void
aux_pattern_set_pixel (int x, int y, int width, unsigned char *bitmap,
		       unsigned char red, unsigned char green,
		       unsigned char blue, unsigned char alpha)
{
/* set pixel */
    unsigned char *p_out = bitmap + (y * width * 4) + (x * 4);
    int little_endian = rl2cr_endian_arch ();
    if (little_endian)
      {
	  *p_out++ = blue;
	  *p_out++ = green;
	  *p_out++ = red;
	  *p_out++ = alpha;
      }
    else
      {
	  *p_out++ = alpha;
	  *p_out++ = red;
	  *p_out++ = green;
	  *p_out++ = blue;
      }
}

RL2_DECLARE int
rl2_graph_pattern_recolor (rl2GraphicsPatternPtr ptrn, unsigned char r,
			   unsigned char g, unsigned char b)
{
/* recoloring a Monochrome Pattern */
    int x;
    int y;
    int width;
    int height;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
    unsigned char xred;
    unsigned char xgreen;
    unsigned char xblue;
    unsigned char xalpha;
    int valid = 0;
    int has_black = 0;
    unsigned char *bitmap;
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) ptrn;
    if (pattern == NULL)
	return RL2_ERROR;

    width = pattern->width;
    height = pattern->height;
    cairo_surface_flush (pattern->bitmap);
    bitmap = cairo_image_surface_get_data (pattern->bitmap);
    if (bitmap == NULL)
	return RL2_ERROR;
/* checking for a Monochrome Pattern */
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		aux_pattern_get_pixel (x, y, width, bitmap, &red, &green,
				       &blue, &alpha);
		if (alpha != 0)
		  {
		      if (red < 64 && green < 64 && blue < 64)
			  has_black++;
		      if (valid)
			{
			    if (xred == red && xgreen == green
				&& xblue == blue && alpha == xalpha)
				;
			    else
				goto not_mono;
			}
		      else
			{
			    xred = red;
			    xgreen = green;
			    xblue = blue;
			    xalpha = alpha;
			    valid = 1;
			}
		  }
	    }
      }
/* all right, applying the new color */
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		aux_pattern_get_pixel (x, y, width, bitmap, &red, &green,
				       &blue, &alpha);
		if (alpha != 0)
		    aux_pattern_set_pixel (x, y, width, bitmap, r, g, b, 255);
		else
		    aux_pattern_set_pixel (x, y, width, bitmap, 0, 0, 0, 0);
	    }
      }
    cairo_surface_mark_dirty (pattern->bitmap);
    return RL2_OK;

  not_mono:
    if (has_black)
      {
	  /* recoloring only the black pixels */
	  for (y = 0; y < height; y++)
	    {
		for (x = 0; x < width; x++)
		  {
		      aux_pattern_get_pixel (x, y, width, bitmap, &red,
					     &green, &blue, &alpha);
		      if (red < 64 && green < 64 && blue < 64)
			  aux_pattern_set_pixel (x, y, width, bitmap, r, g, b,
						 255);
		      else
			  aux_pattern_set_pixel (x, y, width, bitmap, 0, 0, 0,
						 0);
		  }
	    }
	  cairo_surface_mark_dirty (pattern->bitmap);
	  return RL2_OK;
      }
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_graph_pattern_transparency (rl2GraphicsPatternPtr ptrn, unsigned char aleph)
{
/* changing the Pattern's transparency */
    int x;
    int y;
    int width;
    int height;
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char alpha;
    unsigned char *bitmap;
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) ptrn;
    if (pattern == NULL)
	return RL2_ERROR;

    width = pattern->width;
    height = pattern->height;
    cairo_surface_flush (pattern->bitmap);
    bitmap = cairo_image_surface_get_data (pattern->bitmap);
    if (bitmap == NULL)
	return RL2_ERROR;
/* applying the new transparency */
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		aux_pattern_get_pixel (x, y, width, bitmap, &red, &green,
				       &blue, &alpha);
		if (alpha != 0)
		    aux_pattern_set_pixel (x, y, width, bitmap, red, green,
					   blue, aleph);
	    }
      }
    cairo_surface_mark_dirty (pattern->bitmap);
    return RL2_OK;
}

RL2_DECLARE rl2GraphicsFontPtr
rl2_graph_create_toy_font (const char *facename, double size, int style,
			   int weight)
{
/* creating a font based on CAIRO internal "toy" fonts */
    RL2GraphFontPtr fnt;
    int len;

    fnt = malloc (sizeof (RL2GraphFont));
    if (fnt == NULL)
	return NULL;
    fnt->toy_font = 1;
    fnt->cairo_scaled_font = NULL;
    fnt->tt_font = NULL;
    if (facename == NULL)
	facename = "monospace";
    if (strcasecmp (facename, "serif") == 0)
      {
	  len = strlen ("serif");
	  fnt->facename = malloc (len + 1);
	  strcpy (fnt->facename, "serif");
      }
    else if (strcasecmp (facename, "sans-serif") == 0)
      {
	  len = strlen ("sans-serif");
	  fnt->facename = malloc (len + 1);
	  strcpy (fnt->facename, "sans-serif");
      }
    else if (strcasecmp (facename, "monospace") == 0)
      {
	  len = strlen ("monospace");
	  fnt->facename = malloc (len + 1);
	  strcpy (fnt->facename, "monospace");
      }
    else
      {
	  free (fnt);
	  return NULL;
      }
    if (size < 1.0)
	fnt->size = 1.0;
    else if (size > 72.0)
	fnt->size = 72.0;
    else
	fnt->size = size;
    if (style == RL2_FONTSTYLE_ITALIC)
	fnt->style = RL2_FONTSTYLE_ITALIC;
    else if (style == RL2_FONTSTYLE_OBLIQUE)
	fnt->style = RL2_FONTSTYLE_OBLIQUE;
    else
	fnt->style = RL2_FONTSTYLE_NORMAL;
    if (weight == RL2_FONTWEIGHT_BOLD)
	fnt->weight = RL2_FONTWEIGHT_BOLD;
    else
	fnt->weight = RL2_FONTWEIGHT_NORMAL;
    fnt->font_red = 0.0;
    fnt->font_green = 0.0;
    fnt->font_blue = 0.0;
    fnt->font_alpha = 1.0;
    fnt->with_halo = 0;
    fnt->halo_radius = 0.0;
    fnt->halo_red = 0.0;
    fnt->halo_green = 0.0;
    fnt->halo_blue = 0.0;
    fnt->halo_alpha = 1.0;
    return (rl2GraphicsFontPtr) fnt;
}

RL2_PRIVATE void
rl2_destroy_private_tt_font (struct rl2_private_tt_font *font)
{
/* destroying a private font */
    if (font == NULL)
	return;

    if (font->facename != NULL)
	free (font->facename);
    if (font->FTface != NULL)
	FT_Done_Face ((FT_Face) (font->FTface));
    if (font->ttf_data != NULL)
	free (font->ttf_data);
    free (font);
}

static void
rl2_font_destructor_callback (void *data)
{
/* font destructor callback */
    struct rl2_private_tt_font *font = (struct rl2_private_tt_font *) data;
    if (font == NULL)
	return;

/* adjusting the double-linked list */
    if (font == font->container->first_font
	&& font == font->container->last_font)
      {
	  /* going to remove the unique item from the list */
	  font->container->first_font = NULL;
	  font->container->last_font = NULL;
      }
    else if (font == font->container->first_font)
      {
	  /* going to remove the first item from the list */
	  font->next->prev = NULL;
	  font->container->first_font = font->next;
      }
    else if (font == font->container->last_font)
      {
	  /* going to remove the last item from the list */
	  font->prev->next = NULL;
	  font->container->last_font = font->prev;
      }
    else
      {
	  /* normal case */
	  font->prev->next = font->next;
	  font->next->prev = font->prev;
      }

/* destroying the cached font */
    rl2_destroy_private_tt_font (font);
}

RL2_DECLARE rl2GraphicsFontPtr
rl2_graph_create_TrueType_font (const void *priv_data,
				const unsigned char *ttf, int ttf_bytes,
				double size)
{
/* creating a TrueType font */
    RL2GraphFontPtr fnt;
    char *facename;
    int is_bold;
    int is_italic;
    unsigned char *font = NULL;
    int font_sz;
    FT_Error error;
    FT_Library library;
    FT_Face face;
    static const cairo_user_data_key_t key;
    struct rl2_private_data *cache = (struct rl2_private_data *) priv_data;
    struct rl2_private_tt_font *tt_font;
    if (cache == NULL)
	return NULL;
    if (cache->FTlibrary == NULL)
	return NULL;
    library = (FT_Library) (cache->FTlibrary);

/* testing the BLOB-encoded TTF object for validity */
    if (ttf == NULL || ttf_bytes <= 0)
	return NULL;
    if (rl2_is_valid_encoded_font (ttf, ttf_bytes) != RL2_OK)
	return NULL;
    facename = rl2_get_encoded_font_facename (ttf, ttf_bytes);
    if (facename == NULL)
	return NULL;
    is_bold = rl2_is_encoded_font_bold (ttf, ttf_bytes);
    is_italic = rl2_is_encoded_font_italic (ttf, ttf_bytes);

/* decoding the TTF BLOB */
    if (rl2_font_decode (ttf, ttf_bytes, &font, &font_sz) != RL2_OK)
	return NULL;
/* creating a FreeType font object */
    error = FT_New_Memory_Face (library, font, font_sz, 0, &face);
    if (error)
      {
	  free (facename);
	  return NULL;
      }

    fnt = malloc (sizeof (RL2GraphFont));
    if (fnt == NULL)
      {
	  free (facename);
	  FT_Done_Face (face);
	  return NULL;
      }
    tt_font = malloc (sizeof (struct rl2_private_tt_font));
    if (tt_font == NULL)
      {
	  free (facename);
	  FT_Done_Face (face);
	  free (fnt);
	  return NULL;
      }
    fnt->toy_font = 0;
    fnt->tt_font = tt_font;
    fnt->tt_font->facename = facename;
    fnt->tt_font->is_bold = is_bold;
    fnt->tt_font->is_italic = is_italic;
    fnt->tt_font->container = cache;
    fnt->tt_font->FTface = face;
    fnt->tt_font->ttf_data = font;
    fnt->cairo_font = cairo_ft_font_face_create_for_ft_face (face, 0);
    if (fnt->cairo_font == NULL)
      {
	  rl2_priv_graph_destroy_font (fnt);
	  return NULL;
      }
    fnt->cairo_scaled_font = NULL;
/* inserting into the cache */
    tt_font->prev = cache->last_font;
    tt_font->next = NULL;
    if (cache->first_font == NULL)
	cache->first_font = tt_font;
    if (cache->last_font != NULL)
	cache->last_font->next = tt_font;
    cache->last_font = tt_font;
/* registering the destructor callback */
    if (cairo_font_face_set_user_data
	(fnt->cairo_font, &key, tt_font,
	 rl2_font_destructor_callback) != CAIRO_STATUS_SUCCESS)
      {
	  rl2_priv_graph_destroy_font (fnt);
	  return NULL;
      }
    if (size < 1.0)
	fnt->size = 1.0;
    else if (size > 72.0)
	fnt->size = 72.0;
    else
	fnt->size = size;
    if (is_italic)
	fnt->style = RL2_FONTSTYLE_ITALIC;
    else
	fnt->style = RL2_FONTSTYLE_NORMAL;
    if (is_bold)
	fnt->weight = RL2_FONTWEIGHT_BOLD;
    else
	fnt->weight = RL2_FONTWEIGHT_NORMAL;
    fnt->font_red = 0.0;
    fnt->font_green = 0.0;
    fnt->font_blue = 0.0;
    fnt->font_alpha = 1.0;
    fnt->with_halo = 0;
    fnt->halo_radius = 0.0;
    fnt->halo_red = 0.0;
    fnt->halo_green = 0.0;
    fnt->halo_blue = 0.0;
    fnt->halo_alpha = 1.0;
    return (rl2GraphicsFontPtr) fnt;
}

RL2_DECLARE int
rl2_graph_release_font (rl2GraphicsContextPtr context)
{
/* selecting a default font so to releasee the currently set Font */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;

    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    cairo_select_font_face (cairo, "", CAIRO_FONT_SLANT_NORMAL,
			    CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size (cairo, 10.0);
    return 1;
}

RL2_DECLARE void
rl2_graph_destroy_font (rl2GraphicsFontPtr font)
{
/* destroying a font */
    RL2GraphFontPtr fnt = (RL2GraphFontPtr) font;
    if (fnt == NULL)
	return;
    if (fnt->toy_font == 0)
      {
	  /* True Type Font */
	  if (fnt->cairo_scaled_font != NULL)
	    {
		if (cairo_scaled_font_get_reference_count
		    (fnt->cairo_scaled_font) > 0)
		    cairo_scaled_font_destroy (fnt->cairo_scaled_font);
	    }
	  if (fnt->cairo_font != NULL)
	    {
		if (cairo_font_face_get_reference_count (fnt->cairo_font) > 0)
		    cairo_font_face_destroy (fnt->cairo_font);
	    }
      }
    else
      {
	  /* Cairo Toy Font */
	  if (fnt->facename != NULL)
	      free (fnt->facename);
	  /*
	     if (fnt->cairo_scaled_font != NULL)
	     {
	     if (cairo_scaled_font_get_reference_count(fnt->cairo_scaled_font) > 0)
	     cairo_scaled_font_destroy (fnt->cairo_scaled_font);
	     }
	     if (fnt->cairo_font != NULL)
	     {
	     if (cairo_font_face_get_reference_count (fnt->cairo_font) > 0)
	     cairo_font_face_destroy (fnt->cairo_font);
	     }
	   */
      }
    free (fnt);
}

RL2_DECLARE int
rl2_graph_font_set_color (rl2GraphicsFontPtr font, unsigned char red,
			  unsigned char green, unsigned char blue,
			  unsigned char alpha)
{
/* setting up the font color */
    RL2GraphFontPtr fnt = (RL2GraphFontPtr) font;

    if (fnt == NULL)
	return 0;

    fnt->font_red = (double) red / 255.0;
    fnt->font_green = (double) green / 255.0;
    fnt->font_blue = (double) blue / 255.0;
    fnt->font_alpha = (double) alpha / 255.0;
    return 1;
}

RL2_DECLARE int
rl2_graph_font_set_halo (rl2GraphicsFontPtr font, double radius,
			 unsigned char red, unsigned char green,
			 unsigned char blue, unsigned char alpha)
{
/* setting up the font Halo */
    RL2GraphFontPtr fnt = (RL2GraphFontPtr) font;

    if (fnt == NULL)
	return 0;

    if (radius <= 0.0)
      {
	  fnt->with_halo = 0;
	  fnt->halo_radius = 0.0;
      }
    else
      {
	  fnt->with_halo = 1;
	  fnt->halo_radius = radius;
	  fnt->halo_red = (double) red / 255.0;
	  fnt->halo_green = (double) green / 255.0;
	  fnt->halo_blue = (double) blue / 255.0;
	  fnt->halo_alpha = (double) alpha / 255.0;
      }
    return 1;
}

RL2_DECLARE rl2GraphicsBitmapPtr
rl2_graph_create_bitmap (unsigned char *rgbaArray, int width, int height)
{
/* creating a bitmap */
    RL2GraphBitmapPtr bmp;

    if (rgbaArray == NULL)
	return NULL;

    adjust_for_endianness (rgbaArray, width, height);
    bmp = malloc (sizeof (RL2GraphBitmap));
    if (bmp == NULL)
	return NULL;
    bmp->width = width;
    bmp->height = height;
    bmp->rgba = rgbaArray;
    bmp->bitmap =
	cairo_image_surface_create_for_data (rgbaArray, CAIRO_FORMAT_ARGB32,
					     width, height, width * 4);
    bmp->pattern = cairo_pattern_create_for_surface (bmp->bitmap);
    return (rl2GraphicsBitmapPtr) bmp;
}

RL2_DECLARE void
rl2_graph_destroy_bitmap (rl2GraphicsBitmapPtr bitmap)
{
/* destroying a bitmap */
    RL2GraphBitmapPtr bmp = (RL2GraphBitmapPtr) bitmap;

    if (bmp == NULL)
	return;

    cairo_pattern_destroy (bmp->pattern);
    cairo_surface_destroy (bmp->bitmap);
    if (bmp->rgba != NULL)
	free (bmp->rgba);
    free (bmp);
}

static void
set_current_brush (RL2GraphContextPtr ctx)
{
/* setting up the current Brush */
    cairo_t *cairo;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    if (ctx->current_brush.is_solid_color)
      {
	  /* using a Solid Color Brush */
	  cairo_set_source_rgba (cairo, ctx->current_brush.red,
				 ctx->current_brush.green,
				 ctx->current_brush.blue,
				 ctx->current_brush.alpha);
      }
    else if (ctx->current_brush.is_linear_gradient)
      {
	  /* using a Linear Gradient Brush */
	  cairo_pattern_t *pattern =
	      cairo_pattern_create_linear (ctx->current_brush.x0,
					   ctx->current_brush.y0,
					   ctx->current_brush.x1,
					   ctx->current_brush.y1);
	  cairo_pattern_add_color_stop_rgba (pattern, 0.0,
					     ctx->current_brush.red,
					     ctx->current_brush.green,
					     ctx->current_brush.blue,
					     ctx->current_brush.alpha);
	  cairo_pattern_add_color_stop_rgba (pattern, 1.0,
					     ctx->current_brush.red2,
					     ctx->current_brush.green2,
					     ctx->current_brush.blue2,
					     ctx->current_brush.alpha2);
	  cairo_set_source (cairo, pattern);
	  cairo_pattern_destroy (pattern);
      }
    else if (ctx->current_brush.is_pattern)
      {
	  /* using a Pattern Brush */
	  cairo_set_source (cairo, ctx->current_brush.pattern);
      }
}

RL2_DECLARE int
rl2_graph_release_pattern_brush (rl2GraphicsContextPtr context)
{
/* releasing the current Pattern Brush */
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    cairo_t *cairo;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    if (ctx->current_brush.is_pattern)
      {
	  ctx->current_brush.is_solid_color = 1;
	  ctx->current_brush.is_pattern = 0;
	  cairo_set_source_rgba (cairo, 0.0, 0.0, 0.0, 1.0);
	  ctx->current_brush.pattern = NULL;
	  return 1;
      }
    else
	return 0;
}

static void
set_current_pen (RL2GraphContextPtr ctx)
{
/* setting up the current Pen */
    cairo_t *cairo;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    cairo_set_line_width (cairo, ctx->current_pen.width);
    if (ctx->current_pen.is_solid_color)
      {
	  /* using a Solid Color Pen */
	  cairo_set_source_rgba (cairo, ctx->current_pen.red,
				 ctx->current_pen.green,
				 ctx->current_pen.blue, ctx->current_pen.alpha);
      }
    else if (ctx->current_pen.is_linear_gradient)
      {
	  /* using a Linear Gradient Pen */
	  cairo_pattern_t *pattern =
	      cairo_pattern_create_linear (ctx->current_pen.x0,
					   ctx->current_pen.y0,
					   ctx->current_pen.x1,
					   ctx->current_pen.y1);
	  cairo_pattern_add_color_stop_rgba (pattern, 0.0,
					     ctx->current_pen.red,
					     ctx->current_pen.green,
					     ctx->current_pen.blue,
					     ctx->current_pen.alpha);
	  cairo_pattern_add_color_stop_rgba (pattern, 1.0,
					     ctx->current_pen.red2,
					     ctx->current_pen.green2,
					     ctx->current_pen.blue2,
					     ctx->current_pen.alpha2);
	  cairo_set_source (cairo, pattern);
	  cairo_pattern_destroy (pattern);
      }
    else if (ctx->current_pen.is_pattern)
      {
	  /* using a Pattern Pen */
	  cairo_set_source (cairo, ctx->current_pen.pattern);
      }
    switch (ctx->current_pen.line_cap)
      {
      case RL2_PEN_CAP_ROUND:
	  cairo_set_line_cap (cairo, CAIRO_LINE_CAP_ROUND);
	  break;
      case RL2_PEN_CAP_SQUARE:
	  cairo_set_line_cap (cairo, CAIRO_LINE_CAP_SQUARE);
	  break;
      default:
	  cairo_set_line_cap (cairo, CAIRO_LINE_CAP_BUTT);
	  break;
      };
    switch (ctx->current_pen.line_join)
      {
      case RL2_PEN_JOIN_ROUND:
	  cairo_set_line_join (cairo, CAIRO_LINE_JOIN_ROUND);
	  break;
      case RL2_PEN_JOIN_BEVEL:
	  cairo_set_line_join (cairo, CAIRO_LINE_JOIN_BEVEL);
	  break;
      default:
	  cairo_set_line_join (cairo, CAIRO_LINE_JOIN_MITER);
	  break;
      };
    if (ctx->current_pen.dash_count == 0 || ctx->current_pen.dash_array == NULL)
	cairo_set_dash (cairo, NULL, 0, 0.0);
    else
	cairo_set_dash (cairo, ctx->current_pen.dash_array,
			ctx->current_pen.dash_count,
			ctx->current_pen.dash_offset);
}

RL2_DECLARE int
rl2_graph_release_pattern_pen (rl2GraphicsContextPtr context)
{
/* releasing the current Pattern Pen */
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    cairo_t *cairo;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    if (ctx->current_pen.is_pattern)
      {
	  ctx->current_pen.is_solid_color = 1;
	  ctx->current_pen.is_pattern = 0;
	  cairo_set_source_rgba (cairo, 0.0, 0.0, 0.0, 1.0);
	  ctx->current_pen.pattern = NULL;
	  return 1;
      }
    else
	return 0;
}

RL2_DECLARE int
rl2_graph_draw_rectangle (rl2GraphicsContextPtr context, double x, double y,
			  double width, double height)
{
/* Drawing a filled rectangle */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_rectangle (cairo, x, y, width, height);
    set_current_brush (ctx);
    cairo_fill_preserve (cairo);
    set_current_pen (ctx);
    cairo_stroke (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_draw_rounded_rectangle (rl2GraphicsContextPtr context, double x,
				  double y, double width, double height,
				  double radius)
{
/* Drawing a filled rectangle with rounded corners */
    cairo_t *cairo;
    double degrees = M_PI / 180.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_new_sub_path (cairo);
    cairo_arc (cairo, x + width - radius, y + radius, radius,
	       -90 * degrees, 0 * degrees);
    cairo_arc (cairo, x + width - radius, y + height - radius, radius,
	       0 * degrees, 90 * degrees);
    cairo_arc (cairo, x + radius, y + height - radius, radius,
	       90 * degrees, 180 * degrees);
    cairo_arc (cairo, x + radius, y + radius, radius, 180 * degrees,
	       270 * degrees);
    cairo_close_path (cairo);
    set_current_brush (ctx);
    cairo_fill_preserve (cairo);
    set_current_pen (ctx);
    cairo_stroke (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_draw_ellipse (rl2GraphicsContextPtr context, double x, double y,
			double width, double height)
{
/* Drawing a filled ellipse */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_save (cairo);
    cairo_translate (cairo, x + (width / 2.0), y + (height / 2.0));
    cairo_scale (cairo, width / 2.0, height / 2.0);
    cairo_arc (cairo, 0.0, 0.0, 1.0, 0.0, 2.0 * M_PI);
    cairo_restore (cairo);
    set_current_brush (ctx);
    cairo_fill_preserve (cairo);
    set_current_pen (ctx);
    cairo_stroke (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_draw_circle_sector (rl2GraphicsContextPtr context, double center_x,
			      double center_y, double radius,
			      double from_angle, double to_angle)
{
/* drawing a filled circular sector */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_move_to (cairo, center_x, center_y);
    cairo_arc (cairo, center_x, center_y, radius, from_angle, to_angle);
    cairo_line_to (cairo, center_x, center_y);
    set_current_brush (ctx);
    cairo_fill_preserve (cairo);
    set_current_pen (ctx);
    cairo_stroke (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_stroke_line (rl2GraphicsContextPtr context, double x0, double y0,
		       double x1, double y1)
{
/* Stroking a line */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_move_to (cairo, x0, y0);
    cairo_line_to (cairo, x1, y1);
    set_current_pen (ctx);
    cairo_stroke (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_move_to_point (rl2GraphicsContextPtr context, double x, double y)
{
/* Moving to a Path Point */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    cairo_move_to (cairo, x, y);
    return 1;
}

RL2_DECLARE int
rl2_graph_add_line_to_path (rl2GraphicsContextPtr context, double x, double y)
{
/* Adding a Line to a Path */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    cairo_line_to (cairo, x, y);
    return 1;
}

RL2_DECLARE int
rl2_graph_close_subpath (rl2GraphicsContextPtr context)
{
/* Closing a SubPath */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;
    cairo_close_path (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_stroke_path (rl2GraphicsContextPtr context, int preserve)
{
/* Stroking a path */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    set_current_pen (ctx);
    if (preserve == RL2_PRESERVE_PATH)
	cairo_stroke_preserve (cairo);
    else
	cairo_stroke (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_fill_path (rl2GraphicsContextPtr context, int preserve)
{
/* Filling a path */
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    set_current_brush (ctx);
    cairo_set_fill_rule (cairo, CAIRO_FILL_RULE_EVEN_ODD);
    if (preserve == RL2_PRESERVE_PATH)
	cairo_fill_preserve (cairo);
    else
	cairo_fill (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_get_text_extent (rl2GraphicsContextPtr context, const char *text,
			   double *pre_x, double *pre_y, double *width,
			   double *height, double *post_x, double *post_y)
{
/* measuring the text extent (using the current font) */
    cairo_t *cairo;
    cairo_text_extents_t extents;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (text == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    cairo_text_extents (cairo, text, &extents);
    *pre_x = extents.x_bearing;
    *pre_y = extents.y_bearing;
    *width = extents.width;
    *height = extents.height;
    *post_x = extents.x_advance;
    *post_y = extents.y_advance;
    return 1;
}

static int
do_eval_mbrs_intersection (struct rl2_label_rect *r1, struct rl2_label_rect *r2)
{
/* evaluating if the two MBRs do actually intersects */
    double r1_minx;
    double r1_miny;
    double r1_maxx;
    double r1_maxy;
    double r2_minx;
    double r2_miny;
    double r2_maxx;
    double r2_maxy;

    if (!do_parse_label_mbr
	(r1->blob, r1->blob_size, &r1_minx, &r1_miny, &r1_maxx, &r1_maxy))
	return 0;
    if (!do_parse_label_mbr
	(r2->blob, r2->blob_size, &r2_minx, &r2_miny, &r2_maxx, &r2_maxy))
	return 0;

    if (r1_minx > r2_maxx)
	return 0;
    if (r1_miny > r2_maxy)
	return 0;
    if (r1_maxx < r2_minx)
	return 0;
    if (r1_maxy < r2_miny)
	return 0;
    if (r2_minx > r1_maxx)
	return 0;
    if (r2_miny > r1_maxy)
	return 0;
    if (r2_maxx < r1_minx)
	return 0;
    if (r2_maxy < r1_miny)
	return 0;
    return 1;
}

static int
do_eval_collision (struct rl2_advanced_labeling *labeling, sqlite3_stmt * stmt,
		   struct rl2_label_rect *bbox)
{
/* querying the anti-collision list */
    int ret;
    struct rl2_label_rect *ptr = labeling->first_rect;
    while (ptr != NULL)
      {
	  if (do_eval_mbrs_intersection (ptr, bbox))
	    {
		sqlite3_reset (stmt);
		sqlite3_clear_bindings (stmt);
		sqlite3_bind_blob (stmt, 1, bbox->blob, bbox->blob_size,
				   SQLITE_STATIC);
		sqlite3_bind_blob (stmt, 2, ptr->blob, ptr->blob_size,
				   SQLITE_STATIC);
		while (1)
		  {
		      /* scrolling the result set rows */
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE)
			  break;	/* end of result set */
		      if (ret == SQLITE_ROW)
			{
			    if (sqlite3_column_type (stmt, 0) == SQLITE_INTEGER)
				if (sqlite3_column_int (stmt, 0) == 1)
				    return 1;
			}
		  }
	    }
	  ptr = ptr->next;
      }
    return 0;
}

static int
do_check_collision (struct rl2_advanced_labeling *labeling, sqlite3_stmt * stmt,
		    double x, double y, double angle, double anchor_point_x,
		    double anchor_point_y, double pre_x, double pre_y,
		    double width, double height, double post_x, double post_y,
		    int add_to_list, int pre_checked)
{
/* checking for an eventual collision between labels */
    int ret;
    double rad = angle * 0.0174532925199432958;
    double cosine = cos (rad);
    double sine = sin (rad);
    double shift_x;
    double shift_y;
    double adj_y;
    double px0;
    double py0;
    double px1;
    double py1;
    double px2;
    double py2;
    double px3;
    double py3;
    double x0;
    double y0;
    double x1;
    double y1;
    double x2;
    double y2;
    double x3;
    double y3;
    double minx;
    double miny;
    double maxx;
    double maxy;
    struct rl2_label_rect bbox;
    struct rl2_label_rect *ptr;
    int blob_size;

    if (post_y < 0.0)
	fprintf (stderr,
		 "Ouch ... AntiLabelCollision found an unexpected NEGATIVE post_y !!!\n");

/* repositioning the label Anchor Point */
    shift_x = width * anchor_point_x;
    shift_y = height * anchor_point_y;
    adj_y = 0.0;
    if (pre_y < 0.0)
	adj_y = height + pre_y;

/* determining the corners of the label BBOX (before rotation) */
    px0 = 0.0 - shift_x - 2.0;
    if (pre_x < 0.0)
	px0 -= pre_x;
    if (post_x < 0.0)
	px1 = px0 + post_x;
    else
	px1 = px0 + width;
    if (pre_x < 0.0)
	px1 -= pre_x;
    px1 += 4.0;
    px2 = px1;
    px3 = px0;
    py0 = 0.0 - shift_y - adj_y - 2.0;
    py2 = py0 + height;
    py2 += 4.0;
    py1 = py0;
    py3 = py2;

/* rotating the label BBOX */
    x0 = x + ((px0 * cosine) + (py0 * sine));
    y0 = y - ((py0 * cosine) - (px0 * sine));
    x1 = x + ((px1 * cosine) + (py1 * sine));
    y1 = y - ((py1 * cosine) - (px1 * sine));
    x2 = x + ((px2 * cosine) + (py2 * sine));
    y2 = y - ((py2 * cosine) - (px2 * sine));
    x3 = x + ((px3 * cosine) + (py3 * sine));
    y3 = y - ((py3 * cosine) - (px3 * sine));
/* computing min-max X and Y */
    minx = x0;
    miny = y0;
    maxx = x0;
    maxy = y0;
    if (x1 < minx)
	minx = x1;
    if (y1 < miny)
	miny = y1;
    if (x1 > maxx)
	maxx = x1;
    if (y1 > maxy)
	maxy = y1;
    if (x2 < minx)
	minx = x2;
    if (y2 < miny)
	miny = y2;
    if (x2 > maxx)
	maxx = x2;
    if (y2 > maxy)
	maxy = y2;
    if (x3 < minx)
	minx = x3;
    if (y3 < miny)
	miny = y3;
    if (x3 > maxx)
	maxx = x3;
    if (y3 > maxy)
	maxy = y3;
/* building the BLOB geometry */
    bbox.blob =
	do_create_label_mbr (minx, miny, maxx, maxy, x0, y0, x1, y1, x2, y2, x3,
			     y3, &blob_size);
    bbox.blob_size = blob_size;

    if (!pre_checked)
      {
	  /* handling the anti-collision list */
	  ret = do_eval_collision (labeling, stmt, &bbox);
	  if (ret)
	    {
		free (bbox.blob);
		return 1;	/* collision detected */
	    }
      }

    if (add_to_list)
      {
	  /* updating the list of obstacles */
	  ptr = malloc (sizeof (struct rl2_label_rect));
	  ptr->blob = bbox.blob;
	  ptr->blob_size = bbox.blob_size;
	  ptr->next = NULL;
	  if (labeling->first_rect == NULL)
	      labeling->first_rect = ptr;
	  if (labeling->last_rect != NULL)
	      labeling->last_rect->next = ptr;
	  labeling->last_rect = ptr;
      }
    else
	free (bbox.blob);
    return 0;
}

static int
draw_text_common (rl2GraphicsContextPtr context, const char *text,
		  double x, double y, double angle, double anchor_point_x,
		  double anchor_point_y, int pre_checked)
{
/* drawing a text string (common implementation) */
    double rads;
    double pre_x;
    double pre_y;
    double width;
    double height;
    double post_x;
    double post_y;
    double center_x;
    double center_y;
    double cx;
    double cy;
    cairo_t *cairo;
    int anti_collision = 0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (ctx->labeling == NULL)
	return 0;
    anti_collision = ctx->labeling->no_colliding_labels;
    if (text == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    rl2_graph_get_text_extent (ctx, text, &pre_x, &pre_y, &width, &height,
			       &post_x, &post_y);
    if (anti_collision)
      {
	  int ret;
	  int real_intersection;
	  sqlite3_stmt *stmt;
	  sqlite3 *sqlite = ctx->labeling->sqlite;
	  const char *sql = "SELECT ST_Intersects(?, ?)";
	  ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
	  if (ret != SQLITE_OK)
	      return 0;
	  real_intersection =
	      do_check_collision (ctx->labeling, stmt, x, y, angle,
				  anchor_point_x, anchor_point_y, pre_x,
				  pre_y, width, height, post_x, post_y,
				  1, pre_checked);
	  sqlite3_finalize (stmt);
	  if (real_intersection)
	      return 1;
      }

/* setting the Anchor Point */
    if (anchor_point_x < 0.0 || anchor_point_x > 1.0 || anchor_point_x == 0.5)
	center_x = width / 2.0;
    else
	center_x = width * anchor_point_x;
    if (anchor_point_y < 0.0 || anchor_point_y > 1.0 || anchor_point_y == 0.5)
	center_y = height / 2.0;
    else
	center_y = height * anchor_point_y;
    cx = 0.0 - center_x;
    cy = 0.0 + center_y;

    cairo_save (cairo);
    cairo_translate (cairo, x, y);
    rads = angle * .0174532925199432958;
    cairo_rotate (cairo, rads);
    if (ctx->with_font_halo)
      {
	  /* font with Halo */
	  cairo_move_to (cairo, cx, cy);
	  cairo_text_path (cairo, text);
	  cairo_set_source_rgba (cairo, ctx->font_red, ctx->font_green,
				 ctx->font_blue, ctx->font_alpha);
	  cairo_fill_preserve (cairo);
	  cairo_set_source_rgba (cairo, ctx->halo_red, ctx->halo_green,
				 ctx->halo_blue, ctx->halo_alpha);
	  cairo_set_line_width (cairo, ctx->halo_radius);
	  cairo_stroke (cairo);
      }
    else
      {
	  /* no Halo */
	  cairo_set_source_rgba (cairo, ctx->font_red, ctx->font_green,
				 ctx->font_blue, ctx->font_alpha);
	  cairo_move_to (cairo, cx, cy);
	  cairo_show_text (cairo, text);
      }
    cairo_restore (cairo);
    return 1;
}

RL2_DECLARE int
rl2_graph_draw_text (rl2GraphicsContextPtr context, const char *text,
		     double x, double y, double angle, double anchor_point_x,
		     double anchor_point_y)
{
/* drawing a text string (using the current font) */
    return draw_text_common (context, text, x, y, angle, anchor_point_x,
			     anchor_point_y, 0);
}

RL2_DECLARE int
rl2_graph_draw_prechecked_text (rl2GraphicsContextPtr context, const char *text,
				double x, double y, double angle,
				double anchor_point_x, double anchor_point_y)
{
/* drawing a text string (using the current font) */
    return draw_text_common (context, text, x, y, angle, anchor_point_x,
			     anchor_point_y, 1);
}

RL2_PRIVATE int
rl2_pre_check_collision (const void *context, double xl,
			 double yl, const char *label_left,
			 double xr, double yr,
			 const char *label_right,
			 double angle,
			 double anchor_point_x, double anchor_point_y)
{
/* anticipating label collision check for labels split in two lines */
    double pre_x;
    double pre_y;
    double width;
    double height;
    double post_x;
    double post_y;
    int ret;
    int real_intersection;
    sqlite3_stmt *stmt;
    sqlite3 *sqlite;
    const char *sql = "SELECT ST_Intersects(?, ?)";
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (ctx->labeling == NULL)
	return 1;
    if (ctx->labeling->no_colliding_labels == 0)
	return 1;
    if (label_left == NULL)
	return 1;
    sqlite = ctx->labeling->sqlite;

    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return 0;

/* testing the left half - first line of the label */
    rl2_graph_get_text_extent (ctx, label_left, &pre_x, &pre_y, &width, &height,
			       &post_x, &post_y);
    real_intersection =
	do_check_collision (ctx->labeling, stmt, xl, yl, angle,
			    anchor_point_x, anchor_point_y, pre_x, pre_y,
			    width, height, post_x, post_y, 0, 0);
    if (real_intersection)
	goto colliding;

    if (label_right != NULL)
      {
	  /* testing the right half - second line of the label */
	  rl2_graph_get_text_extent (ctx, label_right, &pre_x, &pre_y, &width,
				     &height, &post_x, &post_y);
	  real_intersection =
	      do_check_collision (ctx->labeling, stmt, xr, yr, angle,
				  anchor_point_x, anchor_point_y, pre_x, pre_y,
				  width, height, post_x, post_y, 0, 0);
	  if (real_intersection)
	      goto colliding;
      }

    sqlite3_finalize (stmt);
    return 1;

  colliding:
    sqlite3_finalize (stmt);
    return 0;
}

static double
do_estimate_char_radius (double font_width, double font_height)
{
/* estimating the mean size of a character */
    double factor = 3.0;	/* heuristic values for scale adjusting */
    double radius = sqrt ((font_width * font_width) +
			  (font_height * font_height)) / factor;
    return radius;
}

RL2_PRIVATE void
rl2_estimate_text_length (void *context, const char *text, double *length,
			  double *extra)
{
/* estimating the text length */
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    cairo_t *cairo;
    const char *p = text;
    int count = 0;
    double radius = 0.0;
    cairo_font_extents_t extents;

    *length = 0.0;
    *extra = 0.0;

    if (ctx == NULL)
	return;
    if (text == NULL)
	return;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    while (*p++ != '\0')
	count++;
    cairo_font_extents (cairo, &extents);
    radius = do_estimate_char_radius (extents.max_x_advance, extents.height);
    *length = radius * count;
    *extra = radius;
}

RL2_PRIVATE void
rl2_estimate_text_length_height (void *context, const char *text,
				 double *length, double *extra, double *height)
{
/* estimating the text length */
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    cairo_t *cairo;
    const char *p = text;
    int count = 0;
    double radius = 0.0;
    cairo_font_extents_t extents;

    *length = 0.0;
    *extra = 0.0;
    *height = 0.0;

    if (ctx == NULL)
	return;
    if (text == NULL)
	return;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    while (*p++ != '\0')
	count++;
    cairo_font_extents (cairo, &extents);
    radius = do_estimate_char_radius (extents.max_x_advance, extents.height);
    *length = radius * count;
    *extra = radius;
    *height = extents.height;
}

static void
do_estimate_text_length (cairo_t * cairo, const char *text, double *length,
			 double *extra)
{
/* estimating the text length */
    const char *p = text;
    int count = 0;
    double radius = 0.0;
    cairo_font_extents_t extents;

    while (*p++ != '\0')
	count++;
    cairo_font_extents (cairo, &extents);
    radius = do_estimate_char_radius (extents.max_x_advance, extents.height);
    *length = radius * count;
    *extra = radius;
}

static void
get_aux_start_point (rl2GeometryPtr geom, double *x, double *y)
{
/* extracting the first point from a Curve */
    double x0;
    double y0;
    rl2LinestringPtr ln = geom->first_linestring;
    rl2GetPoint (ln->coords, 0, &x0, &y0);
    *x = x0;
    *y = y0;
}

static int
get_aux_interception_point (sqlite3 * handle, rl2GeometryPtr geom,
			    rl2GeometryPtr circle, double *x, double *y)
{
/* computing an interception point */
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int ret;
    unsigned char *blob1;
    int size1;
    unsigned char *blob2;
    int size2;
    int ok = 0;

    rl2_serialize_linestring (geom->first_linestring, &blob1, &size1);
    rl2_serialize_linestring (circle->first_linestring, &blob2, &size2);

/* preparing the SQL query statement */
    sql = "SELECT ST_Intersection(?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob1, size1, free);
    sqlite3_bind_blob (stmt, 2, blob2, size2, free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob3 =
			  sqlite3_column_blob (stmt, 0);
		      int size3 = sqlite3_column_bytes (stmt, 0);
		      rl2GeometryPtr result =
			  rl2_geometry_from_blob (blob3, size3);
		      if (result == NULL)
			  break;
		      if (result->first_point == NULL)
			  break;
		      *x = result->first_point->x;
		      *y = result->first_point->y;
		      rl2_destroy_geometry (result);
		      ok = 1;
		  }
	    }
	  else
	      goto error;
      }
    if (ok == 0)
	goto error;
    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
aux_is_discarded_portion (rl2LinestringPtr ln, double x, double y)
{
/* attempting to identify the already processed portion */
    double xx;
    double yy;
    rl2GetPoint (ln->coords, 0, &xx, &yy);
    if (xx == x && yy == y)
	return 1;
    rl2GetPoint (ln->coords, ln->points - 1, &xx, &yy);
    if (xx == x && yy == y)
	return 1;
    return 0;
}

static rl2GeometryPtr
aux_reduce_curve (sqlite3 * handle, rl2GeometryPtr geom,
		  rl2GeometryPtr circle, double x, double y)
{
/* reducing a Curve by discarding the alreasdy processed portion */
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int ret;
    unsigned char *blob1;
    int size1;
    unsigned char *blob2;
    int size2;
    rl2GeometryPtr out = NULL;
    rl2LinestringPtr ln;
    int count = 0;

    rl2_serialize_linestring (geom->first_linestring, &blob1, &size1);
    rl2_serialize_linestring (circle->first_linestring, &blob2, &size2);

/* preparing the SQL query statement */
    sql = "SELECT ST_Split(?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob1, size1, free);
    sqlite3_bind_blob (stmt, 2, blob2, size2, free);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob3 =
			  sqlite3_column_blob (stmt, 0);
		      int size3 = sqlite3_column_bytes (stmt, 0);
		      rl2GeometryPtr result =
			  rl2_geometry_from_blob (blob3, size3);
		      if (result == NULL)
			  break;
		      ln = result->first_linestring;
		      while (ln != NULL)
			{
			    if (aux_is_discarded_portion (ln, x, y))
				;
			    else
			      {
				  if (out != NULL)
				      rl2_destroy_geometry (out);
				  out = rl2_clone_linestring (ln);
				  count++;
			      }
			    ln = ln->next;
			}
		      rl2_destroy_geometry (result);
		  }
	    }
	  else
	      goto error;
      }
    if (out == NULL || count != 1)
	goto error;
    sqlite3_finalize (stmt);
    return out;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (out != NULL)
	rl2_destroy_geometry (out);
    return NULL;
}

static int
interpolate_point (sqlite3 * handle, rl2GeometryPtr geom, double percent,
		   double *x, double *y)
{
/* interpolating a point on a Line */
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int ret;
    unsigned char *blob;
    int size;
    int ok = 0;

    rl2_serialize_linestring (geom->first_linestring, &blob, &size);

/* preparing the SQL query statement */
    sql = "SELECT ST_Line_Interpolate_Point(?, ?)";
    ret = sqlite3_prepare_v2 (handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, size, free);
    sqlite3_bind_double (stmt, 2, percent);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      const unsigned char *blob2 =
			  sqlite3_column_blob (stmt, 0);
		      int size2 = sqlite3_column_bytes (stmt, 0);
		      rl2GeometryPtr result =
			  rl2_geometry_from_blob (blob2, size2);
		      if (result == NULL)
			  break;
		      if (result->first_point == NULL)
			  break;
		      *x = result->first_point->x;
		      *y = result->first_point->y;
		      rl2_destroy_geometry (result);
		      ok = 1;
		  }
	    }
	  else
	      goto error;
      }
    if (ok == 0)
	goto error;
    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static int
check_reverse (sqlite3 * sqlite, rl2GeometryPtr geom, double text_length)
{
/* testing for an inverse label */
    rl2LinestringPtr ln;
    double x0;
    double y0;
    double x1;
    double y1;
    double width;
    double line_length;
    double percent;

    if (geom == NULL)
	return 0;
    ln = geom->first_linestring;
    if (ln == NULL)
	return 0;

    line_length = rl2_compute_curve_length (geom);
    if (line_length < text_length)
	return 0;
    percent = text_length / line_length;
    if (!interpolate_point (sqlite, geom, percent, &x1, &y1))
	return 0;

    rl2GetPoint (ln->coords, 0, &x0, &y0);
    width = fabs (x0 - x1);
    if (width > 10.0)
      {
	  if (x0 > x1)
	      return 1;
      }
    else
      {
	  if (y0 > y1)
	      return 1;
      }
    return 0;
}

static rl2GeometryPtr
rl2_draw_wrapped_label (sqlite3 * handle, rl2GraphicsContextPtr context,
			cairo_t * cairo, const char *text, rl2GeometryPtr geom)
{
/* placing each character along the modelling line */
    double x0;
    double y0;
    double x1;
    double y1;
    double radius;
    double m;
    double rads;
    double angle;
    int i;
    rl2GeometryPtr g2;
    rl2GeometryPtr g;
    rl2GeometryPtr circle;
    cairo_font_extents_t extents;
    struct aux_utf8_text *utf8;
    int collision = 0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    int anti_collision = 0;
    if (ctx == NULL)
	return NULL;
    if (ctx->labeling == NULL)
	return NULL;
    anti_collision = ctx->labeling->no_colliding_labels;

    if (text == NULL)
	return NULL;
    if (strlen (text) == 0)
	return NULL;
    if (geom == NULL)
	return NULL;
    g = rl2_clone_curve (geom);
    if (g == NULL)
	return NULL;
    utf8 = create_aux_utf8_text (text);

    cairo_font_extents (cairo, &extents);
    radius = do_estimate_char_radius (extents.max_x_advance, extents.height);

    if (check_reverse (handle, g, radius * strlen (text)))
      {
	  /* reverse text */
	  reverse_aux_utf8_text (utf8);
      }

    if (anti_collision)
      {
	  /* pre-checking for eventual collisions */
	  int ret;
	  rl2GeometryPtr auxg = rl2_clone_curve (geom);
	  if (auxg == NULL)
	      goto end_collision;
	  for (i = 0; i < utf8->n_chars; i++)
	    {
		/* testing all characters one at each time */
		if (auxg == NULL)
		    break;
		get_aux_start_point (auxg, &x0, &y0);
		circle = rl2_build_circle (x0, y0, radius);
		if (!get_aux_interception_point
		    (handle, auxg, circle, &x1, &y1))
		  {
		      rl2_destroy_geometry (circle);
		      rl2_destroy_geometry (auxg);
		      auxg = NULL;
		      break;
		  }
		m = (y1 - y0) / (x1 - x0);
		rads = atan (m);
		angle = rads / .0174532925199432958;
		if (x1 < x0 && utf8->is_reverse == 0)
		    angle += 180.0;
		ret =
		    rl2_pre_check_collision (context, x0, y0,
					     *(utf8->chars + i), 0.0, 0.0, NULL,
					     angle, 0.5, 0.5);
		g2 = aux_reduce_curve (handle, auxg, circle, x0, y0);
		rl2_destroy_geometry (circle);
		rl2_destroy_geometry (auxg);
		auxg = g2;
		if (!ret)
		  {
		      /* at least a character collides */
		      collision = 1;
		      break;
		  }
	    }
	end_collision:
	  if (auxg != NULL)
	      rl2_destroy_geometry (auxg);
      }
    if (collision)
      {
	  /* a collision was detected: quitting */
	  destroy_aux_utf8_text (utf8);
	  if (g != NULL)
	      rl2_destroy_geometry (g);
	  return NULL;
      }

    for (i = 0; i < utf8->n_chars; i++)
      {
	  /* actually drawing the warped label */
	  if (g == NULL)
	      break;
	  get_aux_start_point (g, &x0, &y0);
	  circle = rl2_build_circle (x0, y0, radius);
	  if (!get_aux_interception_point (handle, g, circle, &x1, &y1))
	    {
		rl2_destroy_geometry (circle);
		rl2_destroy_geometry (g);
		g = NULL;
		break;
	    }
	  m = (y1 - y0) / (x1 - x0);
	  rads = atan (m);
	  angle = rads / .0174532925199432958;
	  if (x1 < x0 && utf8->is_reverse == 0)
	      angle += 180.0;
	  rl2_graph_draw_prechecked_text (context, *(utf8->chars + i), x0, y0,
					  angle, 0.5, 0.5);
	  g2 = aux_reduce_curve (handle, g, circle, x0, y0);
	  rl2_destroy_geometry (circle);
	  rl2_destroy_geometry (g);
	  g = g2;
      }
    destroy_aux_utf8_text (utf8);
    return g;
}

RL2_DECLARE int
rl2_graph_draw_warped_text (sqlite3 * handle, rl2GraphicsContextPtr context,
			    const char *text, int points, double *x,
			    double *y, double initial_gap, double gap,
			    int repeated)
{
/* drawing a text string warped along a modelling curve (using the current font) */
    double curve_len;
    double text_len;
    double extra_len;
    double start;
    double from;
    rl2GeometryPtr geom = NULL;
    rl2GeometryPtr geom2 = NULL;
    rl2GeometryPtr geom3 = NULL;
    cairo_t *cairo;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (text == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
	cairo = ctx->clip_cairo;
    else
	cairo = ctx->cairo;

    geom = rl2_curve_from_XY (points, x, y);
    if (geom == NULL)
	return 0;

    curve_len = rl2_compute_curve_length (geom);
    do_estimate_text_length (cairo, text, &text_len, &extra_len);
    if ((initial_gap + text_len + (2.0 * extra_len)) > curve_len)
      {
	  rl2_destroy_geometry (geom);
	  return 0;		/* not enough room to place the label */
      }

    if (repeated)
      {
	  /* repeated labels */
	  int first = 1;
	  geom3 = rl2_clone_linestring (geom->first_linestring);
	  while (geom3 != NULL)
	    {
		if (first)
		  {
		      start = initial_gap + extra_len;
		      first = 0;
		  }
		else
		    start = gap + extra_len;
		curve_len = rl2_compute_curve_length (geom3);
		if ((start + text_len + extra_len) > curve_len)
		    break;	/* not enough room to place the label */
		from = start / curve_len;
		/* extracting the sub-path modelling the label */
		geom2 = rl2_curve_substring (handle, geom3, from, 1.0);
		rl2_destroy_geometry (geom3);
		if (geom2 == NULL)
		    goto error;
		geom3 =
		    rl2_draw_wrapped_label (handle, context, cairo, text,
					    geom2);
		rl2_destroy_geometry (geom2);
	    }
	  rl2_destroy_geometry (geom3);
      }
    else
      {
	  /* single label */
	  start = (curve_len - text_len) / 2.0;
	  from = start / curve_len;
	  /* extracting the sub-path modelling the label */
	  geom2 = rl2_curve_substring (handle, geom, from, 1.0);
	  if (geom2 == NULL)
	      goto error;
	  geom3 = rl2_draw_wrapped_label (handle, context, cairo, text, geom2);
	  rl2_destroy_geometry (geom2);
	  if (geom3 != geom2)
	      rl2_destroy_geometry (geom3);
      }

    rl2_destroy_geometry (geom);
    return 1;

  error:
    rl2_destroy_geometry (geom);
    return 0;
}

RL2_DECLARE int
rl2_graph_draw_bitmap (rl2GraphicsContextPtr context,
		       rl2GraphicsBitmapPtr bitmap, double x, double y)
{
/* drawing a symbol bitmap */
    cairo_t *cairo;
    cairo_surface_t *surface;
    RL2GraphBitmapPtr bmp = (RL2GraphBitmapPtr) bitmap;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (bmp == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
      {
	  surface = ctx->clip_surface;
	  cairo = ctx->clip_cairo;
      }
    else
      {
	  surface = ctx->surface;
	  cairo = ctx->cairo;
      }

    cairo_save (cairo);
    cairo_scale (cairo, 1, 1);
    cairo_translate (cairo, x, y);
    cairo_set_source (cairo, bmp->pattern);
    cairo_rectangle (cairo, 0, 0, bmp->width, bmp->height);
    cairo_fill (cairo);
    cairo_restore (cairo);
    cairo_surface_flush (surface);
    return 1;
}

RL2_DECLARE int
rl2_graph_draw_rescaled_bitmap (rl2GraphicsContextPtr context,
				rl2GraphicsBitmapPtr bitmap, double scale_x,
				double scale_y, double x, double y)
{
/* drawing a rescaled bitmap */
    cairo_t *cairo;
    cairo_surface_t *surface;
    RL2GraphBitmapPtr bmp = (RL2GraphBitmapPtr) bitmap;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (bmp == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
      {
	  surface = ctx->clip_surface;
	  cairo = ctx->clip_cairo;
      }
    else
      {
	  surface = ctx->surface;
	  cairo = ctx->cairo;
      }

    cairo_save (cairo);
    cairo_translate (cairo, x, y);
    cairo_scale (cairo, scale_x, scale_y);
    cairo_set_source (cairo, bmp->pattern);
    cairo_paint (cairo);
    cairo_restore (cairo);
    cairo_surface_flush (surface);
    return 1;
}

RL2_DECLARE rl2AffineTransformDataPtr
rl2_create_affine_transform (double xx, double yx, double xy, double yy,
			     double xoff, double yoff, int max_threads)
{
/* creating an incomplete Affine Transform Data object */
    rl2PrivAffineTransformDataPtr ptr =
	malloc (sizeof (rl2PrivAffineTransformData));
    ptr->xx = xx;
    ptr->yx = yx;
    ptr->xy = xy;
    ptr->yy = yy;
    ptr->x_off = xoff;
    ptr->y_off = yoff;
    ptr->max_threads = max_threads;
    ptr->orig_ok = 0;
    ptr->dest_ok = 0;
    return (rl2AffineTransformDataPtr) ptr;
}

RL2_DECLARE void
rl2_destroy_affine_transform (rl2AffineTransformDataPtr xptr)
{
/* destroying an Affine Transform Data object */
    rl2PrivAffineTransformDataPtr ptr = (rl2PrivAffineTransformDataPtr) xptr;
    if (ptr == NULL)
	return;
    free (ptr);
}

RL2_DECLARE int
rl2_set_affine_transform_origin (rl2AffineTransformDataPtr xptr, int width,
				 int height, double minx, double miny,
				 double maxx, double maxy)
{
/* setting the Origin context */
    double ext_x = maxx - minx;
    double ext_y = maxy - miny;
    double x_res = ext_x / (double) width;
    double y_res = ext_y / (double) height;
    rl2PrivAffineTransformDataPtr ptr = (rl2PrivAffineTransformDataPtr) xptr;
    if (ptr == NULL)
	return 0;
    if (x_res <= 0.0 || y_res <= 0.0)
	return 0;
    ptr->orig_width = width;
    ptr->orig_height = height;
    ptr->orig_minx = minx;
    ptr->orig_miny = miny;
    ptr->orig_x_res = x_res;
    ptr->orig_y_res = y_res;
    ptr->orig_ok = 1;
    return 1;
}

RL2_DECLARE int
rl2_set_affine_transform_destination (rl2AffineTransformDataPtr xptr, int width,
				      int height, double minx, double miny,
				      double maxx, double maxy)
{
/* setting the Destination context */
    double ext_x = maxx - minx;
    double ext_y = maxy - miny;
    double x_res = ext_x / (double) width;
    double y_res = ext_y / (double) height;
    rl2PrivAffineTransformDataPtr ptr = (rl2PrivAffineTransformDataPtr) xptr;
    if (ptr == NULL)
	return 0;
    if (x_res <= 0.0 || y_res <= 0.0)
	return 0;
    ptr->dest_width = width;
    ptr->dest_height = height;
    ptr->dest_minx = minx;
    ptr->dest_miny = miny;
    ptr->dest_x_res = x_res;
    ptr->dest_y_res = y_res;
    ptr->dest_ok = 1;
    return 1;
}

RL2_DECLARE int
rl2_is_valid_affine_transform (rl2AffineTransformDataPtr xptr)
{
/* testing an Affine Transform Data object for validity */
    rl2PrivAffineTransformDataPtr ptr = (rl2PrivAffineTransformDataPtr) xptr;
    if (ptr == NULL)
	return 0;
    if (ptr->orig_ok && ptr->dest_ok)
	return 1;
    return 0;
}

#if defined(_WIN32) && !defined(__MINGW32__)
DWORD WINAPI
doRunTransformThread (void *arg)
#else
void *
doRunTransformThread (void *arg)
#endif
{
/* threaded function: Affine Transform */
    rl2TransformParamsPtr params = (rl2TransformParamsPtr) arg;
    int x;
    int y;
    unsigned char *p_in;
    unsigned char *p_out;
    rl2PrivAffineTransformDataPtr atm =
	(rl2PrivAffineTransformDataPtr) (params->at_data);
    RL2GraphBitmapPtr in = (RL2GraphBitmapPtr) (params->in);
    RL2GraphBitmapPtr out = (RL2GraphBitmapPtr) (params->out);
    for (y = params->base_row; y < atm->dest_height; y += params->row_incr)
      {
	  int y_rev = atm->dest_height - y - 1;
	  for (x = 0; x < atm->dest_width; x++)
	    {
		double x_out = atm->dest_minx + (atm->dest_x_res * (double) x);
		double y_out =
		    atm->dest_miny + (atm->dest_y_res * (double) y_rev);
		double x_in =
		    (atm->xx * x_out) + (atm->xy * y_out) + atm->x_off;
		double y_in =
		    (atm->yx * x_out) + (atm->yy * y_out) + atm->y_off;
		int x0 = (x_in - atm->orig_minx) / atm->orig_x_res;
		int y0 =
		    (atm->orig_height - 1) - (y_in -
					      atm->orig_miny) / atm->orig_y_res;
		if (x0 >= 0 && x0 < atm->orig_width && y0 >= 0
		    && y0 < atm->orig_height)
		  {
		      /* copying a transformed pixel */
		      p_in = in->rgba + (y0 * atm->orig_width * 4) + (x0 * 4);
		      p_out = out->rgba + (y * atm->dest_width * 4) + (x * 4);
		      *p_out++ = *p_in++;	/* red */
		      *p_out++ = *p_in++;	/* green */
		      *p_out++ = *p_in++;	/* blue */
		      *p_out++ = *p_in++;	/* alpha */
		  }
	    }
      }
#if defined(_WIN32) && !defined(__MINGW32__)
    return 0;
#else
    pthread_exit (NULL);
    return NULL;
#endif
}

static void
start_transform_thread (rl2TransformParamsPtr params)
{
/* starting a concurrent thread */
#if defined(_WIN32) && !defined(__MINGW32__)
    HANDLE thread_handle;
    HANDLE *p_thread;
    DWORD dwThreadId;
    thread_handle =
	CreateThread (NULL, 0, doRunTransformThread, params, 0, &dwThreadId);
    SetThreadPriority (thread_handle, THREAD_PRIORITY_IDLE);
    p_thread = malloc (sizeof (HANDLE));
    *p_thread = thread_handle;
    params->opaque_thread_id = p_thread;
#else
    pthread_t thread_id;
    pthread_t *p_thread;
    int ok_prior = 0;
    int policy;
    int min_prio;
    pthread_attr_t attr;
    struct sched_param sp;
    pthread_attr_init (&attr);
    if (pthread_attr_setschedpolicy (&attr, SCHED_RR) == 0)
      {
	  /* attempting to set the lowest priority */
	  if (pthread_attr_getschedpolicy (&attr, &policy) == 0)
	    {
		min_prio = sched_get_priority_min (policy);
		sp.sched_priority = min_prio;
		if (pthread_attr_setschedparam (&attr, &sp) == 0)
		  {
		      /* ok, setting the lowest priority */
		      ok_prior = 1;
		      pthread_create (&thread_id, &attr, doRunTransformThread,
				      params);
		  }
	    }
      }
    if (!ok_prior)
      {
	  /* failure: using standard priority */
	  pthread_create (&thread_id, NULL, doRunTransformThread, params);
      }
    p_thread = malloc (sizeof (pthread_t));
    *p_thread = thread_id;
    params->opaque_thread_id = p_thread;
#endif
}

static void
do_multi_thread_transform (rl2TransformParamsPtr params_array, int count)
{
/* applying the Affine Transform  - multi-thread */
    rl2TransformParamsPtr params;
    int i;
#if defined(_WIN32) && !defined(__MINGW32__)
    HANDLE *handles;
#endif

    for (i = 0; i < count; i++)
      {
	  /* starting all children threads */
	  params = params_array + i;
	  start_transform_thread (params);
      }

/* waiting until all child threads exit */
#if defined(_WIN32) && !defined(__MINGW32__)
    handles = malloc (sizeof (HANDLE) * count);
    for (i = 0; i < count; i++)
      {
	  /* initializing the HANDLEs array */
	  HANDLE *pOpaque;
	  params = params_array + i;
	  pOpaque = (HANDLE *) (params->opaque_thread_id);
	  *(handles + i) = *pOpaque;
      }
    WaitForMultipleObjects (count, handles, TRUE, INFINITE);
    free (handles);
#else
    for (i = 0; i < count; i++)
      {
	  pthread_t *pOpaque;
	  params = params_array + i;
	  pOpaque = (pthread_t *) (params->opaque_thread_id);
	  pthread_join (*pOpaque, NULL);
      }
#endif

/* all children threads have now finished: resuming the main thread */
    for (i = 0; i < count; i++)
      {
	  params = params_array + i;
	  params->at_data = NULL;
	  params->in = NULL;
	  params->out = NULL;
	  if (params->opaque_thread_id != NULL)
	      free (params->opaque_thread_id);
	  params->opaque_thread_id = NULL;
      }
}

static void
do_mono_thread_transform (rl2TransformParamsPtr params)
{
/* applying the Affine Transform  - single thread */
    int x;
    int y;
    unsigned char *p_in;
    unsigned char *p_out;
    rl2PrivAffineTransformDataPtr atm =
	(rl2PrivAffineTransformDataPtr) (params->at_data);
    RL2GraphBitmapPtr in = (RL2GraphBitmapPtr) (params->in);
    RL2GraphBitmapPtr out = (RL2GraphBitmapPtr) (params->out);
    for (y = 0; y < atm->dest_height; y++)
      {
	  int y_rev = atm->dest_height - y - 1;
	  for (x = 0; x < atm->dest_width; x++)
	    {
		double x_out = atm->dest_minx + (atm->dest_x_res * (double) x);
		double y_out =
		    atm->dest_miny + (atm->dest_y_res * (double) y_rev);
		double x_in =
		    (atm->xx * x_out) + (atm->xy * y_out) + atm->x_off;
		double y_in =
		    (atm->yx * x_out) + (atm->yy * y_out) + atm->y_off;
		int x0 = (x_in - atm->orig_minx) / atm->orig_x_res;
		int y0 =
		    (atm->orig_height - 1) - (y_in -
					      atm->orig_miny) / atm->orig_y_res;
		if (x0 >= 0 && x0 < atm->orig_width && y0 >= 0
		    && y0 < atm->orig_height)
		  {
		      /* copying a transformed pixel */
		      p_in = in->rgba + (y0 * atm->orig_width * 4) + (x0 * 4);
		      p_out = out->rgba + (y * atm->dest_width * 4) + (x * 4);
		      *p_out++ = *p_in++;	/* red */
		      *p_out++ = *p_in++;	/* green */
		      *p_out++ = *p_in++;	/* blue */
		      *p_out++ = *p_in++;	/* alpha */
		  }
	    }
      }
}

RL2_DECLARE int
rl2_transform_bitmap (rl2AffineTransformDataPtr at_data,
		      rl2GraphicsBitmapPtr * bitmap)
{
/* creating a new Graphics Bitmap object by applying an Affine Transform */
    rl2TransformParamsPtr params_array = NULL;
    rl2TransformParamsPtr params;
    int ipar;
    int x;
    int y;
    int max_threads;
    unsigned char *p_out;
    unsigned char *rgba = NULL;
    int rgba_sz;
    rl2GraphicsBitmapPtr out_bitmap = NULL;
    rl2PrivAffineTransformDataPtr atm = (rl2PrivAffineTransformDataPtr) at_data;
    RL2GraphBitmapPtr in = *((RL2GraphBitmapPtr *) bitmap);
    RL2GraphBitmapPtr out;
    if (atm == NULL)
	goto error;
    if (!rl2_is_valid_affine_transform (at_data))
	goto error;
    if (in == NULL)
	goto error;
    if (in->width != atm->orig_width || in->height != atm->orig_height)
	goto error;

/* creating the output bitmap */
    rgba_sz = atm->dest_width * atm->dest_height * 4;
    rgba = malloc (rgba_sz);
    if (rgba == NULL)
	goto error;
    p_out = rgba;
    for (y = 0; y < atm->dest_height; y++)
      {
	  for (x = 0; x < atm->dest_width; x++)
	    {
		/* priming a transparent background */
		*p_out++ = 0;	/* red */
		*p_out++ = 0;	/* green */
		*p_out++ = 0;	/* blue */
		*p_out++ = 0;	/* alpha */
	    }
      }
    out_bitmap =
	rl2_graph_create_bitmap (rgba, atm->dest_width, atm->dest_height);
    if (out_bitmap == NULL)
	goto error;

    out = (RL2GraphBitmapPtr) out_bitmap;

    max_threads = atm->max_threads;
    if (max_threads < 1)
	max_threads = 1;
    if (max_threads > 64)
	max_threads = 64;
/* allocating the Transform Params array */
    params_array = malloc (sizeof (rl2TransformParams) * max_threads);
    if (params_array == NULL)
	goto error;
    for (ipar = 0; ipar < max_threads; ipar++)
      {
	  /* initializing Transform Params */
	  params = params_array + ipar;
	  params->at_data = at_data;
	  params->in = in;
	  params->out = out;
	  params->opaque_thread_id = NULL;
	  params->base_row = ipar;
	  params->row_incr = max_threads;
      }
    if (max_threads > 1)
      {
	  /* adopting a multithreaded strategy */
	  do_multi_thread_transform (params_array, max_threads);
      }
    else
      {
	  /* single thread execution */
	  do_mono_thread_transform (params);
      }
    free (params_array);
    rl2_graph_destroy_bitmap (in);
    *bitmap = out_bitmap;
    return 1;

  error:
    if (params_array != NULL)
	free (params_array);
    if (in != NULL)
	rl2_graph_destroy_bitmap (in);
    if (out_bitmap != NULL)
	rl2_graph_destroy_bitmap (out_bitmap);
    else
      {
	  if (rgba != NULL)
	      free (rgba);
      }
    *bitmap = NULL;
    return 0;
}

RL2_DECLARE int
rl2_rescale_pixbuf (const unsigned char *inbuf, unsigned int inwidth,
		    unsigned int inheight, unsigned char pixtype,
		    unsigned char *outbuf, unsigned int outwidth,
		    unsigned int outheight)
{
/* drawing a rescaled pixbufx (RGB or GRAYSCALE) */
    unsigned char *pixbuf = NULL;
    int bufsz;
    const unsigned char *p_in;
    unsigned char *p_out;
    unsigned int x;
    unsigned int y;
    int stride;
    double scale_x = (double) outwidth / (double) inwidth;
    double scale_y = (double) outheight / (double) inheight;
    int little_endian = rl2cr_endian_arch ();
    cairo_t *cairo;
    cairo_surface_t *surface;
    cairo_surface_t *bitmap;
    cairo_pattern_t *pattern;
    if (pixtype != RL2_PIXEL_RGB && pixtype != RL2_PIXEL_GRAYSCALE)
	return 0;

/* creating a Cairo context */
    surface =
	cairo_image_surface_create (CAIRO_FORMAT_ARGB32, outwidth, outheight);
    if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    cairo = cairo_create (surface);
    if (cairo_status (cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* allocating and populating data for Cairo */
    stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, inwidth);
    bufsz = stride * inheight;
    pixbuf = malloc (bufsz);
    if (pixbuf == NULL)
      {
	  goto error2;
      }
    p_in = inbuf;
    p_out = pixbuf;
    for (y = 0; y < inheight; y++)
      {
	  for (x = 0; x < inwidth; x++)
	    {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		if (pixtype == RL2_PIXEL_RGB)
		  {
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		  }
		else
		  {
		      r = *p_in++;
		      g = r;
		      b = r;
		  }
		if (little_endian)
		  {
		      *p_out++ = b;
		      *p_out++ = g;
		      *p_out++ = r;
		      *p_out++ = 0xff;
		  }
		else
		  {
		      *p_out++ = 0xff;
		      *p_out++ = r;
		      *p_out++ = g;
		      *p_out++ = b;
		  }
	    }
      }

/* creating the input pattern */
    bitmap =
	cairo_image_surface_create_for_data (pixbuf, CAIRO_FORMAT_ARGB32,
					     inwidth, inheight, stride);
    pattern = cairo_pattern_create_for_surface (bitmap);
    cairo_pattern_set_extend (pattern, CAIRO_EXTEND_NONE);

/* rescaling the image */
    cairo_save (cairo);
    cairo_scale (cairo, scale_x, scale_y);
    cairo_set_source (cairo, pattern);
    cairo_paint (cairo);
    cairo_restore (cairo);
    cairo_surface_flush (surface);

/* cleaning up the input pattern */
    cairo_pattern_destroy (pattern);
    cairo_surface_destroy (bitmap);
    free (pixbuf);

/* exporting the rescaled image */
    p_in = cairo_image_surface_get_data (surface);
    p_out = outbuf;
    for (y = 0; y < outheight; y++)
      {
	  for (x = 0; x < outwidth; x++)
	    {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
		if (little_endian)
		  {
		      b = *p_in++;
		      g = *p_in++;
		      r = *p_in++;
		      a = *p_in++;
		  }
		else
		  {
		      a = *p_in++;
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		  }
		if (pixtype == RL2_PIXEL_RGB)
		  {
		      *p_out++ = unpremultiply (r, a);
		      *p_out++ = unpremultiply (g, a);
		      *p_out++ = unpremultiply (b, a);

		  }
		else
		    *p_out++ = unpremultiply (r, a);
	    }
      }

/* destroying the Cairo context */
    cairo_destroy (cairo);
    cairo_surface_destroy (surface);
    return 1;

  error2:
    if (pixbuf != NULL)
	free (pixbuf);
    cairo_destroy (cairo);
    cairo_surface_destroy (surface);
    return 0;
  error1:
    cairo_surface_destroy (surface);
    return 0;
}

RL2_DECLARE int
rl2_rescale_pixbuf_transparent (const unsigned char *inbuf,
				const unsigned char *inmask,
				unsigned int inwidth, unsigned int inheight,
				unsigned char pixtype, unsigned char *outbuf,
				unsigned char *outmask, unsigned int outwidth,
				unsigned int outheight)
{
/* drawing a rescaled pixbufx (RGB or GRAYSCALE) */
    unsigned char *pixbuf = NULL;
    int bufsz;
    const unsigned char *p_in;
    const unsigned char *p_msk;
    unsigned char *p_out;
    unsigned char *p_outmsk;
    unsigned int x;
    unsigned int y;
    int stride;
    double scale_x = (double) outwidth / (double) inwidth;
    double scale_y = (double) outheight / (double) inheight;
    int little_endian = rl2cr_endian_arch ();
    cairo_t *cairo;
    cairo_surface_t *surface;
    cairo_surface_t *bitmap;
    cairo_pattern_t *pattern;
    if (pixtype != RL2_PIXEL_RGB && pixtype != RL2_PIXEL_GRAYSCALE)
	return 0;

/* creating a Cairo context */
    surface =
	cairo_image_surface_create (CAIRO_FORMAT_ARGB32, outwidth, outheight);
    if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    cairo = cairo_create (surface);
    if (cairo_status (cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* allocating and populating data for Cairo */
    stride = cairo_format_stride_for_width (CAIRO_FORMAT_ARGB32, inwidth);
    bufsz = stride * inheight;
    pixbuf = malloc (bufsz);
    if (pixbuf == NULL)
      {
	  goto error2;
      }
    p_in = inbuf;
    p_msk = inmask;
    p_out = pixbuf;
    for (y = 0; y < inheight; y++)
      {
	  for (x = 0; x < inwidth; x++)
	    {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		int transparent;
		if (pixtype == RL2_PIXEL_RGB)
		  {
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		  }
		else
		  {
		      r = *p_in++;
		      g = r;
		      b = r;
		  }
		transparent = 0;
		if (*p_msk++ != 0)
		    transparent = 1;
		if (little_endian)
		  {
		      *p_out++ = b;
		      *p_out++ = g;
		      *p_out++ = r;
		      if (transparent)
			  *p_out++ = 0x00;
		      else
			  *p_out++ = 0xff;
		  }
		else
		  {
		      if (transparent)
			  *p_out++ = 0x00;
		      else
			  *p_out++ = 0xff;
		      *p_out++ = r;
		      *p_out++ = g;
		      *p_out++ = b;
		  }
	    }
      }

/* creating the input pattern */
    bitmap =
	cairo_image_surface_create_for_data (pixbuf, CAIRO_FORMAT_ARGB32,
					     inwidth, inheight, stride);
    pattern = cairo_pattern_create_for_surface (bitmap);
    cairo_pattern_set_extend (pattern, CAIRO_EXTEND_NONE);

/* rescaling the image */
    cairo_save (cairo);
    cairo_scale (cairo, scale_x, scale_y);
    cairo_set_source (cairo, pattern);
    cairo_paint (cairo);
    cairo_restore (cairo);
    cairo_surface_flush (surface);

/* cleaning up the input pattern */
    cairo_pattern_destroy (pattern);
    cairo_surface_destroy (bitmap);
    free (pixbuf);

/* exporting the rescaled image */
    p_in = cairo_image_surface_get_data (surface);
    p_out = outbuf;
    p_outmsk = outmask;
    for (y = 0; y < outheight; y++)
      {
	  for (x = 0; x < outwidth; x++)
	    {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
		if (little_endian)
		  {
		      b = *p_in++;
		      g = *p_in++;
		      r = *p_in++;
		      a = *p_in++;
		  }
		else
		  {
		      a = *p_in++;
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		  }
		if (pixtype == RL2_PIXEL_RGB)
		  {
		      *p_out++ = unpremultiply (r, a);
		      *p_out++ = unpremultiply (g, a);
		      *p_out++ = unpremultiply (b, a);

		  }
		else
		    *p_out++ = unpremultiply (r, a);
		if (a != 0)
		    *p_outmsk++ = 0;
		else
		    *p_outmsk++ = 1;
	    }
      }

/* destroying the Cairo context */
    cairo_destroy (cairo);
    cairo_surface_destroy (surface);
    return 1;

  error2:
    if (pixbuf != NULL)
	free (pixbuf);
    cairo_destroy (cairo);
    cairo_surface_destroy (surface);
    return 0;
  error1:
    cairo_surface_destroy (surface);
    return 0;
}

RL2_DECLARE int
rl2_graph_draw_graphic_symbol (rl2GraphicsContextPtr context,
			       rl2GraphicsPatternPtr symbol, double width,
			       double height, double x,
			       double y, double angle,
			       double anchor_point_x, double anchor_point_y)
{
/* drawing a Graphic Symbol */
    double rads;
    double scale_x;
    double scale_y;
    double center_x;
    double center_y;
    cairo_t *cairo;
    cairo_surface_t *surface;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    RL2PrivGraphPatternPtr pattern = (RL2PrivGraphPatternPtr) symbol;

    if (ctx == NULL)
	return 0;
    if (pattern == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
      {
	  surface = ctx->clip_surface;
	  cairo = ctx->clip_cairo;
      }
    else
      {
	  surface = ctx->surface;
	  cairo = ctx->cairo;
      }

/* setting the Anchor Point */
    scale_x = width / (double) (pattern->width);
    scale_y = height / (double) (pattern->height);
    if (anchor_point_x < 0.0 || anchor_point_x > 1.0 || anchor_point_x == 0.5)
	center_x = (double) (pattern->width) / 2.0;
    else
	center_x = (double) (pattern->width) * anchor_point_x;
    if (anchor_point_y < 0.0 || anchor_point_y > 1.0 || anchor_point_y == 0.5)
	center_y = (double) (pattern->height) / 2.0;
    else
	center_y = (double) (pattern->height) * anchor_point_y;

    cairo_save (cairo);
    cairo_translate (cairo, x, y);
    cairo_scale (cairo, scale_x, scale_y);
    rads = angle * .0174532925199432958;
    cairo_rotate (cairo, rads);
    cairo_translate (cairo, 0.0 - center_x, 0.0 - center_y);
    cairo_set_source (cairo, pattern->pattern);
    cairo_paint (cairo);
    cairo_restore (cairo);
    cairo_surface_flush (surface);

    return 1;
}

RL2_DECLARE int
rl2_graph_draw_mark_symbol (rl2GraphicsContextPtr context, int mark_type,
			    double size,
			    double x, double y,
			    double angle,
			    double anchor_point_x,
			    double anchor_point_y, int fill, int stroke)
{
/* drawing a Mark Symbol */
    double xsize;
    double size2 = size / 2.0;
    double size4 = size / 4.0;
    double size6 = size / 6.0;
    double size8 = size / 8.0;
    double size13 = size / 3.0;
    double size23 = (size / 3.0) * 2.0;
    int i;
    double rads;
    double center_x;
    double center_y;
    cairo_t *cairo;
    cairo_surface_t *surface;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return 0;
    if (ctx->type == RL2_SURFACE_PDF)
      {
	  surface = ctx->clip_surface;
	  cairo = ctx->clip_cairo;
      }
    else
      {
	  surface = ctx->surface;
	  cairo = ctx->cairo;
      }
    cairo_save (cairo);
    cairo_translate (cairo, x, y);
    rads = angle * .0174532925199432958;
    cairo_rotate (cairo, rads);

/* setting the Anchor Point */
    xsize = size;
    if (mark_type == RL2_GRAPHIC_MARK_CIRCLE
	|| mark_type == RL2_GRAPHIC_MARK_TRIANGLE
	|| mark_type == RL2_GRAPHIC_MARK_STAR)
	xsize = size23 * 2.0;
    if (anchor_point_x < 0.0 || anchor_point_x > 1.0 || anchor_point_x == 0.5)
	center_x = 0.0;
    else
	center_x = 0.0 + (xsize / 2.0) - (xsize * anchor_point_x);
    if (anchor_point_y < 0.0 || anchor_point_y > 1.0 || anchor_point_y == 0.5)
	center_y = 0.0;
    else
	center_y = 0.0 - (xsize / 2.0) + (xsize * anchor_point_y);
    x = center_x;
    y = center_y;
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
		    rl2_graph_move_to_point (ctx, cx, cy);
		else
		    rl2_graph_add_line_to_path (ctx, cx, cy);
		rads += tic;
	    }
	  rl2_graph_close_subpath (ctx);
	  break;
      case RL2_GRAPHIC_MARK_TRIANGLE:
	  rads = 0.0;
	  for (i = 0; i < 3; i++)
	    {
		double tic = 6.28318530718 / 3.0;
		double cx = x + (size23 * sin (rads));
		double cy = y + (size23 * cos (rads));
		if (i == 0)
		    rl2_graph_move_to_point (ctx, cx, cy);
		else
		    rl2_graph_add_line_to_path (ctx, cx, cy);
		rads += tic;
	    }
	  rl2_graph_close_subpath (ctx);
	  break;
      case RL2_GRAPHIC_MARK_STAR:
	  rads = 3.14159265359;
	  for (i = 0; i < 10; i++)
	    {
		double tic = (i % 2) ? size4 : size23;
		double cx = x + (tic * sin (rads));
		double cy = y + (tic * cos (rads));
		if (i == 0)
		    rl2_graph_move_to_point (ctx, cx, cy);
		else
		    rl2_graph_add_line_to_path (ctx, cx, cy);
		rads += 0.628318530718;
	    }
	  rl2_graph_close_subpath (ctx);
	  break;
      case RL2_GRAPHIC_MARK_CROSS:
	  rl2_graph_move_to_point (ctx, x - size8, y - size2);
	  rl2_graph_add_line_to_path (ctx, x + size8, y - size2);
	  rl2_graph_add_line_to_path (ctx, x + size8, y - size8);
	  rl2_graph_add_line_to_path (ctx, x + size2, y - size8);
	  rl2_graph_add_line_to_path (ctx, x + size2, y + size8);
	  rl2_graph_add_line_to_path (ctx, x + size8, y + size8);
	  rl2_graph_add_line_to_path (ctx, x + size8, y + size2);
	  rl2_graph_add_line_to_path (ctx, x - size8, y + size2);
	  rl2_graph_add_line_to_path (ctx, x - size8, y + size8);
	  rl2_graph_add_line_to_path (ctx, x - size2, y + size8);
	  rl2_graph_add_line_to_path (ctx, x - size2, y - size8);
	  rl2_graph_add_line_to_path (ctx, x - size8, y - size8);
	  rl2_graph_close_subpath (ctx);
	  break;
      case RL2_GRAPHIC_MARK_X:
	  rl2_graph_move_to_point (ctx, x, y - size6);
	  rl2_graph_add_line_to_path (ctx, x - size4, y - size2);
	  rl2_graph_add_line_to_path (ctx, x - size2, y - size2);
	  rl2_graph_add_line_to_path (ctx, x - size8, y);
	  rl2_graph_add_line_to_path (ctx, x - size2, y + size2);
	  rl2_graph_add_line_to_path (ctx, x - size4, y + size2);
	  rl2_graph_add_line_to_path (ctx, x, y + size6);
	  rl2_graph_add_line_to_path (ctx, x + size4, y + size2);
	  rl2_graph_add_line_to_path (ctx, x + size2, y + size2);
	  rl2_graph_add_line_to_path (ctx, x + size8, y);
	  rl2_graph_add_line_to_path (ctx, x + size2, y - size2);
	  rl2_graph_add_line_to_path (ctx, x + size4, y - size2);
	  rl2_graph_close_subpath (ctx);
	  break;
      case RL2_GRAPHIC_MARK_SQUARE:
      default:
	  rl2_graph_move_to_point (ctx, x - size2, y - size2);
	  rl2_graph_add_line_to_path (ctx, x - size2, y + size2);
	  rl2_graph_add_line_to_path (ctx, x + size2, y + size2);
	  rl2_graph_add_line_to_path (ctx, x + size2, y - size2);
	  rl2_graph_close_subpath (ctx);
	  break;
      };

/* filling and stroking the path */
    if (fill && !stroke)
	rl2_graph_fill_path (ctx, RL2_CLEAR_PATH);
    else if (stroke && !fill)
	rl2_graph_stroke_path (ctx, RL2_CLEAR_PATH);
    else
      {
	  rl2_graph_fill_path (ctx, RL2_PRESERVE_PATH);
	  rl2_graph_stroke_path (ctx, RL2_CLEAR_PATH);
      }
    cairo_restore (cairo);
    cairo_surface_flush (surface);

    return 1;
}

RL2_DECLARE int
rl2_graph_merge (rl2GraphicsContextPtr context_out,
		 rl2GraphicsContextPtr context_in)
{
/* merging two Images into a single one */
    RL2GraphContextPtr ctx_in = (RL2GraphContextPtr) context_in;
    RL2GraphContextPtr ctx_out = (RL2GraphContextPtr) context_out;

    if (ctx_in == NULL || ctx_out == NULL)
	return RL2_ERROR;
    if (cairo_image_surface_get_width (ctx_in->surface) !=
	cairo_image_surface_get_width (ctx_out->surface))
	return RL2_ERROR;
    if (cairo_image_surface_get_height (ctx_in->surface) !=
	cairo_image_surface_get_height (ctx_out->surface))
	return RL2_ERROR;

/* merging */
    cairo_set_source_surface (ctx_out->cairo, ctx_in->surface, 0, 0);
    cairo_paint (ctx_out->cairo);
    return RL2_OK;
}

RL2_DECLARE unsigned char *
rl2_graph_get_context_rgba_array (rl2GraphicsContextPtr context)
{
/* creating an RGBA buffer from the given Context */
    int width;
    int height;
    int x;
    int y;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *rgba;
    int little_endian = rl2cr_endian_arch ();
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return NULL;

    width = cairo_image_surface_get_width (ctx->surface);
    height = cairo_image_surface_get_height (ctx->surface);
    rgba = malloc (width * height * 4);
    if (rgba == NULL)
	return NULL;

    p_in = cairo_image_surface_get_data (ctx->surface);
    p_out = rgba;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
		if (little_endian)
		  {
		      b = *p_in++;
		      g = *p_in++;
		      r = *p_in++;
		      a = *p_in++;
		  }
		else
		  {
		      a = *p_in++;
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		  }
		*p_out++ = r;
		*p_out++ = g;
		*p_out++ = b;
		*p_out++ = a;
	    }
      }
    return rgba;
}

RL2_DECLARE int
rl2_graph_get_context_data (rl2GraphicsContextPtr context, unsigned char **data,
			    int *size)
{
/* creating a Cairo ARGB buffer from the given Context */
    int width;
    int height;
    int x;
    int y;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *cairo_data;
    int sz;
    int little_endian = rl2cr_endian_arch ();
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    *data = NULL;
    *size = 0;
    if (ctx == NULL)
	return 0;

    width = cairo_image_surface_get_width (ctx->surface);
    height = cairo_image_surface_get_height (ctx->surface);
    sz = width * height * 4;
    cairo_data = malloc (sz);
    if (cairo_data == NULL)
	return 0;

    p_in = cairo_image_surface_get_data (ctx->surface);
    p_out = cairo_data;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		if (little_endian)
		  {
		      unsigned char b = *p_in++;
		      unsigned char g = *p_in++;
		      unsigned char r = *p_in++;
		      unsigned char a = *p_in++;
		      *p_out++ = r;
		      *p_out++ = g;
		      *p_out++ = b;
		      *p_out++ = a;
		  }
		else
		  {
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		      *p_out++ = *p_in++;
		  }
	    }
      }
    *data = cairo_data;
    *size = sz;
    return 1;
}

RL2_DECLARE unsigned char *
rl2_graph_get_context_rgb_array (rl2GraphicsContextPtr context)
{
/* creating an RGB buffer from the given Context */
    int width;
    int height;
    int x;
    int y;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *rgb;
    int little_endian = rl2cr_endian_arch ();
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;

    if (ctx == NULL)
	return NULL;

    width = cairo_image_surface_get_width (ctx->surface);
    height = cairo_image_surface_get_height (ctx->surface);
    rgb = malloc (width * height * 3);
    if (rgb == NULL)
	return NULL;

    p_in = cairo_image_surface_get_data (ctx->surface);
    p_out = rgb;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		unsigned char r;
		unsigned char g;
		unsigned char b;
		unsigned char a;
		if (little_endian)
		  {
		      b = *p_in++;
		      g = *p_in++;
		      r = *p_in++;
		      a = *p_in++;
		  }
		else
		  {
		      a = *p_in++;
		      r = *p_in++;
		      g = *p_in++;
		      b = *p_in++;
		  }
		*p_out++ = unpremultiply (r, a);
		*p_out++ = unpremultiply (g, a);
		*p_out++ = unpremultiply (b, a);
	    }
      }
    return rgb;
}

RL2_DECLARE unsigned char *
rl2_graph_get_context_alpha_array (rl2GraphicsContextPtr context,
				   int *half_transparent)
{
/* creating an Alpha buffer from the given Context */
    int width;
    int height;
    int x;
    int y;
    unsigned char *p_in;
    unsigned char *p_out;
    unsigned char *alpha;
    int real_alpha = 0;
    int little_endian = rl2cr_endian_arch ();
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    *half_transparent = 0;

    if (ctx == NULL)
	return NULL;

    width = cairo_image_surface_get_width (ctx->surface);
    height = cairo_image_surface_get_height (ctx->surface);
    alpha = malloc (width * height);
    if (alpha == NULL)
	return NULL;

    p_in = cairo_image_surface_get_data (ctx->surface);
    p_out = alpha;
    for (y = 0; y < height; y++)
      {
	  for (x = 0; x < width; x++)
	    {
		if (little_endian)
		  {
		      p_in += 3;	/* skipping RGB */
		      if (*p_in >= 1 && *p_in <= 254)
			  real_alpha = 1;
		      *p_out++ = *p_in++;
		  }
		else
		  {
		      if (*p_in >= 1 && *p_in <= 254)
			  real_alpha = 1;
		      *p_out++ = *p_in++;
		      p_in += 3;	/* skipping RGB */
		  }
	    }
      }
    if (real_alpha)
	*half_transparent = 1;
    return alpha;
}

RL2_DECLARE int
rl2_rgba_to_pdf (const void *priv_data, unsigned int width, unsigned int height,
		 unsigned char *rgba, unsigned char **pdf, int *pdf_size)
{
/* attempting to create an RGB PDF map */
    rl2MemPdfPtr mem = NULL;
    cairo_surface_t *surface = NULL;
    cairo_t *cairo = NULL;
    rl2GraphicsBitmapPtr bmp = NULL;
    double page_width = 2480;
    double page_height = 3508;
    int dpi = 300;
    double margin_horz = 0.5 * dpi;
    double margin_vert = 0.5 * dpi;
    double active_width;
    double active_height;
    double scale = 1.0;
    double h_margin;
    double v_margin;
    struct rl2_private_data *data = (struct rl2_private_data *) priv_data;
    unsigned char orientation = RL2_PDF_PORTRAIT;

    if (data != NULL)
      {
	  switch (data->pdf_paper_format)
	    {
		/* setting page width in Pixels */
	    case RL2_PDF_PAPER_FORMAT_A0:
		switch (data->pdf_dpi)
		  {
		  case RL2_PDF_DPI_72:
		      page_width = 2383;
		      page_height = 3370;
		      break;
		  case RL2_PDF_DPI_150:
		      page_width = 4966;
		      page_height = 7021;
		      break;
		  case RL2_PDF_DPI_600:
		      page_width = 19866;
		      page_height = 28086;
		      break;
		  case RL2_PDF_DPI_300:
		  default:
		      page_width = 9933;
		      page_height = 14943;
		      break;
		  };
		break;
	    case RL2_PDF_PAPER_FORMAT_A1:
		switch (data->pdf_dpi)
		  {
		  case RL2_PDF_DPI_72:
		      page_width = 1683;
		      page_height = 2383;
		      break;
		  case RL2_PDF_DPI_150:
		      page_width = 3508;
		      page_height = 4966;
		      break;
		  case RL2_PDF_DPI_600:
		      page_width = 14032;
		      page_height = 19866;
		      break;
		  case RL2_PDF_DPI_300:
		  default:
		      page_width = 7016;
		      page_height = 9933;
		      break;
		  };
		break;
	    case RL2_PDF_PAPER_FORMAT_A2:
		switch (data->pdf_dpi)
		  {
		  case RL2_PDF_DPI_72:
		      page_width = 1190;
		      page_height = 1683;
		      break;
		  case RL2_PDF_DPI_150:
		      page_width = 2480;
		      page_height = 3508;
		      break;
		  case RL2_PDF_DPI_600:
		      page_width = 9922;
		      page_height = 14032;
		      break;
		  case RL2_PDF_DPI_300:
		  default:
		      page_width = 4961;
		      page_height = 7016;
		      break;
		  };
		break;
	    case RL2_PDF_PAPER_FORMAT_A3:
		switch (data->pdf_dpi)
		  {
		  case RL2_PDF_DPI_72:
		      page_width = 841;
		      page_height = 1190;
		      break;
		  case RL2_PDF_DPI_150:
		      page_width = 1754;
		      page_height = 2480;
		      break;
		  case RL2_PDF_DPI_600:
		      page_width = 7016;
		      page_height = 9922;
		      break;
		  case RL2_PDF_DPI_300:
		  default:
		      page_width = 3508;
		      page_height = 4961;
		      break;
		  };
		break;
	    case RL2_PDF_PAPER_FORMAT_A5:
		switch (data->pdf_dpi)
		  {
		  case RL2_PDF_DPI_72:
		      page_width = 419;
		      page_height = 595;
		      break;
		  case RL2_PDF_DPI_150:
		      page_width = 874;
		      page_height = 1240;
		      break;
		  case RL2_PDF_DPI_600:
		      page_width = 3496;
		      page_height = 4960;
		      break;
		  case RL2_PDF_DPI_300:
		  default:
		      page_width = 1748;
		      page_height = 2480;
		      break;
		  };
		break;
	    case RL2_PDF_PAPER_FORMAT_A4:
	    default:
		switch (data->pdf_dpi)
		  {
		  case RL2_PDF_DPI_72:
		      page_width = 595;
		      page_height = 841;
		      break;
		  case RL2_PDF_DPI_150:
		      page_width = 1240;
		      page_height = 1754;
		      break;
		  case RL2_PDF_DPI_600:
		      page_width = 4960;
		      page_height = 7016;
		      break;
		  case RL2_PDF_DPI_300:
		  default:
		      page_width = 2480;
		      page_height = 3508;
		      break;
		  };
		break;
	    };
	  switch (data->pdf_dpi)
	    {
		/* setting DPI */
	    case RL2_PDF_DPI_72:
		dpi = 72;
		break;
	    case RL2_PDF_DPI_150:
		dpi = 150;
		break;
	    case RL2_PDF_DPI_600:
		dpi = 600;
		break;
	    case RL2_PDF_DPI_300:
	    default:
		dpi = 300;
		break;
	    };
	  /* setting margins in Pixels */
	  if (data->pdf_margin_uom == RL2_PDF_MARGIN_MILLIMS)
	    {
		margin_horz = data->pdf_margin_horz * 0.0393701 * dpi;
		margin_vert = data->pdf_margin_vert * 0.0393701 * dpi;
	    }
	  else
	    {
		margin_horz = data->pdf_margin_horz * dpi;
		margin_vert = data->pdf_margin_vert * dpi;
	    }
	  orientation = data->pdf_orientation;
      }
    if (orientation != RL2_PDF_PORTRAIT)
      {
	  double horz = page_width;
	  page_width = page_height;
	  page_height = horz;
      }
/* setting the printable area of the PDF */
    active_width = page_width - (margin_horz * 2);
    active_height = page_height - (margin_vert * 2);
    if (active_width == width && active_height == height)
      {
	  scale = 1.0;
      }
    else if (active_width < width || active_height < height)
      {
	  /* scaling the image so to fit inside the printable area of the PDF */
	  scale = 1.0;
	  h_margin = width;
	  v_margin = height;
	  while (h_margin > active_width || v_margin > active_height)
	    {
		scale -= 0.001;
		h_margin = (double) width *scale;
		v_margin = (double) height *scale;
	    }
      }
/* centering the image on the PDF surface */
    h_margin = (page_width - (width * scale)) / 2.0;
    v_margin = (page_height - (height * scale)) / 2.0;

    *pdf = NULL;
    *pdf_size = 0;

    mem = rl2_create_mem_pdf_target ();
    if (mem == NULL)
	goto error;

    surface =
	cairo_pdf_surface_create_for_stream (pdf_write_func, mem, page_width,
					     page_height);
    if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    cairo = cairo_create (surface);
    if (cairo_status (cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* priming a transparent background */
    cairo_rectangle (cairo, 0, 0, page_width, page_height);
    cairo_set_source_rgba (cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (cairo);

    bmp = rl2_graph_create_bitmap (rgba, width, height);
    rgba = NULL;
    if (bmp == NULL)
	goto error;
/* rendering the Bitmap */
    cairo_save (cairo);
    cairo_translate (cairo, h_margin, v_margin);
    cairo_scale (cairo, scale, scale);
    cairo_set_source (cairo, bmp->pattern);
    cairo_rectangle (cairo, 0, 0, bmp->width, bmp->height);
    cairo_fill (cairo);
    cairo_restore (cairo);
    cairo_surface_flush (surface);
    rl2_graph_destroy_bitmap (bmp);

    cairo_surface_show_page (surface);
    cairo_destroy (cairo);
    cairo_surface_finish (surface);
    cairo_surface_destroy (surface);
/* retrieving the PDF memory block */
    if (rl2_get_mem_pdf_buffer (mem, pdf, pdf_size) != RL2_OK)
	goto error;
    rl2_destroy_mem_pdf_target (mem);
    return RL2_OK;

  error2:
    if (cairo != NULL)
	cairo_destroy (cairo);
  error1:
    if (surface != NULL)
	cairo_surface_destroy (surface);
  error:
    if (bmp != NULL)
	rl2_graph_destroy_bitmap (bmp);
    if (mem != NULL)
	rl2_destroy_mem_pdf_target (mem);
    return RL2_ERROR;
}

RL2_DECLARE int
rl2_gray_pdf (const void *priv_data, unsigned int width, unsigned int height,
	      unsigned char **pdf, int *pdf_size)
{
/* attempting to create an all-Gray PDF */
    rl2MemPdfPtr mem = NULL;
    cairo_surface_t *surface = NULL;
    cairo_t *cairo = NULL;
    double page_width = 2480;
    double page_height = 3508;
    int dpi = 300;
    double margin_horz = 0.5 * dpi;
    double margin_vert = 0.5 * dpi;
    double active_width;
    double active_height;
    double scale = 1.0;
    double h_margin;
    double v_margin;
    struct rl2_private_data *data = (struct rl2_private_data *) priv_data;
    unsigned char orientation = RL2_PDF_PORTRAIT;

    if (data != NULL)
      {
	  switch (data->pdf_paper_format)
	    {
		/* setting page width in Pixels */
	    case RL2_PDF_PAPER_FORMAT_A0:
		switch (data->pdf_dpi)
		  {
		  case RL2_PDF_DPI_72:
		      page_width = 2383;
		      page_height = 3370;
		      break;
		  case RL2_PDF_DPI_150:
		      page_width = 4966;
		      page_height = 7021;
		      break;
		  case RL2_PDF_DPI_600:
		      page_width = 19866;
		      page_height = 28086;
		      break;
		  case RL2_PDF_DPI_300:
		  default:
		      page_width = 9933;
		      page_height = 14943;
		      break;
		  };
		break;
	    case RL2_PDF_PAPER_FORMAT_A1:
		switch (data->pdf_dpi)
		  {
		  case RL2_PDF_DPI_72:
		      page_width = 1683;
		      page_height = 2383;
		      break;
		  case RL2_PDF_DPI_150:
		      page_width = 3508;
		      page_height = 4966;
		      break;
		  case RL2_PDF_DPI_600:
		      page_width = 14032;
		      page_height = 19866;
		      break;
		  case RL2_PDF_DPI_300:
		  default:
		      page_width = 7016;
		      page_height = 9933;
		      break;
		  };
		break;
	    case RL2_PDF_PAPER_FORMAT_A2:
		switch (data->pdf_dpi)
		  {
		  case RL2_PDF_DPI_72:
		      page_width = 1190;
		      page_height = 1683;
		      break;
		  case RL2_PDF_DPI_150:
		      page_width = 2480;
		      page_height = 3508;
		      break;
		  case RL2_PDF_DPI_600:
		      page_width = 9922;
		      page_height = 14032;
		      break;
		  case RL2_PDF_DPI_300:
		  default:
		      page_width = 4961;
		      page_height = 7016;
		      break;
		  };
		break;
	    case RL2_PDF_PAPER_FORMAT_A3:
		switch (data->pdf_dpi)
		  {
		  case RL2_PDF_DPI_72:
		      page_width = 841;
		      page_height = 1190;
		      break;
		  case RL2_PDF_DPI_150:
		      page_width = 1754;
		      page_height = 2480;
		      break;
		  case RL2_PDF_DPI_600:
		      page_width = 7016;
		      page_height = 9922;
		      break;
		  case RL2_PDF_DPI_300:
		  default:
		      page_width = 3508;
		      page_height = 4961;
		      break;
		  };
		break;
	    case RL2_PDF_PAPER_FORMAT_A5:
		switch (data->pdf_dpi)
		  {
		  case RL2_PDF_DPI_72:
		      page_width = 419;
		      page_height = 595;
		      break;
		  case RL2_PDF_DPI_150:
		      page_width = 874;
		      page_height = 1240;
		      break;
		  case RL2_PDF_DPI_600:
		      page_width = 3496;
		      page_height = 4960;
		      break;
		  case RL2_PDF_DPI_300:
		  default:
		      page_width = 1748;
		      page_height = 2480;
		      break;
		  };
		break;
	    case RL2_PDF_PAPER_FORMAT_A4:
	    default:
		switch (data->pdf_dpi)
		  {
		  case RL2_PDF_DPI_72:
		      page_width = 595;
		      page_height = 841;
		      break;
		  case RL2_PDF_DPI_150:
		      page_width = 1240;
		      page_height = 1754;
		      break;
		  case RL2_PDF_DPI_600:
		      page_width = 4960;
		      page_height = 7016;
		      break;
		  case RL2_PDF_DPI_300:
		  default:
		      page_width = 2480;
		      page_height = 3508;
		      break;
		  };
		break;
	    };
	  switch (data->pdf_dpi)
	    {
		/* setting DPI */
	    case RL2_PDF_DPI_72:
		dpi = 72;
		break;
	    case RL2_PDF_DPI_150:
		dpi = 150;
		break;
	    case RL2_PDF_DPI_600:
		dpi = 600;
		break;
	    case RL2_PDF_DPI_300:
	    default:
		dpi = 300;
		break;
	    };
	  /* setting margins in Pixels */
	  if (data->pdf_margin_uom == RL2_PDF_MARGIN_MILLIMS)
	    {
		margin_horz = data->pdf_margin_horz * 0.0393701 * dpi;
		margin_vert = data->pdf_margin_vert * 0.0393701 * dpi;
	    }
	  else
	    {
		margin_horz = data->pdf_margin_horz * dpi;
		margin_vert = data->pdf_margin_vert * dpi;
	    }
	  orientation = data->pdf_orientation;
      }
    if (orientation != RL2_PDF_PORTRAIT)
      {
	  double horz = page_width;
	  page_width = page_height;
	  page_height = horz;
      }
/* setting the printable area of the PDF */
    active_width = page_width - (margin_horz * 2);
    active_height = page_height - (margin_vert * 2);
    if (active_width == width && active_height == height)
      {
	  scale = 1.0;
      }
    else if (active_width < width || active_height < height)
      {
	  /* scaling the image so to fit inside the printable area of the PDF */
	  scale = 1.0;
	  h_margin = width;
	  v_margin = height;
	  while (h_margin > active_width || v_margin > active_height)
	    {
		scale -= 0.001;
		h_margin = (double) width *scale;
		v_margin = (double) height *scale;
	    }
      }
/* centering the image on the PDF surface */
    h_margin = (page_width - (width * scale)) / 2.0;
    v_margin = (page_height - (height * scale)) / 2.0;

    *pdf = NULL;
    *pdf_size = 0;

    mem = rl2_create_mem_pdf_target ();
    if (mem == NULL)
	goto error;

    surface =
	cairo_pdf_surface_create_for_stream (pdf_write_func, mem, page_width,
					     page_height);
    if (cairo_surface_status (surface) == CAIRO_STATUS_SUCCESS)
	;
    else
	goto error1;
    cairo = cairo_create (surface);
    if (cairo_status (cairo) == CAIRO_STATUS_NO_MEMORY)
	goto error2;

/* priming a transparent background */
    cairo_rectangle (cairo, 0, 0, page_width, page_height);
    cairo_set_source_rgba (cairo, 0.0, 0.0, 0.0, 0.0);
    cairo_fill (cairo);
/* painting a gray rectangle */
    cairo_save (cairo);
    cairo_translate (cairo, h_margin, v_margin);
    cairo_scale (cairo, scale, scale);
    cairo_set_source_rgba (cairo, 128.0, 128.0, 128.0, 255.0);
    cairo_rectangle (cairo, 0, 0, width, height);
    cairo_fill (cairo);
    cairo_restore (cairo);
    cairo_surface_flush (surface);
    cairo_surface_show_page (surface);
    cairo_destroy (cairo);
    cairo_surface_finish (surface);
    cairo_surface_destroy (surface);
/* retrieving the PDF memory block */
    if (rl2_get_mem_pdf_buffer (mem, pdf, pdf_size) != RL2_OK)
	goto error;
    rl2_destroy_mem_pdf_target (mem);
    return RL2_OK;

  error2:
    if (cairo != NULL)
	cairo_destroy (cairo);
  error1:
    if (surface != NULL)
	cairo_surface_destroy (surface);
  error:
    if (mem != NULL)
	rl2_destroy_mem_pdf_target (mem);
    return RL2_ERROR;
}

RL2_DECLARE rl2MemPdfPtr
rl2_create_mem_pdf_target (void)
{
/* creating an initially empty in-memory PDF target */
    rl2PrivMemPdfPtr mem = malloc (sizeof (rl2PrivMemPdf));
    if (mem == NULL)
	return NULL;
    mem->write_offset = 0;
    mem->size = 64 * 1024;
    mem->buffer = malloc (mem->size);
    if (mem->buffer == NULL)
      {
	  free (mem);
	  return NULL;
      }
    return (rl2MemPdfPtr) mem;
}

RL2_DECLARE void
rl2_destroy_mem_pdf_target (rl2MemPdfPtr target)
{
/* memory cleanup - destroying an in-memory PDF target */
    rl2PrivMemPdfPtr mem = (rl2PrivMemPdfPtr) target;
    if (mem == NULL)
	return;
    if (mem->buffer != NULL)
	free (mem->buffer);
    free (mem);
}

RL2_DECLARE int
rl2_get_mem_pdf_buffer (rl2MemPdfPtr target, unsigned char **buffer, int *size)
{
/* exporting the internal buffer */
    rl2PrivMemPdfPtr mem = (rl2PrivMemPdfPtr) target;
    if (mem == NULL)
	return RL2_ERROR;
    if (mem->buffer == NULL)
	return RL2_ERROR;
    *buffer = mem->buffer;
    mem->buffer = NULL;
    *size = mem->write_offset;
    return RL2_OK;
}

RL2_DECLARE rl2CanvasPtr
rl2_create_vector_canvas (rl2GraphicsContextPtr ref_ctx)
{
/* allocating and initializing a Canvas object (generic Vector) */
    rl2PrivCanvasPtr canvas = NULL;
    if (ref_ctx == NULL)
	return NULL;

    canvas = malloc (sizeof (rl2PrivCanvas));
    if (canvas == NULL)
	return NULL;
    canvas->type = RL2_VECTOR_CANVAS;
    canvas->ref_ctx = ref_ctx;
    canvas->ref_ctx_nodes = NULL;
    canvas->ref_ctx_edges = NULL;
    canvas->ref_ctx_links = NULL;
    canvas->ref_ctx_faces = NULL;
    canvas->ref_ctx_edge_seeds = NULL;
    canvas->ref_ctx_link_seeds = NULL;
    canvas->ref_ctx_face_seeds = NULL;
    canvas->ctx_ready = RL2_FALSE;
    canvas->ctx_nodes_ready = RL2_FALSE;
    canvas->ctx_edges_ready = RL2_FALSE;
    canvas->ctx_links_ready = RL2_FALSE;
    canvas->ctx_faces_ready = RL2_FALSE;
    canvas->ctx_edge_seeds_ready = RL2_FALSE;
    canvas->ctx_link_seeds_ready = RL2_FALSE;
    canvas->ctx_face_seeds_ready = RL2_FALSE;
    return (rl2CanvasPtr) canvas;
}

RL2_DECLARE rl2CanvasPtr
rl2_create_topology_canvas (rl2GraphicsContextPtr ref_ctx,
			    rl2GraphicsContextPtr ref_ctx_nodes,
			    rl2GraphicsContextPtr ref_ctx_edges,
			    rl2GraphicsContextPtr ref_ctx_faces,
			    rl2GraphicsContextPtr ref_ctx_edge_seeds,
			    rl2GraphicsContextPtr ref_ctx_face_seeds)
{
/* allocating and initializing a Canvas object (Topology) */
    rl2PrivCanvasPtr canvas = NULL;
    if (ref_ctx == NULL)
	return NULL;
    if (ref_ctx_nodes == NULL && ref_ctx_edges == NULL && ref_ctx_faces == NULL
	&& ref_ctx_edge_seeds && ref_ctx_face_seeds == NULL)
	return NULL;

    canvas = malloc (sizeof (rl2PrivCanvas));
    if (canvas == NULL)
	return NULL;
    canvas->type = RL2_TOPOLOGY_CANVAS;
    canvas->ref_ctx = ref_ctx;
    canvas->ref_ctx_nodes = ref_ctx_nodes;
    canvas->ref_ctx_edges = ref_ctx_edges;
    canvas->ref_ctx_links = NULL;
    canvas->ref_ctx_faces = ref_ctx_faces;
    canvas->ref_ctx_edge_seeds = ref_ctx_edge_seeds;
    canvas->ref_ctx_link_seeds = NULL;
    canvas->ref_ctx_face_seeds = ref_ctx_face_seeds;
    canvas->ctx_ready = RL2_FALSE;
    canvas->ctx_nodes_ready = RL2_FALSE;
    canvas->ctx_edges_ready = RL2_FALSE;
    canvas->ctx_links_ready = RL2_FALSE;
    canvas->ctx_faces_ready = RL2_FALSE;
    canvas->ctx_edge_seeds_ready = RL2_FALSE;
    canvas->ctx_link_seeds_ready = RL2_FALSE;
    canvas->ctx_face_seeds_ready = RL2_FALSE;
    return (rl2CanvasPtr) canvas;
}

RL2_DECLARE rl2CanvasPtr
rl2_create_network_canvas (rl2GraphicsContextPtr ref_ctx,
			   rl2GraphicsContextPtr ref_ctx_nodes,
			   rl2GraphicsContextPtr ref_ctx_links,
			   rl2GraphicsContextPtr ref_ctx_link_seeds)
{
/* allocating and initializing a Canvas object (Network) */
    rl2PrivCanvasPtr canvas = NULL;
    if (ref_ctx == NULL)
	return NULL;
    if (ref_ctx_nodes == NULL && ref_ctx_links && ref_ctx_link_seeds == NULL)
	return NULL;

    canvas = malloc (sizeof (rl2PrivCanvas));
    if (canvas == NULL)
	return NULL;
    canvas->type = RL2_NETWORK_CANVAS;
    canvas->ref_ctx = ref_ctx;
    canvas->ref_ctx_nodes = ref_ctx_nodes;
    canvas->ref_ctx_edges = NULL;
    canvas->ref_ctx_links = ref_ctx_links;
    canvas->ref_ctx_faces = NULL;
    canvas->ref_ctx_edge_seeds = NULL;
    canvas->ref_ctx_link_seeds = ref_ctx_link_seeds;
    canvas->ref_ctx_face_seeds = NULL;
    canvas->ctx_ready = RL2_FALSE;
    canvas->ctx_nodes_ready = RL2_FALSE;
    canvas->ctx_edges_ready = RL2_FALSE;
    canvas->ctx_links_ready = RL2_FALSE;
    canvas->ctx_faces_ready = RL2_FALSE;
    canvas->ctx_edge_seeds_ready = RL2_FALSE;
    canvas->ctx_link_seeds_ready = RL2_FALSE;
    canvas->ctx_face_seeds_ready = RL2_FALSE;
    return (rl2CanvasPtr) canvas;
}

RL2_DECLARE rl2CanvasPtr
rl2_create_raster_canvas (rl2GraphicsContextPtr ref_ctx)
{
/* allocating and initializing a Canvas object (Raster) */
    rl2PrivCanvasPtr canvas = NULL;
    if (ref_ctx == NULL)
	return NULL;

    canvas = malloc (sizeof (rl2PrivCanvas));
    if (canvas == NULL)
	return NULL;
    canvas->type = RL2_RASTER_CANVAS;
    canvas->ref_ctx = ref_ctx;
    canvas->ref_ctx_nodes = NULL;
    canvas->ref_ctx_edges = NULL;
    canvas->ref_ctx_links = NULL;
    canvas->ref_ctx_faces = NULL;
    canvas->ref_ctx_edge_seeds = NULL;
    canvas->ref_ctx_link_seeds = NULL;
    canvas->ref_ctx_face_seeds = NULL;
    canvas->ctx_ready = RL2_FALSE;
    canvas->ctx_nodes_ready = RL2_FALSE;
    canvas->ctx_edges_ready = RL2_FALSE;
    canvas->ctx_links_ready = RL2_FALSE;
    canvas->ctx_faces_ready = RL2_FALSE;
    canvas->ctx_edge_seeds_ready = RL2_FALSE;
    canvas->ctx_link_seeds_ready = RL2_FALSE;
    canvas->ctx_face_seeds_ready = RL2_FALSE;
    return (rl2CanvasPtr) canvas;
}

RL2_DECLARE rl2CanvasPtr
rl2_create_wms_canvas (rl2GraphicsContextPtr ref_ctx)
{
/* allocating and initializing a Canvas object (WMS) */
    rl2PrivCanvasPtr canvas = NULL;
    if (ref_ctx == NULL)
	return NULL;

    canvas = malloc (sizeof (rl2PrivCanvas));
    if (canvas == NULL)
	return NULL;
    canvas->type = RL2_WMS_CANVAS;
    canvas->ref_ctx = ref_ctx;
    canvas->ref_ctx_nodes = NULL;
    canvas->ref_ctx_edges = NULL;
    canvas->ref_ctx_links = NULL;
    canvas->ref_ctx_faces = NULL;
    canvas->ref_ctx_edge_seeds = NULL;
    canvas->ref_ctx_link_seeds = NULL;
    canvas->ref_ctx_face_seeds = NULL;
    canvas->ctx_ready = RL2_FALSE;
    canvas->ctx_nodes_ready = RL2_FALSE;
    canvas->ctx_edges_ready = RL2_FALSE;
    canvas->ctx_links_ready = RL2_FALSE;
    canvas->ctx_faces_ready = RL2_FALSE;
    canvas->ctx_edge_seeds_ready = RL2_FALSE;
    canvas->ctx_link_seeds_ready = RL2_FALSE;
    canvas->ctx_face_seeds_ready = RL2_FALSE;
    return (rl2CanvasPtr) canvas;
}

RL2_DECLARE void
rl2_destroy_canvas (rl2CanvasPtr ptr)
{
/* memory cleanup - destroying a Canvas object */
    rl2PrivCanvasPtr canvas = (rl2PrivCanvasPtr) ptr;
    if (canvas == NULL)
	return;
    free (canvas);
}

RL2_DECLARE int
rl2_get_canvas_type (rl2CanvasPtr ptr)
{
/* return the Type from a Canvas */
    rl2PrivCanvasPtr canvas = (rl2PrivCanvasPtr) ptr;
    if (canvas == NULL)
	return RL2_UNKNOWN_CANVAS;
    return canvas->type;
}

RL2_DECLARE int
rl2_is_canvas_ready (rl2CanvasPtr ptr, int which)
{
/* checks if a Canvas is ready (rendered)  */
    rl2PrivCanvasPtr canvas = (rl2PrivCanvasPtr) ptr;
    if (canvas == NULL)
	return RL2_FALSE;
    switch (canvas->type)
      {
      case RL2_VECTOR_CANVAS:
	  switch (which)
	    {
	    case RL2_CANVAS_BASE_CTX:
		return canvas->ctx_ready;
	    };
	  break;
      case RL2_RASTER_CANVAS:
      case RL2_WMS_CANVAS:
	  switch (which)
	    {
	    case RL2_CANVAS_BASE_CTX:
		return canvas->ctx_ready;
	    };
	  break;
      case RL2_TOPOLOGY_CANVAS:
	  switch (which)
	    {
	    case RL2_CANVAS_BASE_CTX:
		return canvas->ctx_ready;
	    case RL2_CANVAS_NODES_CTX:
		return canvas->ctx_nodes_ready;
	    case RL2_CANVAS_EDGES_CTX:
		return canvas->ctx_edges_ready;
	    case RL2_CANVAS_FACES_CTX:
		return canvas->ctx_faces_ready;
	    case RL2_CANVAS_EDGE_SEEDS_CTX:
		return canvas->ctx_edge_seeds_ready;
	    case RL2_CANVAS_FACE_SEEDS_CTX:
		return canvas->ctx_face_seeds_ready;
	    };
	  break;
      case RL2_NETWORK_CANVAS:
	  switch (which)
	    {
	    case RL2_CANVAS_BASE_CTX:
		return canvas->ctx_ready;
	    case RL2_CANVAS_NODES_CTX:
		return canvas->ctx_nodes_ready;
	    case RL2_CANVAS_LINKS_CTX:
		return canvas->ctx_links_ready;
	    case RL2_CANVAS_LINK_SEEDS_CTX:
		return canvas->ctx_link_seeds_ready;
	    };
	  break;
      };
    return RL2_FALSE;
}

RL2_DECLARE rl2GraphicsContextPtr
rl2_get_canvas_ctx (rl2CanvasPtr ptr, int which)
{
/* return a pointer to some Graphics Context from a Canvas */
    rl2PrivCanvasPtr canvas = (rl2PrivCanvasPtr) ptr;
    if (canvas == NULL)
	return NULL;
    switch (canvas->type)
      {
      case RL2_VECTOR_CANVAS:
	  switch (which)
	    {
	    case RL2_CANVAS_BASE_CTX:
		return canvas->ref_ctx;
	    };
	  break;
      case RL2_RASTER_CANVAS:
      case RL2_WMS_CANVAS:
	  switch (which)
	    {
	    case RL2_CANVAS_BASE_CTX:
		return canvas->ref_ctx;
	    };
	  break;
      case RL2_TOPOLOGY_CANVAS:
	  switch (which)
	    {
	    case RL2_CANVAS_BASE_CTX:
		return canvas->ref_ctx;
	    case RL2_CANVAS_NODES_CTX:
		return canvas->ref_ctx_nodes;
	    case RL2_CANVAS_EDGES_CTX:
		return canvas->ref_ctx_edges;
	    case RL2_CANVAS_FACES_CTX:
		return canvas->ref_ctx_faces;
	    case RL2_CANVAS_EDGE_SEEDS_CTX:
		return canvas->ref_ctx_edge_seeds;
	    case RL2_CANVAS_FACE_SEEDS_CTX:
		return canvas->ref_ctx_face_seeds;
	    };
	  break;
      case RL2_NETWORK_CANVAS:
	  switch (which)
	    {
	    case RL2_CANVAS_BASE_CTX:
		return canvas->ref_ctx;
	    case RL2_CANVAS_NODES_CTX:
		return canvas->ref_ctx_nodes;
	    case RL2_CANVAS_LINKS_CTX:
		return canvas->ref_ctx_links;
	    case RL2_CANVAS_LINK_SEEDS_CTX:
		return canvas->ref_ctx_link_seeds;
	    };
	  break;
      };
    return NULL;
}

RL2_DECLARE void
rl2_prime_background (void *pctx, unsigned char red, unsigned char green,
		      unsigned char blue, unsigned char alpha)
{
/* priming the background */
    double r = (double) red / 255.0;
    double g = (double) green / 255.0;
    double b = (double) blue / 255.0;
    double a = (double) alpha / 255.0;
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) pctx;
    int width = cairo_image_surface_get_width (ctx->surface);
    int height = cairo_image_surface_get_height (ctx->surface);
    cairo_rectangle (ctx->cairo, 0, 0, width, height);
    cairo_set_source_rgba (ctx->cairo, r, g, b, a);
    cairo_fill (ctx->cairo);
}

RL2_PRIVATE struct rl2_advanced_labeling *
rl2_get_labeling_ref (const void *context)
{
/* returning the internal pointer to AdvancedLabeling */
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    if (ctx == NULL)
	return NULL;
    return ctx->labeling;
}

RL2_DECLARE int
rl2_copy_wms_tile (rl2GraphicsContextPtr out, rl2GraphicsContextPtr in,
		   int base_x, int base_y)
{
// copying a WMS tile into the full frame
    RL2GraphContextPtr ctx_in = (RL2GraphContextPtr) in;
    RL2GraphContextPtr ctx_out = (RL2GraphContextPtr) out;

    if (ctx_out == NULL || ctx_in == NULL)
	return RL2_ERROR;

    cairo_save (ctx_out->cairo);
    cairo_translate (ctx_out->cairo, base_x, base_y);
    cairo_set_source_surface (ctx_out->cairo, ctx_in->surface, 0, 0);
    cairo_paint (ctx_out->cairo);
    cairo_restore (ctx_out->cairo);
    return RL2_OK;
}

RL2_DECLARE int
rl2_aux_prepare_image (rl2GraphicsContextPtr context, void *data, int width,
		       int height, int format_id, int quality,
		       unsigned char **blob, unsigned int *blob_size)
{
/* creating an image from a Graphics context */
    RL2GraphContextPtr ctx = (RL2GraphContextPtr) context;
    int ok_format = 0;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    int half_transparent = 0;
    unsigned char *image = NULL;
    int image_size;

    *blob = NULL;
    *blob_size = 0;

    if (ctx == NULL || data == NULL)
	return RL2_ERROR;

    if (format_id == RL2_OUTPUT_FORMAT_PNG)
	ok_format = 1;
    if (format_id == RL2_OUTPUT_FORMAT_JPEG)
	ok_format = 1;
    if (format_id == RL2_OUTPUT_FORMAT_TIFF)
	ok_format = 1;
    if (!ok_format)
	return RL2_ERROR;

    /* retrieving RGB and ALPHA pixels */
    rgb = rl2_graph_get_context_rgb_array (ctx);
    alpha = rl2_graph_get_context_alpha_array (ctx, &half_transparent);
    if (rgb == NULL || alpha == NULL)
	return RL2_ERROR;

    /* attempting to create the output image */
    if (!get_payload_from_rgb_rgba_transparent
	(width, height, data, rgb, alpha, format_id,
	 quality, &image, &image_size, 1.0, half_transparent))
      {
	  free (rgb);
	  free (alpha);
	  return RL2_ERROR;
      }

    free (rgb);
    free (alpha);
    *blob = image;
    *blob_size = image_size;
    return RL2_OK;
}

RL2_DECLARE const char *
rl2_cairo_version (void)
{
/* returning the CAIRO version string */
    return cairo_version_string ();
}
