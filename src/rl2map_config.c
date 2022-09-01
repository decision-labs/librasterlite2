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

#include "spatialite/gaiaaux.h"

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

    map_config->valid = 1;
    map_config->name = NULL;
    map_config->title = NULL;
    map_config->abstract = NULL;
    map_config->min_scale = -1.0;
    map_config->max_scale = -1.0;
    map_config->multithread_enabled = 0;
    map_config->max_threads = 1;
    map_config->srid = 0;
    map_config->autotransform_enabled = 0;
    map_config->dms = 0;
    map_config->map_background_red = 255;
    map_config->map_background_green = 255;
    map_config->map_background_blue = 255;
    map_config->map_background_transparent = 0;
    map_config->raster_wms_auto_switch = 0;
    map_config->max_wms_retries = 5;
    map_config->wms_pause = 1000;
    map_config->label_anti_collision = 0;
    map_config->label_wrap_text = 0;
    map_config->label_auto_rotate = 0;
    map_config->label_shift_position = 0;
    map_config->bbox = NULL;
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
    if (stl->http_proxy != NULL)
	free (stl->http_proxy);
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
    if (map_config->bbox != NULL)
	free (map_config->bbox);
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
    style->text_alone = 0;
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
    style->http_proxy = NULL;
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
							      (grandchild2->content);
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
				  lyr->vector_style->text_alone = 0;
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    lyr->vector_style->text_alone = 1;
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
			    if (lyr->topology_internal_style->
				style_internal_name != NULL)
				free (lyr->topology_internal_style->
				      style_internal_name);
			    lyr->topology_internal_style->style_internal_name =
				NULL;
			    if (name != NULL)
			      {
				  len = strlen (name);
				  lyr->topology_internal_style->
				      style_internal_name = malloc (len + 1);
				  strcpy (lyr->topology_internal_style->
					  style_internal_name, name);
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
				      lyr->topology_internal_style->
					  show_edge_seeds = 1;
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
				      lyr->topology_internal_style->
					  show_face_seeds = 1;
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
			    if (lyr->network_internal_style->
				style_internal_name != NULL)
				free (lyr->network_internal_style->
				      style_internal_name);
			    lyr->network_internal_style->style_internal_name =
				NULL;
			    if (name != NULL)
			      {
				  len = strlen (name);
				  lyr->network_internal_style->
				      style_internal_name = malloc (len + 1);
				  strcpy (lyr->network_internal_style->
					  style_internal_name, name);
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
				      lyr->network_internal_style->
					  show_link_seeds = 1;
			      }
			}
		  }
	    }
	  attr = attr->next;
      }
}

static void
parse_http_proxy (xmlNodePtr node, rl2MapWmsLayerStylePtr style)
{
/* parsing a <WmsLayerStyle><HttpProxy> tag */
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
		      if (style->http_proxy != NULL)
			{
			    free (style->http_proxy);
			    style->http_proxy = NULL;
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
					style->http_proxy = malloc (len + 1);
					strcpy (style->http_proxy, value);
				    }
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
					      free (lyr->wms_style->
						    background_color);
					      lyr->wms_style->background_color =
						  NULL;
					  }
				    }
				  else
				    {
					if (lyr->wms_style->background_color !=
					    NULL)
					    free (lyr->wms_style->
						  background_color);
					len = strlen (value);
					lyr->wms_style->background_color =
					    malloc (len + 1);
					strcpy (lyr->wms_style->
						background_color, value);
				    }
			      }
			    text = text->next;
			}
		  }
		if (strcmp (name, "HttpProxy") == 0)
		    parse_http_proxy (node, lyr->wms_style);
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
			  parse_vector_layer_style (node, lyr);
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
					    map_config->map_background_transparent
						= 1;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_raster_wms_auto_switch (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <RasterWmsAutoSwitch> tag */
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
		      map_config->raster_wms_auto_switch = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    map_config->raster_wms_auto_switch =
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
parse_wms (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <WMS> tag */
    const char *value;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "MaxRetries") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->max_wms_retries = 5;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					map_config->max_wms_retries =
					    atoi (value);
					if (map_config->max_wms_retries < 1)
					    map_config->max_wms_retries = 1;
					if (map_config->max_wms_retries > 10)
					    map_config->max_wms_retries = 10;
				    }
			      }
			}
		  }
		if (strcmp (name, "WmsPause") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->wms_pause = 1000;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					map_config->wms_pause = atoi (value);
					if (map_config->wms_pause < 1)
					    map_config->wms_pause = 1;
					if (map_config->wms_pause > 30000)
					    map_config->wms_pause = 30000;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_label_advanced_options (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <LabelAdvancedOption> tag */
    const char *value;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "AntiCollisionEnabled") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->label_anti_collision = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    map_config->label_anti_collision =
						1;
				    }
			      }
			}
		  }
		if (strcmp (name, "WrapTextEnabled") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->label_wrap_text = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    map_config->label_wrap_text = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "AutoRotateEnabled") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->label_auto_rotate = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    map_config->label_auto_rotate = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "ShiftPositionEnabled") == 0)
		  {
		      xmlNode *text = attr->children;
		      map_config->label_shift_position = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    map_config->label_shift_position =
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
parse_map_bbox (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <MapBoundingBox> tag */
    struct _xmlAttr *attr;

    if (map_config->bbox == NULL)
	map_config->bbox = malloc (sizeof (rl2MapBoundingBox));
    map_config->bbox->minx = 0.0;
    map_config->bbox->miny = 0.0;
    map_config->bbox->maxx = 0.0;
    map_config->bbox->maxy = 0.0;

    attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *nm = (const char *) (attr->name);
		if (strcmp (nm, "MinX") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				map_config->bbox->minx =
				    atof ((const char *) text->content);
			}
		  }
		if (strcmp (nm, "MinY") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				map_config->bbox->miny =
				    atof ((const char *) text->content);
			}
		  }
		if (strcmp (nm, "MaxX") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				map_config->bbox->maxx =
				    atof ((const char *) text->content);
			}
		  }
		if (strcmp (nm, "MaxY") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
				map_config->bbox->maxy =
				    atof ((const char *) text->content);
			}
		  }
	    }
	  attr = attr->next;
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
		if (strcmp (name, "RasterWmsAutoSwitch") == 0)
		    parse_raster_wms_auto_switch (node, map_config);
		if (strcmp (name, "WMS") == 0)
		    parse_wms (node, map_config);
		if (strcmp (name, "LabelAdvancedOptions") == 0)
		    parse_label_advanced_options (node, map_config);
	    }
	  node = node->next;
      }
}

static void
parse_map_min_scale (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <MinScaleDenominator> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MinScaleDenominator") == 0)
		  {
		      xmlNode *text = node->children;
		      map_config->min_scale = -1.0;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      map_config->min_scale = -1.0;
				  else
				      map_config->min_scale = atof (value);
			      }
			    text = text->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_map_max_scale (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <MaxScaleDenominator> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MaxScaleDenominator") == 0)
		  {
		      xmlNode *text = node->children;
		      map_config->max_scale = -1.0;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      map_config->max_scale = -1.0;
				  else
				      map_config->max_scale = atof (value);
			      }
			    text = text->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_map_visibility (xmlNodePtr node, rl2MapConfigPtr map_config)
{
/* parsing a <VisibilityScaleRange> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MinScaleDenominator") == 0)
		    parse_map_min_scale (node, map_config);
		if (strcmp (name, "MaxScaleDenominator") == 0)
		    parse_map_max_scale (node, map_config);
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
		if (strcmp (name, "VisibilityScaleRange") == 0)
		    parse_map_visibility (node->children, map_config);
		if (strcmp (name, "MapBoundingBox") == 0)
		    parse_map_bbox (node, map_config);
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

static rl2MapChannelSelectionPtr
clone_channel_selection (rl2MapChannelSelectionPtr old_cs)
{
/* cloning a Channel Selection */
    rl2MapChannelSelectionPtr new_cs = malloc (sizeof (rl2MapChannelSelection));
    new_cs->rgb = old_cs->rgb;
    new_cs->red_channel = old_cs->red_channel;
    new_cs->green_channel = old_cs->green_channel;
    new_cs->blue_channel = old_cs->blue_channel;
    new_cs->gray_channel = old_cs->gray_channel;
    return new_cs;
}

static rl2MapColorRampPtr
clone_color_ramp (rl2MapColorRampPtr old_cr)
{
/* cloning a Color Ramp */
    int len;
    rl2MapColorRampPtr new_cr = malloc (sizeof (rl2MapColorRamp));
    new_cr->min_value = old_cr->min_value;
    new_cr->max_value = old_cr->max_value;
    if (old_cr->min_color == NULL)
	new_cr->min_color = NULL;
    else
      {
	  len = strlen (old_cr->min_color);
	  new_cr->min_color = malloc (len + 1);
	  strcpy (new_cr->min_color, old_cr->min_color);
      }
    if (old_cr->max_color == NULL)
	new_cr->max_color = NULL;
    else
      {
	  len = strlen (old_cr->max_color);
	  new_cr->max_color = malloc (len + 1);
	  strcpy (new_cr->max_color, old_cr->max_color);
      }
    return new_cr;
}

static rl2MapContrastEnhancementPtr
clone_contrast_enhancement (rl2MapContrastEnhancementPtr old_ce)
{
/* cloning a Contrast Enhancement */
    rl2MapContrastEnhancementPtr new_ce =
	malloc (sizeof (rl2MapContrastEnhancement));
    new_ce->normalize = old_ce->normalize;
    new_ce->histogram = old_ce->histogram;
    new_ce->gamma = old_ce->gamma;
    new_ce->gamma_value = old_ce->gamma_value;
    return new_ce;
}

static rl2MapRasterLayerStylePtr
clone_raster_style (rl2MapRasterLayerStylePtr old_style)
{
/* cloning a Raster Style */
    int len;
    rl2MapRasterLayerStylePtr new_style =
	malloc (sizeof (rl2MapRasterLayerStyle));
    new_style->opacity = old_style->opacity;
    if (old_style->channel_selection == NULL)
	new_style->channel_selection = NULL;
    else
	new_style->channel_selection =
	    clone_channel_selection (old_style->channel_selection);
    if (old_style->color_map_name == NULL)
	new_style->color_map_name = NULL;
    else
      {
	  len = strlen (old_style->color_map_name);
	  new_style->color_map_name = malloc (len + 1);
	  strcpy (new_style->color_map_name, old_style->color_map_name);
      }
    if (old_style->color_ramp == NULL)
	new_style->color_ramp = NULL;
    else
	new_style->color_ramp = clone_color_ramp (old_style->color_ramp);
    if (old_style->contrast_enhancement == NULL)
	new_style->contrast_enhancement = NULL;
    else
	new_style->contrast_enhancement =
	    clone_contrast_enhancement (old_style->contrast_enhancement);
    new_style->shaded_relief = old_style->shaded_relief;
    new_style->relief_factor = old_style->relief_factor;
    return new_style;
}

static rl2MapColorPtr
clone_color_replacement (rl2MapColorPtr old_cr)
{
/* cloning a Color Replacement */
    rl2MapColorPtr new_cr = malloc (sizeof (rl2MapColor));
    new_cr->red = old_cr->red;
    new_cr->green = old_cr->green;
    new_cr->blue = old_cr->blue;
    return new_cr;
}

static rl2MapGraphicFillPtr
clone_graphic (rl2MapGraphicFillPtr old_graphic)
{
/* cloning a Graphic */
    int len;
    rl2MapGraphicFillPtr new_graphic = malloc (sizeof (rl2MapGraphicFill));
    if (old_graphic->resource == NULL)
	new_graphic->resource = NULL;
    else
      {
	  len = strlen (old_graphic->resource);
	  new_graphic->resource = malloc (len + 1);
	  strcpy (new_graphic->resource, old_graphic->resource);
      }
    if (old_graphic->format == NULL)
	new_graphic->format = NULL;
    else
      {
	  len = strlen (old_graphic->format);
	  new_graphic->format = malloc (len + 1);
	  strcpy (new_graphic->format, old_graphic->format);
      }
    if (old_graphic->color == NULL)
	new_graphic->color = NULL;
    else
	new_graphic->color = clone_color_replacement (old_graphic->color);
    return new_graphic;
}

static rl2MapFillPtr
clone_fill (rl2MapFillPtr old_fill)
{
/* cloning a Fill */
    rl2MapFillPtr new_fill = malloc (sizeof (rl2MapFill));
    if (old_fill->graphic == NULL)
	new_fill->graphic = NULL;
    else
	new_fill->graphic = clone_graphic (old_fill->graphic);
    new_fill->red = old_fill->red;
    new_fill->green = old_fill->green;
    new_fill->blue = old_fill->blue;
    new_fill->opacity = old_fill->opacity;
    return new_fill;
}

static rl2MapStrokePtr
clone_stroke (rl2MapStrokePtr old_stroke)
{
/* cloning a Stroke */
    rl2MapStrokePtr new_stroke = malloc (sizeof (rl2MapStroke));
    new_stroke->red = old_stroke->red;
    new_stroke->green = old_stroke->green;
    new_stroke->blue = old_stroke->blue;
    new_stroke->opacity = old_stroke->opacity;
    new_stroke->width = old_stroke->width;
    new_stroke->dot_style = old_stroke->dot_style;
    return new_stroke;
}

static rl2MapMarkPtr
clone_mark (rl2MapMarkPtr old_mark)
{
/* cloning a Mark (Point) */
    rl2MapMarkPtr new_mark = malloc (sizeof (rl2MapMark));
    new_mark->type = old_mark->type;
    if (old_mark->fill == NULL)
	new_mark->fill = NULL;
    else
	new_mark->fill = clone_fill (old_mark->fill);
    if (old_mark->stroke == NULL)
	new_mark->stroke = NULL;
    else
	new_mark->stroke = clone_stroke (old_mark->stroke);
    return new_mark;
}

static rl2MapPointSymbolizerPtr
clone_point_symbolizer (rl2MapPointSymbolizerPtr old_sym)
{
/* cloning a Point Symbolizer */
    rl2MapPointSymbolizerPtr new_sym = malloc (sizeof (rl2MapPointSymbolizer));
    if (old_sym->mark == NULL)
	new_sym->mark = NULL;
    else
	new_sym->mark = clone_mark (old_sym->mark);
    if (old_sym->graphic == NULL)
	new_sym->graphic = NULL;
    else
	new_sym->graphic = clone_graphic (old_sym->graphic);
    new_sym->opacity = old_sym->opacity;
    new_sym->size = old_sym->size;
    new_sym->anchor_x = old_sym->anchor_x;
    new_sym->anchor_y = old_sym->anchor_y;
    new_sym->displacement_x = old_sym->displacement_x;
    new_sym->displacement_y = old_sym->displacement_y;
    new_sym->rotation = old_sym->rotation;
    return new_sym;
}

static rl2MapLineSymbolizerPtr
clone_line_symbolizer (rl2MapLineSymbolizerPtr old_sym)
{
/* cloning a Line Symbolizer */
    rl2MapLineSymbolizerPtr new_sym = malloc (sizeof (rl2MapLineSymbolizer));
    if (old_sym->stroke == NULL)
	new_sym->stroke = NULL;
    else
	new_sym->stroke = clone_stroke (old_sym->stroke);
    new_sym->perpendicular_offset = old_sym->perpendicular_offset;
    new_sym->next = NULL;
    return new_sym;
}

static rl2MapPolygonSymbolizerPtr
clone_polygon_symbolizer (rl2MapPolygonSymbolizerPtr old_sym)
{
/* cloning a Polygon Symbolizer */
    rl2MapPolygonSymbolizerPtr new_sym =
	malloc (sizeof (rl2MapPolygonSymbolizer));
    if (old_sym->fill == NULL)
	new_sym->fill = NULL;
    else
	new_sym->fill = clone_fill (old_sym->fill);
    if (old_sym->stroke == NULL)
	new_sym->stroke = NULL;
    else
	new_sym->stroke = clone_stroke (old_sym->stroke);
    new_sym->displacement_x = old_sym->displacement_x;
    new_sym->displacement_y = old_sym->displacement_y;
    new_sym->perpendicular_offset = old_sym->perpendicular_offset;
    return new_sym;
}

static rl2MapFontPtr
clone_font (rl2MapFontPtr old_font)
{
/* cloning a Font */
    int len;
    rl2MapFontPtr new_font = malloc (sizeof (rl2MapFont));
    if (old_font->family == NULL)
	new_font->family = NULL;
    else
      {
	  len = strlen (old_font->family);
	  new_font->family = malloc (len + 1);
	  strcpy (new_font->family, old_font->family);
      }
    new_font->style = old_font->style;
    new_font->weight = old_font->weight;
    new_font->size = old_font->size;
    return new_font;
}

static rl2MapPointPlacementPtr
clone_point_placement (rl2MapPointPlacementPtr old_pl)
{
/* cloning a Label Point Placement */
    rl2MapPointPlacementPtr new_pl = malloc (sizeof (rl2MapPointPlacement));
    new_pl->anchor_x = old_pl->anchor_x;
    new_pl->anchor_y = old_pl->anchor_y;
    new_pl->displacement_x = old_pl->displacement_x;
    new_pl->displacement_y = old_pl->displacement_y;
    new_pl->rotation = old_pl->rotation;
    return new_pl;
}

static rl2MapLinePlacementPtr
clone_line_placement (rl2MapLinePlacementPtr old_pl)
{
/* cloning a Label Line Placement */
    rl2MapLinePlacementPtr new_pl = malloc (sizeof (rl2MapLinePlacement));
    new_pl->perpendicular_offset = old_pl->perpendicular_offset;
    new_pl->repeated = old_pl->repeated;
    new_pl->initial_gap = old_pl->initial_gap;
    new_pl->gap = old_pl->gap;
    new_pl->aligned = old_pl->aligned;
    new_pl->generalize = old_pl->generalize;
    return new_pl;
}

static rl2MapPlacementPtr
clone_label_placement (rl2MapPlacementPtr old_pl)
{
/* cloning a Label Placement */
    rl2MapPlacementPtr new_pl = malloc (sizeof (rl2MapPlacement));
    if (old_pl->point == NULL)
	new_pl->point = NULL;
    else
	new_pl->point = clone_point_placement (old_pl->point);
    if (old_pl->line == NULL)
	new_pl->line = NULL;
    else
	new_pl->line = clone_line_placement (old_pl->line);
    return new_pl;
}

static rl2MapHaloPtr
clone_halo (rl2MapHaloPtr old_halo)
{
/* cloning a Halo */
    rl2MapHaloPtr new_halo = malloc (sizeof (rl2MapHalo));
    new_halo->radius = old_halo->radius;
    if (old_halo->fill == NULL)
	new_halo->fill = NULL;
    else
	new_halo->fill = clone_fill (old_halo->fill);
    return new_halo;
}

static rl2MapTextSymbolizerPtr
clone_text_symbolizer (rl2MapTextSymbolizerPtr old_sym)
{
/* cloning a Text Symbolizer */
    int len;
    rl2MapTextSymbolizerPtr new_sym = malloc (sizeof (rl2MapTextSymbolizer));
    if (old_sym->label == NULL)
	new_sym->label = NULL;
    else
      {
	  len = strlen (old_sym->label);
	  new_sym->label = malloc (len + 1);
	  strcpy (new_sym->label, old_sym->label);
      }
    if (old_sym->font == NULL)
	new_sym->font = NULL;
    else
	new_sym->font = clone_font (old_sym->font);
    if (old_sym->placement == NULL)
	new_sym->placement = NULL;
    else
	new_sym->placement = clone_label_placement (old_sym->placement);
    if (old_sym->halo == NULL)
	new_sym->halo = NULL;
    else
	new_sym->halo = clone_halo (old_sym->halo);
    if (old_sym->fill == NULL)
	new_sym->fill = NULL;
    else
	new_sym->fill = clone_fill (old_sym->fill);
    return new_sym;
}

static rl2MapVectorLayerStylePtr
clone_vector_style (rl2MapVectorLayerStylePtr old_style)
{
/* cloning a Vector Style */
    rl2MapLineSymbolizerPtr line;
    rl2MapVectorLayerStylePtr new_style =
	malloc (sizeof (rl2MapVectorLayerStyle));
    if (old_style->point_sym == NULL)
	new_style->point_sym = NULL;
    else
	new_style->point_sym = clone_point_symbolizer (old_style->point_sym);
    new_style->first_line_sym = NULL;
    new_style->last_line_sym = NULL;
    line = old_style->first_line_sym;
    while (line != NULL)
      {
	  rl2MapLineSymbolizerPtr new_ln = clone_line_symbolizer (line);
	  if (new_style->first_line_sym == NULL)
	      new_style->first_line_sym = new_ln;
	  if (new_style->last_line_sym != NULL)
	      new_style->last_line_sym->next = new_ln;
	  new_style->last_line_sym = new_ln;
	  line = line->next;
      }
    if (old_style->polygon_sym == NULL)
	new_style->polygon_sym = NULL;
    else
	new_style->polygon_sym =
	    clone_polygon_symbolizer (old_style->polygon_sym);
    if (old_style->text_sym == NULL)
	new_style->text_sym = NULL;
    else
	new_style->text_sym = clone_text_symbolizer (old_style->text_sym);
    new_style->text_alone = old_style->text_alone;
    return new_style;
}

static rl2MapTopologyLayerStylePtr
clone_topology_style (rl2MapTopologyLayerStylePtr old_style)
{
/* cloning a Topology Style */
    rl2MapTopologyLayerStylePtr new_style =
	malloc (sizeof (rl2MapTopologyLayerStyle));
    new_style->show_faces = old_style->show_faces;
    new_style->show_edges = old_style->show_edges;
    new_style->show_nodes = old_style->show_nodes;
    new_style->show_edge_seeds = old_style->show_edge_seeds;
    new_style->show_face_seeds = old_style->show_face_seeds;
    if (old_style->faces_sym == NULL)
	new_style->faces_sym = NULL;
    else
	new_style->faces_sym = clone_polygon_symbolizer (old_style->faces_sym);
    if (old_style->edges_sym == NULL)
	new_style->edges_sym = NULL;
    else
	new_style->edges_sym = clone_line_symbolizer (old_style->edges_sym);
    if (old_style->nodes_sym == NULL)
	new_style->nodes_sym = NULL;
    else
	new_style->nodes_sym = clone_point_symbolizer (old_style->nodes_sym);
    if (old_style->edge_seeds_sym == NULL)
	new_style->edge_seeds_sym = NULL;
    else
	new_style->edge_seeds_sym =
	    clone_point_symbolizer (old_style->edge_seeds_sym);
    if (old_style->face_seeds_sym == NULL)
	new_style->face_seeds_sym = NULL;
    else
	new_style->face_seeds_sym =
	    clone_point_symbolizer (old_style->face_seeds_sym);
    return new_style;
}

static rl2MapTopologyLayerInternalStylePtr
clone_topology_internal_style (rl2MapTopologyLayerInternalStylePtr old_style)
{
/* cloning a Topology Internal Style */
    int len;
    rl2MapTopologyLayerInternalStylePtr new_style =
	malloc (sizeof (rl2MapTopologyLayerInternalStyle));
    if (old_style->style_internal_name == NULL)
	new_style->style_internal_name = NULL;
    else
      {
	  len = strlen (old_style->style_internal_name);
	  new_style->style_internal_name = malloc (len + 1);
	  strcpy (new_style->style_internal_name,
		  old_style->style_internal_name);
      }
    new_style->show_faces = old_style->show_faces;
    new_style->show_edges = old_style->show_edges;
    new_style->show_nodes = old_style->show_nodes;
    new_style->show_edge_seeds = old_style->show_edge_seeds;
    new_style->show_face_seeds = old_style->show_face_seeds;
    return new_style;
}

static rl2MapNetworkLayerStylePtr
clone_network_style (rl2MapNetworkLayerStylePtr old_style)
{
/* cloning a Network Style */
    rl2MapNetworkLayerStylePtr new_style =
	malloc (sizeof (rl2MapNetworkLayerStyle));
    new_style->show_links = old_style->show_links;
    new_style->show_nodes = old_style->show_nodes;
    new_style->show_link_seeds = old_style->show_link_seeds;
    if (old_style->links_sym == NULL)
	new_style->links_sym = NULL;
    else
	new_style->links_sym = clone_line_symbolizer (old_style->links_sym);
    if (old_style->nodes_sym == NULL)
	new_style->nodes_sym = NULL;
    else
	new_style->nodes_sym = clone_point_symbolizer (old_style->nodes_sym);
    if (old_style->link_seeds_sym == NULL)
	new_style->link_seeds_sym = NULL;
    else
	new_style->link_seeds_sym =
	    clone_point_symbolizer (old_style->link_seeds_sym);
    return new_style;
}

static rl2MapNetworkLayerInternalStylePtr
clone_network_internal_style (rl2MapNetworkLayerInternalStylePtr old_style)
{
/* cloning a Network Internal Style */
    int len;
    rl2MapNetworkLayerInternalStylePtr new_style =
	malloc (sizeof (rl2MapNetworkLayerInternalStyle));
    if (old_style->style_internal_name == NULL)
	new_style->style_internal_name = NULL;
    else
      {
	  len = strlen (old_style->style_internal_name);
	  new_style->style_internal_name = malloc (len + 1);
	  strcpy (new_style->style_internal_name,
		  old_style->style_internal_name);
      }
    new_style->show_links = old_style->show_links;
    new_style->show_nodes = old_style->show_nodes;
    new_style->show_link_seeds = old_style->show_link_seeds;
    return new_style;
}

static rl2MapWmsLayerStylePtr
clone_wms_style (rl2MapWmsLayerStylePtr old_style)
{
/* cloning a WMS Style */
    int len;
    rl2MapWmsLayerStylePtr new_style = malloc (sizeof (rl2MapWmsLayerStyle));
    if (old_style->http_proxy == NULL)
	new_style->http_proxy = NULL;
    else
      {
	  len = strlen (old_style->http_proxy);
	  new_style->http_proxy = malloc (len + 1);
	  strcpy (new_style->http_proxy, old_style->http_proxy);
      }
    if (old_style->get_map_url == NULL)
	new_style->get_map_url = NULL;
    else
      {
	  len = strlen (old_style->get_map_url);
	  new_style->get_map_url = malloc (len + 1);
	  strcpy (new_style->get_map_url, old_style->get_map_url);
      }
    if (old_style->get_feature_info_url == NULL)
	new_style->get_feature_info_url = NULL;
    else
      {
	  len = strlen (old_style->get_feature_info_url);
	  new_style->get_feature_info_url = malloc (len + 1);
	  strcpy (new_style->get_feature_info_url,
		  old_style->get_feature_info_url);
      }
    if (old_style->wms_protocol == NULL)
	new_style->wms_protocol = NULL;
    else
      {
	  len = strlen (old_style->wms_protocol);
	  new_style->wms_protocol = malloc (len + 1);
	  strcpy (new_style->wms_protocol, old_style->wms_protocol);
      }
    if (old_style->crs == NULL)
	new_style->crs = NULL;
    else
      {
	  len = strlen (old_style->crs);
	  new_style->crs = malloc (len + 1);
	  strcpy (new_style->crs, old_style->crs);
      }
    new_style->swap_xy = old_style->swap_xy;
    if (old_style->style == NULL)
	new_style->style = NULL;
    else
      {
	  len = strlen (old_style->style);
	  new_style->style = malloc (len + 1);
	  strcpy (new_style->style, old_style->style);
      }
    if (old_style->image_format == NULL)
	new_style->image_format = NULL;
    else
      {
	  len = strlen (old_style->image_format);
	  new_style->image_format = malloc (len + 1);
	  strcpy (new_style->image_format, old_style->image_format);
      }
    new_style->opaque = old_style->opaque;
    if (old_style->background_color == NULL)
	new_style->background_color = NULL;
    else
      {
	  len = strlen (old_style->background_color);
	  new_style->background_color = malloc (len + 1);
	  strcpy (new_style->background_color, old_style->background_color);
      }
    new_style->is_tiled = old_style->is_tiled;
    new_style->tile_width = old_style->tile_width;
    new_style->tile_height = old_style->tile_height;
    return new_style;
}

static void
clone_map_layer (rl2MapLayerPtr old_lyr, rl2MapConfigPtr new_conf)
{
/* cloning a MapLayer object */
    int len;
    rl2MapLayerPtr new_lyr;

    if (old_lyr == NULL)
	return;

    new_lyr = malloc (sizeof (rl2MapLayer));
    new_lyr->type = old_lyr->type;
    if (old_lyr->prefix == NULL)
	new_lyr->prefix = NULL;
    else
      {
	  len = strlen (old_lyr->prefix);
	  new_lyr->prefix = malloc (len + 1);
	  strcpy (new_lyr->prefix, old_lyr->prefix);
      }
    if (old_lyr->name == NULL)
	new_lyr->name = NULL;
    else
      {
	  len = strlen (old_lyr->name);
	  new_lyr->name = malloc (len + 1);
	  strcpy (new_lyr->name, old_lyr->name);
      }
    new_lyr->visible = old_lyr->visible;
    new_lyr->ok_min_scale = old_lyr->ok_min_scale;
    new_lyr->min_scale = old_lyr->min_scale;
    new_lyr->ok_max_scale = old_lyr->ok_max_scale;
    new_lyr->max_scale = old_lyr->max_scale;
    if (old_lyr->raster_style_internal_name == NULL)
	new_lyr->raster_style_internal_name = NULL;
    else
      {
	  len = strlen (old_lyr->raster_style_internal_name);
	  new_lyr->raster_style_internal_name = malloc (len + 1);
	  strcpy (new_lyr->raster_style_internal_name,
		  old_lyr->raster_style_internal_name);
      }
    if (old_lyr->vector_style_internal_name == NULL)
	new_lyr->vector_style_internal_name = NULL;
    else
      {
	  len = strlen (old_lyr->vector_style_internal_name);
	  new_lyr->vector_style_internal_name = malloc (len + 1);
	  strcpy (new_lyr->vector_style_internal_name,
		  old_lyr->vector_style_internal_name);
      }
    if (old_lyr->raster_style == NULL)
	new_lyr->raster_style = NULL;
    else
	new_lyr->raster_style = clone_raster_style (old_lyr->raster_style);
    if (old_lyr->vector_style == NULL)
	new_lyr->vector_style = NULL;
    else
	new_lyr->vector_style = clone_vector_style (old_lyr->vector_style);
    if (old_lyr->topology_style == NULL)
	new_lyr->topology_style = NULL;
    else
	new_lyr->topology_style =
	    clone_topology_style (old_lyr->topology_style);
    if (old_lyr->topology_internal_style == NULL)
	new_lyr->topology_internal_style = NULL;
    else
	new_lyr->topology_internal_style =
	    clone_topology_internal_style (old_lyr->topology_internal_style);
    if (old_lyr->network_style == NULL)
	new_lyr->network_style = NULL;
    else
	new_lyr->network_style = clone_network_style (old_lyr->network_style);
    if (old_lyr->network_internal_style == NULL)
	new_lyr->network_internal_style = NULL;
    else
	new_lyr->network_internal_style =
	    clone_network_internal_style (old_lyr->network_internal_style);
    if (old_lyr->wms_style == NULL)
	new_lyr->wms_style = NULL;
    else
	new_lyr->wms_style = clone_wms_style (old_lyr->wms_style);
    new_lyr->next = NULL;

    if (new_conf->first_lyr == NULL)
	new_conf->first_lyr = new_lyr;
    if (new_conf->last_lyr != NULL)
	new_conf->last_lyr->next = new_lyr;
    new_conf->last_lyr = new_lyr;
}

static void
clone_map_attached_db (rl2MapAttachedDbPtr old_db, rl2MapConfigPtr new_conf)
{
/* cloning a MapAttachedDb object */
    int len;
    rl2MapAttachedDbPtr new_db;

    if (old_db == NULL)
	return;

    new_db = malloc (sizeof (rl2MapAttachedDb));
    if (old_db->prefix == NULL)
	new_db->prefix = NULL;
    else
      {
	  len = strlen (old_db->prefix);
	  new_db->prefix = malloc (len + 1);
	  strcpy (new_db->prefix, old_db->prefix);
      }
    if (old_db->path == NULL)
	new_db->path = NULL;
    else
      {
	  len = strlen (old_db->path);
	  new_db->path = malloc (len + 1);
	  strcpy (new_db->path, old_db->path);
      }
    new_db->next = NULL;

    if (new_conf->first_db == NULL)
	new_conf->first_db = new_db;
    if (new_conf->last_db != NULL)
	new_conf->last_db->next = new_db;
    new_conf->last_db = new_db;
}

RL2_DECLARE rl2MapConfigPtr
rl2_clone_map_config (rl2MapConfigPtr old_conf)
{
/* cloning a MapConfig object */
    int len;
    rl2MapConfigPtr new_conf;
    rl2MapAttachedDbPtr db;
    rl2MapLayerPtr lyr;

    if (old_conf == NULL)
	return NULL;

    new_conf = malloc (sizeof (rl2MapConfig));
    if (old_conf->name == NULL)
	new_conf->name = NULL;
    else
      {
	  len = strlen (old_conf->name);
	  new_conf->name = malloc (len + 1);
	  strcpy (new_conf->name, old_conf->name);
      }
    if (old_conf->title == NULL)
	new_conf->title = NULL;
    else
      {
	  len = strlen (old_conf->title);
	  new_conf->title = malloc (len + 1);
	  strcpy (new_conf->title, old_conf->title);
      }
    if (old_conf->abstract == NULL)
	new_conf->abstract = NULL;
    else
      {
	  len = strlen (old_conf->abstract);
	  new_conf->abstract = malloc (len + 1);
	  strcpy (new_conf->abstract, old_conf->abstract);
      }
    new_conf->min_scale = old_conf->min_scale;
    new_conf->max_scale = old_conf->max_scale;
    new_conf->multithread_enabled = old_conf->multithread_enabled;
    new_conf->max_threads = old_conf->max_threads;
    new_conf->srid = old_conf->srid;
    new_conf->autotransform_enabled = old_conf->autotransform_enabled;
    new_conf->dms = old_conf->dms;
    new_conf->map_background_red = old_conf->map_background_red;
    new_conf->map_background_green = old_conf->map_background_green;
    new_conf->map_background_blue = old_conf->map_background_blue;
    new_conf->map_background_transparent = old_conf->map_background_transparent;
    new_conf->raster_wms_auto_switch = old_conf->raster_wms_auto_switch;
    new_conf->max_wms_retries = old_conf->max_wms_retries;
    new_conf->wms_pause = old_conf->wms_pause;
    new_conf->label_anti_collision = old_conf->label_anti_collision;
    new_conf->label_wrap_text = old_conf->label_wrap_text;
    new_conf->label_auto_rotate = old_conf->label_auto_rotate;
    new_conf->label_shift_position = old_conf->label_shift_position;
    if (old_conf->bbox == NULL)
	new_conf->bbox = NULL;
    else
      {
	  new_conf->bbox = malloc (sizeof (rl2MapBoundingBox));
	  new_conf->bbox->minx = old_conf->bbox->minx;
	  new_conf->bbox->miny = old_conf->bbox->miny;
	  new_conf->bbox->maxx = old_conf->bbox->maxx;
	  new_conf->bbox->maxy = old_conf->bbox->maxy;
      }
    new_conf->first_db = NULL;
    new_conf->last_db = NULL;
    db = old_conf->first_db;
    while (db != NULL)
      {
	  clone_map_attached_db (db, new_conf);
	  db = db->next;
      }
    new_conf->first_lyr = NULL;
    new_conf->last_lyr = NULL;
    lyr = old_conf->first_lyr;
    while (lyr != NULL)
      {
	  clone_map_layer (lyr, new_conf);
	  lyr = lyr->next;
      }
    return new_conf;
}

static int
cmp_strings (const char *str1, const char *str2)
{
/* generic comparison of two text strings */
    if (str1 == NULL && str2 == NULL)
	return 1;
    if (str1 == NULL || str2 == NULL)
	return 0;
    if (strcmp (str1, str2) == 0)
	return 1;
    return 0;
}

static int
cmp_visibility_range (rl2MapConfigPtr conf1, rl2MapConfigPtr conf2)
{
/* comparison of Visibility Range */
    if (conf1->min_scale < 0.0 && conf2->min_scale < 0.0
	&& conf1->max_scale < 0.0 && conf2->max_scale < 0.0)
	return 1;
    if (conf1->min_scale != conf2->min_scale)
	return 0;
    if (conf1->max_scale != conf2->max_scale)
	return 0;
    return 1;
}

static int
cmp_bbox (rl2MapBoundingBoxPtr bbox1, rl2MapBoundingBoxPtr bbox2)
{
/*  comparison of two Map Bounding Boxes */
    if (bbox1 == NULL && bbox2 == NULL)
	return 1;
    if (bbox1 == NULL || bbox2 == NULL)
	return 0;
    if (bbox1->minx != bbox2->minx)
	return 0;
    if (bbox1->miny != bbox2->miny)
	return 0;
    if (bbox1->maxx != bbox2->maxx)
	return 0;
    if (bbox1->maxy != bbox2->maxy)
	return 0;
    return 1;
}

static int
cmp_attached_dbs (rl2MapAttachedDbPtr db1, rl2MapAttachedDbPtr db2)
{
/* comparison of two Attached Databases */
    if (!cmp_strings (db1->prefix, db2->prefix))
	return 0;
    if (!cmp_strings (db1->path, db2->path))
	return 0;
    return 1;
}

static int
cmp_channel_selections (rl2MapChannelSelectionPtr cs1,
			rl2MapChannelSelectionPtr cs2)
{
/* comparing two Channel Selections */
    if (cs1 == NULL && cs2 == NULL)
	return 1;
    if (cs1 == NULL || cs2 == NULL)
	return 0;
    if (cs1->rgb != cs2->rgb)
	return 0;
    if (cs1->red_channel != cs2->red_channel)
	return 0;
    if (cs1->green_channel != cs2->green_channel)
	return 0;
    if (cs1->blue_channel != cs2->blue_channel)
	return 0;
    if (cs1->gray_channel != cs2->gray_channel)
	return 0;
    return 1;
}

static int
cmp_color_ramps (rl2MapColorRampPtr cr1, rl2MapColorRampPtr cr2)
{
/* comparing two Color Ramps */
    if (cr1 == NULL && cr2 == NULL)
	return 1;
    if (cr1 == NULL || cr2 == NULL)
	return 0;
    if (cr1->min_value != cr2->min_value)
	return 0;
    if (cr1->max_value != cr2->max_value)
	return 0;
    if (!cmp_strings (cr1->min_color, cr2->min_color))
	return 0;
    if (!cmp_strings (cr1->max_color, cr2->max_color))
	return 0;
    return 1;
}

static int
cmp_contrast_enhancements (rl2MapContrastEnhancementPtr ce1,
			   rl2MapContrastEnhancementPtr ce2)
{
/* comparing two Contrast Enhancements */
    if (ce1 == NULL && ce2 == NULL)
	return 1;
    if (ce1 == NULL || ce2 == NULL)
	return 0;
    if (ce1->normalize != ce2->normalize)
	return 0;
    if (ce1->histogram != ce2->histogram)
	return 0;
    if (ce1->gamma != ce2->gamma)
	return 0;
    if (ce1->gamma_value != ce2->gamma_value)
	return 0;
    return 1;
}

static int
cmp_raster_styles (rl2MapRasterLayerStylePtr style1,
		   rl2MapRasterLayerStylePtr style2)
{
/*  comparison of two Raster Styles */
    if (style1 == NULL && style2 == NULL)
	return 1;
    if (style1 == NULL || style2 == NULL)
	return 0;
    if (style1->opacity != style2->opacity)
	return 0;
    if (!cmp_channel_selections
	(style1->channel_selection, style2->channel_selection))
	return 0;
    if (!cmp_strings (style1->color_map_name, style2->color_map_name))
	return 0;
    if (!cmp_color_ramps (style1->color_ramp, style2->color_ramp))
	return 0;
    if (!cmp_contrast_enhancements
	(style1->contrast_enhancement, style2->contrast_enhancement))
	return 0;
    if (style1->shaded_relief != style2->shaded_relief)
	return 0;
    if (style1->relief_factor != style2->relief_factor)
	return 0;
    return 1;
}

static int
cmp_remap_colors (rl2MapColorPtr cr1, rl2MapColorPtr cr2)
{
/*  comparison of two Color Replacements */
    if (cr1 == NULL && cr2 == NULL)
	return 1;
    if (cr1 == NULL || cr2 == NULL)
	return 0;
    if (cr1->red != cr2->red)
	return 0;
    if (cr1->green != cr2->green)
	return 0;
    if (cr1->blue != cr2->blue)
	return 0;
    return 1;
}

static int
cmp_external_graphics (rl2MapGraphicFillPtr graphic1,
		       rl2MapGraphicFillPtr graphic2)
{
/*  comparison of two External Graphics */
    if (graphic1 == NULL && graphic2 == NULL)
	return 1;
    if (graphic1 == NULL || graphic2 == NULL)
	return 0;
    if (!cmp_strings (graphic1->resource, graphic2->resource))
	return 0;
    if (!cmp_strings (graphic1->format, graphic2->format))
	return 0;
    if (!cmp_remap_colors (graphic1->color, graphic2->color))
	return 0;
    return 1;
}

static int
cmp_fills (rl2MapFillPtr fill1, rl2MapFillPtr fill2)
{
/*  comparison of two Fills */
    if (fill1 == NULL && fill2 == NULL)
	return 1;
    if (fill1 == NULL || fill2 == NULL)
	return 0;
    if (!cmp_external_graphics (fill1->graphic, fill2->graphic))
	return 0;
    if (fill1->red != fill2->red)
	return 0;
    if (fill1->green != fill2->green)
	return 0;
    if (fill1->blue != fill2->blue)
	return 0;
    if (fill1->opacity != fill2->opacity)
	return 0;
    return 1;
}

static int
cmp_strokes (rl2MapStrokePtr stroke1, rl2MapStrokePtr stroke2)
{
/*  comparison of two Strokes */
    if (stroke1 == NULL && stroke2 == NULL)
	return 1;
    if (stroke1 == NULL || stroke2 == NULL)
	return 0;
    if (stroke1->red != stroke2->red)
	return 0;
    if (stroke1->green != stroke2->green)
	return 0;
    if (stroke1->blue != stroke2->blue)
	return 0;
    if (stroke1->opacity != stroke2->opacity)
	return 0;
    if (stroke1->opacity != stroke2->opacity)
	return 0;
    if (stroke1->width != stroke2->width)
	return 0;
    if (stroke1->dot_style != stroke2->dot_style)
	return 0;
    return 1;
}

static int
cmp_marks (rl2MapMarkPtr mark1, rl2MapMarkPtr mark2)
{
/*  comparison of two Marks */
    if (mark1 == NULL && mark2 == NULL)
	return 1;
    if (mark1 == NULL || mark2 == NULL)
	return 0;
    if (mark1->type != mark2->type)
	return 0;
    if (!cmp_fills (mark1->fill, mark2->fill))
	return 0;
    if (!cmp_strokes (mark1->stroke, mark2->stroke))
	return 0;
    return 1;
}

static int
cmp_point_symbolizers (rl2MapPointSymbolizerPtr sym1,
		       rl2MapPointSymbolizerPtr sym2)
{
/*  comparison of two Point Symbolizers */
    if (sym1 == NULL && sym2 == NULL)
	return 1;
    if (sym1 == NULL || sym2 == NULL)
	return 0;
    if (!cmp_marks (sym1->mark, sym2->mark))
	return 0;
    if (!cmp_external_graphics (sym1->graphic, sym2->graphic))
	return 0;
    if (sym1->opacity != sym2->opacity)
	return 0;
    if (sym1->size != sym2->size)
	return 0;
    if (sym1->anchor_x != sym2->anchor_x)
	return 0;
    if (sym1->anchor_y != sym2->anchor_y)
	return 0;
    if (sym1->displacement_x != sym2->displacement_x)
	return 0;
    if (sym1->displacement_y != sym2->displacement_y)
	return 0;
    if (sym1->rotation != sym2->rotation)
	return 0;
    return 1;
}

static int
cmp_line_symbolizers (rl2MapLineSymbolizerPtr sym1,
		      rl2MapLineSymbolizerPtr sym2)
{
/*  comparison of two Line Symbolizers */
    if (sym1 == NULL && sym2 == NULL)
	return 1;
    if (sym1 == NULL || sym2 == NULL)
	return 0;
    if (!cmp_strokes (sym1->stroke, sym2->stroke))
	return 0;
    if (sym1->perpendicular_offset != sym2->perpendicular_offset)
	return 0;
    return 1;
}

static int
cmp_polygon_symbolizers (rl2MapPolygonSymbolizerPtr sym1,
			 rl2MapPolygonSymbolizerPtr sym2)
{
/*  comparison of two Polygon Symbolizers */
    if (sym1 == NULL && sym2 == NULL)
	return 1;
    if (sym1 == NULL || sym2 == NULL)
	return 0;
    if (!cmp_fills (sym1->fill, sym2->fill))
	return 0;
    if (!cmp_strokes (sym1->stroke, sym2->stroke))
	return 0;
    if (sym1->displacement_x != sym2->displacement_x)
	return 0;
    if (sym1->displacement_y != sym2->displacement_y)
	return 0;
    if (sym1->perpendicular_offset != sym2->perpendicular_offset)
	return 0;
    return 1;
}

static int
cmp_fonts (rl2MapFontPtr font1, rl2MapFontPtr font2)
{
/*  comparison of two Fonts */
    if (font1 == NULL && font2 == NULL)
	return 1;
    if (font1 == NULL || font2 == NULL)
	return 0;
    if (!cmp_strings (font1->family, font2->family))
	return 0;
    if (font1->style != font2->style)
	return 0;
    if (font1->weight != font2->weight)
	return 0;
    if (font1->size != font2->size)
	return 0;
    return 1;
}

static int
cmp_point_placements (rl2MapPointPlacementPtr pl1, rl2MapPointPlacementPtr pl2)
{
/*  comparison of two Point Placements */
    if (pl1 == NULL && pl2 == NULL)
	return 1;
    if (pl1 == NULL || pl2 == NULL)
	return 0;
    if (pl1->anchor_x != pl2->anchor_x)
	return 0;
    if (pl1->anchor_y != pl2->anchor_y)
	return 0;
    if (pl1->displacement_x != pl2->displacement_x)
	return 0;
    if (pl1->displacement_y != pl2->displacement_y)
	return 0;
    if (pl1->rotation != pl2->rotation)
	return 0;
    return 1;
}

static int
cmp_line_placements (rl2MapLinePlacementPtr pl1, rl2MapLinePlacementPtr pl2)
{
/*  comparison of two Line Placements */
    if (pl1 == NULL && pl2 == NULL)
	return 1;
    if (pl1 == NULL || pl2 == NULL)
	return 0;
    if (pl1->perpendicular_offset != pl2->perpendicular_offset)
	return 0;
    if (pl1->repeated != pl2->repeated)
	return 0;
    if (pl1->initial_gap != pl2->initial_gap)
	return 0;
    if (pl1->gap != pl2->gap)
	return 0;
    if (pl1->aligned != pl2->aligned)
	return 0;
    if (pl1->generalize != pl2->generalize)
	return 0;
    return 1;
}

static int
cmp_placements (rl2MapPlacementPtr pl1, rl2MapPlacementPtr pl2)
{
/*  comparison of two Placements */
    if (pl1 == NULL && pl2 == NULL)
	return 1;
    if (pl1 == NULL || pl2 == NULL)
	return 0;
    if (!cmp_point_placements (pl1->point, pl2->point))
	return 0;
    if (!cmp_line_placements (pl1->line, pl2->line))
	return 0;
    return 1;
}

static int
cmp_halos (rl2MapHaloPtr halo1, rl2MapHaloPtr halo2)
{
/*  comparison of two Halos */
    if (halo1 == NULL && halo2 == NULL)
	return 1;
    if (halo1 == NULL || halo2 == NULL)
	return 0;
    if (halo1->radius != halo2->radius)
	return 0;
    if (!cmp_fills (halo1->fill, halo2->fill))
	return 0;
    return 1;
}

static int
cmp_text_symbolizers (rl2MapTextSymbolizerPtr sym1,
		      rl2MapTextSymbolizerPtr sym2)
{
/*  comparison of two Text Symbolizers */
    if (sym1 == NULL && sym2 == NULL)
	return 1;
    if (sym1 == NULL || sym2 == NULL)
	return 0;
    if (!cmp_strings (sym1->label, sym2->label))
	return 0;
    if (!cmp_fonts (sym1->font, sym2->font))
	return 0;
    if (!cmp_placements (sym1->placement, sym2->placement))
	return 0;
    if (!cmp_halos (sym1->halo, sym2->halo))
	return 0;
    if (!cmp_fills (sym1->fill, sym2->fill))
	return 0;
    return 1;
}

static int
cmp_vector_styles (rl2MapVectorLayerStylePtr style1,
		   rl2MapVectorLayerStylePtr style2)
{
/*  comparison of two Vector Styles */
    int count1;
    int count2;
    rl2MapLineSymbolizerPtr line1;
    rl2MapLineSymbolizerPtr line2;
    if (style1 == NULL && style2 == NULL)
	return 1;
    if (style1 == NULL || style2 == NULL)
	return 0;
    if (!cmp_point_symbolizers (style1->point_sym, style2->point_sym))
	return 0;
    if (!cmp_polygon_symbolizers (style1->polygon_sym, style2->polygon_sym))
	return 0;
    if (!cmp_text_symbolizers (style1->text_sym, style2->text_sym))
	return 0;
    if (style1->text_alone != style2->text_alone)
	return 0;

/* checking Line Symbolyzers */
    count1 = 0;
    line1 = style1->first_line_sym;
    while (line1 != NULL)
      {
	  count1++;
	  line1 = line1->next;
      }
    count2 = 0;
    line2 = style2->first_line_sym;
    while (line2 != NULL)
      {
	  count2++;
	  line2 = line2->next;
      }
    if (count1 != count2)
	return 0;
    line1 = style1->first_line_sym;
    line2 = style2->first_line_sym;
    while (line1 != NULL && line2 != NULL)
      {
	  if (!cmp_line_symbolizers (line1, line2))
	      return 0;
	  line1 = line1->next;
	  line2 = line2->next;
      }
    return 1;
}

static int
cmp_topology_styles (rl2MapTopologyLayerStylePtr style1,
		     rl2MapTopologyLayerStylePtr style2)
{
/*  comparison of two Topology Styles */
    if (style1 == NULL && style2 == NULL)
	return 1;
    if (style1 == NULL || style2 == NULL)
	return 0;
    if (style1->show_faces != style2->show_faces)
	return 0;
    if (style1->show_edges != style2->show_edges)
	return 0;
    if (style1->show_nodes != style2->show_nodes)
	return 0;
    if (style1->show_edge_seeds != style2->show_edge_seeds)
	return 0;
    if (style1->show_face_seeds != style2->show_face_seeds)
	return 0;
    if (!cmp_polygon_symbolizers (style1->faces_sym, style2->faces_sym))
	return 0;
    if (!cmp_line_symbolizers (style1->edges_sym, style2->edges_sym))
	return 0;
    if (!cmp_point_symbolizers (style1->nodes_sym, style2->nodes_sym))
	return 0;
    if (!cmp_point_symbolizers (style1->edge_seeds_sym, style2->edge_seeds_sym))
	return 0;
    if (!cmp_point_symbolizers (style1->face_seeds_sym, style2->face_seeds_sym))
	return 0;
    return 1;
}

static int
cmp_topology_internal_styles (rl2MapTopologyLayerInternalStylePtr style1,
			      rl2MapTopologyLayerInternalStylePtr style2)
{
/*  comparison of two Topology Internal Styles */
    if (style1 == NULL && style2 == NULL)
	return 1;
    if (style1 == NULL || style2 == NULL)
	return 0;
    if (!cmp_strings (style1->style_internal_name, style2->style_internal_name))
	return 0;
    if (style1->show_faces != style2->show_faces)
	return 0;
    if (style1->show_edges != style2->show_edges)
	return 0;
    if (style1->show_nodes != style2->show_nodes)
	return 0;
    if (style1->show_edge_seeds != style2->show_edge_seeds)
	return 0;
    if (style1->show_face_seeds != style2->show_face_seeds)
	return 0;
    return 1;
}

static int
cmp_network_styles (rl2MapNetworkLayerStylePtr style1,
		    rl2MapNetworkLayerStylePtr style2)
{
/*  comparison of two Network Styles */
    if (style1 == NULL && style2 == NULL)
	return 1;
    if (style1 == NULL || style2 == NULL)
	return 0;
    if (style1->show_links != style2->show_links)
	return 0;
    if (style1->show_nodes != style2->show_nodes)
	return 0;
    if (style1->show_link_seeds != style2->show_link_seeds)
	return 0;
    if (!cmp_line_symbolizers (style1->links_sym, style2->links_sym))
	return 0;
    if (!cmp_point_symbolizers (style1->nodes_sym, style2->nodes_sym))
	return 0;
    if (!cmp_point_symbolizers (style1->link_seeds_sym, style2->link_seeds_sym))
	return 0;
    return 1;
}

static int
cmp_network_internal_styles (rl2MapNetworkLayerInternalStylePtr style1,
			     rl2MapNetworkLayerInternalStylePtr style2)
{
/*  comparison of two Network Internal Styles */
    if (style1 == NULL && style2 == NULL)
	return 1;
    if (style1 == NULL || style2 == NULL)
	return 0;
    if (!cmp_strings (style1->style_internal_name, style2->style_internal_name))
	return 0;
    if (style1->show_links != style2->show_links)
	return 0;
    if (style1->show_nodes != style2->show_nodes)
	return 0;
    if (style1->show_link_seeds != style2->show_link_seeds)
	return 0;
    return 1;
}

static int
cmp_wms_styles (rl2MapWmsLayerStylePtr style1, rl2MapWmsLayerStylePtr style2)
{
/*  comparison of two WMS Styles */
    if (style1 == NULL && style2 == NULL)
	return 1;
    if (style1 == NULL || style2 == NULL)
	return 0;
    if (!cmp_strings (style1->http_proxy, style2->http_proxy))
	return 0;
    if (!cmp_strings (style1->get_map_url, style2->get_map_url))
	return 0;
    if (!cmp_strings
	(style1->get_feature_info_url, style2->get_feature_info_url))
	return 0;
    if (!cmp_strings (style1->wms_protocol, style2->wms_protocol))
	return 0;
    if (!cmp_strings (style1->crs, style2->crs))
	return 0;
    if (style1->swap_xy != style2->swap_xy)
	return 0;
    if (!cmp_strings (style1->style, style2->style))
	return 0;
    if (!cmp_strings (style1->image_format, style2->image_format))
	return 0;
    if (style1->opaque != style2->opaque)
	return 0;
    if (!cmp_strings (style1->background_color, style2->background_color))
	return 0;
    if (style1->is_tiled != style2->is_tiled)
	return 0;
    if (style1->tile_width != style2->tile_width)
	return 0;
    if (style1->tile_height != style2->tile_height)
	return 0;
    return 1;
}

static int
cmp_layers (rl2MapLayerPtr lyr1, rl2MapLayerPtr lyr2)
{
/* comparison of two Layers */
    if (lyr1->type != lyr2->type)
	return 0;
    if (!cmp_strings (lyr1->prefix, lyr2->prefix))
	return 0;
    if (!cmp_strings (lyr1->name, lyr2->name))
	return 0;
    if (lyr1->visible != lyr2->visible)
	return -1;
    if (lyr1->ok_min_scale != lyr2->ok_min_scale)
	return -2;
    if (lyr1->min_scale != lyr2->min_scale)
	return -2;
    if (lyr1->ok_max_scale != lyr2->ok_max_scale)
	return -2;
    if (lyr1->max_scale != lyr2->max_scale)
	return -2;
    if (!cmp_strings
	(lyr1->raster_style_internal_name, lyr2->raster_style_internal_name))
	return -3;
    if (!cmp_strings
	(lyr1->vector_style_internal_name, lyr2->vector_style_internal_name))
	return -3;
    if (!cmp_raster_styles (lyr1->raster_style, lyr2->raster_style))
	return -3;
    if (!cmp_vector_styles (lyr1->vector_style, lyr2->vector_style))
	return -3;
    if (!cmp_topology_styles (lyr1->topology_style, lyr2->topology_style))
	return -3;
    if (!cmp_topology_internal_styles
	(lyr1->topology_internal_style, lyr2->topology_internal_style))
	return -3;
    if (!cmp_network_styles (lyr1->network_style, lyr2->network_style))
	return -3;
    if (!cmp_network_internal_styles
	(lyr1->network_internal_style, lyr2->network_internal_style))
	return -3;
    if (!cmp_wms_styles (lyr1->wms_style, lyr2->wms_style))
	return -3;
    return 1;
}

static rl2MapConfigChangesPtr
alloc_changes ()
{
/* allocating an empty list of Changed Items */
    rl2MapConfigChangesPtr list = malloc (sizeof (rl2MapConfigChanges));
    list->first = NULL;
    list->last = NULL;
    list->count = 0;
    list->array = NULL;
    return list;
}

RL2_DECLARE void
rl2_destroy_map_config_changes (rl2MapConfigChangesPtr list)
{
/* memory cleanup: destroying a list of MapConfig Changes */
    rl2MapConfigChangedItem *p;
    rl2MapConfigChangedItem *pn;
    if (list == NULL)
	return;

    p = list->first;
    while (p != NULL)
      {
	  pn = p->next;
	  if (p->description != NULL)
	      free (p->description);
	  free (p);
	  p = pn;
      }
    if (list->array != NULL)
	free (list->array);
    free (list);
}

RL2_DECLARE int
rl2_map_config_changes_get_count (rl2MapConfigChangesPtr list)
{
/* will return the number of items into the array */
    if (list == NULL)
	return 0;
    return list->count;
}

RL2_DECLARE const char *
rl2_map_config_changes_get_item (rl2MapConfigChangesPtr list, int idx)
{
/* will return the description for the Nth item */
    rl2MapConfigChangedItem *p;
    if (list == NULL)
	return NULL;
    if (idx < 0 || idx >= list->count)
	return NULL;
    p = *(list->array + idx);
    return p->description;
}

static void
add_change (rl2MapConfigChangesPtr list, const char *desc)
{
/* adding a changed item to the list */
    rl2MapConfigChangedItemPtr item;
    int len;
    if (list == NULL)
	return;

    item = malloc (sizeof (rl2MapConfigChangedItem));
    len = strlen (desc);
    item->description = malloc (len + 1);
    strcpy (item->description, desc);
    item->next = NULL;

    if (list->first == NULL)
	list->first = item;
    if (list->last != NULL)
	list->last->next = item;
    list->last = item;
}

static void
prepare_changes (rl2MapConfigChangesPtr list)
{
/* preparing the array of changes */
    int i;
    rl2MapConfigChangedItem *p;
    if (list == NULL)
	return;

    list->count = 0;
    p = list->first;
    while (p != NULL)
      {
	  list->count += 1;
	  p = p->next;
      }
    if (list->array != NULL)
	free (list->array);
    if (list->count == 0)
      {
	  list->array = NULL;
	  return;
      }
    list->array = malloc (sizeof (rl2MapConfigChangedItemPtr) * list->count);
    p = list->first;
    i = 0;
    while (p != NULL)
      {
	  *(list->array + i) = p;
	  i++;
	  p = p->next;
      }
}

RL2_DECLARE rl2MapConfigChangesPtr
rl2_compare_map_configs (rl2MapConfigPtr conf1, rl2MapConfigPtr conf2)
{
/* comparing two MapConfig objects for identity */
    int count1;
    int count2;
    rl2MapAttachedDbPtr db1;
    rl2MapAttachedDbPtr db2;
    rl2MapLayerPtr lyr1;
    rl2MapLayerPtr lyr2;
    rl2MapConfigChangesPtr changes = alloc_changes ();

    if (conf1 == NULL && conf2 == NULL)
	return NULL;
    if (conf1 == NULL || conf2 == NULL)
      {
	  add_change (changes, "*** CRITICAL: found a NULL Map Configuration");
	  return changes;
      }

/* checking MapConfiguration */
    if (!cmp_strings (conf1->name, conf2->name))
	add_change (changes, "MapConfig Name");
    if (!cmp_strings (conf1->title, conf2->title))
	add_change (changes, "MapConfig Title");
    if (!cmp_strings (conf1->abstract, conf2->abstract))
	add_change (changes, "MapConfig Abstract");
    if (conf1->multithread_enabled != conf2->multithread_enabled)
	add_change (changes, "MapConfig MultithreadEnabled");
    if (conf1->max_threads != conf2->max_threads)
	add_change (changes, "MapConfig MaxThreads");
    if (conf1->srid != conf2->srid)
	add_change (changes, "MapConfig SRID");
    if (conf1->autotransform_enabled != conf2->autotransform_enabled)
	add_change (changes, "MapConfig AutoTransformEnabled");
    if (conf1->dms != conf2->dms)
	add_change (changes, "MapConfig DD/DMS");
    if ((conf1->map_background_red != conf2->map_background_red)
	|| (conf1->map_background_green != conf2->map_background_green)
	|| (conf1->map_background_blue != conf2->map_background_blue))
	add_change (changes, "MapConfig BgColor");
    if (conf1->map_background_transparent != conf2->map_background_transparent)
	add_change (changes, "MapConfig BgColorTransparent");
    if (conf1->raster_wms_auto_switch != conf2->raster_wms_auto_switch)
	add_change (changes, "MapConfig RasterWmsAutoSwitch");
    if (conf1->max_wms_retries != conf2->max_wms_retries)
	add_change (changes, "MapConfig MaxWmsRetries");
    if (conf1->wms_pause != conf2->wms_pause)
	add_change (changes, "MapConfig WmsPause");
    if (conf1->label_anti_collision != conf2->label_anti_collision)
	add_change (changes, "MapConfig LabelAntiCollision");
    if (conf1->label_wrap_text != conf2->label_wrap_text)
	add_change (changes, "MapConfig LabelWrapText");
    if (conf1->label_auto_rotate != conf2->label_auto_rotate)
	add_change (changes, "MapConfig LabelAutoRotate");
    if (conf1->label_shift_position != conf2->label_shift_position)
	add_change (changes, "MapConfig LabelAutoShift");
    if (!cmp_visibility_range (conf1, conf2))
	add_change (changes, "VisibilityScaleRange");
    if (!cmp_bbox (conf1->bbox, conf2->bbox))
	add_change (changes, "MapConfig BBOX");

/* checking ATTACHED-DBs */
    count1 = 0;
    db1 = conf1->first_db;
    while (db1 != NULL)
      {
	  count1++;
	  db1 = db1->next;
      }
    count2 = 0;
    db2 = conf2->first_db;
    while (db2 != NULL)
      {
	  count2++;
	  db2 = db2->next;
      }
    if (count1 != count2)
	add_change (changes, "MapConfig AttachedDB");
    else
      {
	  db1 = conf1->first_db;
	  db2 = conf2->first_db;
	  while (db1 != NULL && db2 != NULL)
	    {
		if (!cmp_attached_dbs (db1, db2))
		    add_change (changes, "MapConfig AttachedDB");
		db1 = db1->next;
		db2 = db2->next;
	    }
      }

/* checking Layers */
    count1 = 0;
    lyr1 = conf1->first_lyr;
    while (lyr1 != NULL)
      {
	  count1++;
	  lyr1 = lyr1->next;
      }
    count2 = 0;
    lyr2 = conf2->first_lyr;
    while (lyr2 != NULL)
      {
	  count2++;
	  lyr2 = lyr2->next;
      }
    if (count1 != count2)
	add_change (changes, "MapConfig Layer");
    else
      {
	  lyr1 = conf1->first_lyr;
	  lyr2 = conf2->first_lyr;
	  while (lyr1 != NULL && lyr2 != NULL)
	    {
		int ret = cmp_layers (lyr1, lyr2);
		if (ret == 0)
		    add_change (changes, "MapConfig Layer");
		else if (ret < 0)
		  {
		      char *str2;
		      char *str = sqlite3_mprintf ("MapConfig Layer %s.%s",
						   lyr1->prefix, lyr1->name);
		      if (ret == -1)
			  str2 = sqlite3_mprintf ("%s Visibility", str);
		      else if (ret == -2)
			  str2 = sqlite3_mprintf ("%s ScaleLimits", str);
		      else
			  str2 = sqlite3_mprintf ("%s Style", str);
		      add_change (changes, str2);
		      sqlite3_free (str);
		      sqlite3_free (str2);
		  }
		lyr1 = lyr1->next;
		lyr2 = lyr2->next;
	    }
      }

    if (changes->first == NULL)
      {
	  rl2_destroy_map_config_changes (changes);
	  return NULL;
      }
    prepare_changes (changes);
    return changes;
}

RL2_DECLARE unsigned char *
rl2_xml_from_registered_map_config (sqlite3 * sqlite, const char *mapconf_name)
{
/* attemping to read a registered MapConfiguratio */
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *sql;
    unsigned char *xml = NULL;
    int count = 0;

    sql = sqlite3_mprintf ("SELECT XB_GetDocument(config) "
			   "FROM rl2map_configurations WHERE name = %Q",
			   mapconf_name);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  // scrolling the result set rows
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		// end of result set
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *docxml = sqlite3_column_text (stmt, 0);
		int bytes = sqlite3_column_bytes (stmt, 0);
		if (xml != NULL)
		    free (xml);
		xml = (unsigned char *) malloc (bytes + 1);
		memset (xml, '\0', bytes + 1);
		memcpy (xml, docxml, bytes);
		count++;
	    }
      }
    sqlite3_finalize (stmt);

    if (count == 1)
	return xml;

  error:
    if (xml != NULL)
	free (xml);
    return NULL;
}

static int
do_check_prefix (rl2PrivMapConfigAuxPtr aux, const char *prefix,
		 const char **alias)
{
/* checking if some DB Prefix is valid (may be by adopting an alias name) */
    rl2PrivMapAttachedDbPtr aux_db;
    *alias = NULL;

    if (strcasecmp (prefix, "main") == 0)
      {
	  /* MAIN db - never requires remapping */
	  *alias = prefix;
	  return 1;
      }

    aux_db = aux->first_db;
    while (aux_db != NULL)
      {
	  if (strcasecmp (aux_db->attached->prefix, prefix) == 0)
	    {
		if (aux_db->valid == 0)
		    return 0;
		if (aux_db->remapped == NULL)
		    *alias = aux_db->attached->prefix;
		else
		    *alias = aux_db->remapped;
		return 1;
	    }
	  aux_db = aux_db->next;
      }
    return 0;
}

static char *
rl2DoubleQuotedSql (const char *value)
{
/*
/ returns a well formatted TEXT value for SQL
/ 1] strips trailing spaces
/ 2] masks any DOUBLE-QUOTE inside the string, appending another DOUBLE-QUOTE
*/
    const char *p_in;
    const char *p_end;
    char qt = '"';
    char *out;
    char *p_out;
    int len = 0;
    int i;

    if (!value)
	return NULL;

    p_end = value;
    for (i = (strlen (value) - 1); i >= 0; i--)
      {
	  /* stripping trailing spaces */
	  p_end = value + i;
	  if (value[i] != ' ')
	      break;
      }

    p_in = value;
    while (p_in <= p_end)
      {
	  /* computing the output length */
	  len++;
	  if (*p_in == qt)
	      len++;
	  p_in++;
      }
    if (len == 1 && *value == ' ')
      {
	  /* empty string */
	  len = 0;
      }

    out = malloc (len + 1);
    if (!out)
	return NULL;

    if (len == 0)
      {
	  /* empty string */
	  *out = '\0';
	  return out;
      }

    p_out = out;
    p_in = value;
    while (p_in <= p_end)
      {
	  /* creating the output string */
	  if (*p_in == qt)
	      *p_out++ = qt;
	  *p_out++ = *p_in++;
      }
    *p_out = '\0';
    return out;
}

static void
do_check_layer_raster (sqlite3 * sqlite, const char *prefix,
		       rl2PrivMapLayerPtr aux_lyr)
{
/* verifying a Raster Layer configuration */
    char *sql;
    char *qprefix;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *value;
    int valid = 0;

    if (prefix == NULL)
	aux_lyr->prefix = "MAIN";
    else
	aux_lyr->prefix = prefix;
    qprefix = rl2DoubleQuotedSql (aux_lyr->prefix);
    sql = sqlite3_mprintf ("SELECT Count(*), "
			   "RL2_GetBandStatistics_Min(statistics, 0), RL2_GetBandStatistics_Max(statistics, 0) "
			   "FROM \"%s\".raster_coverages AS r "
			   "JOIN \"%s\".data_licenses AS l ON (r.license = l.id) "
			   "WHERE  Upper(r.coverage_name) = Upper(%Q) AND "
			   "r.geo_minx IS NOT NULL AND r.geo_miny IS NOT NULL AND "
			   "r.geo_maxx IS NOT NULL AND r.geo_maxy IS NOT NULL AND "
			   "r.extent_minx IS NOT NULL AND r.extent_miny IS NOT NULL AND "
			   "r.extent_maxx IS NOT NULL AND r.extent_maxy IS NOT NULL",
			   qprefix, qprefix, aux_lyr->layer->name);
    free (qprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CheckLayerRaster error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  aux_lyr->valid = 0;
	  return;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) == 1)
		    valid = 1;
		if (results[(i * columns) + 1] != NULL)
		    aux_lyr->min_value = atof (results[(i * columns) + 1]);
		if (results[(i * columns) + 2] != NULL)
		    aux_lyr->max_value = atof (results[(i * columns) + 2]);
	    }
      }
    sqlite3_free_table (results);
    aux_lyr->valid = valid;
}

static void
do_check_layer_WMS (sqlite3 * sqlite, const char *prefix,
		    rl2PrivMapLayerPtr aux_lyr)
{
/* verifying a WMW Layer configuration */
    char *sql;
    char *qprefix;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *value;
    int valid = 0;

    if (prefix == NULL)
	qprefix = rl2DoubleQuotedSql ("MAIN");
    else
	qprefix = rl2DoubleQuotedSql (prefix);
    sql = sqlite3_mprintf ("SELECT Count(*) "
			   "FROM \"%s\".wms_getmap AS w "
			   "JOIN \"%s\".data_licenses AS l ON (w.license = l.id)"
			   "LEFT JOIN \"%s\".wms_ref_sys AS g ON (w.id = g.parent_id AND g.srs = 'EPSG:4326') "
			   "LEFT JOIN \"%s\".wms_ref_sys AS d ON (w.id = d.parent_id AND d.is_default = 1) "
			   "WHERE Upper(w.layer_name) = Upper(%Q)",
			   qprefix, qprefix, qprefix, qprefix,
			   aux_lyr->layer->name);
    free (qprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CheckLayerWMS error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  aux_lyr->valid = 0;
	  return;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) == 1)
		    valid = 1;
	    }
      }
    sqlite3_free_table (results);
    aux_lyr->valid = valid;
}

static void
do_check_layer_vector (sqlite3 * sqlite, const char *prefix,
		       rl2PrivMapLayerPtr aux_lyr)
{
/* verifying a Vector Layer configuration */
    char *sql;
    char *qprefix;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *value;
    int valid = 0;

    if (prefix == NULL)
	qprefix = rl2DoubleQuotedSql ("MAIN");
    else
	qprefix = rl2DoubleQuotedSql (prefix);
    sql = sqlite3_mprintf ("SELECT Count(*) "
			   "FROM \"%s\".vector_coverages AS v "
			   "JOIN \"%s\".geometry_columns AS g ON (v.f_table_name = g.f_table_name AND "
			   "v.f_geometry_column = g.f_geometry_column) "
			   "JOIN \"%s\".data_licenses AS l ON (v.license = l.id) "
			   "WHERE Upper(v.coverage_name) = Upper(%Q) AND "
			   "v.topology_name IS NULL AND v.network_name IS NULL AND "
			   "v.geo_minx IS NOT NULL AND v.geo_miny IS NOT NULL AND "
			   "v.geo_maxx IS NOT NULL AND v.geo_maxy IS NOT NULL AND "
			   "v.extent_minx IS NOT NULL AND v.extent_miny IS NOT NULL AND "
			   "v.extent_maxx IS NOT NULL AND v.extent_maxy IS NOT NULL",
			   qprefix, qprefix, qprefix, aux_lyr->layer->name);
    free (qprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CheckLayerVector error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  aux_lyr->valid = 0;
	  return;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) == 1)
		    valid = 1;
	    }
      }
    sqlite3_free_table (results);
    aux_lyr->valid = valid;
}

static void
do_check_layer_vector_view (sqlite3 * sqlite, const char *prefix,
			    rl2PrivMapLayerPtr aux_lyr)
{
/* verifying a VectorView Layer configuration */
    char *sql;
    char *qprefix;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *value;
    int valid = 0;

    if (prefix == NULL)
	qprefix = rl2DoubleQuotedSql ("MAIN");
    else
	qprefix = rl2DoubleQuotedSql (prefix);
    sql = sqlite3_mprintf ("SELECT Count(*) "
			   "FROM \"%s\".vector_coverages AS v "
			   "JOIN \"%s\".views_geometry_columns AS x ON (v.view_name = x.view_name "
			   "AND v.view_geometry = x.view_geometry) "
			   "JOIN \"%s\".geometry_columns AS g ON (x.f_table_name = g.f_table_name "
			   "AND x.f_geometry_column = g.f_geometry_column) "
			   "JOIN \"%s\".data_licenses AS l ON (v.license = l.id) "
			   "WHERE Upper(v.coverage_name) = Upper(%Q) AND "
			   "v.view_name IS NOT NULL AND v.view_geometry IS NOT NULL AND "
			   "v.geo_minx IS NOT NULL AND v.geo_miny IS NOT NULL AND "
			   "v.geo_maxx IS NOT NULL AND v.geo_maxy IS NOT NULL AND "
			   "v.extent_minx IS NOT NULL AND v.extent_miny IS NOT NULL AND "
			   "v.extent_maxx IS NOT NULL AND v.extent_maxy IS NOT NULL",
			   qprefix, qprefix, qprefix, qprefix,
			   aux_lyr->layer->name);
    free (qprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CheckLayerVectorView error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  aux_lyr->valid = 0;
	  return;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) == 1)
		    valid = 1;
	    }
      }
    sqlite3_free_table (results);
    aux_lyr->valid = valid;
}

static void
do_check_layer_vector_virtual (sqlite3 * sqlite, const char *prefix,
			       rl2PrivMapLayerPtr aux_lyr)
{
/* verifying a VectorVirtual Layer configuration */
    char *sql;
    char *qprefix;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *value;
    int valid = 0;

    if (prefix == NULL)
	qprefix = rl2DoubleQuotedSql ("MAIN");
    else
	qprefix = rl2DoubleQuotedSql (prefix);
    sql = sqlite3_mprintf ("SELECT Count(*) "
			   "FROM \"%s\".vector_coverages AS v "
			   "JOIN \"%s\".virts_geometry_columns AS s ON (v.virt_name = s.virt_name "
			   "AND v.virt_geometry = s.virt_geometry) "
			   "JOIN \"%s\".data_licenses AS l ON (v.license = l.id) "
			   "WHERE Upper(v.coverage_name) = Upper(%Q) AND "
			   "v.virt_name IS NOT NULL AND v.virt_geometry IS NOT NULL AND "
			   "v.geo_minx IS NOT NULL AND v.geo_miny IS NOT NULL AND "
			   "v.geo_maxx IS NOT NULL AND v.geo_maxy IS NOT NULL AND "
			   "v.extent_minx IS NOT NULL AND v.extent_miny IS NOT NULL AND "
			   "v.extent_maxx IS NOT NULL AND v.extent_maxy IS NOT NULL",
			   qprefix, qprefix, qprefix, aux_lyr->layer->name);
    free (qprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CheckLayerVectorVirtual error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  aux_lyr->valid = 0;
	  return;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) == 1)
		    valid = 1;
	    }
      }
    sqlite3_free_table (results);
    aux_lyr->valid = valid;
}

static void
do_check_layer_topology (sqlite3 * sqlite, const char *prefix,
			 rl2PrivMapLayerPtr aux_lyr)
{
/* verifying a Topology Layer configuration */
    char *sql;
    char *qprefix;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *value;
    int valid = 0;

    if (prefix == NULL)
	qprefix = rl2DoubleQuotedSql ("MAIN");
    else
	qprefix = rl2DoubleQuotedSql (prefix);
    sql = sqlite3_mprintf ("SELECT Count(*) "
			   "FROM \"%s\".vector_coverages AS v "
			   "JOIN \"%s\".topologies AS t ON (v.topology_name = t.topology_name) "
			   "JOIN \"%s\".data_licenses AS l ON (v.license = l.id) "
			   "WHERE Upper(v.coverage_name) = Upper(%Q) AND "
			   "v.topology_name IS NOT NULL AND "
			   "v.geo_minx IS NOT NULL AND v.geo_miny IS NOT NULL AND "
			   "v.geo_maxx IS NOT NULL AND v.geo_maxy IS NOT NULL AND "
			   "v.extent_minx IS NOT NULL AND v.extent_miny IS NOT NULL AND "
			   "v.extent_maxx IS NOT NULL AND v.extent_maxy IS NOT NULL",
			   qprefix, qprefix, qprefix, aux_lyr->layer->name);
    free (qprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CheckLayerTopology error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  aux_lyr->valid = 0;
	  return;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) == 1)
		    valid = 1;
	    }
      }
    sqlite3_free_table (results);
    aux_lyr->valid = valid;
}

static void
do_check_layer_network (sqlite3 * sqlite, const char *prefix,
			rl2PrivMapLayerPtr aux_lyr)
{
/* completing a Network Layer configuration */
    char *sql;
    char *qprefix;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;
    char *value;
    int valid = 0;

    if (prefix == NULL)
	qprefix = rl2DoubleQuotedSql ("MAIN");
    else
	qprefix = rl2DoubleQuotedSql (prefix);
    sql = sqlite3_mprintf ("SELECT Count(*) "
			   "FROM \"%s\".vector_coverages AS v "
			   "JOIN \"%s\".networks AS n ON (v.network_name = n.network_name) "
			   "JOIN \"%s\".data_licenses AS l ON (v.license = l.id) "
			   "WHERE Upper(v.coverage_name) = Upper(%Q) AND "
			   "v.network_name IS NOT NULL AND "
			   "v.geo_minx IS NOT NULL AND v.geo_miny IS NOT NULL AND "
			   "v.geo_maxx IS NOT NULL AND v.geo_maxy IS NOT NULL AND "
			   "v.extent_minx IS NOT NULL AND v.extent_miny IS NOT NULL AND "
			   "v.extent_maxx IS NOT NULL AND v.extent_maxy IS NOT NULL",
			   qprefix, qprefix, qprefix, aux_lyr->layer->name);
    free (qprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "CheckLayerNetwork error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  aux_lyr->valid = 0;
	  return;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		value = results[(i * columns) + 0];
		if (atoi (value) == 1)
		    valid = 1;
	    }
      }
    sqlite3_free_table (results);
    aux_lyr->valid = valid;
}

static void
do_fetch_minmax_values (sqlite3 * sqlite, rl2PrivMapLayerPtr lyr)
{
/* Fetching Min/Max values from Raster Statistics */
    char *sql;
    char *qprefix;
    int ret;
    int i;
    char **results;
    int rows;
    int columns;
    char *errMsg = NULL;

    if (lyr->prefix == NULL)
	lyr->prefix = "MAIN";
    qprefix = rl2DoubleQuotedSql (lyr->prefix);
    sql =
	sqlite3_mprintf
	("SELECT RL2_GetBandStatistics_Min(statistics, 0), RL2_GetBandStatistics_Max(statistics, 0) "
	 "FROM \"%s\".raster_coverages "
	 "WHERE  Upper(coverage_name) = Upper(%Q)", qprefix, lyr->layer->name);
    free (qprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, &errMsg);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "Fetch Raster MinMax error: %s\n", errMsg);
	  sqlite3_free (errMsg);
	  return;
      }
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		if (results[(i * columns) + 0] != NULL)
		    lyr->min_value = atof (results[(i * columns) + 0]);
		if (results[(i * columns) + 1] != NULL)
		    lyr->max_value = atof (results[(i * columns) + 1]);
	    }
      }
    sqlite3_free_table (results);
}

static int
is_already_attached_db (sqlite3 * sqlite, const char *prefix, const char *path)
{
/* testing for an already attached DB */
    int ret;
    const char *sql;
    int i;
    char **results;
    int rows;
    int columns;
    int found = 0;

    sql = "PRAGMA database_list";
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *db_prefix = results[(i * columns) + 1];
		const char *db_path = results[(i * columns) + 2];
		if (strcasecmp (db_prefix, prefix) == 0
		    && strcasecmp (db_path, path) == 0)
		    found = 1;
	    }
      }
    sqlite3_free_table (results);
    return found;
}

static int
check_valid_db_file (const char *path)
{
/* testing if the given DB-file do really exists and is accessible */
    int ret;
    sqlite3 *sqlite;
    ret = sqlite3_open_v2 (path, &sqlite, SQLITE_OPEN_READWRITE, NULL);
    if (ret != SQLITE_OK)
	return 0;
    sqlite3_close (sqlite);
    return 1;
}

static int
not_already_attached (sqlite3 * sqlite, const char *alias)
{
/* testing for an unused DB Prefix */
    int ret;
    const char *sql;
    int i;
    char **results;
    int rows;
    int columns;
    int not_used = 1;

    sql = "PRAGMA database_list";
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	return 1;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *db_prefix = results[(i * columns) + 1];
		if (strcasecmp (db_prefix, alias) == 0)
		    not_used = 0;
	    }
      }
    sqlite3_free_table (results);
    return not_used;
}

static void
validate_attached_db (sqlite3 * sqlite, rl2PrivMapAttachedDbPtr aux_db)
{
/* attempting to validate an Attached DB */
    char *sql;
    const char *prefix = aux_db->attached->prefix;
    const char *db_path = aux_db->attached->path;
    const char *real_prefix;
    char *qprefix;
    char *qpath;
    char alias[64];
    int ret;

    if (is_already_attached_db (sqlite, prefix, db_path))
      {
	  /* already attached */
	  aux_db->valid = 1;
	  return;
      }
    if (!check_valid_db_file (db_path))
      {
	  /* not existing (or not accessible) DB-file */
	  aux_db->valid = 0;
	  return;
      }

    if (not_already_attached (sqlite, prefix))
	real_prefix = prefix;
    else
      {
	  int idx = 0;
	  while (1)
	    {
		/* searching a free Alias Prefix */
		sprintf (alias, "alias_#%d", ++idx);
		if (not_already_attached (sqlite, alias))
		  {
		      real_prefix = alias;
		      break;
		  }
	    }
      }
    qpath = rl2DoubleQuotedSql (db_path);
    qprefix = rl2DoubleQuotedSql (real_prefix);
    sql = sqlite3_mprintf ("ATTACH DATABASE \"%s\" AS \"%s\"", qpath, qprefix);
    free (qpath);
    free (qprefix);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
      {
	  aux_db->valid = 0;
	  return;
      }
    aux_db->valid = 1;
    if (aux_db->remapped != NULL)
	free (aux_db->remapped);
    aux_db->remapped = NULL;
    if (strcasecmp (real_prefix, prefix) != 0)
      {
	  int len = strlen (real_prefix);
	  aux_db->remapped = (char *) malloc (len + 1);
	  strcpy (aux_db->remapped, real_prefix);
      }
}

static void
raster_wms_auto_switch (rl2MapLayerPtr first_layer, double scale)
{
/* auto switching on/off Raster and WMS Layers */
    rl2MapLayerPtr lyr = first_layer;
    while (lyr != NULL)
      {
	  /* switching off Layers forbidden by current Scale */
	  if (lyr->type == RL2_MAP_LAYER_WMS
	      || lyr->type == RL2_MAP_LAYER_RASTER)
	    {
		/* evaluating a WMS or Raster Layer */
		if (lyr->visible)
		  {
		      if (lyr->ok_min_scale)
			{
			    if (scale < lyr->min_scale)
				lyr->visible = 0;
			}
		      if (lyr->ok_max_scale)
			{
			    if (scale > lyr->max_scale)
				lyr->visible = 0;
			}
		  }
	    }
	  lyr = lyr->next;
      }

    lyr = first_layer;
    while (lyr != NULL)
      {
	  /* testing for at least one visible WMS or Raster Layer */
	  if (lyr->type == RL2_MAP_LAYER_RASTER
	      || lyr->type == RL2_MAP_LAYER_WMS)
	    {
		if (lyr->visible)
		    return;
	    }
	  lyr = lyr->next;
      }

    lyr = first_layer;
    while (lyr != NULL)
      {
	  /* attemtping to set a visible WMS or Raster Layer */
	  if (lyr->type == RL2_MAP_LAYER_WMS
	      || lyr->type == RL2_MAP_LAYER_RASTER)
	    {
		/* evaluating a WMS or Raster Layer */
		int visible = 1;
		if (lyr->ok_min_scale)
		  {
		      if (scale < lyr->min_scale)
			  visible = 0;
		  }
		if (lyr->ok_max_scale)
		  {
		      if (scale > lyr->max_scale)
			  visible = 0;
		  }
		if (visible)
		  {
		      lyr->visible = 1;
		      return;
		  }
	    }
	  lyr = lyr->next;
      }
}

RL2_PRIVATE rl2PrivMapConfigAuxPtr
rl2_create_map_config_aux (sqlite3 * sqlite, const void *data,
			   rl2MapConfigPtr map_config, int width, int height,
			   const char *format, int quality,
			   const unsigned char *blob, int blob_sz, int reaspect)
{
/* allocating a MapConfig Aux object */
    int ok_format = 0;
    int format_id;
    rl2MapAttachedDbPtr db;
    rl2MapLayerPtr lyr;
    rl2PrivMapAttachedDbPtr aux_db;
    rl2PrivMapLayerPtr aux_lyr;
    rl2PrivMapConfigAuxPtr aux = malloc (sizeof (rl2PrivMapConfigAux));
    aux->valid = 1;
    aux->options.multithread_enabled = map_config->multithread_enabled;
    aux->options.max_threads = map_config->max_threads;
    aux->options.srid = map_config->srid;
    aux->options.autotransform_enabled = map_config->autotransform_enabled;
    aux->options.max_wms_retries = map_config->max_wms_retries;
    aux->options.wms_pause = map_config->wms_pause;
    aux->options.label_anti_collision = map_config->label_anti_collision;
    aux->options.label_wrap_text = map_config->label_wrap_text;
    aux->options.label_auto_rotate = map_config->label_auto_rotate;
    aux->options.label_shift_position = map_config->label_shift_position;
    aux->width = width;
    aux->height = height;
    aux->pixel_ratio = 0.0;
    aux->scale = 0.0;
    aux->map_config = map_config;
    aux->ctx = NULL;
    aux->image = NULL;
    aux->image_size = 0;
    aux->first_db = NULL;
    aux->last_db = NULL;
    aux->first_lyr = NULL;
    aux->last_lyr = NULL;
    aux->has_labels = 0;
    aux->update_labels = 0;
    aux->ctx_labels = NULL;
    aux->canvas_labels = NULL;
    if (strcmp (format, "image/png") == 0)
      {
	  format_id = RL2_OUTPUT_FORMAT_PNG;
	  ok_format = 1;
      }
    if (strcmp (format, "image/jpeg") == 0)
      {
	  format_id = RL2_OUTPUT_FORMAT_JPEG;
	  ok_format = 1;
      }
    if (strcmp (format, "image/tiff") == 0)
      {
	  format_id = RL2_OUTPUT_FORMAT_TIFF;
	  ok_format = 1;
      }
    if (strcmp (format, "application/x-pdf") == 0
	|| strcmp (format, "application/pdf") == 0)
      {
	  format_id = RL2_OUTPUT_FORMAT_PDF;
	  ok_format = 1;
      }
    if (strcmp (format, "image/vnd.rl2cairorgba") == 0)
      {
	  format_id = RL2_OUTPUT_FORMAT_CAIRO_RGBA;
	  ok_format = 1;
      }
    if (!ok_format)
      {
	  aux->format_id = RL2_OUTPUT_FORMAT_UNKNOWN;
	  goto error;
      }
    else
	aux->format_id = format_id;
    if (quality > 0 && quality < 100)
	aux->quality = quality;
    else
	aux->quality = 80;

    if (reaspect == 0)
      {
	  /* testing for consistent aspect ratios */
	  double aspect_org;
	  double aspect_dst;
	  double confidence;
	  aspect_org = do_compute_bbox_aspect_ratio (sqlite, blob, blob_sz);
	  if (aspect_org < 0.0)
	      goto error;
	  aspect_dst = (double) width / (double) height;
	  confidence = aspect_org / 100.0;
	  if (aspect_dst >= (aspect_org - confidence)
	      && aspect_dst <= (aspect_org + confidence))
	      ;
	  else
	      goto error;
      }

/* checking the Geometry */
    if (rl2_parse_bbox_srid
	(sqlite, blob, blob_sz, &(aux->srid), &(aux->min_x), &(aux->min_y),
	 &(aux->max_x), &(aux->max_y)) != RL2_OK)
	goto error;

/* computing the current Scale ratio */
    aux->pixel_ratio = rl2_pixel_ratio (aux->width, aux->height,
					aux->max_x - aux->min_x,
					aux->max_y - aux->min_y);
    aux->scale =
	rl2_standard_scale (sqlite, aux->srid, aux->width, aux->height,
			    aux->max_x - aux->min_x, aux->max_y - aux->min_y);
    if (aux->map_config->raster_wms_auto_switch)
	raster_wms_auto_switch (map_config->first_lyr, aux->scale);

    db = map_config->first_db;
    while (db != NULL)
      {
	  /* inserting all ATTACHED Database into the AUX struct */
	  aux_db = malloc (sizeof (rl2PrivMapAttachedDb));
	  aux_db->attached = db;
	  aux_db->remapped = NULL;
	  aux_db->valid = 0;
	  aux_db->next = NULL;

	  if (aux->first_db == NULL)
	      aux->first_db = aux_db;
	  if (aux->last_db != NULL)
	      aux->last_db->next = aux_db;
	  aux->last_db = aux_db;
	  db = db->next;
      }

    lyr = map_config->first_lyr;
    while (lyr != NULL)
      {
	  /* inserting all Map Layers into the AUX struct */
	  aux_lyr = malloc (sizeof (rl2PrivMapLayer));
	  aux_lyr->prefix = NULL;
	  aux_lyr->layer = lyr;
	  aux_lyr->min_value = 0.0;
	  aux_lyr->max_value = 0.0;
	  aux_lyr->style_name = NULL;
	  aux_lyr->xml_style = NULL;
	  aux_lyr->syntetic_band = RL2_SYNTETIC_NONE;
	  aux_lyr->ctx = NULL;
	  aux_lyr->ctx_nodes = NULL;
	  aux_lyr->ctx_edges = NULL;
	  aux_lyr->ctx_faces = NULL;
	  aux_lyr->ctx_edge_seeds = NULL;
	  aux_lyr->ctx_face_seeds = NULL;
	  aux_lyr->ctx_links = NULL;
	  aux_lyr->ctx_link_seeds = NULL;
	  aux_lyr->canvas = NULL;
	  aux_lyr->valid = lyr->visible;
	  aux_lyr->has_labels = 0;
	  aux_lyr->next = NULL;

	  if (aux->first_lyr == NULL)
	      aux->first_lyr = aux_lyr;
	  if (aux->last_lyr != NULL)
	      aux->last_lyr->next = aux_lyr;
	  aux->last_lyr = aux_lyr;
	  lyr = lyr->next;
      }

    aux_db = aux->first_db;
    while (aux_db != NULL)
      {
	  /* validating all Attached Database */
	  validate_attached_db (sqlite, aux_db);
	  aux_db = aux_db->next;
      }

    aux_lyr = aux->first_lyr;
    while (aux_lyr != NULL)
      {
	  /* validating all Map Layers */
	  const char *alias;
	  if (aux_lyr->layer->visible == 0)
	    {
		/* skipping any invisible Map Layer */
		aux_lyr = aux_lyr->next;
		continue;
	    }
	  switch (aux_lyr->layer->type)
	    {
	    case RL2_MAP_LAYER_RASTER:
		if (do_check_prefix (aux, aux_lyr->layer->prefix, &alias))
		    do_check_layer_raster (sqlite, alias, aux_lyr);
		if (aux_lyr->valid)
		  {
		      /* creating a Canvas */
		      aux_lyr->ctx =
			  rl2_graph_create_context (data, width, height);
		      if (aux_lyr->ctx != NULL)
			{
			    rl2_prime_background (aux_lyr->ctx, 0, 0, 0, 0);	/* transparent background */
			    aux_lyr->canvas =
				rl2_create_raster_canvas (aux_lyr->ctx);
			}
		      if (aux_lyr->canvas == NULL)
			  aux_lyr->valid = 0;
		      if (aux_lyr->canvas == NULL)
			  goto error;
		  }
		break;
	    case RL2_MAP_LAYER_WMS:
		if (do_check_prefix (aux, aux_lyr->layer->prefix, &alias))
		    do_check_layer_WMS (sqlite, alias, aux_lyr);
		if (aux_lyr->valid)
		  {
		      /* creating a Canvas */
		      aux_lyr->ctx =
			  rl2_graph_create_context (data, width, height);
		      if (aux_lyr->ctx != NULL)
			{
			    rl2_prime_background (aux_lyr->ctx, 0, 0, 0, 0);	/* transparent background */
			    aux_lyr->canvas =
				rl2_create_raster_canvas (aux_lyr->ctx);
			}
		      if (aux_lyr->canvas == NULL)
			  aux_lyr->valid = 0;
		      if (aux_lyr->canvas == NULL)
			  goto error;
		  }
		break;
	    case RL2_MAP_LAYER_VECTOR:
		if (do_check_prefix (aux, aux_lyr->layer->prefix, &alias))
		    do_check_layer_vector (sqlite, alias, aux_lyr);
		if (aux_lyr->valid)
		  {
		      /* creating a Canvas */
		      aux_lyr->ctx =
			  rl2_graph_create_context (data, width, height);
		      if (aux_lyr->ctx != NULL)
			{
			    rl2_prime_background (aux_lyr->ctx, 0, 0, 0, 0);	/* transparent background */
			    aux_lyr->canvas =
				rl2_create_vector_canvas (aux_lyr->ctx);
			}
		      if (aux_lyr->canvas == NULL)
			  aux_lyr->valid = 0;
		      if (aux_lyr->canvas == NULL)
			  goto error;
		  }
		break;
	    case RL2_MAP_LAYER_VECTOR_VIEW:
		if (do_check_prefix (aux, aux_lyr->layer->prefix, &alias))
		    do_check_layer_vector_view (sqlite, alias, aux_lyr);
		if (aux_lyr->valid)
		  {
		      /* creating a Canvas */
		      aux_lyr->ctx =
			  rl2_graph_create_context (data, width, height);
		      if (aux_lyr->ctx != NULL)
			{
			    rl2_prime_background (aux_lyr->ctx, 0, 0, 0, 0);	/* transparent background */
			    aux_lyr->canvas =
				rl2_create_vector_canvas (aux_lyr->ctx);
			}
		      if (aux_lyr->canvas == NULL)
			  aux_lyr->valid = 0;
		      if (aux_lyr->canvas == NULL)
			  goto error;
		  }
		break;
	    case RL2_MAP_LAYER_VECTOR_VIRTUAL:
		if (do_check_prefix (aux, aux_lyr->layer->prefix, &alias))
		    do_check_layer_vector_virtual (sqlite, alias, aux_lyr);
		if (aux_lyr->valid)
		  {
		      /* creating a Canvas */
		      aux_lyr->ctx =
			  rl2_graph_create_context (data, width, height);
		      if (aux_lyr->ctx != NULL)
			{
			    rl2_prime_background (aux_lyr->ctx, 0, 0, 0, 0);	/* transparent background */
			    aux_lyr->canvas =
				rl2_create_vector_canvas (aux_lyr->ctx);
			}
		      if (aux_lyr->canvas == NULL)
			  aux_lyr->valid = 0;
		      if (aux_lyr->canvas == NULL)
			  goto error;
		  }
		break;
	    case RL2_MAP_LAYER_TOPOLOGY:
		if (do_check_prefix (aux, aux_lyr->layer->prefix, &alias))
		    do_check_layer_topology (sqlite, alias, aux_lyr);
		if (aux_lyr->valid)
		  {
		      /* creating all Topology Canvas */
		      aux_lyr->ctx =
			  rl2_graph_create_context (data, width, height);
		      aux_lyr->ctx_nodes =
			  rl2_graph_create_context (data, width, height);
		      aux_lyr->ctx_edges =
			  rl2_graph_create_context (data, width, height);
		      aux_lyr->ctx_faces =
			  rl2_graph_create_context (data, width, height);
		      aux_lyr->ctx_edge_seeds =
			  rl2_graph_create_context (data, width, height);
		      aux_lyr->ctx_face_seeds =
			  rl2_graph_create_context (data, width, height);
		      if (aux_lyr->ctx != NULL && aux_lyr->ctx_nodes != NULL
			  && aux_lyr->ctx_edges != NULL
			  && aux_lyr->ctx_faces != NULL
			  && aux_lyr->ctx_edge_seeds != NULL
			  && aux_lyr->ctx_face_seeds != NULL)
			{
			    rl2_prime_background (aux_lyr->ctx, 0, 0, 0, 0);	/* transparent background */
			    rl2_prime_background (aux_lyr->ctx_nodes, 0, 0, 0, 0);	/* transparent background */
			    rl2_prime_background (aux_lyr->ctx_edges, 0, 0, 0, 0);	/* transparent background */
			    rl2_prime_background (aux_lyr->ctx_faces, 0, 0, 0, 0);	/* transparent background */
			    rl2_prime_background (aux_lyr->ctx_edge_seeds, 0, 0, 0, 0);	/* transparent background */
			    rl2_prime_background (aux_lyr->ctx_face_seeds, 0, 0, 0, 0);	/* transparent background */
			    aux_lyr->canvas =
				rl2_create_topology_canvas (aux_lyr->ctx,
							    aux_lyr->ctx_nodes,
							    aux_lyr->ctx_edges,
							    aux_lyr->ctx_faces,
							    aux_lyr->ctx_edge_seeds,
							    aux_lyr->ctx_face_seeds);
			}
		      if (aux_lyr->canvas == NULL)
			  aux_lyr->valid = 0;
		      if (aux_lyr->canvas == NULL)
			  goto error;
		  }
		break;
	    case RL2_MAP_LAYER_NETWORK:
		if (do_check_prefix (aux, aux_lyr->layer->prefix, &alias))
		    do_check_layer_network (sqlite, alias, aux_lyr);
		if (aux_lyr->valid)
		  {
		      /* creating all Network Canvas */
		      aux_lyr->ctx =
			  rl2_graph_create_context (data, width, height);
		      aux_lyr->ctx_nodes =
			  rl2_graph_create_context (data, width, height);
		      aux_lyr->ctx_links =
			  rl2_graph_create_context (data, width, height);
		      aux_lyr->ctx_link_seeds =
			  rl2_graph_create_context (data, width, height);
		      if (aux_lyr->ctx != NULL && aux_lyr->ctx_nodes != NULL
			  && aux_lyr->ctx_links != NULL
			  && aux_lyr->ctx_link_seeds != NULL)
			{
			    rl2_prime_background (aux_lyr->ctx, 0, 0, 0, 0);	/* transparent background */
			    rl2_prime_background (aux_lyr->ctx_nodes, 0, 0, 0, 0);	/* transparent background */
			    rl2_prime_background (aux_lyr->ctx_links, 0, 0, 0, 0);	/* transparent background */
			    rl2_prime_background (aux_lyr->ctx_link_seeds, 0, 0, 0, 0);	/* transparent background */
			    aux_lyr->canvas =
				rl2_create_network_canvas (aux_lyr->ctx,
							   aux_lyr->ctx_nodes,
							   aux_lyr->ctx_links,
							   aux_lyr->ctx_link_seeds);
			}
		      if (aux_lyr->canvas == NULL)
			  aux_lyr->valid = 0;
		      if (aux_lyr->canvas == NULL)
			  goto error;
		  }
		break;
	    };
	  aux_lyr = aux_lyr->next;
      }

    aux_lyr = aux->first_lyr;
    while (aux_lyr != NULL)
      {
	  /* creating all XML Quick Styles */
	  if (aux_lyr->valid)
	    {
		rl2MapLayerPtr layer = aux_lyr->layer;
		if (layer != NULL)
		  {
		      if (layer->raster_style != NULL)
			  do_fetch_minmax_values (sqlite, aux_lyr);
		  }
		rl2_create_xml_quick_style (aux_lyr);
	    }
	  aux_lyr = aux_lyr->next;
      }

/* preparing the Map canvas */
    aux->ctx = rl2_graph_create_context (data, width, height);
    rl2_prime_background (aux->ctx, 0, 0, 0, 0);	/* transparent background */

    return aux;

  error:
    aux->valid = 0;
    return aux;
}

RL2_PRIVATE void
rl2_destroy_map_config_aux (rl2PrivMapConfigAuxPtr aux)
{
/* memory cleanup - destroying a MapConfig Aux object */
    rl2PrivMapAttachedDbPtr db;
    rl2PrivMapAttachedDbPtr db_n;
    rl2PrivMapLayerPtr lyr;
    rl2PrivMapLayerPtr lyr_n;
    if (aux == NULL)
	return;

    db = aux->first_db;
    while (db != NULL)
      {
	  db_n = db->next;
	  if (db->remapped != NULL)
	      free (db->remapped);
	  free (db);
	  db = db_n;
      }
    lyr = aux->first_lyr;
    while (lyr != NULL)
      {
	  lyr_n = lyr->next;
	  if (lyr->style_name != NULL)
	      free (lyr->style_name);
	  if (lyr->xml_style != NULL)
	      sqlite3_free (lyr->xml_style);
	  if (lyr->canvas != NULL)
	      rl2_destroy_canvas (lyr->canvas);
	  if (lyr->ctx != NULL)
	      rl2_graph_destroy_context (lyr->ctx);
	  if (lyr->ctx_nodes != NULL)
	      rl2_graph_destroy_context (lyr->ctx_nodes);
	  if (lyr->ctx_edges != NULL)
	      rl2_graph_destroy_context (lyr->ctx_edges);
	  if (lyr->ctx_faces != NULL)
	      rl2_graph_destroy_context (lyr->ctx_faces);
	  if (lyr->ctx_edge_seeds != NULL)
	      rl2_graph_destroy_context (lyr->ctx_edge_seeds);
	  if (lyr->ctx_face_seeds != NULL)
	      rl2_graph_destroy_context (lyr->ctx_face_seeds);
	  if (lyr->ctx_links != NULL)
	      rl2_graph_destroy_context (lyr->ctx_links);
	  if (lyr->ctx_link_seeds != NULL)
	      rl2_graph_destroy_context (lyr->ctx_link_seeds);
	  free (lyr);
	  lyr = lyr_n;
      }

    if (aux->ctx != NULL)
	rl2_graph_destroy_context (aux->ctx);
    if (aux->canvas_labels != NULL)
	rl2_destroy_canvas (aux->canvas_labels);
    if (aux->ctx_labels != NULL)
	rl2_graph_destroy_context (aux->ctx_labels);
    free (aux);
}
