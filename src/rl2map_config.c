/*

 rl2map_config -- handling Map Configuration
 version 0.1, 2020 July 1

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
 
Portions created by the Initial Developer are Copyright (C) 2020
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

#include "config.h"

#include <libxml/parser.h>

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2mapconfig.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2_private.h"

#define RL2_UNUSED() if (argc || argv) argc = argc;

static void
dummySilentError (void *ctx, const char *msg, ...)
{
/* shutting up XML Errors */
    if (ctx != NULL)
	ctx = NULL;		/* suppressing stupid compiler warnings (unused args) */
    if (msg != NULL)
	ctx = NULL;		/* suppressing stupid compiler warnings (unused args) */
}

static int
parse_hex (unsigned char hi, unsigned char lo, unsigned char *val)
{
/* attempting to parse an hexadecimal byte */
    unsigned char value;
    switch (hi)
      {
      case '0':
	  value = 0;
	  break;
      case '1':
	  value = 1 * 16;
	  break;
      case '2':
	  value = 2 * 16;
	  break;
      case '3':
	  value = 3 * 16;
	  break;
      case '4':
	  value = 4 * 16;
	  break;
      case '5':
	  value = 5 * 16;
	  break;
      case '6':
	  value = 6 * 16;
	  break;
      case '7':
	  value = 7 * 16;
	  break;
      case '8':
	  value = 8 * 16;
	  break;
      case '9':
	  value = 9 * 16;
	  break;
      case 'a':
      case 'A':
	  value = 10 * 16;
	  break;
      case 'b':
      case 'B':
	  value = 11 * 16;
	  break;
      case 'c':
      case 'C':
	  value = 12 * 16;
	  break;
      case 'd':
      case 'D':
	  value = 13 * 16;
	  break;
      case 'e':
      case 'E':
	  value = 14 * 16;
	  break;
      case 'f':
      case 'F':
	  value = 15 * 16;
	  break;
      default:
	  return 0;
	  break;
      };
    switch (lo)
      {
      case '0':
	  value += 0;
	  break;
      case '1':
	  value += 1;
	  break;
      case '2':
	  value += 2;
	  break;
      case '3':
	  value += 3;
	  break;
      case '4':
	  value += 4;
	  break;
      case '5':
	  value += 5;
	  break;
      case '6':
	  value += 6;
	  break;
      case '7':
	  value += 7;
	  break;
      case '8':
	  value += 8;
	  break;
      case '9':
	  value += 9;
	  break;
      case 'a':
      case 'A':
	  value += 10;
	  break;
      case 'b':
      case 'B':
	  value += 11;
	  break;
      case 'c':
      case 'C':
	  value += 12;
	  break;
      case 'd':
      case 'D':
	  value += 13;
	  break;
      case 'e':
      case 'E':
	  value += 14;
	  break;
      case 'f':
      case 'F':
	  value += 15;
	  break;
      default:
	  return 0;
	  break;
      };
    *val = value;
    return 1;
}

static int
parse_hex_color (const char *color, unsigned char *red,
		 unsigned char *green, unsigned char *blue)
{
/* attempting to parse a #RRGGBB hexadecimal color */
    unsigned char r;
    unsigned char g;
    unsigned char b;
    if (strlen (color) != 7)
	return 0;
    if (*color != '#')
	return 0;
    if (!parse_hex (*(color + 1), *(color + 2), &r))
	return 0;
    if (!parse_hex (*(color + 3), *(color + 4), &g))
	return 0;
    if (!parse_hex (*(color + 5), *(color + 6), &b))
	return 0;
    *red = r;
    *green = g;
    *blue = b;
    return 1;
}

static rl2MapConfigPtr
do_create_map_config ()
{
/* creating an empyt MapConfig */
    rl2MapConfigPtr map_config = malloc (sizeof (rl2MapConfig));
    if (map_config == NULL)
	return NULL;

    map_config->name = NULL;
    map_config->title = NULL;
    map_config->abstract = NULL;
    map_config->multithread_enabled = 0;
    map_config->max_threads = 1;
    map_config->srid = 0;
    map_config->autotransform_enabled = 0;
    map_config->dms = 0;
    map_config->map_background_red = 255;
    map_config->map_background_green = 255;
    map_config->map_background_blue = 255;
    map_config->map_background_transparent = 0;
    map_config->first_db = NULL;
    map_config->last_db = NULL;
    map_config->first_lyr = NULL;
    map_config->last_lyr = NULL;
    return map_config;
}

static void
do_destroy_map_attached_db (rl2MapAttachedDbPtr db)
{
/* memory cleanup - destroying a MapAttachedDB object */
    if (db == NULL)
	return;
    if (db->prefix != NULL)
	free (db->prefix);
    if (db->path != NULL)
	free (db->path);
    free (db);
}

static void
do_destroy_color_ramp (rl2MapColorRampPtr ramp)
{
/* memory cleanup - destroying a Color Ramp Object */
    if (ramp == NULL)
	return;
    if (ramp->min_color != NULL)
	free (ramp->min_color);
    if (ramp->max_color != NULL)
	free (ramp->max_color);
    free (ramp);
}

static void
do_destroy_raster_style (rl2MapRasterLayerStylePtr stl)
{
/* memory cleanup - destroying a Map Raster Layer Style object */
    if (stl == NULL)
	return;
    if (stl->contrast_enhancement != NULL)
	free (stl->contrast_enhancement);
    if (stl->channel_selection != NULL)
	free (stl->channel_selection);
    if (stl->color_map_name != NULL)
	free (stl->color_map_name);
    if (stl->color_ramp != NULL)
	do_destroy_color_ramp (stl->color_ramp);
    free (stl);
}

static void
do_destroy_graphic_fill (rl2MapGraphicFillPtr graphic)
{
/* memory cleanup - destroying an ExternalGraphic object */
    if (graphic == NULL)
	return;
    if (graphic->resource != NULL)
	free (graphic->resource);
    if (graphic->format != NULL)
	free (graphic->format);
    if (graphic->color != NULL)
	free (graphic->color);
    free (graphic);
}

static void
do_destroy_fill (rl2MapFillPtr fill)
{
/* memory cleanup - destroying a Fill object */
    if (fill == NULL)
	return;
    if (fill->graphic != NULL)
	do_destroy_graphic_fill (fill->graphic);
    free (fill);
}

static void
do_destroy_mark (rl2MapMarkPtr mark)
{
/* memory cleanup - destroying a Makr object */
    if (mark == NULL)
	return;
    if (mark->fill != NULL)
	do_destroy_fill (mark->fill);
    if (mark->stroke != NULL)
	free (mark->stroke);
    free (mark);
}

static void
do_destroy_placement (rl2MapPlacementPtr placement)
{
/* memory cleanup - destroying a LablePlacement object */
    if (placement == NULL)
	return;
    if (placement->point != NULL)
	free (placement->point);
    if (placement->line != NULL)
	free (placement->line);
    free (placement);
}

static void
do_destroy_point_sym (rl2MapPointSymbolizerPtr sym)
{
/* memory cleanup - destroying a PointSymbolyzer object */
    if (sym == NULL)
	return;
    if (sym->mark != NULL)
	do_destroy_mark (sym->mark);
    if (sym->graphic != NULL)
	do_destroy_graphic_fill (sym->graphic);
    free (sym);
}

static void
do_destroy_line_sym (rl2MapLineSymbolizerPtr sym)
{
/* memory cleanup - destroying a LineSymbolyzer object */
    if (sym == NULL)
	return;
    if (sym->stroke != NULL)
	free (sym->stroke);
    free (sym);
}

static void
do_destroy_polygon_sym (rl2MapPolygonSymbolizerPtr sym)
{
/* memory cleanup - destroying a PolygonSymbolyzer object */
    if (sym == NULL)
	return;
    if (sym->fill != NULL)
	do_destroy_fill (sym->fill);
    if (sym->stroke != NULL)
	free (sym->stroke);
    free (sym);
}

static void
do_destroy_text_sym (rl2MapTextSymbolizerPtr sym)
{
/* memory cleanup - destroying a TextSymbolyzer object */
    if (sym == NULL)
	return;
    if (sym->label != NULL)
	free (sym->label);
    if (sym->font != NULL)
      {
	  if (sym->font->family != NULL)
	      free (sym->font->family);
	  free (sym->font);
      }
    if (sym->placement != NULL)
	do_destroy_placement (sym->placement);
    if (sym->halo != NULL)
      {
	  do_destroy_fill (sym->halo->fill);
	  free (sym->halo);
      }
    if (sym->fill != NULL)
	do_destroy_fill (sym->fill);
    free (sym);
}

static void
do_destroy_vector_style (rl2MapVectorLayerStylePtr stl)
{
/* memory cleanup - destroying a Map Vector Layer Style object */
    rl2MapLineSymbolizerPtr pL;
    rl2MapLineSymbolizerPtr pLn;
    if (stl == NULL)
	return;
    if (stl->point_sym != NULL)
	do_destroy_point_sym (stl->point_sym);
    pL = stl->first_line_sym;
    while (pL != NULL)
      {
	  pLn = pL->next;
	  do_destroy_line_sym (pL);
	  pL = pLn;
      }
    if (stl->polygon_sym != NULL)
	do_destroy_polygon_sym (stl->polygon_sym);
    if (stl->text_sym != NULL)
	do_destroy_text_sym (stl->text_sym);
    free (stl);
}

static void
do_destroy_topology_style (rl2MapTopologyLayerStylePtr stl)
{
/* memory cleanup - destroying a Map Topology Layer Style object */
    if (stl == NULL)
	return;
    if (stl->faces_sym != NULL)
	do_destroy_polygon_sym (stl->faces_sym);
    if (stl->edges_sym != NULL)
	do_destroy_line_sym (stl->edges_sym);
    if (stl->nodes_sym != NULL)
	do_destroy_point_sym (stl->nodes_sym);
    if (stl->edge_seeds_sym != NULL)
	do_destroy_point_sym (stl->edge_seeds_sym);
    if (stl->face_seeds_sym != NULL)
	do_destroy_point_sym (stl->face_seeds_sym);
    free (stl);
}

static void
do_destroy_topology_internal_style (rl2MapTopologyLayerInternalStylePtr stl)
{
/* memory cleanup - destroying a Map Topology Layer Internal Style object */
    if (stl == NULL)
	return;
    if (stl->style_internal_name != NULL)
	free (stl->style_internal_name);
    free (stl);
}

static void
do_destroy_network_style (rl2MapNetworkLayerStylePtr stl)
{
/* memory cleanup - destroying a Map Network Layer Style object */
    if (stl == NULL)
	return;
    if (stl->links_sym != NULL)
	do_destroy_line_sym (stl->links_sym);
    if (stl->nodes_sym != NULL)
	do_destroy_point_sym (stl->nodes_sym);
    if (stl->link_seeds_sym != NULL)
	do_destroy_point_sym (stl->link_seeds_sym);
    free (stl);
}

static void
do_destroy_network_internal_style (rl2MapNetworkLayerInternalStylePtr stl)
{
/* memory cleanup - destroying a Map Network Layer Internal Style object */
    if (stl == NULL)
	return;
    if (stl->style_internal_name != NULL)
	free (stl->style_internal_name);
    free (stl);
}

static void
do_destroy_wms_style (rl2MapWmsLayerStylePtr stl)
{
/* memory cleanup - destroying a Map WMS Layer Style object */
    if (stl == NULL)
	return;
    if (stl->get_map_url != NULL)
	free (stl->get_map_url);
    if (stl->get_feature_info_url != NULL)
	free (stl->get_feature_info_url);
    if (stl->wms_protocol != NULL)
	free (stl->wms_protocol);
    if (stl->style != NULL)
	free (stl->style);
    if (stl->crs != NULL)
	free (stl->crs);
    if (stl->image_format != NULL)
	free (stl->image_format);
    if (stl->background_color != NULL)
	free (stl->background_color);
    free (stl);
}

static void
do_destroy_map_layer (rl2MapLayerPtr lyr)
{
/* memory cleanup - destroying a MapLayer object */
    if (lyr == NULL)
	return;
    if (lyr->prefix != NULL)
	free (lyr->prefix);
    if (lyr->name != NULL)
	free (lyr->name);
    if (lyr->raster_style_internal_name != NULL)
	free (lyr->raster_style_internal_name);
    if (lyr->vector_style_internal_name != NULL)
	free (lyr->vector_style_internal_name);
    if (lyr->raster_style != NULL)
	do_destroy_raster_style (lyr->raster_style);
    if (lyr->vector_style != NULL)
	do_destroy_vector_style (lyr->vector_style);
    if (lyr->topology_style != NULL)
	do_destroy_topology_style (lyr->topology_style);
    if (lyr->topology_internal_style != NULL)
	do_destroy_topology_internal_style (lyr->topology_internal_style);
    if (lyr->network_style != NULL)
	do_destroy_network_style (lyr->network_style);
    if (lyr->network_internal_style != NULL)
	do_destroy_network_internal_style (lyr->network_internal_style);
    if (lyr->wms_style != NULL)
	do_destroy_wms_style (lyr->wms_style);
    free (lyr);
}

RL2_DECLARE void
rl2_destroy_map_config (rl2MapConfigPtr ptr)
{
/* memory cleanup - destroying a MapConfig object */
    rl2MapAttachedDbPtr pDB;
    rl2MapAttachedDbPtr pDBn;
    rl2MapLayerPtr pL;
    rl2MapLayerPtr pLn;
    rl2MapConfigPtr map_config = (rl2MapConfigPtr) ptr;
    if (map_config == NULL)
	return;
    if (map_config->name != NULL)
	free (map_config->name);
    if (map_config->title != NULL)
	free (map_config->title);
    if (map_config->abstract != NULL)
	free (map_config->abstract);
    pDB = map_config->first_db;
    while (pDB != NULL)
      {
	  pDBn = pDB->next;
	  do_destroy_map_attached_db (pDB);
	  pDB = pDBn;
      }
    pL = map_config->first_lyr;
    while (pL != NULL)
      {
	  pLn = pL->next;
	  do_destroy_map_layer (pL);
	  pL = pLn;
      }
    free (map_config);
}

static rl2MapChannelSelectionPtr
do_add_channel_selection (rl2MapRasterLayerStylePtr style)
{
/* adding a Channel Selection to a Raster Layer */
    rl2MapChannelSelectionPtr channels;
    if (style == NULL)
	return NULL;
    if (style->channel_selection != NULL)
	free (style->channel_selection);
    style->channel_selection = NULL;

    channels = malloc (sizeof (rl2MapChannelSelection));
    if (channels == NULL)
	return NULL;
    channels->rgb = 0;
    channels->red_channel = 0;
    channels->green_channel = 0;
    channels->blue_channel = 0;
    channels->gray_channel = 0;
    style->channel_selection = channels;
    return channels;
}

static rl2MapContrastEnhancementPtr
do_add_contrast_enhancement (rl2MapRasterLayerStylePtr style)
{
/* adding a Constrant Enhancement to a Raster Layer */
    rl2MapContrastEnhancementPtr contrast;
    if (style == NULL)
	return NULL;
    if (style->contrast_enhancement != NULL)
	free (style->contrast_enhancement);
    style->contrast_enhancement = NULL;

    contrast = malloc (sizeof (rl2MapContrastEnhancement));
    if (contrast == NULL)
	return NULL;
    contrast->normalize = 0;
    contrast->histogram = 0;
    contrast->gamma = 0;
    contrast->gamma_value = 1.0;
    style->contrast_enhancement = contrast;
    return contrast;
}

static rl2MapColorRampPtr
do_add_color_ramp (rl2MapRasterLayerStylePtr style)
{
/* adding a Color Ramp to a Raster Layer */
    rl2MapColorRampPtr ramp;
    if (style == NULL)
	return NULL;
    if (style->color_ramp != NULL)
	free (style->color_ramp);
    style->color_ramp = NULL;

    ramp = malloc (sizeof (rl2MapColorRamp));
    if (ramp == NULL)
	return NULL;
    ramp->min_value = 0.0;
    ramp->max_value = 0.0;
    ramp->min_color = NULL;
    ramp->max_color = NULL;
    style->color_ramp = ramp;
    return ramp;
}

static rl2MapRasterLayerStylePtr
do_add_raster_style (rl2MapLayerPtr lyr)
{
/* adding a Raster Style for a Map Layer */
    rl2MapRasterLayerStylePtr style;
    if (lyr == NULL)
	return NULL;
    if (lyr->raster_style != NULL)
	do_destroy_raster_style (lyr->raster_style);
    lyr->raster_style = NULL;

    style = malloc (sizeof (rl2MapRasterLayerStyle));
    if (style == NULL)
	return NULL;
    style->opacity = 1.0;
    style->contrast_enhancement = NULL;
    style->color_map_name = NULL;
    style->color_ramp = NULL;
    style->channel_selection = NULL;
    style->shaded_relief = 0;
    style->relief_factor = 0.0;
    lyr->raster_style = style;
    return style;
}

static rl2MapVectorLayerStylePtr
do_add_vector_style (rl2MapLayerPtr lyr)
{
/* adding a Vector Style for a Map Layer */
    rl2MapVectorLayerStylePtr style;
    if (lyr == NULL)
	return NULL;
    if (lyr->vector_style != NULL)
	do_destroy_vector_style (lyr->vector_style);
    lyr->vector_style = NULL;

    style = malloc (sizeof (rl2MapVectorLayerStyle));
    if (style == NULL)
	return NULL;
    style->point_sym = NULL;
    style->first_line_sym = NULL;
    style->last_line_sym = NULL;
    style->polygon_sym = NULL;
    style->text_sym = NULL;
    lyr->vector_style = style;
    return style;
}

static rl2MapTopologyLayerStylePtr
do_add_topology_style (rl2MapLayerPtr lyr)
{
/* adding a Topology Style for a Map Layer */
    rl2MapTopologyLayerStylePtr style;
    if (lyr == NULL)
	return NULL;
    if (lyr->topology_style != NULL)
	do_destroy_topology_style (lyr->topology_style);
    lyr->topology_style = NULL;

    style = malloc (sizeof (rl2MapTopologyLayerStyle));
    if (style == NULL)
	return NULL;
    style->show_faces = 0;
    style->show_edges = 1;
    style->show_nodes = 1;
    style->show_edge_seeds = 1;
    style->show_face_seeds = 1;
    style->faces_sym = NULL;
    style->edges_sym = NULL;
    style->nodes_sym = NULL;
    style->edge_seeds_sym = NULL;
    style->face_seeds_sym = NULL;
    lyr->topology_style = style;
    return style;
}

static rl2MapTopologyLayerInternalStylePtr
do_add_topology_internal_style (rl2MapLayerPtr lyr)
{
/* adding a Topology Internal Style for a Map Layer */
    rl2MapTopologyLayerInternalStylePtr style;
    if (lyr == NULL)
	return NULL;
    if (lyr->topology_internal_style != NULL)
	do_destroy_topology_internal_style (lyr->topology_internal_style);
    lyr->topology_internal_style = NULL;

    style = malloc (sizeof (rl2MapTopologyLayerInternalStyle));
    if (style == NULL)
	return NULL;
    style->style_internal_name = NULL;
    style->show_faces = 0;
    style->show_edges = 1;
    style->show_nodes = 1;
    style->show_edge_seeds = 1;
    style->show_face_seeds = 1;
    lyr->topology_internal_style = style;
    return style;
}

static rl2MapNetworkLayerStylePtr
do_add_network_style (rl2MapLayerPtr lyr)
{
/* adding a Network Style for a Map Layer */
    rl2MapNetworkLayerStylePtr style;
    if (lyr == NULL)
	return NULL;
    if (lyr->network_style != NULL)
	do_destroy_network_style (lyr->network_style);
    lyr->network_style = NULL;

    style = malloc (sizeof (rl2MapNetworkLayerStyle));
    if (style == NULL)
	return NULL;
    style->show_links = 1;
    style->show_nodes = 1;
    style->show_link_seeds = 1;
    style->links_sym = NULL;
    style->nodes_sym = NULL;
    style->link_seeds_sym = NULL;
    lyr->network_style = style;
    return style;
}

static rl2MapNetworkLayerInternalStylePtr
do_add_network_internal_style (rl2MapLayerPtr lyr)
{
/* adding a Network Internal Style for a Map Layer */
    rl2MapNetworkLayerInternalStylePtr style;
    if (lyr == NULL)
	return NULL;
    if (lyr->network_internal_style != NULL)
	do_destroy_network_internal_style (lyr->network_internal_style);
    lyr->network_internal_style = NULL;

    style = malloc (sizeof (rl2MapNetworkLayerStyle));
    if (style == NULL)
	return NULL;
    style->style_internal_name = NULL;
    style->show_links = 1;
    style->show_nodes = 1;
    style->show_link_seeds = 1;
    lyr->network_internal_style = style;
    return style;
}

static rl2MapWmsLayerStylePtr
do_add_wms_style (rl2MapLayerPtr lyr)
{
/* adding a WMS Style for a Map Layer */
    rl2MapWmsLayerStylePtr style;
    if (lyr == NULL)
	return NULL;
    if (lyr->wms_style != NULL)
	do_destroy_wms_style (lyr->wms_style);
    lyr->wms_style = NULL;

    style = malloc (sizeof (rl2MapWmsLayerStyle));
    if (style == NULL)
	return NULL;
    style->get_map_url = NULL;
    style->get_feature_info_url = NULL;
    style->wms_protocol = NULL;
    style->style = NULL;
    style->crs = NULL;
    style->swap_xy = 0;
    style->image_format = NULL;
    style->opaque = 0;
    style->background_color = NULL;
    style->is_tiled = 0;
    style->tile_width = 0;
    style->tile_height = 0;
    lyr->wms_style = style;
    return style;
}

static rl2MapMarkPtr
do_add_mark (rl2MapPointSymbolizerPtr sym)
{
/* adding Mark to a Point Symbolizer */
    rl2MapMarkPtr mark;
    if (sym == NULL)
	return NULL;
    if (sym->mark != NULL)
	do_destroy_mark (sym->mark);
    sym->mark = NULL;

    mark = malloc (sizeof (rl2MapMark));
    if (mark == NULL)
	return NULL;
    mark->type = RL2_GRAPHIC_MARK_SQUARE;
    mark->fill = NULL;
    mark->stroke = NULL;
    sym->mark = mark;
    return mark;
}

static rl2MapFillPtr
do_add_mark_fill (rl2MapMarkPtr mark)
{
/* adding Fill to a Mark */
    rl2MapFillPtr fill;
    if (mark == NULL)
	return NULL;
    if (mark->fill != NULL)
	do_destroy_fill (mark->fill);
    mark->fill = NULL;

    fill = malloc (sizeof (rl2MapFill));
    if (fill == NULL)
	return NULL;
    fill->graphic = NULL;
    fill->red = 128;
    fill->green = 128;
    fill->blue = 128;
    fill->opacity = 1.0;
    mark->fill = fill;
    return fill;
}

static rl2MapStrokePtr
do_add_mark_stroke (rl2MapMarkPtr mark)
{
/* adding Stroke to a Mark */
    rl2MapStrokePtr stroke;
    if (mark == NULL)
	return NULL;
    if (mark->stroke != NULL)
	free (mark->stroke);
    mark->stroke = NULL;

    stroke = malloc (sizeof (rl2MapStroke));
    if (stroke == NULL)
	return NULL;
    stroke->red = 0;
    stroke->green = 0;
    stroke->blue = 0;
    stroke->opacity = 1.0;
    stroke->width = 1.0;
    stroke->dot_style = EXT_QUICK_STYLE_SOLID_LINE;
    mark->stroke = stroke;
    return stroke;
}


static rl2MapFillPtr
do_add_polygon_fill (rl2MapPolygonSymbolizerPtr sym)
{
/* adding Fill to a Polygon Symbolizer */
    rl2MapFillPtr fill;
    if (sym == NULL)
	return NULL;
    if (sym->fill != NULL)
	do_destroy_fill (sym->fill);
    sym->fill = NULL;

    fill = malloc (sizeof (rl2MapFill));
    if (fill == NULL)
	return NULL;
    fill->graphic = NULL;
    fill->red = 128;
    fill->green = 128;
    fill->blue = 128;
    fill->opacity = 1.0;
    sym->fill = fill;
    return fill;
}

static rl2MapStrokePtr
do_add_line_stroke (rl2MapLineSymbolizerPtr sym)
{
/* adding Stroke to a Line Symbolizer */
    rl2MapStrokePtr stroke;
    if (sym == NULL)
	return NULL;
    if (sym->stroke != NULL)
	free (sym->stroke);
    sym->stroke = NULL;

    stroke = malloc (sizeof (rl2MapStroke));
    if (stroke == NULL)
	return NULL;
    stroke->red = 0;
    stroke->green = 0;
    stroke->blue = 0;
    stroke->opacity = 1.0;
    stroke->width = 1.0;
    stroke->dot_style = EXT_QUICK_STYLE_SOLID_LINE;
    sym->stroke = stroke;
    return stroke;
}

static rl2MapStrokePtr
do_add_polygon_stroke (rl2MapPolygonSymbolizerPtr sym)
{
/* adding Stroke to a Polygon Symbolizer */
    rl2MapStrokePtr stroke;
    if (sym == NULL)
	return NULL;
    if (sym->stroke != NULL)
	free (sym->stroke);
    sym->stroke = NULL;

    stroke = malloc (sizeof (rl2MapStroke));
    if (stroke == NULL)
	return NULL;
    stroke->red = 0;
    stroke->green = 0;
    stroke->blue = 0;
    stroke->opacity = 1.0;
    stroke->opacity = 1.0;
    stroke->width = 1.0;
    stroke->dot_style = EXT_QUICK_STYLE_SOLID_LINE;
    sym->stroke = stroke;
    return stroke;
}

static rl2MapPointSymbolizerPtr
do_add_point_symbolizer (rl2MapVectorLayerStylePtr style)
{
/* adding a Point Symbolizer */
    rl2MapPointSymbolizerPtr sym;
    if (style == NULL)
	return NULL;
    if (style->point_sym != NULL)
	do_destroy_point_sym (style->point_sym);
    style->point_sym = NULL;

    sym = malloc (sizeof (rl2MapPointSymbolizer));
    sym->mark = NULL;
    sym->graphic = NULL;
    sym->opacity = 1.0;
    sym->size = 16.0;
    sym->anchor_x = 0.5;
    sym->anchor_y = 0.5;
    sym->displacement_x = 0.0;
    sym->displacement_y = 0.0;
    sym->rotation = 0.0;
    style->point_sym = sym;
    return sym;
}

static rl2MapLineSymbolizerPtr
do_add_line_symbolizer (rl2MapVectorLayerStylePtr style)
{
/* adding a Line Symbolizer */
    rl2MapLineSymbolizerPtr sym;
    if (style == NULL)
	return NULL;

    sym = malloc (sizeof (rl2MapLineSymbolizer));
    sym->stroke = NULL;
    sym->perpendicular_offset = 0.0;
    sym->next = NULL;
    if (style->first_line_sym == NULL)
	style->first_line_sym = sym;
    if (style->last_line_sym != NULL)
	style->last_line_sym->next = sym;
    style->last_line_sym = sym;
    return sym;
}

static rl2MapPolygonSymbolizerPtr
do_add_polygon_symbolizer (rl2MapVectorLayerStylePtr style)
{
/* adding a Polygon Symbolizer */
    rl2MapPolygonSymbolizerPtr sym;
    if (style == NULL)
	return NULL;
    if (style->polygon_sym != NULL)
	do_destroy_polygon_sym (style->polygon_sym);
    style->polygon_sym = NULL;

    sym = malloc (sizeof (rl2MapPolygonSymbolizer));
    sym->fill = NULL;
    sym->stroke = NULL;
    sym->displacement_x = 0.0;
    sym->displacement_y = 0.0;
    sym->perpendicular_offset = 0.0;
    style->polygon_sym = sym;
    return sym;
}

static rl2MapPolygonSymbolizerPtr
do_add_faces_symbolizer (rl2MapTopologyLayerStylePtr style)
{
/* adding a Polygon Symbolizer (Faces) */
    rl2MapPolygonSymbolizerPtr sym;
    if (style == NULL)
	return NULL;
    if (style->faces_sym != NULL)
	do_destroy_polygon_sym (style->faces_sym);
    style->faces_sym = NULL;

    sym = malloc (sizeof (rl2MapPolygonSymbolizer));
    sym->fill = NULL;
    sym->stroke = NULL;
    sym->displacement_x = 0.0;
    sym->displacement_y = 0.0;
    sym->perpendicular_offset = 0.0;
    style->faces_sym = sym;
    return sym;
}

static rl2MapLineSymbolizerPtr
do_add_edges_symbolizer (rl2MapTopologyLayerStylePtr style)
{
/* adding a Line Symbolizer (Edges) */
    rl2MapLineSymbolizerPtr sym;
    if (style == NULL)
	return NULL;
    if (style->edges_sym != NULL)
	do_destroy_line_sym (style->edges_sym);
    style->edges_sym = NULL;

    sym = malloc (sizeof (rl2MapLineSymbolizer));
    sym->stroke = NULL;
    sym->perpendicular_offset = 0.0;
    style->edges_sym = sym;
    return sym;
}

static rl2MapPointSymbolizerPtr
do_add_nodes_symbolizer (rl2MapTopologyLayerStylePtr style)
{
/* adding a Point Symbolizer (Nodes) */
    rl2MapPointSymbolizerPtr sym;
    if (style == NULL)
	return NULL;
    if (style->nodes_sym != NULL)
	do_destroy_point_sym (style->nodes_sym);
    style->nodes_sym = NULL;

    sym = malloc (sizeof (rl2MapPointSymbolizer));
    sym->mark = NULL;
    sym->graphic = NULL;
    sym->opacity = 1.0;
    sym->size = 16.0;
    sym->anchor_x = 0.5;
    sym->anchor_y = 0.5;
    sym->displacement_x = 0.0;
    sym->displacement_y = 0.0;
    sym->rotation = 0.0;
    style->nodes_sym = sym;
    return sym;
}

static rl2MapPointSymbolizerPtr
do_add_edge_seeeds_symbolizer (rl2MapTopologyLayerStylePtr style)
{
/* adding a Point Symbolizer (Edge  Seeeds) */
    rl2MapPointSymbolizerPtr sym;
    if (style == NULL)
	return NULL;
    if (style->edge_seeds_sym != NULL)
	do_destroy_point_sym (style->edge_seeds_sym);
    style->edge_seeds_sym = NULL;

    sym = malloc (sizeof (rl2MapPointSymbolizer));
    sym->mark = NULL;
    sym->graphic = NULL;
    sym->opacity = 1.0;
    sym->size = 16.0;
    sym->anchor_x = 0.5;
    sym->anchor_y = 0.5;
    sym->displacement_x = 0.0;
    sym->displacement_y = 0.0;
    sym->rotation = 0.0;
    style->edge_seeds_sym = sym;
    return sym;
}

static rl2MapPointSymbolizerPtr
do_add_face_seeeds_symbolizer (rl2MapTopologyLayerStylePtr style)
{
/* adding a Point Symbolizer (Face  Seeeds) */
    rl2MapPointSymbolizerPtr sym;
    if (style == NULL)
	return NULL;
    if (style->face_seeds_sym != NULL)
	do_destroy_point_sym (style->face_seeds_sym);
    style->face_seeds_sym = NULL;

    sym = malloc (sizeof (rl2MapPointSymbolizer));
    sym->mark = NULL;
    sym->graphic = NULL;
    sym->opacity = 1.0;
    sym->size = 16.0;
    sym->anchor_x = 0.5;
    sym->anchor_y = 0.5;
    sym->displacement_x = 0.0;
    sym->displacement_y = 0.0;
    sym->rotation = 0.0;
    style->face_seeds_sym = sym;
    return sym;
}

static rl2MapLineSymbolizerPtr
do_add_links_symbolizer (rl2MapNetworkLayerStylePtr style)
{
/* adding a Line Symbolizer (Links) */
    rl2MapLineSymbolizerPtr sym;
    if (style == NULL)
	return NULL;
    if (style->links_sym != NULL)
	do_destroy_line_sym (style->links_sym);
    style->links_sym = NULL;

    sym = malloc (sizeof (rl2MapLineSymbolizer));
    sym->stroke = NULL;
    sym->perpendicular_offset = 0.0;
    style->links_sym = sym;
    return sym;
}

static rl2MapPointSymbolizerPtr
do_add_net_nodes_symbolizer (rl2MapNetworkLayerStylePtr style)
{
/* adding a Point Symbolizer (Network Nodes) */
    rl2MapPointSymbolizerPtr sym;
    if (style == NULL)
	return NULL;
    if (style->nodes_sym != NULL)
	do_destroy_point_sym (style->nodes_sym);
    style->nodes_sym = NULL;

    sym = malloc (sizeof (rl2MapPointSymbolizer));
    sym->mark = NULL;
    sym->graphic = NULL;
    sym->opacity = 1.0;
    sym->size = 16.0;
    sym->anchor_x = 0.5;
    sym->anchor_y = 0.5;
    sym->displacement_x = 0.0;
    sym->displacement_y = 0.0;
    sym->rotation = 0.0;
    style->nodes_sym = sym;
    return sym;
}

static rl2MapPointSymbolizerPtr
do_add_link_seeeds_symbolizer (rl2MapNetworkLayerStylePtr style)
{
/* adding a Point Symbolizer (Link  Seeeds) */
    rl2MapPointSymbolizerPtr sym;
    if (style == NULL)
	return NULL;
    if (style->link_seeds_sym != NULL)
	do_destroy_point_sym (style->link_seeds_sym);
    style->link_seeds_sym = NULL;

    sym = malloc (sizeof (rl2MapPointSymbolizer));
    sym->mark = NULL;
    sym->graphic = NULL;
    sym->opacity = 1.0;
    sym->size = 16.0;
    sym->anchor_x = 0.5;
    sym->anchor_y = 0.5;
    sym->displacement_x = 0.0;
    sym->displacement_y = 0.0;
    sym->rotation = 0.0;
    style->link_seeds_sym = sym;
    return sym;
}

static rl2MapFontPtr
do_add_text_font (rl2MapTextSymbolizerPtr sym)
{
/* adding Font to a Text Symbolizer */
    rl2MapFontPtr font;
    if (sym == NULL)
	return NULL;
    if (sym->font != NULL)
      {
	  if (sym->font->family != NULL)
	      free (sym->font->family);
	  free (sym->font);
      }
    sym->font = NULL;

    font = malloc (sizeof (rl2MapFont));
    if (font == NULL)
	return NULL;
    font->family = NULL;
    font->style = RL2_FONTSTYLE_NORMAL;
    font->weight = RL2_FONTWEIGHT_NORMAL;
    font->size = 10.0;
    sym->font = font;
    return font;
}

static rl2MapFillPtr
do_add_text_fill (rl2MapTextSymbolizerPtr sym)
{
/* adding Fill to a Text Symbolizer */
    rl2MapFillPtr fill;
    if (sym == NULL)
	return NULL;
    if (sym->fill != NULL)
	do_destroy_fill (sym->fill);
    sym->fill = NULL;

    fill = malloc (sizeof (rl2MapFill));
    if (fill == NULL)
	return NULL;
    fill->graphic = NULL;
    fill->red = 128;
    fill->green = 128;
    fill->blue = 128;
    fill->opacity = 1.0;
    sym->fill = fill;
    return fill;
}

static rl2MapPointPlacementPtr
do_add_point_placement (rl2MapPlacementPtr placement)
{
/* adding a Point Placement */
    rl2MapPointPlacementPtr point;
    if (placement == NULL)
	return NULL;
    if (placement->point != NULL)
      {
	  free (placement->point);
	  placement->point = NULL;
      }

    point = malloc (sizeof (rl2MapPointPlacement));
    if (point == NULL)
	return NULL;
    point->anchor_x = 0.5;
    point->anchor_y = 0.5;
    point->displacement_x = 0.0;
    point->displacement_y = 0.0;
    point->rotation = 0.0;
    placement->point = point;
    return point;
}

static rl2MapLinePlacementPtr
do_add_line_placement (rl2MapPlacementPtr placement)
{
/* adding a Line Placement */
    rl2MapLinePlacementPtr line;
    if (placement == NULL)
	return NULL;
    if (placement->line != NULL)
      {
	  free (placement->line);
	  placement->line = NULL;
      }

    line = malloc (sizeof (rl2MapLinePlacement));
    if (line == NULL)
	return NULL;
    line->perpendicular_offset = 0.0;
    line->repeated = 0;
    line->initial_gap = 0.0;
    line->gap = 0.0;
    line->aligned = 0;
    line->generalize = 0;
    placement->line = line;
    return line;
}

static rl2MapPlacementPtr
do_add_text_placement (rl2MapTextSymbolizerPtr sym)
{
/* adding LabelPlacement to a Text Symbolizer */
    rl2MapPlacementPtr placement;
    if (sym == NULL)
	return NULL;
    if (sym->placement != NULL)
      {
	  do_destroy_placement (sym->placement);
	  sym->placement = NULL;
      }

    placement = malloc (sizeof (rl2MapPlacement));
    if (placement == NULL)
	return NULL;
    placement->point = NULL;
    placement->line = NULL;
    sym->placement = placement;
    return placement;
}

static rl2MapHaloPtr
do_add_text_halo (rl2MapTextSymbolizerPtr sym)
{
/* adding Fill to a Text Symbolizer */
    rl2MapHaloPtr halo;
    if (sym == NULL)
	return NULL;
    if (sym->halo != NULL)
      {
	  if (sym->halo->fill != NULL)
	      do_destroy_fill (sym->halo->fill);
	  free (sym->halo);
      }
    sym->halo = NULL;

    halo = malloc (sizeof (rl2MapHalo));
    if (halo == NULL)
	return NULL;
    halo->radius = 1.0;
    halo->fill = NULL;
    sym->halo = halo;
    return halo;
}

static rl2MapFillPtr
do_add_halo_fill (rl2MapHaloPtr halo)
{
/* adding Fill to Halo */
    rl2MapFillPtr fill;
    if (halo == NULL)
	return NULL;
    if (halo->fill != NULL)
	do_destroy_fill (halo->fill);
    halo->fill = NULL;

    fill = malloc (sizeof (rl2MapFill));
    if (fill == NULL)
	return NULL;
    fill->graphic = NULL;
    fill->red = 128;
    fill->green = 128;
    fill->blue = 128;
    fill->opacity = 1.0;
    halo->fill = fill;
    return fill;
}

static rl2MapTextSymbolizerPtr
do_add_text_symbolizer (rl2MapVectorLayerStylePtr style)
{
/* adding a Text Symbolizer */
    rl2MapTextSymbolizerPtr sym;
    if (style == NULL)
	return NULL;
    if (style->text_sym != NULL)
	do_destroy_text_sym (style->text_sym);
    style->text_sym = NULL;

    sym = malloc (sizeof (rl2MapTextSymbolizer));
    sym->label = NULL;
    sym->font = NULL;
    sym->placement = NULL;
    sym->halo = NULL;
    sym->fill = NULL;
    sym->alone = 0;
    style->text_sym = sym;
    return sym;
}

static void
do_add_map_attached_db (rl2MapConfigPtr map_config, const char *dbPrefix,
			const char *path)
{
/* adding a MapAttachedDB */
    int len;
    rl2MapAttachedDbPtr db = malloc (sizeof (rl2MapAttachedDb));
    if (db == NULL)
	return;

    len = strlen (dbPrefix);
    db->prefix = malloc (len + 1);
    strcpy (db->prefix, dbPrefix);
    len = strlen (path);
    db->path = malloc (len + 1);
    strcpy (db->path, path);
    db->next = NULL;

    if (map_config->first_db == NULL)
	map_config->first_db = db;
    if (map_config->last_db != NULL)
	map_config->last_db->next = db;
    map_config->last_db = db;
}

static rl2MapLayerPtr
do_add_map_layer (rl2MapConfigPtr map_config, const char *type,
		  const char *prefix, const char *name, int visible)
{
// adding a MapLayer */
    int len;
    rl2MapLayerPtr lyr;
    int tp = RL2_MAP_LAYER_UNKNOWN;
    if (strcmp (type, "raster") == 0)
	tp = RL2_MAP_LAYER_RASTER;
    if (strcmp (type, "wms") == 0)
	tp = RL2_MAP_LAYER_WMS;
    if (strcmp (type, "vector") == 0)
	tp = RL2_MAP_LAYER_VECTOR;
    if (strcmp (type, "vector_view") == 0)
	tp = RL2_MAP_LAYER_VECTOR_VIEW;
    if (strcmp (type, "vector_virtual") == 0)
	tp = RL2_MAP_LAYER_VECTOR_VIRTUAL;
    if (strcmp (type, "topology") == 0)
	tp = RL2_MAP_LAYER_TOPOLOGY;
    if (strcmp (type, "network") == 0)
	tp = RL2_MAP_LAYER_NETWORK;
    if (strcmp (type, "raster") == 0)
	tp = RL2_MAP_LAYER_RASTER;

    if (tp == RL2_MAP_LAYER_UNKNOWN)
	return NULL;

    lyr = malloc (sizeof (rl2MapLayer));
    if (lyr == NULL)
	return NULL;

    lyr->type = tp;
    if (prefix == NULL)
	lyr->prefix = NULL;
    else
      {
	  len = strlen (prefix);
	  lyr->prefix = malloc (len + 1);
	  strcpy (lyr->prefix, prefix);
      }
    len = strlen (name);
    lyr->name = malloc (len + 1);
    strcpy (lyr->name, name);
    lyr->visible = visible;
    lyr->raster_style_internal_name = NULL;
    lyr->vector_style_internal_name = NULL;
    lyr->raster_style = NULL;
    lyr->vector_style = NULL;
    lyr->topology_style = NULL;
    lyr->topology_internal_style = NULL;
    lyr->network_style = NULL;
    lyr->network_internal_style = NULL;
    lyr->wms_style = NULL;
    lyr->next = NULL;
    lyr->ok_min_scale = 0;
    lyr->ok_max_scale = 0;
    lyr->min_scale = 0.0;
    lyr->max_scale = 0.0;
    if (map_config->first_lyr == NULL)
	map_config->first_lyr = lyr;
    if (map_config->last_lyr != NULL)
	map_config->last_lyr->next = lyr;
    map_config->last_lyr = lyr;
    return lyr;
}

static void
do_add_graphic_fill (rl2MapFillPtr fill)
{
/* adding an External Graphic */
    rl2MapGraphicFillPtr graphic;

    if (fill == NULL)
	return;
    if (fill->graphic != NULL)
      {
	  do_destroy_graphic_fill (fill->graphic);
	  fill->graphic = NULL;
      }

    graphic = malloc (sizeof (rl2MapGraphicFill));
    if (graphic == NULL)
	return;

    graphic->resource = NULL;
    graphic->format = NULL;
    graphic->color = NULL;
    fill->graphic = graphic;
}

static void
do_add_graphic_fill_color (rl2MapGraphicFillPtr graphic, unsigned char red,
			   unsigned char green, unsigned char blue)
{
/* adding Color Replacement */
    rl2MapColorPtr color;

    if (graphic == NULL)
	return;
    if (graphic->color != NULL)
      {
	  free (graphic->color);
	  graphic->color = NULL;
      }

    color = malloc (sizeof (rl2MapColor));
    if (color == NULL)
	return;

    color->red = red;
    color->green = green;
    color->blue = blue;
    graphic->color = color;
}

static int
svg_parameter_name (xmlNodePtr node, const char **name, const char **value)
{
/* return the Name and Value from a <SvgParameter> */
    struct _xmlAttr *attr;

    *name = NULL;
    *value = NULL;
    attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *nm = (const char *) (attr->name);
		if (strcmp (nm, "name") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				*name = (const char *) (text->content);
			}
		  }
	    }
	  attr = attr->next;
      }
    if (name != NULL)
      {
	  xmlNodePtr child = node->children;
	  while (child)
	    {
		if (child->type == XML_TEXT_NODE && child->content != NULL)
		  {
		      *value = (const char *) (child->content);
		      return 1;
		  }
		child = child->next;
	    }
      }
    return 0;
}

static void
parse_scale_min_max (xmlNodePtr node, int *ok_min, double *min_scale,
		     int *ok_max, double *max_scale)
{
/* parsing <MinScaleDenominator> and <MaxScaleDenominator> */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MinScaleDenominator") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				    {
					*ok_min = 1;
					*min_scale = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "MaxScaleDenominator") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				    {
					*ok_max = 1;
					*max_scale = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_raster_opacity (xmlNodePtr node, rl2MapRasterLayerStylePtr style)
{
/* parsing an <Opacity> (RasterSymbolizer) tag */
    const char *value;

    while (node)
      {
	  if (node->type == XML_TEXT_NODE)
	    {
		value = (const char *) (node->content);
		if (value != NULL)
		    style->opacity = atof (value);
	    }
	  node = node->next;
      }
}

static int
parse_channel_name (xmlNodePtr node, int *channel)
{
/* parsing <SourceChannelName> */
    int ok = 0;
    int sel;
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "SourceChannelName") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (child->content);
				  if (value != NULL)
				    {
					sel = atoi (value);
					ok = 1;
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
    if (ok)
	*channel = sel;
    return ok;
}

static void
parse_channel_selection (xmlNodePtr node, rl2MapChannelSelectionPtr channels)
{
/* parsing <ChannelSelection> */
    int ok_red = 0;
    int ok_green = 0;
    int ok_blue = 0;
    int ok_gray = 0;
    int red;
    int green;
    int blue;
    int gray;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "RedChannel") == 0)
		    ok_red = parse_channel_name (node->children, &red);
		if (strcmp (name, "GreenChannel") == 0)
		    ok_green = parse_channel_name (node->children, &green);
		if (strcmp (name, "BlueChannel") == 0)
		    ok_blue = parse_channel_name (node->children, &blue);
		if (strcmp (name, "GrayChannel") == 0)
		    ok_gray = parse_channel_name (node->children, &gray);
	    }
	  node = node->next;
      }
    if (ok_red && ok_green && ok_blue)
      {
	  channels->rgb = 1;
	  channels->red_channel = red;
	  channels->green_channel = green;
	  channels->blue_channel = blue;
	  channels->gray_channel = 0;
      }
    else if (ok_gray)
      {
	  channels->rgb = 0;
	  channels->red_channel = 0;
	  channels->green_channel = 0;
	  channels->blue_channel = 0;
	  channels->gray_channel = gray;
      }
    else
      {
	  channels->rgb = 0;
	  channels->red_channel = 0;
	  channels->green_channel = 0;
	  channels->blue_channel = 0;
	  channels->gray_channel = 0;
      }
}

static void
parse_contrast_enhancement (xmlNodePtr node,
			    rl2MapContrastEnhancementPtr contrast)
{
/* parsing <ContrastEnhancement> */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Normalize") == 0)
		  {
		      contrast->normalize = 1;
		      contrast->histogram = 0;
		      contrast->gamma = 0;
		  }
		if (strcmp (name, "Histogram") == 0)
		  {
		      contrast->normalize = 0;
		      contrast->histogram = 1;
		      contrast->gamma = 0;
		  }
		if (strcmp (name, "Gamma") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				    {
					contrast->normalize = 0;
					contrast->histogram = 0;
					contrast->gamma = 1;
					contrast->gamma_value = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_shaded_relief (xmlNodePtr node, rl2MapRasterLayerStylePtr style)
{
/* parsing <ShadedRelief> */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ReliefFactor") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				    {
					style->shaded_relief = 1;
					style->relief_factor = atof (value);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_color_map (xmlNodePtr node, rl2MapRasterLayerStylePtr style)
{
/* parsing a <ColorMap> tag */
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "name") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (style->color_map_name != NULL)
				    {
					free (style->color_map_name);
					style->color_map_name = NULL;
				    }
				  if (value != NULL)
				    {
					int len = strlen (value);
					style->color_map_name =
					    malloc (len + 1);
					strcpy (style->color_map_name, value);
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_color_ramp (xmlNodePtr node, rl2MapColorRampPtr ramp)
{
/* parsing a <ColorRamp> tag */
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "MinValue") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (value != NULL)
				      ramp->min_value = atof (value);
			      }
			}
		  }
		if (strcmp (name, "MinColor") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (ramp->min_color != NULL)
				    {
					free (ramp->min_color);
					ramp->min_color = NULL;
				    }
				  if (value != NULL)
				    {
					int len = strlen (value);
					ramp->min_color = malloc (len + 1);
					strcpy (ramp->min_color, value);
				    }
			      }
			}
		  }
		if (strcmp (name, "MaxValue") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (value != NULL)
				      ramp->max_value = atof (value);
			      }
			}
		  }
		if (strcmp (name, "MaxColor") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (ramp->max_color != NULL)
				    {
					free (ramp->max_color);
					ramp->max_color = NULL;
				    }
				  if (value != NULL)
				    {
					int len = strlen (value);
					ramp->max_color = malloc (len + 1);
					strcpy (ramp->max_color, value);
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_raster_layer_style (xmlNodePtr node, rl2MapLayerPtr lyr)
{
/* parsing a <RasterLayerStyle> tag */
    rl2MapRasterLayerStylePtr style = lyr->raster_style;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Rule") == 0)
		  {
		      int ok_min = 0;
		      int ok_max = 0;
		      double min_scale = 0.0;
		      double max_scale = 0.0;
		      parse_scale_min_max (node->children, &ok_min, &min_scale,
					   &ok_max, &max_scale);
		      lyr->ok_min_scale = ok_min;
		      lyr->min_scale = min_scale;
		      lyr->ok_max_scale = ok_max;
		      lyr->max_scale = max_scale;
		  }
		if (strcmp (name, "RasterSymbolizer") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    name = (const char *) (child->name);
			    if (strcmp (name, "Opacity") == 0)
				parse_raster_opacity (child->children, style);
			    if (strcmp (name, "ChannelSelection") == 0)
			      {
				  rl2MapChannelSelectionPtr channels =
				      do_add_channel_selection (style);
				  if (channels != NULL)
				      parse_channel_selection (child->children,
							       channels);
			      }
			    if (strcmp (name, "ContrastEnhancement") == 0)
			      {
				  rl2MapContrastEnhancementPtr contrast =
				      do_add_contrast_enhancement (style);
				  if (contrast != NULL)
				      parse_contrast_enhancement
					  (child->children, contrast);
			      }
			    if (strcmp (name, "ColorMap") == 0)
				parse_color_map (child, style);
			    if (strcmp (name, "ColorRamp") == 0)
			      {
				  rl2MapColorRampPtr ramp =
				      do_add_color_ramp (style);
				  if (ramp != NULL)
				      parse_color_ramp (child, ramp);
			      }
			    if (strcmp (name, "ShadedRelief") == 0)
				parse_shaded_relief (child->children, style);
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_line_placement (xmlNodePtr node, rl2MapLinePlacementPtr place)
{
/* parsing Line Placement (TextSymbolizer) */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "PerpendicularOffset") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      place->perpendicular_offset =
					  atof (value);
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "IsRepeated") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    place->repeated = 1;
					else
					    place->repeated = 0;
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "InitialGap") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      place->initial_gap = atof (value);
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "Gap") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      place->gap = atof (value);
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "IsAligned") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    place->aligned = 1;
					else
					    place->aligned = 0;
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "GeneralizeLine") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    place->generalize = 1;
					else
					    place->generalize = 0;
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_label_anchor (xmlNodePtr node, rl2MapPointPlacementPtr place)
{
/* parsing TextSymbolizer label Anchor Point */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "AnchorPointX") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      place->anchor_x = atof (value);
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "AnchorPointY") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      place->anchor_y = atof (value);
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_label_displacement (xmlNodePtr node, rl2MapPointPlacementPtr place)
{
/* parsing TextSymbolizer Displacement */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "DisplacementX") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      place->displacement_x = atof (value);
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "DisplacementY") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      place->displacement_y = atof (value);
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_point_placement (xmlNodePtr node, rl2MapPointPlacementPtr place)
{
/* parsing Point Placement (TextSymbolizer) */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "AnchorPoint") == 0)
		    parse_label_anchor (node->children, place);
		if (strcmp (name, "Displacement") == 0)
		    parse_label_displacement (node->children, place);
		if (strcmp (name, "Rotation") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      place->rotation = atof (value);
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_text_placement (xmlNodePtr node, rl2MapPlacementPtr placement)
{
/* parsing TextSymbolizer LabelPlacement */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (node->type == XML_ELEMENT_NODE)
		  {
		      name = (const char *) (node->name);
		      if (strcmp (name, "PointPlacement") == 0)
			{
			    rl2MapPointPlacementPtr point =
				do_add_point_placement (placement);
			    if (point != NULL)
				parse_point_placement (node->children, point);
			}
		      if (strcmp (name, "LinePlacement") == 0)
			{
			    rl2MapLinePlacementPtr line =
				do_add_line_placement (placement);
			    if (line != NULL)
				parse_line_placement (node->children, line);
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_graphic_fill_color_replacement (xmlNodePtr node,
				      rl2MapGraphicFillPtr graphic)
{
/* parsing ColorReplacement (Graphic) */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Recode") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    name = (const char *) (child->name);
			    if (strcmp (name, "MapItem") == 0)
			      {
				  xmlNodePtr grandchild = child->children;
				  while (grandchild)
				    {
					name =
					    (const char *) (grandchild->name);
					if (strcmp (name, "Value") == 0)
					  {
					      xmlNodePtr grandchild2 =
						  grandchild->children;
					      while (grandchild2)
						{
						    if (grandchild2->type ==
							XML_TEXT_NODE)
						      {
							  unsigned char red;
							  unsigned char green;
							  unsigned char blue;
							  const char *value =
							      (const char
							       *)
							      (grandchild2->
							       content);
							  if (parse_hex_color
							      (value, &red,
							       &green, &blue))
							      do_add_graphic_fill_color
								  (graphic, red,
								   green, blue);
						      }
						    grandchild2 =
							grandchild2->next;
						}
					  }
					grandchild = grandchild->next;
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_graphic_fill (xmlNodePtr node, rl2MapGraphicFillPtr graphic)
{
/* parsing Graphic */
    const char *value;
    int len;

    while (node)
      {
	  const char *name = (const char *) (node->name);
	  if (strcmp (name, "ExternalGraphic") == 0)
	    {
		xmlNodePtr child = node->children;
		while (child)
		  {
		      name = (const char *) (child->name);
		      if (strcmp (name, "OnlineResource") == 0)
			{
			    xmlNodePtr grandchild = child->children;
			    while (grandchild)
			      {
				  if (grandchild->type == XML_TEXT_NODE)
				    {
					value =
					    (const char
					     *) (grandchild->content);
					if (graphic->resource != NULL)
					    free (graphic->resource);
					graphic->resource = NULL;
					if (value != NULL)
					  {
					      len = strlen (value);
					      graphic->resource =
						  malloc (len + 1);
					      strcpy (graphic->resource, value);
					  }
				    }
				  grandchild = grandchild->next;
			      }
			}
		      if (strcmp (name, "Format") == 0)
			{
			    xmlNodePtr grandchild = child->children;
			    while (grandchild)
			      {
				  if (grandchild->type == XML_TEXT_NODE)
				    {
					value =
					    (const char
					     *) (grandchild->content);
					if (graphic->format != NULL)
					    free (graphic->format);
					graphic->format = NULL;
					if (value != NULL)
					  {
					      len = strlen (value);
					      graphic->format =
						  malloc (len + 1);
					      strcpy (graphic->format, value);
					  }
				    }
				  grandchild = grandchild->next;
			      }
			}
		      if (strcmp (name, "ColorReplacement") == 0)
			  parse_graphic_fill_color_replacement (child->children,
								graphic);
		      child = child->next;
		  }
	    }

	  node = node->next;
      }
}

static void
parse_fill (xmlNodePtr node, rl2MapFillPtr fill)
{
/* parsing a <Fill> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "SvgParameter") == 0)
		  {
		      const char *svg_name;
		      const char *svg_value;
		      if (!svg_parameter_name (node, &svg_name, &svg_value))
			{
			    node = node->next;
			    continue;
			}
		      if (strcmp (svg_name, "fill") == 0)
			{
			    if (svg_value != NULL)
			      {
				  unsigned char red;
				  unsigned char green;
				  unsigned char blue;
				  if (parse_hex_color
				      (svg_value, &red, &green, &blue))
				    {
					fill->red = red;
					fill->green = green;
					fill->blue = blue;
				    }
			      }
			}
		      if (strcmp (svg_name, "fill-opacity") == 0)
			{
			    if (svg_value != NULL)
				fill->opacity = atof (svg_value);
			}
		  }
		if (strcmp (name, "GraphicFill") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    name = (const char *) (child->name);
			    if (strcmp (name, "Graphic") == 0)
			      {
				  do_add_graphic_fill (fill);
				  if (fill->graphic != NULL)
				      parse_graphic_fill (child->children,
							  fill->graphic);
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_stroke (xmlNodePtr node, rl2MapStrokePtr stroke)
{
/* parsing a <Stroke> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "SvgParameter") == 0)
		  {
		      const char *svg_name;
		      const char *svg_value;
		      if (!svg_parameter_name (node, &svg_name, &svg_value))
			{
			    node = node->next;
			    continue;
			}
		      if (strcmp (svg_name, "stroke") == 0)
			{
			    if (svg_value != NULL)
			      {
				  unsigned char red;
				  unsigned char green;
				  unsigned char blue;
				  if (parse_hex_color
				      (svg_value, &red, &green, &blue))
				    {
					stroke->red = red;
					stroke->green = green;
					stroke->blue = blue;
				    }
			      }
			}
		      if (strcmp (svg_name, "stroke-opacity") == 0)
			{
			    if (svg_value != NULL)
				stroke->opacity = atof (svg_value);
			}
		      if (strcmp (svg_name, "stroke-width") == 0)
			{
			    if (svg_value != NULL)
				stroke->width = atof (svg_value);
			}
		      if (strcmp (svg_name, "stroke-dasharray") == 0)
			{
			    if (svg_value != NULL)
			      {
				  if (strcmp (svg_value, "5.0, 10.0") == 0)
				      stroke->dot_style =
					  EXT_QUICK_STYLE_DOT_LINE;
				  else if (strcmp (svg_value, "20.0, 20.0") ==
					   0)
				      stroke->dot_style =
					  EXT_QUICK_STYLE_DASH_LINE;
				  else if (strcmp
					   (svg_value,
					    "20.0, 10.0, 5.0, 10.0") == 0)
				      stroke->dot_style =
					  EXT_QUICK_STYLE_DASH_DOT_LINE;
				  else
				      stroke->dot_style =
					  EXT_QUICK_STYLE_SOLID_LINE;
			      }
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_wkn (xmlNodePtr node, rl2MapMarkPtr mark)
{
/* parsing a <WellKnownName> (PointSymbolizer) tag */
    const char *value;
    mark->type = RL2_GRAPHIC_MARK_SQUARE;

    while (node)
      {
	  if (node->type == XML_TEXT_NODE)
	    {
		value = (const char *) (node->content);
		if (value != NULL)
		  {
		      if (strcmp (value, "square") == 0)
			  mark->type = RL2_GRAPHIC_MARK_SQUARE;
		      if (strcmp (value, "circle") == 0)
			  mark->type = RL2_GRAPHIC_MARK_CIRCLE;
		      if (strcmp (value, "triangle") == 0)
			  mark->type = RL2_GRAPHIC_MARK_TRIANGLE;
		      if (strcmp (value, "star") == 0)
			  mark->type = RL2_GRAPHIC_MARK_STAR;
		      if (strcmp (value, "cross") == 0)
			  mark->type = RL2_GRAPHIC_MARK_CROSS;
		      if (strcmp (value, "x") == 0)
			  mark->type = RL2_GRAPHIC_MARK_X;
		  }
	    }
	  node = node->next;
      }
}

static void
parse_point_opacity (xmlNodePtr node, rl2MapPointSymbolizerPtr sym)
{
/* parsing an <Opacity> (PointSymbolizer) tag */
    const char *value;

    while (node)
      {
	  if (node->type == XML_TEXT_NODE)
	    {
		value = (const char *) (node->content);
		if (value != NULL)
		    sym->opacity = atof (value);
	    }
	  node = node->next;
      }
}

static void
parse_point_size (xmlNodePtr node, rl2MapPointSymbolizerPtr sym)
{
/* parsing a <Size> (PointSymbolizer) tag */
    const char *value;

    while (node)
      {
	  if (node->type == XML_TEXT_NODE)
	    {
		value = (const char *) (node->content);
		if (value != NULL)
		    sym->size = atof (value);
	    }
	  node = node->next;
      }
}

static void
parse_point_rotation (xmlNodePtr node, rl2MapPointSymbolizerPtr sym)
{
/* parsing an <Rotation> (PointSymbolizer) tag */
    const char *value;

    while (node)
      {
	  if (node->type == XML_TEXT_NODE)
	    {
		value = (const char *) (node->content);
		if (value != NULL)
		    sym->rotation = atof (value);
	    }
	  node = node->next;
      }
}

static void
parse_point_anchor (xmlNodePtr node, rl2MapPointSymbolizerPtr sym)
{
/* parsing PointSymbolizer label Anchor Point */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "AnchorPointX") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      sym->anchor_x = atof (value);
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "AnchorPointY") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      sym->anchor_y = atof (value);
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_point_displacement (xmlNodePtr node, rl2MapPointSymbolizerPtr sym)
{
/* parsing PointSymbolizer Displacement */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "DisplacementX") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      sym->displacement_x = atof (value);
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "DisplacementY") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      sym->displacement_y = atof (value);
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_mark (xmlNodePtr node, rl2MapMarkPtr mark)
{
/* parsing a <Mark> (PointSymbolizer) tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "WellKnownName") == 0)
		    parse_wkn (node->children, mark);
		if (strcmp (name, "Fill") == 0)
		  {
		      rl2MapFillPtr fill = do_add_mark_fill (mark);
		      parse_fill (node->children, fill);
		  }
		if (strcmp (name, "Stroke") == 0)
		  {
		      rl2MapStrokePtr stroke = do_add_mark_stroke (mark);
		      parse_stroke (node->children, stroke);
		  }
	    }
	  node = node->next;
      }
}

static void
parse_graphic (xmlNodePtr node, rl2MapPointSymbolizerPtr sym)
{
/* parsing a <Graphic> (PointSymbolizer) tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Mark") == 0)
		  {
		      rl2MapMarkPtr mark = do_add_mark (sym);
		      parse_mark (node->children, mark);
		  }
		if (strcmp (name, "Opacity") == 0)
		    parse_point_opacity (node->children, sym);
		if (strcmp (name, "Size") == 0)
		    parse_point_size (node->children, sym);
		if (strcmp (name, "Rotation") == 0)
		    parse_point_rotation (node->children, sym);
		if (strcmp (name, "AnchorPoint") == 0)
		    parse_point_anchor (node->children, sym);
		if (strcmp (name, "Displacement") == 0)
		    parse_point_displacement (node->children, sym);
	    }
	  node = node->next;
      }
}

static void
parse_point_symbolizer (xmlNodePtr node, rl2MapPointSymbolizerPtr sym)
{
/* parsing a <PointSymbolizer> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Graphic") == 0)
		    parse_graphic (node->children, sym);
	    }
	  node = node->next;
      }
}

static void
parse_line_perpendicular_offset (xmlNodePtr node, rl2MapLineSymbolizerPtr sym)
{
/* parsing a <PerpendicularOffset> tag */
    const char *value;

    while (node)
      {
	  if (node->type == XML_TEXT_NODE)
	    {
		value = (const char *) (node->content);
		if (value != NULL)
		    sym->perpendicular_offset = atof (value);
	    }
	  node = node->next;
      }
}

static void
parse_line_symbolizer (xmlNodePtr node, rl2MapLineSymbolizerPtr sym)
{
/* parsing a <LineSymbolizer> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Stroke") == 0)
		  {
		      rl2MapStrokePtr stroke = do_add_line_stroke (sym);
		      parse_stroke (node->children, stroke);
		  }
		if (strcmp (name, "PerpendicularOffset") == 0)
		    parse_line_perpendicular_offset (node->children, sym);
	    }
	  node = node->next;
      }
}

static void
parse_polygon_displacement (xmlNodePtr node, rl2MapPolygonSymbolizerPtr sym)
{
/* parsing PolygonSymbolizer Displacement */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "DisplacementX") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      sym->displacement_x = atof (value);
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "DisplacementY") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (value != NULL)
				      sym->displacement_y = atof (value);
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_polygon_perpendicular_offset (xmlNodePtr node,
				    rl2MapPolygonSymbolizerPtr sym)
{
/* parsing a <PerpendicularOffset> tag */
    const char *value;

    while (node)
      {
	  if (node->type == XML_TEXT_NODE)
	    {
		value = (const char *) (node->content);
		if (value != NULL)
		    sym->perpendicular_offset = atof (value);
	    }
	  node = node->next;
      }
}

static void
parse_polygon_symbolizer (xmlNodePtr node, rl2MapPolygonSymbolizerPtr sym)
{
/* parsing a <PolygonSymbolizer> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Fill") == 0)
		  {
		      rl2MapFillPtr fill = do_add_polygon_fill (sym);
		      parse_fill (node->children, fill);
		  }
		if (strcmp (name, "Stroke") == 0)
		  {
		      rl2MapStrokePtr stroke = do_add_polygon_stroke (sym);
		      parse_stroke (node->children, stroke);
		  }
		if (strcmp (name, "Displacement") == 0)
		    parse_polygon_displacement (node->children, sym);
		if (strcmp (name, "PerpendicularOffset") == 0)
		    parse_polygon_perpendicular_offset (node->children, sym);
	    }
	  node = node->next;
      }
}

static void
parse_text_font (xmlNodePtr node, rl2MapFontPtr font)
{
/* parsing TextSymbolizer Font */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "SvgParameter") == 0)
		  {
		      int len;
		      const char *svg_name;
		      const char *svg_value;
		      if (!svg_parameter_name (node, &svg_name, &svg_value))
			{
			    node = node->next;
			    continue;
			}
		      if (strcmp (svg_name, "font-family") == 0)
			{
			    if (font->family != NULL)
			      {
				  free (font->family);
				  font->family = NULL;
			      }
			    if (svg_value != NULL)
			      {
				  len = strlen (svg_value);
				  font->family = malloc (len + 1);
				  strcpy (font->family, svg_value);
			      }
			}
		      if (strcmp (svg_name, "font-style") == 0)
			{
			    if (svg_value != NULL)
			      {
				  if (strcasecmp (svg_value, "normal") == 0)
				      font->style = RL2_FONTSTYLE_NORMAL;
				  if (strcasecmp (svg_value, "italic") == 0)
				      font->style = RL2_FONTSTYLE_ITALIC;
				  if (strcasecmp (svg_value, "oblique") == 0)
				      font->style = RL2_FONTSTYLE_OBLIQUE;
			      }
			}
		      if (strcmp (svg_name, "font-weight") == 0)
			{
			    if (svg_value != NULL)
			      {
				  if (strcasecmp (svg_value, "normal") == 0)
				      font->weight = RL2_FONTWEIGHT_NORMAL;
				  if (strcasecmp (svg_value, "bold") == 0)
				      font->weight = RL2_FONTWEIGHT_BOLD;
			      }
			}
		      if (strcmp (svg_name, "font-size") == 0)
			{
			    if (svg_value != NULL)
				font->size = atof (svg_value);
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_text_symbolizer (xmlNodePtr node, rl2MapTextSymbolizerPtr sym)
{
/* parsing a <TextSymbolizer> tag */
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "TextSymbolizerAlone") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  sym->alone = 0;
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    sym->alone = 1;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }

    node = node->children;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Label") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) child->content;
				  if (sym->label != NULL)
				      free (sym->label);
				  sym->label = NULL;
				  if (value != NULL)
				    {
					int len = strlen (value);
					sym->label = malloc (len + 1);
					strcpy (sym->label, value);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "Font") == 0)
		  {
		      rl2MapFontPtr font = do_add_text_font (sym);
		      parse_text_font (node->children, font);
		  }
		if (strcmp (name, "LabelPlacement") == 0)
		  {
		      rl2MapPlacementPtr placement =
			  do_add_text_placement (sym);
		      parse_text_placement (node->children, placement);
		  }
		if (strcmp (name, "Halo") == 0)
		  {
		      rl2MapHaloPtr halo = do_add_text_halo (sym);
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    const char *name = (const char *) (child->name);
			    if (strcmp (name, "Radius") == 0)
			      {
				  xmlNodePtr grandchild = child->children;
				  while (grandchild)
				    {
					if (grandchild->type == XML_TEXT_NODE)
					  {
					      const char *value =
						  (const char *)
						  grandchild->content;
					      if (value != NULL)
						  halo->radius = atof (value);
					  }
					grandchild = grandchild->next;
				    }
			      }
			    if (strcmp (name, "Fill") == 0)
			      {
				  rl2MapFillPtr fill = do_add_halo_fill (halo);
				  parse_fill (child->children, fill);
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "Fill") == 0)
		  {
		      rl2MapFillPtr fill = do_add_text_fill (sym);
		      parse_fill (node->children, fill);
		  }
	    }
	  node = node->next;
      }
}

static void
parse_vector_layer_style (xmlNodePtr node, rl2MapLayerPtr lyr)
{
/* parsing a <VectorLayerStyle> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Rule") == 0)
		  {
		      int ok_min = 0;
		      int ok_max = 0;
		      double min_scale = 0.0;
		      double max_scale = 0.0;
		      parse_scale_min_max (node->children, &ok_min, &min_scale,
					   &ok_max, &max_scale);
		      lyr->ok_min_scale = ok_min;
		      lyr->min_scale = min_scale;
		      lyr->ok_max_scale = ok_max;
		      lyr->max_scale = max_scale;
		  }
		if (strcmp (name, "PointSymbolizer") == 0)
		  {
		      rl2MapPointSymbolizerPtr sym =
			  do_add_point_symbolizer (lyr->vector_style);
		      if (sym != NULL)
			  parse_point_symbolizer (node->children, sym);
		  }
		if (strcmp (name, "LineSymbolizer") == 0)
		  {
		      rl2MapLineSymbolizerPtr sym =
			  do_add_line_symbolizer (lyr->vector_style);
		      if (sym != NULL)
			  parse_line_symbolizer (node->children, sym);
		  }
		if (strcmp (name, "PolygonSymbolizer") == 0)
		  {
		      rl2MapPolygonSymbolizerPtr sym =
			  do_add_polygon_symbolizer (lyr->vector_style);
		      if (sym != NULL)
			  parse_polygon_symbolizer (node->children, sym);
		  }
		if (strcmp (name, "TextSymbolizer") == 0)
		  {
		      rl2MapTextSymbolizerPtr sym =
			  do_add_text_symbolizer (lyr->vector_style);
		      if (sym != NULL)
			  parse_text_symbolizer (node, sym);
		  }
	    }
	  node = node->next;
      }
}

static void
parse_topology_style (xmlNodePtr node, rl2MapLayerPtr lyr)
{
/* parsing a <TopologyLayerStyle> tag */
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "ShowFaces") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->topology_style->show_faces = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->topology_style->show_faces = 1;
			      }
			}
		  }
		if (strcmp (name, "ShowEdges") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->topology_style->show_edges = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->topology_style->show_edges = 1;
			      }
			}
		  }
		if (strcmp (name, "ShowNodes") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->topology_style->show_nodes = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->topology_style->show_nodes = 1;
			      }
			}
		  }
		if (strcmp (name, "ShowEdgeSeeds") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->topology_style->show_edge_seeds = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->topology_style->show_edge_seeds = 1;
			      }
			}
		  }
		if (strcmp (name, "ShowFaceSeeds") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->topology_style->show_face_seeds = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->topology_style->show_face_seeds = 1;
			      }
			}
		  }
	    }
	  attr = attr->next;
      }
    node = node->children;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Rule") == 0)
		  {
		      int ok_min = 0;
		      int ok_max = 0;
		      double min_scale = 0.0;
		      double max_scale = 0.0;
		      parse_scale_min_max (node->children, &ok_min, &min_scale,
					   &ok_max, &max_scale);
		      lyr->ok_min_scale = ok_min;
		      lyr->min_scale = min_scale;
		      lyr->ok_max_scale = ok_max;
		      lyr->max_scale = max_scale;
		  }
		if (strcmp (name, "FacesStyle") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  const char *name =
				      (const char *) (child->name);
				  if (strcmp (name, "PolygonSymbolizer") == 0)
				    {
					rl2MapPolygonSymbolizerPtr sym =
					    do_add_faces_symbolizer
					    (lyr->topology_style);
					if (sym != NULL)
					    parse_polygon_symbolizer
						(child->children, sym);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "EdgesStyle") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  const char *name =
				      (const char *) (child->name);
				  if (strcmp (name, "LineSymbolizer") == 0)
				    {
					rl2MapLineSymbolizerPtr sym =
					    do_add_edges_symbolizer
					    (lyr->topology_style);
					if (sym != NULL)
					    parse_line_symbolizer
						(child->children, sym);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "NodesStyle") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  const char *name =
				      (const char *) (child->name);
				  if (strcmp (name, "PointSymbolizer") == 0)
				    {
					rl2MapPointSymbolizerPtr sym =
					    do_add_nodes_symbolizer
					    (lyr->topology_style);
					if (sym != NULL)
					    parse_point_symbolizer
						(child->children, sym);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "EdgeSeedsStyle") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  const char *name =
				      (const char *) (child->name);
				  if (strcmp (name, "PointSymbolizer") == 0)
				    {
					rl2MapPointSymbolizerPtr sym =
					    do_add_edge_seeeds_symbolizer
					    (lyr->topology_style);
					if (sym != NULL)
					    parse_point_symbolizer
						(child->children, sym);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "FaceSeedsStyle") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  const char *name =
				      (const char *) (child->name);
				  if (strcmp (name, "PointSymbolizer") == 0)
				    {
					rl2MapPointSymbolizerPtr sym =
					    do_add_face_seeeds_symbolizer
					    (lyr->topology_style);
					if (sym != NULL)
					    parse_point_symbolizer
						(child->children, sym);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_topology_internal_style (xmlNodePtr node, rl2MapLayerPtr lyr)
{
/* parsing a <TopologyLayerInternalStyle> tag */
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "name") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    int len;
			    const char *name;
			    if (text->type == XML_TEXT_NODE)
				name = (const char *) (text->content);
			    if (lyr->
				topology_internal_style->style_internal_name !=
				NULL)
				free (lyr->
				      topology_internal_style->style_internal_name);
			    lyr->topology_internal_style->style_internal_name =
				NULL;
			    if (name != NULL)
			      {
				  len = strlen (name);
				  lyr->
				      topology_internal_style->style_internal_name
				      = malloc (len + 1);
				  strcpy (lyr->
					  topology_internal_style->style_internal_name,
					  name);
			      }
			}
		  }
		if (strcmp (name, "ShowFaces") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->topology_internal_style->show_faces = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->topology_internal_style->show_faces =
					  1;
			      }
			}
		  }
		if (strcmp (name, "ShowEdges") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->topology_internal_style->show_edges = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->topology_internal_style->show_edges =
					  1;
			      }
			}
		  }
		if (strcmp (name, "ShowNodes") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->topology_internal_style->show_nodes = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->topology_internal_style->show_nodes =
					  1;
			      }
			}
		  }
		if (strcmp (name, "ShowEdgeSeeds") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->topology_internal_style->show_edge_seeds = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->
					  topology_internal_style->show_edge_seeds
					  = 1;
			      }
			}
		  }
		if (strcmp (name, "ShowFaceSeeds") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->topology_internal_style->show_face_seeds = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->
					  topology_internal_style->show_face_seeds
					  = 1;
			      }
			}
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_network_style (xmlNodePtr node, rl2MapLayerPtr lyr)
{
/* parsing a <NetworkLayerStyle> tag */
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "ShowLinks") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->network_style->show_links = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->network_style->show_links = 1;
			      }
			}
		  }
		if (strcmp (name, "ShowNodes") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->network_style->show_nodes = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->network_style->show_nodes = 1;
			      }
			}
		  }
		if (strcmp (name, "ShowLinkSeeds") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->network_style->show_link_seeds = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->network_style->show_link_seeds = 1;
			      }
			}
		  }
	    }
	  attr = attr->next;
      }
    node = node->children;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Rule") == 0)
		  {
		      int ok_min = 0;
		      int ok_max = 0;
		      double min_scale = 0.0;
		      double max_scale = 0.0;
		      parse_scale_min_max (node->children, &ok_min, &min_scale,
					   &ok_max, &max_scale);
		      lyr->ok_min_scale = ok_min;
		      lyr->min_scale = min_scale;
		      lyr->ok_max_scale = ok_max;
		      lyr->max_scale = max_scale;
		  }
		if (strcmp (name, "LinksStyle") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  const char *name =
				      (const char *) (child->name);
				  if (strcmp (name, "LineSymbolizer") == 0)
				    {
					rl2MapLineSymbolizerPtr sym =
					    do_add_links_symbolizer
					    (lyr->network_style);
					if (sym != NULL)
					    parse_line_symbolizer
						(child->children, sym);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "NodesStyle") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  const char *name =
				      (const char *) (child->name);
				  if (strcmp (name, "PointSymbolizer") == 0)
				    {
					rl2MapPointSymbolizerPtr sym =
					    do_add_net_nodes_symbolizer
					    (lyr->network_style);
					if (sym != NULL)
					    parse_point_symbolizer
						(child->children, sym);
				    }
			      }
			    child = child->next;
			}
		  }
		if (strcmp (name, "LinkSeedsStyle") == 0)
		  {
		      xmlNodePtr child = node->children;
		      while (child)
			{
			    if (child->type == XML_ELEMENT_NODE)
			      {
				  const char *name =
				      (const char *) (child->name);
				  if (strcmp (name, "PointSymbolizer") == 0)
				    {
					rl2MapPointSymbolizerPtr sym =
					    do_add_link_seeeds_symbolizer
					    (lyr->network_style);
					if (sym != NULL)
					    parse_point_symbolizer
						(child->children, sym);
				    }
			      }
			    child = child->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_network_internal_style (xmlNodePtr node, rl2MapLayerPtr lyr)
{
/* parsing a <NetworkLayerInternalStyle> tag */
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "name") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    int len;
			    const char *name;
			    if (text->type == XML_TEXT_NODE)
				name = (const char *) (text->content);
			    if (lyr->
				network_internal_style->style_internal_name !=
				NULL)
				free (lyr->
				      network_internal_style->style_internal_name);
			    lyr->network_internal_style->style_internal_name =
				NULL;
			    if (name != NULL)
			      {
				  len = strlen (name);
				  lyr->
				      network_internal_style->style_internal_name
				      = malloc (len + 1);
				  strcpy (lyr->
					  network_internal_style->style_internal_name,
					  name);
			      }
			}
		  }
		if (strcmp (name, "ShowLinks") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->network_internal_style->show_links = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->network_internal_style->show_links =
					  1;
			      }
			}
		  }
		if (strcmp (name, "ShowNodes") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->network_internal_style->show_nodes = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->network_internal_style->show_nodes =
					  1;
			      }
			}
		  }
		if (strcmp (name, "ShowLinkSeeds") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr->network_internal_style->show_link_seeds = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  if (strcmp
				      ((const char *) (text->content),
				       "true") == 0)
				      lyr->
					  network_internal_style->show_link_seeds
					  = 1;
			      }
			}
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_wms_get_map (xmlNodePtr node, rl2MapWmsLayerStylePtr style)
{
/* parsing a <WmsLayerStyle><GetMap> tag */
    int len;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "URL") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (style->get_map_url != NULL)
			{
			    free (style->get_map_url);
			    style->get_map_url = NULL;
			}
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (value != NULL)
				    {
					len = strlen (value);
					style->get_map_url = malloc (len + 1);
					strcpy (style->get_map_url, value);
				    }
			      }
			}
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_wms_get_feature_info (xmlNodePtr node, rl2MapWmsLayerStylePtr style)
{
/* parsing a <WmsLayerStyle><GetFeatureInfo> tag */
    int len;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "URL") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (style->get_feature_info_url != NULL)
			{
			    free (style->get_feature_info_url);
			    style->get_feature_info_url = NULL;
			}
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (value != NULL)
				    {
					len = strlen (value);
					style->get_feature_info_url =
					    malloc (len + 1);
					strcpy (style->get_feature_info_url,
						value);
				    }
			      }
			}
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_wms_protocol (xmlNodePtr node, rl2MapWmsLayerStylePtr style)
{
/* parsing a <WmsLayerStyle><WmsProtocol> tag */
    int len;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "Version") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (style->wms_protocol != NULL)
			{
			    free (style->wms_protocol);
			    style->wms_protocol = NULL;
			}
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (value != NULL)
				    {
					len = strlen (value);
					style->wms_protocol = malloc (len + 1);
					strcpy (style->wms_protocol, value);
				    }
			      }
			}
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_wms_reference_system (xmlNodePtr node, rl2MapWmsLayerStylePtr style)
{
/* parsing a <WmsLayerStyle><ReferenceSystem> tag */
    int len;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "Crs") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (style->crs != NULL)
			{
			    free (style->crs);
			    style->crs = NULL;
			}
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (value != NULL)
				    {
					len = strlen (value);
					style->crs = malloc (len + 1);
					strcpy (style->crs, value);
				    }
			      }
			}
		  }
		if (strcmp (name, "SwapXY") == 0)
		  {
		      xmlNode *text = attr->children;
		      style->swap_xy = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    style->swap_xy = 1;
				    }
			      }
			}
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_wms_image_format (xmlNodePtr node, rl2MapWmsLayerStylePtr style)
{
/* parsing a <WmsLayerStyle><ImageFormat> tag */
    int len;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "MimeType") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (style->image_format != NULL)
			{
			    free (style->image_format);
			    style->image_format = NULL;
			}
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (value != NULL)
				    {
					len = strlen (value);
					style->image_format = malloc (len + 1);
					strcpy (style->image_format, value);
				    }
			      }
			}
		  }
		if (strcmp (name, "Opaque") == 0)
		  {
		      xmlNode *text = attr->children;
		      style->opaque = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    style->opaque = 1;
				    }
			      }
			}
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_wms_tile_size (xmlNodePtr node, rl2MapWmsLayerStylePtr style)
{
/* parsing a <WmsLayerStyle><TileSize> tag */
    style->is_tiled = 1;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "Width") == 0)
		  {
		      xmlNode *text = attr->children;

		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  style->tile_width = atof (value);
			      }
			}
		  }
		if (strcmp (name, "Height") == 0)
		  {
		      xmlNode *text = attr->children;

		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  style->tile_height = atof (value);
			      }
			}
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_wms_style (xmlNodePtr node, rl2MapLayerPtr lyr)
{
/* parsing a <WmsLayerStyle> tag */
    int len;
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Rule") == 0)
		  {
		      int ok_min = 0;
		      int ok_max = 0;
		      double min_scale = 0.0;
		      double max_scale = 0.0;
		      parse_scale_min_max (node->children, &ok_min, &min_scale,
					   &ok_max, &max_scale);
		      lyr->ok_min_scale = ok_min;
		      lyr->min_scale = min_scale;
		      lyr->ok_max_scale = ok_max;
		      lyr->max_scale = max_scale;
		  }
		if (strcmp (name, "Style") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (lyr->wms_style->style != NULL)
					  {
					      free (lyr->wms_style->style);
					      lyr->wms_style->style = NULL;
					  }
				    }
				  else
				    {
					if (lyr->wms_style->style != NULL)
					    free (lyr->wms_style->style);
					len = strlen (value);
					lyr->wms_style->style =
					    malloc (len + 1);
					strcpy (lyr->wms_style->style, value);
				    }
			      }
			    text = text->next;
			}
		  }
		if (strcmp (name, "BgColor") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (lyr->wms_style->background_color !=
					    NULL)
					  {
					      free (lyr->
						    wms_style->background_color);
					      lyr->wms_style->background_color =
						  NULL;
					  }
				    }
				  else
				    {
					if (lyr->wms_style->background_color !=
					    NULL)
					    free (lyr->
						  wms_style->background_color);
					len = strlen (value);
					lyr->wms_style->background_color =
					    malloc (len + 1);
					strcpy (lyr->
						wms_style->background_color,
						value);
				    }
			      }
			    text = text->next;
			}
		  }
		if (strcmp (name, "GetMap") == 0)
		    parse_wms_get_map (node, lyr->wms_style);
		if (strcmp (name, "GetFeatureInfo") == 0)
		    parse_wms_get_feature_info (node, lyr->wms_style);
		if (strcmp (name, "WmsProtocol") == 0)
		    parse_wms_protocol (node, lyr->wms_style);
		if (strcmp (name, "ReferenceSystem") == 0)
		    parse_wms_reference_system (node, lyr->wms_style);
		if (strcmp (name, "ImageFormat") == 0)
		    parse_wms_image_format (node, lyr->wms_style);
		if (strcmp (name, "TileSize") == 0)
		    parse_wms_tile_size (node, lyr->wms_style);
	    }
	  node = node->next;
      }
}

static void
parse_raster_layer_internal_style (xmlNodePtr node, rl2MapLayerPtr lyr)
{
/* parsing a <RasterLayerInternalStyle> tag */
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "name") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    int len;
			    const char *name;
			    if (text->type == XML_TEXT_NODE)
				name = (const char *) (text->content);
			    if (lyr->raster_style_internal_name != NULL)
				free (lyr->raster_style_internal_name);
			    lyr->raster_style_internal_name = NULL;
			    if (name != NULL)
			      {
				  len = strlen (name);
				  lyr->raster_style_internal_name =
				      malloc (len + 1);
				  strcpy (lyr->raster_style_internal_name,
					  name);
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_vector_layer_internal_style (xmlNodePtr node, rl2MapLayerPtr lyr)
{
/* parsing a <VectorLayerInternalStyle> tag */
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "name") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    int len;
			    const char *name;
			    if (text->type == XML_TEXT_NODE)
				name = (const char *) (text->content);
			    if (lyr->vector_style_internal_name != NULL)
				free (lyr->vector_style_internal_name);
			    lyr->vector_style_internal_name = NULL;
			    if (name != NULL)
			      {
				  len = strlen (name);
				  lyr->vector_style_internal_name =
				      malloc (len + 1);
				  strcpy (lyr->vector_style_internal_name,
					  name);
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_map_layer (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <MapLayer> tag */
    const char *type = NULL;
    const char *prefix = NULL;
    const char *xname = NULL;
    int visible = 0;
    rl2MapLayerPtr lyr = NULL;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "Type") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				type = (const char *) (text->content);
			}
		  }
		if (strcmp (name, "DbPrefix") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				prefix = (const char *) (text->content);
			}
		  }
		if (strcmp (name, "Name") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				xname = (const char *) (text->content);
			}
		  }
		if (strcmp (name, "Visible") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  const char *value =
				      (const char *) (text->content);
				  visible = 0;
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    visible = 1;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
    if (type != NULL && xname != NULL)
	lyr = do_add_map_layer (map_config, type, prefix, xname, visible);
    if (lyr == NULL)
	return;

    node = node->children;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "RasterLayerInternalStyle") == 0)
		    parse_raster_layer_internal_style (node, lyr);
		if (strcmp (name, "VectorLayerInternalStyle") == 0)
		    parse_vector_layer_internal_style (node, lyr);
		if (strcmp (name, "RasterLayerStyle") == 0)
		  {
		      rl2MapRasterLayerStylePtr style =
			  do_add_raster_style (lyr);
		      if (style != NULL)
			  parse_raster_layer_style (node->children, lyr);
		  }
		if (strcmp (name, "VectorLayerStyle") == 0)
		  {
		      rl2MapVectorLayerStylePtr style =
			  do_add_vector_style (lyr);
		      if (style != NULL)
			  parse_vector_layer_style (node->children, lyr);
		  }
		if (strcmp (name, "TopologyLayerStyle") == 0)
		  {
		      rl2MapTopologyLayerStylePtr style =
			  do_add_topology_style (lyr);
		      if (style != NULL)
			  parse_topology_style (node, lyr);
		  }
		if (strcmp (name, "TopologyLayerInternalStyle") == 0)
		  {
		      rl2MapTopologyLayerInternalStylePtr style =
			  do_add_topology_internal_style (lyr);
		      if (style != NULL)
			  parse_topology_internal_style (node, lyr);
		  }
		if (strcmp (name, "NetworkLayerStyle") == 0)
		  {
		      rl2MapNetworkLayerStylePtr style =
			  do_add_network_style (lyr);
		      if (style != NULL)
			  parse_network_style (node, lyr);
		  }
		if (strcmp (name, "NetworkLayerInternalStyle") == 0)
		  {
		      rl2MapNetworkLayerInternalStylePtr style =
			  do_add_network_internal_style (lyr);
		      if (style != NULL)
			  parse_network_internal_style (node, lyr);
		  }
		if (strcmp (name, "WmsLayerStyle") == 0)
		  {
		      rl2MapWmsLayerStylePtr style = do_add_wms_style (lyr);
		      if (style != NULL)
			  parse_wms_style (node->children, lyr);
		  }
	    }
	  node = node->next;
      }
}

static void
parse_map_attached_DB (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <MapAttachedDB> tag */
    const char *dbPrefix = NULL;
    const char *path = NULL;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "DbPrefix") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				dbPrefix = (const char *) (text->content);
			}
		  }
		if (strcmp (name, "Path") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				path = (const char *) (text->content);
			}
		  }
		attr = attr->next;
	    }
      }
    if (dbPrefix != NULL && path != NULL)
	do_add_map_attached_db (map_config, dbPrefix, path);
}

static void
parse_map_attached_databases (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <MapAttachedDatabases> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MapAttachedDB") == 0)
		    parse_map_attached_DB (node, map_config);
	    }
	  node = node->next;
      }
}

static void
parse_multi_threading (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <MultiThreading> tag */
    const char *value;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "Enabled") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->multithread_enabled = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    map_config->multithread_enabled = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "MaxThreads") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->max_threads = 1;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				      map_config->max_threads = atoi (value);
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_map_crs (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <MapCrs> tag */
    const char *value;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "Crs") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->srid = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strncmp (value, "EPSG:", 5) == 0)
					    map_config->srid = atoi (value + 5);
				    }
			      }
			}
		  }
		if (strcmp (name, "AutoTransformEnabled") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->autotransform_enabled = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    map_config->autotransform_enabled =
						1;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_geographic_coords (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <GeographicCoords> tag */
    const char *value;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "DMS") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->dms = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    map_config->dms = 1;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_map_background (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <MapBackground> tag */
    const char *value;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "Color") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->map_background_red = 255;
		      map_config->map_background_green = 255;
		      map_config->map_background_blue = 255;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					unsigned char red;
					unsigned char green;
					unsigned char blue;
					if (parse_hex_color
					    (value, &red, &green, &blue))
					  {
					      map_config->map_background_red =
						  red;
					      map_config->map_background_green =
						  green;
					      map_config->map_background_blue =
						  blue;
					  }
				    }
			      }
			}
		  }
		if (strcmp (name, "Transparent") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->map_background_transparent = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    map_config->
						map_background_transparent = 1;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_map_options (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <MapOptions> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MultiThreading") == 0)
		    parse_multi_threading (node, map_config);
		if (strcmp (name, "MapCrs") == 0)
		    parse_map_crs (node, map_config);
		if (strcmp (name, "GeographicCoords") == 0)
		    parse_geographic_coords (node, map_config);
		if (strcmp (name, "MapBackground") == 0)
		    parse_map_background (node, map_config);
	    }
	  node = node->next;
      }
}

static void
parse_map_config_description (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <RL2MapConfig><Description> tag */
    const char *value;
    int len;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Title") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (map_config->title != NULL)
					  {
					      free (map_config->title);
					      map_config->title = NULL;
					  }
				    }
				  else
				    {
					if (map_config->title != NULL)
					    free (map_config->title);
					len = strlen (value);
					map_config->title = malloc (len + 1);
					strcpy (map_config->title, value);
				    }
			      }
			    text = text->next;
			}
		  }
		if (strcmp (name, "Abstract") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (map_config->abstract != NULL)
					  {
					      free (map_config->abstract);
					      map_config->abstract = NULL;
					  }
				    }
				  else
				    {
					if (map_config->abstract != NULL)
					    free (map_config->abstract);
					len = strlen (value);
					map_config->abstract = malloc (len + 1);
					strcpy (map_config->abstract, value);
				    }
			      }
			    text = text->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_map_config (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing an <RL2MapConfig> tag */
    const char *value;
    int len;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Name") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (map_config->name != NULL)
					  {
					      free (map_config->name);
					      map_config->name = NULL;
					  }
				    }
				  else
				    {
					if (map_config->name != NULL)
					    free (map_config->name);
					len = strlen (value);
					map_config->name = malloc (len + 1);
					strcpy (map_config->name, value);
				    }
			      }
			    text = text->next;
			}
		  }
		if (strcmp (name, "Description") == 0)
		    parse_map_config_description (node->children, map_config);
		if (strcmp (name, "MapOptions") == 0)
		    parse_map_options (node->children, map_config);
		if (strcmp (name, "MapAttachedDatabases") == 0)
		    parse_map_attached_databases (node->children, map_config);
		if (strcmp (name, "MapLayer") == 0)
		    parse_map_layer (node, map_config);
	    }
	  node = node->next;
      }
}

static int
find_map_config (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* searching an RL2MapConfig tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "RL2MapConfig") == 0)
		  {
		      parse_map_config (node->children, map_config);
		      return 1;
		  }
	    }
	  node = node->next;
      }
    return 0;
}

RL2_DECLARE rl2MapConfigPtr
rl2_parse_map_config_xml (const unsigned char *xml)
{
/* attempting to build a MapConfig object from an XML definition */
    rl2MapConfigPtr map_config = NULL;
    xmlDocPtr xml_doc = NULL;
    xmlNodePtr root;
    xmlGenericErrorFunc silentError = (xmlGenericErrorFunc) dummySilentError;

    map_config = do_create_map_config ();
    if (map_config == NULL)
	return NULL;

/* parsing the XML document */
    xmlSetGenericErrorFunc (NULL, silentError);
    xml_doc =
	xmlReadMemory ((const char *) xml, strlen ((const char *) xml),
		       "noname.xml", NULL, 0);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  goto error;
      }
    root = xmlDocGetRootElement (xml_doc);
    if (root == NULL)
	goto error;
    if (!find_map_config (root, map_config))
	goto error;

    xmlFreeDoc (xml_doc);
    return map_config;

  error:
    if (xml_doc != NULL)
	xmlFreeDoc (xml_doc);
    if (map_config != NULL)
	rl2_destroy_map_config ((rl2MapConfigPtr) map_config);
    return NULL;
}
