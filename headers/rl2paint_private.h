/*

 rl2paint_private -- hidden internals for Cairo

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

/**
 \file rl2pain_private.h

 RasterLite2 private Cairo header file
 */

#include "config.h"

#ifndef _RL2PAINT_PRIVATE_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _RL2PAINT_PRIVATE_H
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#define RL2_SURFACE_IMG 2671
#define RL2_SURFACE_SVG 1267
#define RL2_SURFACE_PDF	1276

    struct rl2_graphics_pen
    {
/* a struct wrapping a Cairo Pen */
	int is_solid_color;
	int is_linear_gradient;
	int is_pattern;
	double red;
	double green;
	double blue;
	double alpha;
	double x0;
	double y0;
	double x1;
	double y1;
	double red2;
	double green2;
	double blue2;
	double alpha2;
	cairo_pattern_t *pattern;
	double width;
	double *dash_array;
	int dash_count;
	double dash_offset;
	int line_cap;
	int line_join;
    };

    struct rl2_graphics_brush
    {
/* a struct wrapping a Cairo Brush */
	int is_solid_color;
	int is_linear_gradient;
	int is_pattern;
	double red;
	double green;
	double blue;
	double alpha;
	double x0;
	double y0;
	double x1;
	double y1;
	double red2;
	double green2;
	double blue2;
	double alpha2;
	cairo_pattern_t *pattern;
    };

    typedef struct rl2_graphics_context
    {
/* a Cairo based painting context */
	int type;
	cairo_surface_t *surface;
	cairo_surface_t *clip_surface;
	cairo_t *cairo;
	cairo_t *clip_cairo;
	struct rl2_graphics_pen current_pen;
	struct rl2_graphics_brush current_brush;
	double font_red;
	double font_green;
	double font_blue;
	double font_alpha;
	int with_font_halo;
	double halo_radius;
	double halo_red;
	double halo_green;
	double halo_blue;
	double halo_alpha;
	struct rl2_advanced_labeling *labeling;
    } RL2GraphContext;
    typedef RL2GraphContext *RL2GraphContextPtr;

    typedef struct rl2_priv_graphics_pattern
    {
/* a Cairo based pattern */
	int width;
	int height;
	unsigned char *rgba;
	cairo_surface_t *bitmap;
	cairo_pattern_t *pattern;
    } RL2PrivGraphPattern;
    typedef RL2PrivGraphPattern *RL2PrivGraphPatternPtr;

    typedef struct rl2_graphics_font
    {
/* a struct wrapping a Cairo/FreeType Font */
	int toy_font;
	char *facename;
	cairo_font_face_t *cairo_font;
	cairo_scaled_font_t *cairo_scaled_font;
	struct rl2_private_tt_font *tt_font;
	double size;
	double font_red;
	double font_green;
	double font_blue;
	double font_alpha;
	int with_halo;
	double halo_radius;
	double halo_red;
	double halo_green;
	double halo_blue;
	double halo_alpha;
	int style;
	int weight;
    } RL2GraphFont;
    typedef RL2GraphFont *RL2GraphFontPtr;

    typedef struct rl2_graphics_bitmap
    {
/* a Cairo based symbol bitmap */
	int width;
	int height;
	unsigned char *rgba;
	cairo_surface_t *bitmap;
	cairo_pattern_t *pattern;
    } RL2GraphBitmap;
    typedef RL2GraphBitmap *RL2GraphBitmapPtr;

    typedef struct rl2_priv_affine_transform_data
    {
	double xx;
	double xy;
	double yx;
	double yy;
	double x_off;
	double y_off;
	int orig_ok;
	int orig_width;
	int orig_height;
	double orig_minx;
	double orig_miny;
	double orig_x_res;
	double orig_y_res;
	int dest_ok;
	int dest_width;
	int dest_height;
	double dest_minx;
	double dest_miny;
	double dest_x_res;
	double dest_y_res;
	int max_threads;
    } rl2PrivAffineTransformData;
    typedef rl2PrivAffineTransformData *rl2PrivAffineTransformDataPtr;

#ifdef __cplusplus
}
#endif

#endif				/* _RL2PAINT_PRIVATE_H */
