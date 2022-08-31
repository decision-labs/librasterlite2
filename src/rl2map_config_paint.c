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

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "config.h"

#include <libxml/parser.h>

#ifdef LOADABLE_EXTENSION
#include "rasterlite2/sqlite.h"
#endif

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2mapconfig.h"
#include "rasterlite2/rl2graphics.h"
#include "rasterlite2/rl2wms.h"
#include "rasterlite2_private.h"

#define RL2_UNUSED() if (argc || argv) argc = argc;

#define TOPO_NODE		1
#define TOPO_EDGE_SEED	2
#define TOPO_FACE_SEED	3
#define NET_NODE		4
#define NET_LINK_SEED	5

static void
do_paint_layer_vector (sqlite3 * sqlite, const void *data,
		       rl2PrivMapLayerPtr aux_lyr, rl2PrivMapConfigAuxPtr aux)
{
/* rendering a MapConfiguration Vector Layer */
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int ret;

    if (aux_lyr == NULL)
	return;
    if (aux_lyr->layer == NULL)
	return;

/* building the Map's Bounding Box */
    sql = "SELECT BuildMbr(?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, aux->min_x);
    sqlite3_bind_double (stmt, 2, aux->min_y);
    sqlite3_bind_double (stmt, 3, aux->max_x);
    sqlite3_bind_double (stmt, 4, aux->max_y);
    sqlite3_bind_int (stmt, 5, aux->srid);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      unsigned char *blob =
			  (unsigned char *) sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      if (aux_lyr->layer->type == RL2_MAP_LAYER_TOPOLOGY)
			  ret =
			      rl2_map_image_paint_from_vector_ex (sqlite, data,
								  (rl2CanvasPtr)
								  aux_lyr->canvas,
								  aux_lyr->prefix,
								  aux_lyr->layer->
								  name, blob,
								  blob_sz, 0,
								  aux_lyr->
								  style_name,
								  (unsigned char
								   *) aux_lyr->
								  xml_style,
								  aux_lyr->layer->topology_style->show_nodes,
								  aux_lyr->layer->topology_style->show_edges,
								  aux_lyr->layer->topology_style->show_faces,
								  aux_lyr->layer->topology_style->show_edge_seeds,
								  aux_lyr->layer->topology_style->show_face_seeds);
		      else if (aux_lyr->layer->type == RL2_MAP_LAYER_NETWORK)
			  ret =
			      rl2_map_image_paint_from_vector_ex (sqlite, data,
								  (rl2CanvasPtr)
								  aux_lyr->canvas,
								  aux_lyr->prefix,
								  aux_lyr->layer->
								  name, blob,
								  blob_sz, 0,
								  aux_lyr->
								  style_name,
								  (unsigned char
								   *) aux_lyr->
								  xml_style,
								  aux_lyr->layer->network_style->show_nodes,
								  aux_lyr->layer->network_style->show_links,
								  0,
								  aux_lyr->layer->network_style->show_link_seeds,
								  0);
		      else
			  ret = rl2_map_image_paint_from_vector (sqlite, data,
								 (rl2CanvasPtr)
								 aux_lyr->canvas,
								 aux_lyr->prefix,
								 aux_lyr->layer->
								 name, blob,
								 blob_sz, 0,
								 aux_lyr->
								 style_name,
								 (unsigned char
								  *) aux_lyr->
								 xml_style);
		      if (ret == RL2_OK)
			  ret = rl2_graph_merge (aux->ctx, aux_lyr->ctx);
		  }
	    }
      }
    sqlite3_finalize (stmt);
}

static void
do_paint_layer_labels (sqlite3 * sqlite, const void *data,
		       rl2PrivMapLayerPtr aux_lyr, rl2PrivMapConfigAuxPtr aux)
{
/* rendering a MapConfiguration Layer Labels */
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int ret;

    if (aux_lyr == NULL)
	return;
    if (aux_lyr->layer == NULL)
	return;

/* building the Map's Bounding Box */
    sql = "SELECT BuildMbr(?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, aux->min_x);
    sqlite3_bind_double (stmt, 2, aux->min_y);
    sqlite3_bind_double (stmt, 3, aux->max_x);
    sqlite3_bind_double (stmt, 4, aux->max_y);
    sqlite3_bind_int (stmt, 5, aux->srid);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      unsigned char *blob =
			  (unsigned char *) sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      ret = rl2_map_image_paint_labels (sqlite, data,
							aux->canvas_labels,
							aux_lyr->prefix,
							aux_lyr->layer->name,
							blob, blob_sz, 0,
							aux_lyr->style_name,
							(unsigned char *)
							aux_lyr->xml_style);
		  }
	    }
      }
    sqlite3_finalize (stmt);
}

static void
do_paint_layer_raster (sqlite3 * sqlite, const void *data,
		       rl2PrivMapLayerPtr aux_lyr, rl2PrivMapConfigAuxPtr aux)
{
/* rendering a MapConfiguration Raster Layer */
    sqlite3_stmt *stmt = NULL;
    const char *sql;
    int ret;

    if (aux_lyr == NULL)
	return;
    if (aux_lyr->layer == NULL)
	return;

/* building the Map's Bounding Box */
    sql = "SELECT BuildMbr(?, ?, ?, ?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	return;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, aux->min_x);
    sqlite3_bind_double (stmt, 2, aux->min_y);
    sqlite3_bind_double (stmt, 3, aux->max_x);
    sqlite3_bind_double (stmt, 4, aux->max_y);
    sqlite3_bind_int (stmt, 5, aux->srid);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      unsigned char *blob =
			  (unsigned char *) sqlite3_column_blob (stmt, 0);
		      int blob_sz = sqlite3_column_bytes (stmt, 0);
		      ret =
			  rl2_map_image_paint_from_raster (sqlite, data,
							   aux_lyr->canvas,
							   aux_lyr->prefix,
							   aux_lyr->layer->name,
							   blob, blob_sz,
							   aux_lyr->style_name,
							   (unsigned char *)
							   aux_lyr->xml_style,
							   aux_lyr->
							   syntetic_band);
		      if (ret == RL2_OK)
			  ret = rl2_graph_merge (aux->ctx, aux_lyr->ctx);
		  }
	    }
      }
    sqlite3_finalize (stmt);
}

static rl2TiledWmsPtr
do_create_tiled_wms (rl2MapLayerPtr layer, rl2MapWmsLayerStylePtr config,
		     const char *layer_name, int max_wms_retries, int wms_pause)
{
/* creating a TiledWms container */
    rl2TiledWmsPtr ptr = malloc (sizeof (rl2TiledWms));
    ptr->layer = layer;
    ptr->config = config;
    ptr->layer_name = layer_name;
    ptr->max_retries = max_wms_retries;
    ptr->pause = wms_pause;
    ptr->first = NULL;
    ptr->last = NULL;
    ptr->finished = 0;
    return ptr;
}

static void
do_destroy_tiles (rl2TiledWmsPtr tiles)
{
    rl2WmsTilePtr p;
    rl2WmsTilePtr p_n;

    p = tiles->first;
    while (p != NULL)
      {
	  p_n = p->next;
	  free (p);
	  p = p_n;
      }
    free (tiles);
}

static void
do_add_tile (rl2TiledWmsPtr tiles, int base_x, int base_y, int width,
	     int height, double min_x, double min_y, double max_x, double max_y)
{
/* adding a WMS tile request */
    rl2WmsTilePtr ptr = malloc (sizeof (rl2WmsTile));
    ptr->base_x = base_x;
    ptr->base_y = base_y;
    ptr->width = width;
    ptr->height = height;
    ptr->min_x = min_x;
    ptr->min_y = min_y;
    ptr->max_x = max_x;
    ptr->max_y = max_y;
    ptr->retries = tiles->max_retries;
    ptr->done = 0;
    ptr->next = NULL;
    if (tiles->first == NULL)
	tiles->first = ptr;
    if (tiles->last != NULL)
	tiles->last->next = ptr;
    tiles->last = ptr;
}

static void
do_eval_tiles (rl2TiledWmsPtr tiles)
{
/* evaluating if the list still contains unfinished elements */
    int finished = 1;
    rl2WmsTilePtr pT = tiles->first;
    while (pT != NULL)
      {
	  if (pT->done)
	    {
		/* skipping any tile already succesfully done */
		pT = pT->next;
		continue;
	    }
	  if (pT->retries <= 0)
	    {
		/* skipping any tile with too many retries */
		pT = pT->next;
		continue;
	    }
	  finished = 0;
	  pT = pT->next;
      }
    if (finished)
	tiles->finished = 1;
}

static void
do_process_tiled_request (const void *data, rl2GraphicsContextPtr ctx_out,
			  rl2TiledWmsPtr tiles)
{
/* processing a WMS tiled request */
    unsigned char *rgba;
    while (tiles->finished != 1)
      {
	  /* starting a retry loop */
	  rl2WmsTilePtr pT = tiles->first;
	  while (pT != NULL)
	    {
		if (pT->done)
		  {
		      /* skipping any tile already succesfully done */
		      pT = pT->next;
		      continue;
		  }
		if (pT->retries <= 0)
		  {
		      /* skipping any tile with too many retries */
		      pT = pT->next;
		      continue;
		  }
		rgba =
		    do_wms_GetMap_get (NULL,
				       tiles->config->get_map_url,
				       tiles->config->http_proxy,
				       tiles->config->wms_protocol,
				       tiles->layer_name, tiles->config->crs,
				       tiles->config->swap_xy, pT->min_x,
				       pT->min_y, pT->max_x, pT->max_y,
				       pT->width, pT->height,
				       tiles->config->style,
				       tiles->config->image_format,
				       tiles->config->opaque, 0);
		if (rgba != NULL)
		  {
		      /* ok, succesfull WMS call */
		      rl2GraphicsContextPtr ctx_in =
			  rl2_graph_create_context_rgba (data, pT->width,
							 pT->height, rgba);
		      if (ctx_out != NULL && ctx_in != NULL)
			  rl2_copy_wms_tile (ctx_out, ctx_in, pT->base_x,
					     pT->base_y);
		      pT->done = 1;
		      if (ctx_in)
			  rl2_graph_destroy_context (ctx_in);
		      free (rgba);
		  }
		pT->retries -= 1;
		pT = pT->next;
	    }
	  do_eval_tiles (tiles);
	  if (tiles->finished == 0)
#ifdef _WIN32
	      Sleep (tiles->pause);
#else
	      usleep (tiles->pause * 1000);
#endif
      }
}

static void
do_paint_layer_wms (const void *data, rl2PrivMapLayerPtr aux_lyr,
		    rl2PrivMapConfigAuxPtr aux)
{
/* rendering a MapConfiguration WMS Layer */
    rl2MapLayerPtr layer;
    rl2MapWmsLayerStylePtr config;
    unsigned char *rgba;
    int visible = 1;

    if (aux_lyr == NULL)
	return;
    layer = aux_lyr->layer;
    if (layer == NULL)
	return;
    config = layer->wms_style;
    if (config == NULL)
	return;

/* testing for conditional visibility depending on current Scale */
    if (layer->ok_min_scale)
      {
	  if (aux->scale < layer->min_scale)
	      visible = 0;
      }
    if (layer->ok_max_scale)
      {
	  if (aux->scale > layer->max_scale)
	      visible = 0;
      }
    if (!visible)		/* layer is not currently visible */
	return;

    if (config->is_tiled)
      {
	  /* checking if a tiled request is required */
	  int requires_tiling = 0;
	  int n_htiles;
	  int n_vtiles;
	  int htile_sz;
	  int vtile_sz;
	  if (aux->width > config->tile_width)
	    {
		n_htiles = aux->width / config->tile_width;
		if ((n_htiles * config->tile_width) < aux->width)
		    n_htiles += 1;
		htile_sz = aux->width / n_htiles;
		if (htile_sz * n_htiles < aux->width)
		    htile_sz += 1;
		requires_tiling = 1;
	    }
	  else
	    {
		n_htiles = 1;
		htile_sz = aux->width;
	    }
	  if (aux->height > config->tile_height)
	    {
		n_vtiles = aux->height / config->tile_height;
		if ((n_vtiles * config->tile_height) < aux->height)
		    n_vtiles += 1;
		vtile_sz = aux->height / n_vtiles;
		if (vtile_sz * n_vtiles < aux->height)
		    vtile_sz += 1;
		requires_tiling = 1;
	    }
	  else
	    {
		n_vtiles = 1;
		vtile_sz = aux->height;
	    }
	  if (requires_tiling)
	    {
		/* preparing a tiled request */
		int x;
		int y;
		int base_x;
		int base_y = aux->height - 1;
		int width;
		int height;
		int remainder_x;
		int remainder_y = aux->height;
		double min_x;
		double max_x;
		double min_y = aux->min_y;
		double max_y;
		rl2TiledWmsPtr tiles =
		    do_create_tiled_wms (layer, config, layer->name,
					 aux->map_config->max_wms_retries,
					 aux->map_config->wms_pause);
		for (y = 0; y < n_vtiles; y++)
		  {
		      base_x = 0;
		      remainder_x = aux->width;
		      if (remainder_y < vtile_sz)
			  height = remainder_y;
		      else
			  height = vtile_sz;
		      remainder_y -= height;
		      min_x = aux->min_x;
		      max_y = min_y + ((double) height * aux->pixel_ratio);
		      for (x = 0; x < n_htiles; x++)
			{
			    if (remainder_x < htile_sz)
				width = remainder_x;
			    else
				width = htile_sz;
			    remainder_x -= width;
			    max_x = min_x + ((double) width * aux->pixel_ratio);
			    do_add_tile (tiles, base_x, base_y - height, width,
					 height, min_x, min_y, max_x, max_y);
			    base_x += width;
			    min_x = max_x;
			}
		      base_y -= height;
		      min_y = max_y;
		  }
		/* processing a tiled request */
		do_process_tiled_request (data, aux->ctx, tiles);
		/* memory cleanup: freeing the tiled requests list */
		do_destroy_tiles (tiles);
		return;
	    }
      }

    rgba =
	do_wms_GetMap_get (NULL, config->get_map_url, config->http_proxy,
			   config->wms_protocol, layer->name, config->crs,
			   config->swap_xy, aux->min_x, aux->min_y, aux->max_x,
			   aux->max_y, aux->width, aux->height,
			   config->style, config->image_format,
			   config->opaque, 0);
    if (rgba != NULL)
      {
	  rl2GraphicsContextPtr ctx_in =
	      rl2_graph_create_context_rgba (data, aux->width, aux->height,
					     rgba);
	  if (aux->ctx != NULL && ctx_in != NULL)
	      rl2_graph_merge (aux->ctx, ctx_in);
	  if (ctx_in)
	      rl2_graph_destroy_context (ctx_in);
	  free (rgba);
      }
}

static void
check_map_labels (sqlite3 * sqlite, rl2PrivMapLayerPtr lyr)
{
/* testing for Layers with Map Labels */
    rl2MapLayerPtr layer = lyr->layer;
    rl2MapVectorLayerStylePtr style = layer->vector_style;
    lyr->has_labels = 0;
    if (style != NULL)
      {
	  /* quick style */
	  if (style->text_sym != NULL)
	      lyr->has_labels = 1;
      }
    else if (layer->vector_style_internal_name != NULL)
      {
	  /* internal style name */
	  rl2FeatureTypeStylePtr lyr_stl =
	      rl2_create_feature_type_style_from_dbms (sqlite, layer->prefix,
						       layer->name,
						       layer->vector_style_internal_name);
	  if (lyr_stl != NULL)
	    {
		lyr->has_labels = rl2_style_has_labels (lyr_stl);
		rl2_destroy_feature_type_style (lyr_stl);
	    }
      }
}

RL2_PRIVATE int
rl2_paint_map_config_aux (sqlite3 * sqlite, const void *data,
			  rl2PrivMapConfigAuxPtr aux)
{
/* rendering a full MapConfiguration */
    rl2PrivMapLayerPtr aux_lyr;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    int half_transparent;
    unsigned char *image = NULL;
    int image_size;

    if (aux == NULL)
	return RL2_ERROR;

    aux->has_labels = 0;
    aux->update_labels = 0;
    aux_lyr = aux->first_lyr;
    while (aux_lyr != NULL)
      {
	  /* testing for Map Labels */
	  switch (aux_lyr->layer->type)
	    {
	    case RL2_MAP_LAYER_VECTOR:
	    case RL2_MAP_LAYER_VECTOR_VIEW:
	    case RL2_MAP_LAYER_VECTOR_VIRTUAL:
	    case RL2_MAP_LAYER_TOPOLOGY:
	    case RL2_MAP_LAYER_NETWORK:
		check_map_labels (sqlite, aux_lyr);
		if (aux_lyr->has_labels)
		    aux->has_labels = 1;
		break;
	    default:
		break;
	    };
	  aux_lyr = aux_lyr->next;
      }

    aux_lyr = aux->first_lyr;
    while (aux_lyr != NULL)
      {
	  /* painting Layers */
	  if (aux_lyr->valid)
	    {
		switch (aux_lyr->layer->type)
		  {
		  case RL2_MAP_LAYER_WMS:
		      do_paint_layer_wms (data, aux_lyr, aux);
		      break;
		  case RL2_MAP_LAYER_RASTER:
		      do_paint_layer_raster (sqlite, data, aux_lyr, aux);
		      break;
		  case RL2_MAP_LAYER_VECTOR:
		  case RL2_MAP_LAYER_VECTOR_VIEW:
		  case RL2_MAP_LAYER_VECTOR_VIRTUAL:
		  case RL2_MAP_LAYER_TOPOLOGY:
		  case RL2_MAP_LAYER_NETWORK:
		      do_paint_layer_vector (sqlite, data, aux_lyr, aux);
		      if (aux_lyr->has_labels)
			  aux->update_labels = 1;
		      break;
		  default:
		      break;
		  };
	    }
	  aux_lyr = aux_lyr->next;
      }

    if (aux->has_labels && aux->update_labels)
      {
	  /* creating a Canvas for Map Labels */
	  aux->ctx_labels =
	      rl2_graph_create_context (data, aux->width, aux->height);
	  if (aux->ctx_labels != NULL)
	    {
		rl2_prime_background (aux->ctx_labels, 0, 0, 0, 0);	/* transparent background */
		aux->canvas_labels = rl2_create_raster_canvas (aux->ctx_labels);
	    }
	  if (aux->canvas_labels != NULL)
	    {
		aux_lyr = aux->first_lyr;
		while (aux_lyr != NULL)
		  {
		      /* painting Layers with Map Labels */
		      if (aux_lyr->valid && aux_lyr->has_labels)
			{
			    switch (aux_lyr->layer->type)
			      {
			      case RL2_MAP_LAYER_VECTOR:
			      case RL2_MAP_LAYER_VECTOR_VIEW:
			      case RL2_MAP_LAYER_VECTOR_VIRTUAL:
			      case RL2_MAP_LAYER_TOPOLOGY:
			      case RL2_MAP_LAYER_NETWORK:
				  do_paint_layer_labels (sqlite, data, aux_lyr,
							 aux);
				  break;
			      default:
				  break;
			      };
			}
		      aux_lyr = aux_lyr->next;
		  }
		rl2_graph_merge (aux->ctx, aux->ctx_labels);
	    }
      }

/* preparing the full rendered Map Image */
    if (aux->format_id == RL2_OUTPUT_FORMAT_CAIRO_RGBA)
      {
	  /* Cairo RGBA raw pixels */
	  if (!rl2_graph_get_context_data (aux->ctx, &image, &image_size))
	      goto error;
	  goto done;
      }

    rgb = rl2_graph_get_context_rgb_array (aux->ctx);
    alpha = rl2_graph_get_context_alpha_array (aux->ctx, &half_transparent);
    if (rgb == NULL || alpha == NULL)
	goto error;
    if (!get_payload_from_rgb_rgba_transparent
	(aux->width, aux->height, data, rgb, alpha, aux->format_id,
	 aux->quality, &image, &image_size, 1.0, half_transparent))
	goto error;

  done:
    free (rgb);
    free (alpha);
    aux->image = image;
    aux->image_size = image_size;
    return RL2_OK;

  error:
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    return RL2_ERROR;
}

static void
do_get_uuid (char *uuid)
{
/* creating an UUID for Quick Styles */
    unsigned char rnd[16];
    char *p = uuid;
    int i;
    sqlite3_randomness (16, rnd);
    for (i = 0; i < 16; i++)
      {
	  if (i == 4 || i == 6 || i == 8 || i == 10)
	      *p++ = '-';
	  sprintf (p, "%02x", rnd[i]);
	  p += 2;
      }
    *p = '\0';
    uuid[14] = '4';
    uuid[19] = '8';
}

static void
do_set_style_name (rl2PrivMapLayerPtr lyr, const char *name)
{
/* setting the Style Name */
    int len;
    if (lyr->style_name != NULL)
      {
	  free (lyr->style_name);
	  lyr->style_name = NULL;
      }
    if (name == NULL)
	return;
    len = strlen (name);
    lyr->style_name = malloc (len + 1);
    strcpy (lyr->style_name, name);
}

static char *
do_create_point_symbolizer (const char *style_internal_name,
			    rl2PrivMapLayerPtr lyr,
			    rl2MapPointSymbolizerPtr sym, int subordered)
{
/* creating a Point Symbolizer */
    char *xml;
    char *prev;
    const char *cstr;
    char uuid[64];
    if (sym->mark == NULL)
	return NULL;

    if (style_internal_name != NULL)
	do_set_style_name (lyr, style_internal_name);
    else
      {
	  do_get_uuid (uuid);
	  do_set_style_name (lyr, uuid);
      }
    if (subordered)
      {
	  xml = sqlite3_mprintf ("<PointSymbolizer>");
	  prev = xml;
      }
    else
      {
	  xml = sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<PointSymbolizer version=\"1.1.0\" ", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%sxsi:schemaLocation=\"http://www.opengis.net/se http://schemas.opengis.net/se/1.1.0/Symbolizer.xsd\" ",
	       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%sxmlns=\"http://www.opengis.net/se\" xmlns:ogc=\"http://www.opengis.net/ogc\" ",
	       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%sxmlns:xlink=\"http://www.w3.org/1999/xlink\" ", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  cstr = "http://www.opengeospatial.org/se/units/pixel";
	  xml =
	      sqlite3_mprintf
	      ("%sxmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" uom=\"%s\">",
	       prev, cstr);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Name>%s</Name>", prev, uuid);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Description>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<Title>%s</Title>", prev,
			       "Quick Style - PointSymbolizer");
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<Abstract>%s</Abstract>", prev,
			       "Created by SpatialiteGUI");
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Description>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s<Graphic>", prev);
    sqlite3_free (prev);
    prev = xml;
/* mark symbol */
    xml = sqlite3_mprintf ("%s<Mark>", prev);
    sqlite3_free (prev);
    prev = xml;
    switch (sym->mark->type)
      {
      case RL2_GRAPHIC_MARK_CIRCLE:
	  cstr = "circle";
	  break;
      case RL2_GRAPHIC_MARK_TRIANGLE:
	  cstr = "triangle";
	  break;
      case RL2_GRAPHIC_MARK_STAR:
	  cstr = "star";
	  break;
      case RL2_GRAPHIC_MARK_CROSS:
	  cstr = "cross";
	  break;
      case RL2_GRAPHIC_MARK_X:
	  cstr = "x";
	  break;
      default:
	  cstr = "square";
	  break;
      };
    xml = sqlite3_mprintf ("%s<WellKnownName>%s</WellKnownName>", prev, cstr);
    sqlite3_free (prev);
    prev = xml;
/* Mark Fill */
    xml = sqlite3_mprintf ("%s<Fill>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"fill\">#%02x%02x%02x</SvgParameter>", prev,
	 sym->mark->fill->red, sym->mark->fill->green, sym->mark->fill->blue);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Fill>", prev);
    sqlite3_free (prev);
    prev = xml;
/* Mark Stroke */
    xml = sqlite3_mprintf ("%s<Stroke>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke\">#%02x%02x%02x</SvgParameter>", prev,
	 sym->mark->stroke->red, sym->mark->stroke->green,
	 sym->mark->stroke->blue);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-width\">%1.2f</SvgParameter>", prev,
	 1.0);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-linejoin\">round</SvgParameter>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-linecap\">round</SvgParameter>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Stroke>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Mark>", prev);
    sqlite3_free (prev);
    prev = xml;
    if (sym->opacity != 1.0)
      {
	  xml =
	      sqlite3_mprintf ("%s<Opacity>%1.2f</Opacity>", prev,
			       sym->opacity);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s<Size>%1.2f</Size>", prev, sym->size);
    sqlite3_free (prev);
    prev = xml;
    if (sym->rotation != 0.0)
      {
	  xml =
	      sqlite3_mprintf ("%s<Rotation>%1.2f</Rotation>", prev,
			       sym->rotation);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (sym->anchor_x != 0.5 || sym->anchor_y != 0.5)
      {
	  xml = sqlite3_mprintf ("%s<AnchorPoint>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<AnchorPointX>%1.4f</AnchorPointX>", prev,
			       sym->anchor_x);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<AnchorPointY>%1.4f</AnchorPointY>", prev,
			       sym->anchor_y);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</AnchorPoint>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (sym->displacement_x != 0.0 || sym->displacement_y != 0.0)
      {
	  xml = sqlite3_mprintf ("%s<Displacement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<DisplacementX>%1.4f</DisplacementX>", prev,
			       sym->displacement_x);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<DisplacementY>%1.4f</DisplacementY>", prev,
			       sym->displacement_y);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Displacement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s</Graphic>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</PointSymbolizer>", prev);
    sqlite3_free (prev);

    return xml;
}

static char *
do_create_line_symbolizer (const char *style_internal_name,
			   rl2PrivMapLayerPtr lyr, rl2MapLineSymbolizerPtr sym,
			   int subordered)
{
/* creating a Line Symbolizer */
    char *xml;
    char *prev;
    const char *cstr;
    char uuid[64];
    const char *dashArray = NULL;

    if (style_internal_name != NULL)
	do_set_style_name (lyr, style_internal_name);
    else
      {
	  do_get_uuid (uuid);
	  do_set_style_name (lyr, uuid);
      }
    if (subordered)
      {
	  xml = sqlite3_mprintf ("<LineSymbolizer>");
	  prev = xml;
      }
    else
      {
	  xml = sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	  prev = xml;
	  xml = sqlite3_mprintf ("<LineSymbolizer version=\"1.1.0\" ", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%sxsi:schemaLocation=\"http://www.opengis.net/se http://schemas.opengis.net/se/1.1.0/Symbolizer.xsd\" ",
	       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%sxmlns=\"http://www.opengis.net/se\" xmlns:ogc=\"http://www.opengis.net/ogc\" ",
	       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%sxmlns:xlink=\"http://www.w3.org/1999/xlink\" ", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  cstr = "http://www.opengeospatial.org/se/units/pixel";
	  xml =
	      sqlite3_mprintf
	      ("%sxmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" uom=\"%s\">",
	       prev, cstr);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Name>%s</Name>", prev, uuid);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Description>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<Title>%s</Title>", prev,
			       "Quick Style - LineSymbolizer");
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<Abstract>%s</Abstract>", prev,
			       "Created by SpatialiteGUI");
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Description>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s<Stroke>", prev);
    sqlite3_free (prev);
    prev = xml;
/* using a Solid Color */
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke\">#%02x%02x%02x</SvgParameter>", prev,
	 sym->stroke->red, sym->stroke->green, sym->stroke->blue);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-opacity\">%1.2f</SvgParameter>", prev,
	 sym->stroke->opacity);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-width\">%1.2f</SvgParameter>", prev,
	 sym->stroke->width);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-linejoin\">round</SvgParameter>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-linecap\">round</SvgParameter>", prev);
    sqlite3_free (prev);
    prev = xml;
    switch (sym->stroke->dot_style)
      {
      case EXT_QUICK_STYLE_DOT_LINE:
	  dashArray = "5.0, 10.0";
	  break;
      case EXT_QUICK_STYLE_DASH_LINE:
	  dashArray = "20.0, 20.0";
	  break;
      case EXT_QUICK_STYLE_DASH_DOT_LINE:
	  dashArray = "20.0, 10.0, 5.0, 10.0";
	  break;
      default:
	  dashArray = NULL;
	  break;
      };
    if (dashArray != NULL)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"stroke-dasharray\">%s</SvgParameter>",
	       prev, dashArray);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s</Stroke>", prev);
    sqlite3_free (prev);
    prev = xml;
    if (sym->perpendicular_offset != 0.0)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<PerpendicularOffset>%1.2f</PerpendicularOffset>", prev,
	       sym->perpendicular_offset);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s</LineSymbolizer>", prev);
    sqlite3_free (prev);

    return xml;
}

static char *
do_create_polygon_symbolizer (const char *style_internal_name,
			      rl2PrivMapLayerPtr lyr,
			      rl2MapPolygonSymbolizerPtr sym, int subordered)
{
/* creating a Polygon Symbolizer */
    char *xml;
    char *prev;
    const char *cstr;
    char uuid[64];

    if (style_internal_name != NULL)
	do_set_style_name (lyr, style_internal_name);
    else
      {
	  do_get_uuid (uuid);
	  do_set_style_name (lyr, uuid);
      }
    if (subordered)
      {
	  xml = sqlite3_mprintf ("<PolygonSymbolizer>");
	  prev = xml;
      }
    else
      {
	  xml = sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<PolygonSymbolizer version=\"1.1.0\" ", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%sxsi:schemaLocation=\"http://www.opengis.net/se http://schemas.opengis.net/se/1.1.0/Symbolizer.xsd\" ",
	       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%sxmlns=\"http://www.opengis.net/se\" xmlns:ogc=\"http://www.opengis.net/ogc\" ",
	       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%sxmlns:xlink=\"http://www.w3.org/1999/xlink\" ", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  cstr = "http://www.opengeospatial.org/se/units/pixel";
	  xml =
	      sqlite3_mprintf
	      ("%sxmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" uom=\"%s\">",
	       prev, cstr);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Name>%s</Name>", prev, uuid);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Description>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<Title>%s</Title>", prev,
			       "Quick Style - Polygon Symbolizer");
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<Abstract>%s</Abstract>", prev,
			       "Created by SpatialiteGUI");
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Description>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (sym->fill != NULL)
      {
	  /* Polygon Fill */
	  xml = sqlite3_mprintf ("%s<Fill>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (sym->fill->graphic == NULL)
	    {
		/* using a Solid Color */
		xml =
		    sqlite3_mprintf
		    ("%s<SvgParameter name=\"fill\">#%02x%02x%02x</SvgParameter>",
		     prev, sym->fill->red, sym->fill->green, sym->fill->blue);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s<SvgParameter name=\"fill-opacity\">%1.2f</SvgParameter>",
		     prev, sym->fill->opacity);
		sqlite3_free (prev);
		prev = xml;
	    }
	  else
	    {
		/* using a pattern brush */
		xml = sqlite3_mprintf ("%s<GraphicFill>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s<Graphic>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s<ExternalGraphic>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s<OnlineResource xlink:type=\"simple\" xlink:href=\"%s\" />",
		     prev, sym->fill->graphic->resource);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s<Format>image/png</Format>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s<ColorReplacement>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s<Recode fallbackValue=\"#000000\">",
				     prev);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s<LookupValue>ExternalGraphic</LookupValue>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s<MapItem>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s<Data>1</Data>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s<Value>#%02x%02x%02x</Value>", prev,
				     sym->fill->graphic->color->red,
				     sym->fill->graphic->color->green,
				     sym->fill->graphic->color->blue);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s</MapItem>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s</Recode>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s</ColorReplacement>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s</ExternalGraphic>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s</Graphic>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s</GraphicFill>", prev);
		sqlite3_free (prev);
		prev = xml;
	    }
	  xml = sqlite3_mprintf ("%s</Fill>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (sym->stroke != NULL)
      {
	  /* Polygon Stroke */
	  xml = sqlite3_mprintf ("%s<Stroke>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  /* using a Solid Color */
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"stroke\">#%02x%02x%02x</SvgParameter>",
	       prev, sym->stroke->red, sym->stroke->green, sym->stroke->blue);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"stroke-opacity\">%1.2f</SvgParameter>",
	       prev, sym->stroke->opacity);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"stroke-width\">%1.2f</SvgParameter>",
	       prev, sym->stroke->width);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"stroke-linejoin\">round</SvgParameter>",
	       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"stroke-linecap\">round</SvgParameter>",
	       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Stroke>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (sym->displacement_x != 0.0 || sym->displacement_y != 0.0)
      {
	  xml = sqlite3_mprintf ("%s<Displacement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<DisplacementX>%1.4f</DisplacementX>", prev,
			       sym->displacement_x);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<DisplacementY>%1.4f</DisplacementY>", prev,
			       sym->displacement_y);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Displacement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (sym->perpendicular_offset != 0.0)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<PerpendicularOffset>%1.4f</PerpendicularOffset>", prev,
	       sym->perpendicular_offset);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s</PolygonSymbolizer>", prev);
    sqlite3_free (prev);

    return xml;
}

static char *
do_create_text_symbolizer (const char *style_internal_name,
			   rl2PrivMapLayerPtr lyr, rl2MapTextSymbolizerPtr sym,
			   int subordered)
{
/* creating a Text Symbolizer */
    char *xml;
    char *prev;
    const char *cstr;
    char uuid[64];
    char *cleared;

    if (style_internal_name != NULL)
	do_set_style_name (lyr, style_internal_name);
    else
      {
	  do_get_uuid (uuid);
	  do_set_style_name (lyr, uuid);
      }
    if (subordered)
      {
	  xml = sqlite3_mprintf ("<TextSymbolizer>");
	  prev = xml;
      }
    else
      {
	  xml = sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<TextSymbolizer version=\"1.1.0\" ", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%sxsi:schemaLocation=\"http://www.opengis.net/se http://schemas.opengis.net/se/1.1.0/Symbolizer.xsd\" ",
	       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%sxmlns=\"http://www.opengis.net/se\" xmlns:ogc=\"http://www.opengis.net/ogc\" ",
	       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%sxmlns:xlink=\"http://www.w3.org/1999/xlink\" ", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  cstr = "http://www.opengeospatial.org/se/units/pixel";
	  xml =
	      sqlite3_mprintf
	      ("%sxmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" uom=\"%s\">",
	       prev, cstr);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Name>%s</Name>", prev, uuid);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Description>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<Title>%s</Title>", prev,
			       "Quick Style - Text Symbolizer");
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<Abstract>%s</Abstract>", prev,
			       "Created by SpatialiteGUI");
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Description>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    cleared = rl2_clean_xml (sym->label);
    xml = sqlite3_mprintf ("%s<Label>%s</Label>", prev, cleared);
    free (cleared);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Font>", prev);
    sqlite3_free (prev);
    prev = xml;
    cleared = rl2_clean_xml (sym->font->family);
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"font-family\">%s</SvgParameter>", prev,
	 cleared);
    free (cleared);
    sqlite3_free (prev);
    prev = xml;
    if (sym->font->style == RL2_FONTSTYLE_ITALIC)
	xml =
	    sqlite3_mprintf
	    ("%s<SvgParameter name=\"font-style\">italic</SvgParameter>", prev);
    else if (sym->font->style == RL2_FONTSTYLE_OBLIQUE)
	xml =
	    sqlite3_mprintf
	    ("%s<SvgParameter name=\"font-style\">oblique</SvgParameter>",
	     prev);
    else
	xml =
	    sqlite3_mprintf
	    ("%s<SvgParameter name=\"font-style\">normal</SvgParameter>", prev);
    sqlite3_free (prev);
    prev = xml;
    if (sym->font->weight == RL2_FONTWEIGHT_BOLD)
	xml =
	    sqlite3_mprintf
	    ("%s<SvgParameter name=\"font-weight\">bold</SvgParameter>", prev);
    else
	xml =
	    sqlite3_mprintf
	    ("%s<SvgParameter name=\"font-weight\">normal</SvgParameter>",
	     prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"font-size\">%1.2f</SvgParameter>", prev,
	 sym->font->size);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Font>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<LabelPlacement>", prev);
    sqlite3_free (prev);
    prev = xml;

    if (sym->placement->point != NULL)
      {
	  /* Text Point Symbolizer */
	  xml = sqlite3_mprintf ("%s<PointPlacement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (sym->placement->point->anchor_x != 0.5
	      || sym->placement->point->anchor_y != 0.5)
	    {
		xml = sqlite3_mprintf ("%s<AnchorPoint>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s<AnchorPointX>%1.4f</AnchorPointX>",
				     prev, sym->placement->point->anchor_x);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s<AnchorPointY>%1.4f</AnchorPointY>",
				     prev, sym->placement->point->anchor_y);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s</AnchorPoint>", prev);
		sqlite3_free (prev);
		prev = xml;
	    }
	  if (sym->placement->point->displacement_x != 0.0
	      || sym->placement->point->displacement_y != 0.0)
	    {
		xml = sqlite3_mprintf ("%s<Displacement>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s<DisplacementX>%1.4f</DisplacementX>",
				     prev,
				     sym->placement->point->displacement_x);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s<DisplacementY>%1.4f</DisplacementY>",
				     prev,
				     sym->placement->point->displacement_y);
		sqlite3_free (prev);
		prev = xml;
		xml = sqlite3_mprintf ("%s</Displacement>", prev);
		sqlite3_free (prev);
		prev = xml;
	    }
	  if (sym->placement->point->rotation != 0.0)
	    {
		xml =
		    sqlite3_mprintf ("%s<Rotation>%1.2f</Rotation>", prev,
				     sym->placement->point->rotation);
		sqlite3_free (prev);
		prev = xml;
	    }
	  xml = sqlite3_mprintf ("%s</PointPlacement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (sym->placement->line != NULL)
      {
	  /* Text Line Symbolizer */
	  xml = sqlite3_mprintf ("%s<LinePlacement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (sym->placement->line->perpendicular_offset != 0.0)
	    {
		xml =
		    sqlite3_mprintf
		    ("%s<PerpendicularOffset>%1.4f</PerpendicularOffset>", prev,
		     sym->placement->line->perpendicular_offset);
		sqlite3_free (prev);
		prev = xml;
	    }
	  if (sym->placement->line->repeated)
	    {
		/* Repeated: InitialGap and Gap */
		xml = sqlite3_mprintf ("%s<IsRepeated>true</IsRepeated>", prev);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s<InitialGap>%1.4f</InitialGap>", prev,
				     sym->placement->line->initial_gap);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s<Gap>%1.4f</Gap>", prev,
				     sym->placement->line->gap);
		sqlite3_free (prev);
		prev = xml;
	    }
	  if (sym->placement->line->aligned)
	    {
		xml = sqlite3_mprintf ("%s<IsAligned>true</IsAligned>", prev);
		sqlite3_free (prev);
		prev = xml;
	    }
	  if (sym->placement->line->generalize)
	    {
		xml =
		    sqlite3_mprintf ("%s<GeneralizeLine>true</GeneralizeLine>",
				     prev);
		sqlite3_free (prev);
		prev = xml;
	    }
	  xml = sqlite3_mprintf ("%s</LinePlacement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }

    xml = sqlite3_mprintf ("%s</LabelPlacement>", prev);
    sqlite3_free (prev);
    prev = xml;
    if (sym->halo != NULL)
      {
	  /* Halo */
	  xml = sqlite3_mprintf ("%s<Halo>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<Radius>%1.2f</Radius>", prev,
			       sym->halo->radius);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Fill>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"fill\">#%02x%02x%02x</SvgParameter>",
	       prev, sym->halo->fill->red, sym->halo->fill->green,
	       sym->halo->fill->blue);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"fill-opacity\">%1.2f</SvgParameter>",
	       prev, sym->halo->fill->opacity);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Fill>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Halo>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s<Fill>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"fill\">#%02x%02x%02x</SvgParameter>", prev,
	 sym->fill->red, sym->fill->green, sym->fill->blue);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"fill-opacity\">%1.2f</SvgParameter>", prev,
	 sym->fill->opacity);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Fill>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</TextSymbolizer>", prev);
    sqlite3_free (prev);

    return xml;
}

static void
do_create_vector_symbolizer (rl2PrivMapLayerPtr lyr)
{
/* creating a simple XML Symbolizer */
    rl2MapLayerPtr layer = lyr->layer;
    if (layer->vector_style->point_sym != NULL)
	lyr->xml_style =
	    do_create_point_symbolizer (layer->vector_style_internal_name, lyr,
					layer->vector_style->point_sym, 0);
    else if (layer->vector_style->first_line_sym != NULL)
	lyr->xml_style =
	    do_create_line_symbolizer (layer->vector_style_internal_name, lyr,
				       layer->vector_style->first_line_sym, 0);
    else if (layer->vector_style->polygon_sym != NULL)
	lyr->xml_style =
	    do_create_polygon_symbolizer (layer->vector_style_internal_name,
					  lyr, layer->vector_style->polygon_sym,
					  0);
    else if (layer->vector_style->text_sym != NULL)
	lyr->xml_style =
	    do_create_text_symbolizer (layer->vector_style_internal_name, lyr,
				       layer->vector_style->text_sym, 0);
}

static void
do_create_vector_symbolizer_xml (rl2PrivMapLayerPtr lyr)
{
/* creating a simple XML Vector Symbolizer */
    rl2MapLayerPtr layer = lyr->layer;
    if (layer->vector_style != NULL)
	do_create_vector_symbolizer (lyr);
}

static void
do_create_feature_type_xml (rl2PrivMapLayerPtr lyr)
{
/* creating a FeatureType XML Style */
    rl2MapLayerPtr layer;
    rl2MapVectorLayerStylePtr vector;
    char *xml;
    char *xml2;
    char *prev;
    char uuid[64];

    layer = lyr->layer;
    if (layer == NULL)
	return;

    if (layer->vector_style_internal_name != NULL)
	do_set_style_name (lyr, layer->vector_style_internal_name);
    else
      {
	  do_get_uuid (uuid);
	  do_set_style_name (lyr, uuid);
      }
    xml = sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<FeatureTypeStyle version=\"1.1.0\" ", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%sxsi:schemaLocation=\"http://www.opengis.net/se http://schemas.opengis.net/se/1.1.0/FeatureStyle.xsd\" ",
	 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%sxmlns=\"http://www.opengis.net/se\" xmlns:ogc=\"http://www.opengis.net/ogc\" ",
	 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%sxmlns:xlink=\"http://www.w3.org/1999/xlink\" ",
			 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%sxmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Name>%s</Name>", prev, uuid);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Description>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Title>%s</Title>", prev, "Quick Style");
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<Abstract>%s</Abstract>", prev,
			 "Created by SpatialiteGUI");
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Description>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Rule>", prev);
    sqlite3_free (prev);
    prev = xml;
    if (layer->ok_min_scale)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<MinScaleDenominator>%1.2f</MinScaleDenominator>", prev,
	       layer->min_scale);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (layer->ok_max_scale)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<MaxScaleDenominator>%1.2f</MaxScaleDenominator>", prev,
	       layer->max_scale);
	  sqlite3_free (prev);
	  prev = xml;
      }

    vector = layer->vector_style;
    if (vector != NULL)
      {
	  /* Vector Symbolizers */
	  int text_alone = 0;
	  if (vector->text_sym != NULL && vector->text_alone)
	      text_alone = 1;
	  if (text_alone)
	      goto skip_geometry;
	  if (vector->point_sym != NULL)
	    {
		/* Point Symbolizer */
		xml2 =
		    do_create_point_symbolizer
		    (layer->vector_style_internal_name, lyr, vector->point_sym,
		     1);
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		      prev = xml;
		  }
	    }
	  if (vector->first_line_sym != NULL)
	    {
		/* Line Symbolizers */
		rl2MapLineSymbolizerPtr line = vector->first_line_sym;
		while (line != NULL)
		  {
		      xml2 =
			  do_create_line_symbolizer
			  (layer->vector_style_internal_name, lyr, line, 1);
		      if (xml2 != NULL)
			{
			    xml = sqlite3_mprintf ("%s%s", prev, xml2);
			    sqlite3_free (prev);
			    sqlite3_free (xml2);
			    prev = xml;
			}
		      line = line->next;
		  }
	    }
	  if (vector->polygon_sym != NULL)
	    {
		/* Polygon Symbolizer */
		xml2 =
		    do_create_polygon_symbolizer
		    (layer->vector_style_internal_name, lyr,
		     vector->polygon_sym, 1);
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		      prev = xml;
		  }
	    }
	skip_geometry:
	  if (vector->text_sym != NULL)
	    {
		/* Text Symbolizer */
		xml2 =
		    do_create_text_symbolizer
		    (layer->vector_style_internal_name, lyr, vector->text_sym,
		     1);
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		      prev = xml;
		  }
	    }
      }
    xml = sqlite3_mprintf ("%s</Rule></FeatureTypeStyle>", prev);
    sqlite3_free (prev);

    lyr->xml_style = xml;
}

static char *
do_create_face_xml (rl2PrivMapLayerPtr lyr)
{
/* creating an XML Style for Faces */
    rl2MapLayerPtr layer = lyr->layer;
    rl2MapPolygonSymbolizerPtr sym = layer->topology_style->faces_sym;
    char *xml;
    char *prev;

    xml = sqlite3_mprintf ("<Rule>");
    prev = xml;
    if (layer->ok_min_scale)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<MinScaleDenominator>%1.2f</MinScaleDenominator>", prev,
	       layer->min_scale);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (layer->ok_max_scale)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<MaxScaleDenominator>%1.2f</MaxScaleDenominator>", prev,
	       layer->max_scale);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s<Name>Face</Name>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<ogc:Filter><ogc:PropertyIsEqualTo>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<ogc:PropertyName>topoclass</ogc:PropertyName>",
			 prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<ogc:Literal>face</ogc:Literal>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ogc:PropertyIsEqualTo></ogc:Filter>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<PolygonSymbolizer>", prev);
    sqlite3_free (prev);
    prev = xml;
    if (sym->fill != NULL)
      {
	  /* Polygon Fill */
	  xml = sqlite3_mprintf ("%s<Fill>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  /* using a Solid Color */
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"fill\">#%02x%02x%02x</SvgParameter>",
	       prev, sym->fill->red, sym->fill->green, sym->fill->blue);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"fill-opacity\">%1.2f</SvgParameter>",
	       prev, sym->fill->opacity);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Fill>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (sym->stroke != NULL)
      {
	  /* Polygon Stroke */
	  xml = sqlite3_mprintf ("%s<Stroke>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  /* using a Solid Color */
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"stroke\">#%02x%02x%02x</SvgParameter>",
	       prev, sym->stroke->red, sym->stroke->green, sym->stroke->blue);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"stroke-opacity\">%1.2f</SvgParameter>",
	       prev, sym->stroke->opacity);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"stroke-width\">%1.2f</SvgParameter>",
	       prev, sym->stroke->width);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"stroke-linejoin\">round</SvgParameter>",
	       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"stroke-linecap\">round</SvgParameter>",
	       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Stroke>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (sym->displacement_x != 0.0 || sym->displacement_y != 0.0)
      {
	  xml = sqlite3_mprintf ("%s<Displacement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<DisplacementX>%1.4f</DisplacementX>", prev,
			       sym->displacement_x);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<DisplacementY>%1.4f</DisplacementY>", prev,
			       sym->displacement_y);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Displacement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (sym->perpendicular_offset != 0.0)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<PerpendicularOffset>%1.4f</PerpendicularOffset>", prev,
	       sym->perpendicular_offset);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s</PolygonSymbolizer>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Rule>", prev);
    sqlite3_free (prev);

    return xml;
}

static char *
do_create_edge_link_xml (rl2PrivMapLayerPtr lyr)
{
/* creating an XML Style for Edges or Links */
    rl2MapLayerPtr layer = lyr->layer;
    rl2MapLineSymbolizerPtr sym = NULL;
    char *xml;
    char *prev;
    const char *cstr;
    const char *ucstr;
    const char *dashArray = NULL;

    if (layer->topology_style != NULL)
      {
	  sym = layer->topology_style->edges_sym;
	  cstr = "Edge";
	  ucstr = "edge";
      }
    if (layer->network_style != NULL)
      {
	  sym = layer->network_style->links_sym;
	  cstr = "Link";
	  ucstr = "link";
      }
    if (sym == NULL)
	return NULL;

    xml = sqlite3_mprintf ("<Rule>");
    prev = xml;
    if (layer->ok_min_scale)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<MinScaleDenominator>%1.2f</MinScaleDenominator>", prev,
	       layer->min_scale);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (layer->ok_max_scale)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<MaxScaleDenominator>%1.2f</MaxScaleDenominator>", prev,
	       layer->max_scale);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s<Name>%s</Name>", prev, cstr);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<ogc:Filter><ogc:PropertyIsEqualTo>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<ogc:PropertyName>topoclass</ogc:PropertyName>",
			 prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<ogc:Literal>%s</ogc:Literal>", prev, ucstr);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ogc:PropertyIsEqualTo></ogc:Filter>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<LineSymbolizer>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Stroke>", prev);
    sqlite3_free (prev);
    prev = xml;
/* using a Solid Color */
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke\">#%02x%02x%02x</SvgParameter>", prev,
	 sym->stroke->red, sym->stroke->green, sym->stroke->blue);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-opacity\">%1.2f</SvgParameter>", prev,
	 sym->stroke->opacity);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-width\">%1.2f</SvgParameter>", prev,
	 sym->stroke->width);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-linejoin\">round</SvgParameter>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-linecap\">round</SvgParameter>", prev);
    sqlite3_free (prev);
    prev = xml;
    switch (sym->stroke->dot_style)
      {
      case EXT_QUICK_STYLE_DOT_LINE:
	  dashArray = "5.0, 10.0";
	  break;
      case EXT_QUICK_STYLE_DASH_LINE:
	  dashArray = "20.0, 20.0";
	  break;
      case EXT_QUICK_STYLE_DASH_DOT_LINE:
	  dashArray = "20.0, 10.0, 5.0, 10.0";
	  break;
      default:
	  dashArray = NULL;
	  break;
      };
    if (dashArray != NULL)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<SvgParameter name=\"stroke-dasharray\">%s</SvgParameter>",
	       prev, dashArray);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s</Stroke>", prev);
    sqlite3_free (prev);
    prev = xml;
    if (sym->perpendicular_offset != 0.0)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<PerpendicularOffset>%1.2f</PerpendicularOffset>", prev,
	       sym->perpendicular_offset);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s</LineSymbolizer>", prev);
    sqlite3_free (prev);
    prev = xml;

    xml = sqlite3_mprintf ("%s</Rule>", prev);
    sqlite3_free (prev);

    return xml;
}

static char *
do_create_node_like_xml (rl2PrivMapLayerPtr lyr, int which)
{
/* creating an XML Style for Nodes, EdgeSeeds, FaceSeeds or LinkSeeds */
    rl2MapLayerPtr layer = lyr->layer;
    rl2MapPointSymbolizerPtr sym = NULL;
    char *xml;
    char *prev;
    const char *cstr = "Unkwnown";
    const char *ucstr = "unknown";

    switch (which)
      {
      case TOPO_NODE:
	  if (layer->topology_style != NULL)
	    {
		if (layer->topology_style->nodes_sym != NULL)
		  {
		      sym = layer->topology_style->nodes_sym;
		      cstr = "Node";
		      ucstr = "node";
		  }
	    }
	  break;
      case TOPO_EDGE_SEED:
	  if (layer->topology_style != NULL)
	    {
		if (layer->topology_style->edge_seeds_sym != NULL)
		  {
		      sym = layer->topology_style->edge_seeds_sym;
		      cstr = "EdgeSeed";
		      ucstr = "edge_seed";
		  }
	    }
	  break;
      case TOPO_FACE_SEED:
	  if (layer->topology_style != NULL)
	    {
		if (layer->topology_style->face_seeds_sym != NULL)
		  {
		      sym = layer->topology_style->face_seeds_sym;
		      cstr = "FaceSeed";
		      ucstr = "face_seed";
		  }
	    }
	  break;
      case NET_NODE:
	  if (layer->network_style != NULL)
	    {
		if (layer->network_style->nodes_sym != NULL)
		  {
		      sym = layer->network_style->nodes_sym;
		      cstr = "Node";
		      ucstr = "node";
		  }
	    }
	  break;
      case NET_LINK_SEED:
	  if (layer->network_style != NULL)
	    {
		if (layer->network_style->link_seeds_sym != NULL)
		  {
		      sym = layer->network_style->link_seeds_sym;
		      cstr = "LinkSeed";
		      ucstr = "link_seed";
		  }
	    }
	  break;
      default:
	  sym = NULL;
	  cstr = "Unknown";
	  ucstr = "unknown";
	  break;
      };
    if (sym == NULL)
	return NULL;

    xml = sqlite3_mprintf ("<Rule>");
    prev = xml;
    if (layer->ok_min_scale)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<MinScaleDenominator>%1.2f</MinScaleDenominator>", prev,
	       layer->min_scale);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (layer->ok_max_scale)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<MaxScaleDenominator>%1.2f</MaxScaleDenominator>", prev,
	       layer->max_scale);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s<Name>%s</Name>", prev, cstr);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<ogc:Filter><ogc:PropertyIsEqualTo>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<ogc:PropertyName>topoclass</ogc:PropertyName>",
			 prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<ogc:Literal>%s</ogc:Literal>", prev, ucstr);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ogc:PropertyIsEqualTo></ogc:Filter>", prev);
    sqlite3_free (prev);
    prev = xml;

    xml = sqlite3_mprintf ("%s<PointSymbolizer>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Graphic>", prev);
    sqlite3_free (prev);
    prev = xml;
/* mark symbol */
    xml = sqlite3_mprintf ("%s<Mark>", prev);
    sqlite3_free (prev);
    prev = xml;
    switch (sym->mark->type)
      {
      case RL2_GRAPHIC_MARK_CIRCLE:
	  cstr = "circle";
	  break;
      case RL2_GRAPHIC_MARK_TRIANGLE:
	  cstr = "triangle";
	  break;
      case RL2_GRAPHIC_MARK_STAR:
	  cstr = "star";
	  break;
      case RL2_GRAPHIC_MARK_CROSS:
	  cstr = "cross";
	  break;
      case RL2_GRAPHIC_MARK_X:
	  cstr = "x";
	  break;
      default:
	  cstr = "square";
	  break;
      };
    xml = sqlite3_mprintf ("%s<WellKnownName>%s</WellKnownName>", prev, cstr);
    sqlite3_free (prev);
    prev = xml;
/* Mark Fill */
    xml = sqlite3_mprintf ("%s<Fill>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"fill\">#%02x%02x%02x</SvgParameter>", prev,
	 sym->mark->fill->red, sym->mark->fill->green, sym->mark->fill->blue);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Fill>", prev);
    sqlite3_free (prev);
    prev = xml;
/* Mark Stroke */
    xml = sqlite3_mprintf ("%s<Stroke>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke\">#%02x%02x%02x</SvgParameter>", prev,
	 sym->mark->stroke->red, sym->mark->stroke->green,
	 sym->mark->stroke->blue);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-width\">%1.2f</SvgParameter>", prev,
	 1.0);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-linejoin\">round</SvgParameter>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%s<SvgParameter name=\"stroke-linecap\">round</SvgParameter>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Stroke>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Mark>", prev);
    sqlite3_free (prev);
    prev = xml;
    if (sym->opacity != 1.0)
      {
	  xml =
	      sqlite3_mprintf ("%s<Opacity>%1.2f</Opacity>", prev,
			       sym->opacity);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s<Size>%1.2f</Size>", prev, sym->size);
    sqlite3_free (prev);
    prev = xml;
    if (sym->rotation != 0.0)
      {
	  xml =
	      sqlite3_mprintf ("%s<Rotation>%1.2f</Rotation>", prev,
			       sym->rotation);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (sym->anchor_x != 0.5 || sym->anchor_y != 0.5)
      {
	  xml = sqlite3_mprintf ("%s<AnchorPoint>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<AnchorPointX>%1.4f</AnchorPointX>", prev,
			       sym->anchor_x);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<AnchorPointY>%1.4f</AnchorPointY>", prev,
			       sym->anchor_y);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</AnchorPoint>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (sym->displacement_x != 0.0 || sym->displacement_y != 0.0)
      {
	  xml = sqlite3_mprintf ("%s<Displacement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<DisplacementX>%1.4f</DisplacementX>", prev,
			       sym->displacement_x);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<DisplacementY>%1.4f</DisplacementY>", prev,
			       sym->displacement_y);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Displacement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s</Graphic>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</PointSymbolizer>", prev);
    sqlite3_free (prev);
    prev = xml;

    xml = sqlite3_mprintf ("%s</Rule>", prev);
    sqlite3_free (prev);

    return xml;
}

static void
do_create_topo_style_xml (rl2PrivMapLayerPtr lyr)
{
/* creating a Topology XML Style */
    rl2MapLayerPtr layer;
    char *xml;
    char *xml2;
    char *prev;
    char uuid[64];

    layer = lyr->layer;
    if (layer == NULL)
	return;

    if (layer->vector_style_internal_name != NULL)
	do_set_style_name (lyr, layer->vector_style_internal_name);
    else
      {
	  do_get_uuid (uuid);
	  do_set_style_name (lyr, uuid);
      }
    xml = sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<FeatureTypeStyle version=\"1.1.0\" ", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%sxsi:schemaLocation=\"http://www.opengis.net/se http://schemas.opengis.net/se/1.1.0/FeatureStyle.xsd\" ",
	 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%sxmlns=\"http://www.opengis.net/se\" xmlns:ogc=\"http://www.opengis.net/ogc\" ",
	 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%sxmlns:xlink=\"http://www.w3.org/1999/xlink\" ",
			 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%sxmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Name>%s</Name>", prev, uuid);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Description>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<Title>%s</Title>", prev, "Quick Style - Topology");
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<Abstract>%s</Abstract>", prev,
			 "Created by SpatialiteGUI");
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Description>", prev);
    sqlite3_free (prev);
    prev = xml;
/* Faces */
    xml2 = do_create_face_xml (lyr);
    if (xml2 != NULL)
      {
	  xml = sqlite3_mprintf ("%s%s", prev, xml2);
	  sqlite3_free (prev);
	  sqlite3_free (xml2);
	  prev = xml;
      }
/* Edges */
    xml2 = do_create_edge_link_xml (lyr);
    if (xml2 != NULL)
      {
	  xml = sqlite3_mprintf ("%s%s", prev, xml2);
	  sqlite3_free (prev);
	  sqlite3_free (xml2);
	  prev = xml;
      }
/* Nodes */
    xml2 = do_create_node_like_xml (lyr, TOPO_NODE);
    if (xml2 != NULL)
      {
	  xml = sqlite3_mprintf ("%s%s", prev, xml2);
	  sqlite3_free (prev);
	  sqlite3_free (xml2);
	  prev = xml;
      }
/* EdgeSeeds */
    xml2 = do_create_node_like_xml (lyr, TOPO_EDGE_SEED);
    if (xml2 != NULL)
      {
	  xml = sqlite3_mprintf ("%s%s", prev, xml2);
	  sqlite3_free (prev);
	  sqlite3_free (xml2);
	  prev = xml;
      }
/* FaceSeeds */
    xml2 = do_create_node_like_xml (lyr, TOPO_FACE_SEED);
    if (xml2 != NULL)
      {
	  xml = sqlite3_mprintf ("%s%s", prev, xml2);
	  sqlite3_free (prev);
	  sqlite3_free (xml2);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s</FeatureTypeStyle>", prev);
    sqlite3_free (prev);
    lyr->xml_style = xml;
}

static void
do_create_net_style_xml (rl2PrivMapLayerPtr lyr)
{
/* creating a Network XML Style */
    rl2MapLayerPtr layer;
    char *xml;
    char *xml2;
    char *prev;
    char uuid[64];

    layer = lyr->layer;
    if (layer == NULL)
	return;

    if (layer->vector_style_internal_name != NULL)
	do_set_style_name (lyr, layer->vector_style_internal_name);
    else
      {
	  do_get_uuid (uuid);
	  do_set_style_name (lyr, uuid);
      }
    xml = sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<FeatureTypeStyle version=\"1.1.0\" ", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%sxsi:schemaLocation=\"http://www.opengis.net/se http://schemas.opengis.net/se/1.1.0/FeatureStyle.xsd\" ",
	 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%sxmlns=\"http://www.opengis.net/se\" xmlns:ogc=\"http://www.opengis.net/ogc\" ",
	 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%sxmlns:xlink=\"http://www.w3.org/1999/xlink\" ",
			 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%sxmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Name>%s</Name>", prev, uuid);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Description>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<Title>%s</Title>", prev, "Quick Style - Network");
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<Abstract>%s</Abstract>", prev,
			 "Created by SpatialiteGUI");
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Description>", prev);
    sqlite3_free (prev);
    prev = xml;
/* Links */
    xml2 = do_create_edge_link_xml (lyr);
    if (xml2 != NULL)
      {
	  xml = sqlite3_mprintf ("%s%s", prev, xml2);
	  sqlite3_free (prev);
	  sqlite3_free (xml2);
	  prev = xml;
      }
/* Nodes */
    xml2 = do_create_node_like_xml (lyr, NET_NODE);
    if (xml2 != NULL)
      {
	  xml = sqlite3_mprintf ("%s%s", prev, xml2);
	  sqlite3_free (prev);
	  sqlite3_free (xml2);
	  prev = xml;
      }
/* LinkSeeds */
    xml2 = do_create_node_like_xml (lyr, NET_LINK_SEED);
    if (xml2 != NULL)
      {
	  xml = sqlite3_mprintf ("%s%s", prev, xml2);
	  sqlite3_free (prev);
	  sqlite3_free (xml2);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s</FeatureTypeStyle>", prev);
    sqlite3_free (prev);
    lyr->xml_style = xml;
}

static char *
do_create_predefined_etopo2 ()
{
/* creating the predefined Etopo2 Style */
    char *prev;
    char *xml = sqlite3_mprintf ("<ColorMap>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<Interpolate fallbackValue=\"#ffffff\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<LookupValue>Rasterdata</LookupValue>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-11000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#000000</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-5000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#000064</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-1000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#3232c8</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-1.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#9696ff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#009600</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>270.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#5aa55a</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>300.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#5aaf5a</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>500.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#32b432</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>1000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#46914b</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>2000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#969c64</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>3000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#dcdcdc</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>4000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#f5f5f5</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>8850.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffffff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Interpolate>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ColorMap>", prev);
    sqlite3_free (prev);
    return xml;
}

static char *
do_create_predefined_srtm ()
{
/* creating the predefined SRTM Style */
    char *prev;
    char *xml = sqlite3_mprintf ("<ColorMap>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<Interpolate fallbackValue=\"#ffffff\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<LookupValue>Rasterdata</LookupValue>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-11000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#000000</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-8000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#000032</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-5000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#0a0a64</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-3000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#1e1e96</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-1000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#4646c8</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-100.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#6464e1</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#aaaaff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.10000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#399769</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>100.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#75c25d</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>500.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#e6e680</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>1000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ca9e4b</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>2000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#b99a64</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>3000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#dcdcdc</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>5000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffffff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>8850.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#6464c8</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Interpolate>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ColorMap>", prev);
    sqlite3_free (prev);
    return xml;
}

static char *
do_create_predefined_terrain ()
{
/* creating the predefined Terrain Style */
    char *prev;
    char *xml = sqlite3_mprintf ("<ColorMap>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<Interpolate fallbackValue=\"#ffffff\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<LookupValue>Rasterdata</LookupValue>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-11000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#000000</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-500.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#00001e</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-100.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#0000c8</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-1.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#9696ff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#007800</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>100.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#009600</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>270.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#5aa55a</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>300.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#5aaf5a</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>500.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#32b432</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>500.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#46aa46</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>1000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#46914b</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>1000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#469b4b</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>2000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#969c64</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>2800.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#dcdcdc</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>3000.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffffff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>8850.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffffff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Interpolate>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ColorMap>", prev);
    sqlite3_free (prev);
    return xml;
}

static char *
do_create_predefined_elevation (double min_value, double max_value)
{
/* creating the predefined Elevation Style */
    double interval = (max_value - min_value) / 5.0;
    double value = min_value;
    char *prev;
    char *xml = sqlite3_mprintf ("<ColorMap>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<Interpolate fallbackValue=\"#ffffff\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<LookupValue>Rasterdata</LookupValue>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#00bfbf</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#00ff00</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffff00</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ff7f00</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#bf7f3f</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, max_value);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#c8c8c8</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Interpolate>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ColorMap>", prev);
    sqlite3_free (prev);
    return xml;
}

static char *
do_create_predefined_aspect_color (double min_value, double max_value)
{
/* creating the predefined AspectColor Style */
    double interval = (max_value - min_value) / 4.0;
    double value = min_value;
    char *prev;
    char *xml = sqlite3_mprintf ("<ColorMap>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<Interpolate fallbackValue=\"#ffffff\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<LookupValue>Rasterdata</LookupValue>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffffff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffff00</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#00ffff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ff0000</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, max_value);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffff00</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Interpolate>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ColorMap>", prev);
    sqlite3_free (prev);
    return xml;
}

static char *
do_create_predefined_rainbow (double min_value, double max_value)
{
/* creating the predefined Rainbow Style */
    double interval = (max_value - min_value) / 5.0;
    double value = min_value;
    char *prev;
    char *xml = sqlite3_mprintf ("<ColorMap>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<Interpolate fallbackValue=\"#ffffff\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<LookupValue>Rasterdata</LookupValue>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffff00</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#00ff00</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#00ffff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#0000ff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ff00ff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, max_value);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ff0000</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Interpolate>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ColorMap>", prev);
    sqlite3_free (prev);
    return xml;
}

static char *
do_create_predefined_wave (double min_value, double max_value)
{
/* creating the predefined Wave */
    double interval = (max_value - min_value) / 6.0;
    double value = min_value;
    char *prev;
    char *xml = sqlite3_mprintf ("<ColorMap>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<Interpolate fallbackValue=\"#ffffff\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<LookupValue>Rasterdata</LookupValue>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ff8585</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#aaaa00</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#55ff55</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#00aaaa</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#5555ff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#aa00aa</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, max_value);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ff5555</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Interpolate>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ColorMap>", prev);
    sqlite3_free (prev);
    return xml;
}

static char *
do_create_predefined_sepia (double min_value, double max_value)
{
/* creating the predefined Sepia */
    double interval = (max_value - min_value) / 10.0;
    double value = min_value;
    char *prev;
    char *xml = sqlite3_mprintf ("<ColorMap>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<Interpolate fallbackValue=\"#ffffff\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<LookupValue>Rasterdata</LookupValue>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#000000</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#221709</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#442f13</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#654821</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#826031</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#9e7a47</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#b79760</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ceae7e</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffc8a2</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, value);
    value += interval;
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#f2e3cc</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>%1.6f</Data>", prev, max_value);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#fffefb</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Interpolate>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ColorMap>", prev);
    sqlite3_free (prev);
    return xml;
}

static char *
do_create_predefined_ndvi ()
{
/* creating the predefined NDVI */
    char *prev;
    char *xml = sqlite3_mprintf ("<ColorMap>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<Interpolate fallbackValue=\"#ffffff\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<LookupValue>Rasterdata</LookupValue>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-1.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#051852</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-0.30000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#051852</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-0.18000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffffff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffffff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.02500000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#cec5b4</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.07500000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#bfa37c</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.12500000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#b3ae60</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.15000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#a3b550</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.17500000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#90aa3c</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.23300000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#a6c31d</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.26600000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#87b703</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.33300000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#79af01</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.36600000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#65a300</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.43300000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#4e9700</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.46600000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#2b8404</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.55000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#007200</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.65000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#005a01</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.75000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#004900</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.85000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#003800</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.95000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#001f00</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>1.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#000000</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Interpolate>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ColorMap>", prev);
    sqlite3_free (prev);
    return xml;
}

static char *
do_create_predefined_ndwi ()
{
/* creating the predefined NDWI */
    char *prev;
    char *xml = sqlite3_mprintf ("<ColorMap>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<Interpolate fallbackValue=\"#ffffff\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<LookupValue>Rasterdata</LookupValue>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-1.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#ffffff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>-0.20000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#cdc1ad</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#969696</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>0.50000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#9696ff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Data>1.00000000</Data>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Value>#0000ff</Value>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Interpolate>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</ColorMap>", prev);
    sqlite3_free (prev);
    return xml;
}

static void
do_create_coverage_xml (rl2PrivMapLayerPtr lyr)
{
/* creating a Coverage XML Style */
    rl2MapLayerPtr layer;
    rl2MapRasterLayerStylePtr raster;
    char *xml;
    char *prev;
    char *xml2;
    char uuid[64];

    layer = lyr->layer;
    if (layer == NULL)
	return;
    raster = layer->raster_style;
    if (raster == NULL)
	return;

    if (layer->raster_style_internal_name != NULL)
	do_set_style_name (lyr, layer->raster_style_internal_name);
    else
      {
	  do_get_uuid (uuid);
	  do_set_style_name (lyr, uuid);
      }
    xml = sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    prev = xml;
    xml = sqlite3_mprintf ("%s<CoverageStyle version=\"1.1.0\" ", prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%sxsi:schemaLocation=\"http://www.opengis.net/se http://schemas.opengis.net/se/1.1.0/FeatureStyle.xsd\" ",
	 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%sxmlns=\"http://www.opengis.net/se\" xmlns:ogc=\"http://www.opengis.net/ogc\" ",
	 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%sxmlns:xlink=\"http://www.w3.org/1999/xlink\" ",
			 prev);
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf
	("%sxmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Name>%s</Name>", prev, uuid);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Description>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Title>%s</Title>", prev, "Quick Style");
    sqlite3_free (prev);
    prev = xml;
    xml =
	sqlite3_mprintf ("%s<Abstract>%s</Abstract>", prev,
			 "Created by SpatialiteGUI");
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Description>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Rule>", prev);
    sqlite3_free (prev);
    prev = xml;
    if (layer->ok_min_scale)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<MinScaleDenominator>%1.2f</MinScaleDenominator>", prev,
	       layer->min_scale);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (layer->ok_max_scale)
      {
	  xml =
	      sqlite3_mprintf
	      ("%s<MaxScaleDenominator>%1.2f</MaxScaleDenominator>", prev,
	       layer->max_scale);
	  sqlite3_free (prev);
	  prev = xml;
      }
    xml = sqlite3_mprintf ("%s<RasterSymbolizer>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s<Opacity>%1.2f</Opacity>", prev, raster->opacity);
    sqlite3_free (prev);
    prev = xml;
    if (raster->color_map_name != NULL)
      {
	  /* predefined Color Maps */
	  if (strcasecmp (raster->color_map_name, "etopo2") == 0)
	    {
		/* special case: predefined Etopo2 style */
		xml2 = do_create_predefined_etopo2 ();
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		  }
		prev = xml;
		goto shaded_relief;
	    }
	  if (strcasecmp (raster->color_map_name, "srtm") == 0)
	    {
		/* special case: predefined SRTM style */
		xml2 = do_create_predefined_srtm ();
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		  }
		prev = xml;
		goto shaded_relief;
	    }
	  if (strcasecmp (raster->color_map_name, "terrain") == 0)
	    {
		/* special case: predefined Terrain style */
		xml2 = do_create_predefined_terrain ();
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		  }
		prev = xml;
		goto shaded_relief;
	    }
	  if (strcasecmp (raster->color_map_name, "elevation") == 0)
	    {
		/* special case: predefined Elevation style */
		xml2 =
		    do_create_predefined_elevation (lyr->min_value,
						    lyr->max_value);
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		  }
		prev = xml;
		goto shaded_relief;
	    }
	  if (strcasecmp (raster->color_map_name, "aspect_color") == 0)
	    {
		/* special case: predefined AspectColor style */
		xml2 =
		    do_create_predefined_aspect_color (lyr->min_value,
						       lyr->max_value);
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		  }
		prev = xml;
		goto shaded_relief;
	    }
	  if (strcasecmp (raster->color_map_name, "rainbow") == 0)
	    {
		/* special case: predefined Rainbow style */
		xml2 =
		    do_create_predefined_rainbow (lyr->min_value,
						  lyr->max_value);
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		  }
		prev = xml;
		goto shaded_relief;
	    }
	  if (strcasecmp (raster->color_map_name, "wave") == 0)
	    {
		/* special case: predefined Wave style */
		xml2 =
		    do_create_predefined_wave (lyr->min_value, lyr->max_value);
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		  }
		prev = xml;
		goto shaded_relief;
	    }
	  if (strcasecmp (raster->color_map_name, "sepia") == 0)
	    {
		/* special case: predefined Sepia style */
		xml2 =
		    do_create_predefined_sepia (lyr->min_value, lyr->max_value);
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		  }
		prev = xml;
		goto shaded_relief;
	    }
	  if (strcasecmp (raster->color_map_name, "ndvi") == 0)
	    {
		/* special case: predefined NDVI style */
		xml2 = do_create_predefined_ndvi ();
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		      lyr->syntetic_band = RL2_SYNTETIC_NDVI;
		  }
		prev = xml;
		goto foot;
	    }
	  if (strcasecmp (raster->color_map_name, "ndwi") == 0)
	    {
		/* special case: predefined NDWI style */
		xml2 = do_create_predefined_ndwi ();
		if (xml2 != NULL)
		  {
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		      lyr->syntetic_band = RL2_SYNTETIC_NDWI;
		  }
		prev = xml;
		goto foot;
	    }
      }
    if (raster->channel_selection != NULL)
      {
	  rl2MapChannelSelectionPtr channels = raster->channel_selection;
	  xml = sqlite3_mprintf ("%s<ChannelSelection>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (channels->rgb)
	    {
		xml =
		    sqlite3_mprintf ("%s<RedChannel><SourceChannelName>%d",
				     prev, channels->red_channel);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s</SourceChannelName></RedChannel>",
				     prev);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s<GreenChannel><SourceChannelName>%d",
				     prev, channels->green_channel);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s</SourceChannelName></GreenChannel>",
				     prev);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s<BlueChannel><SourceChannelName>%d",
				     prev, channels->blue_channel);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s</SourceChannelName></BlueChannel>",
				     prev);
		sqlite3_free (prev);
		prev = xml;
	    }
	  else
	    {
		xml =
		    sqlite3_mprintf ("%s<GrayChannel><SourceChannelName>%d",
				     prev, channels->gray_channel);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s</SourceChannelName></GrayChannel>",
				     prev);
		sqlite3_free (prev);
		prev = xml;
	    }
	  xml = sqlite3_mprintf ("%s</ChannelSelection>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (raster->contrast_enhancement != NULL)
      {
	  rl2MapContrastEnhancementPtr contrast = raster->contrast_enhancement;
	  xml = sqlite3_mprintf ("%s<ContrastEnhancement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (contrast->histogram)
	    {
		xml = sqlite3_mprintf ("%s<Histogram/>", prev);
		sqlite3_free (prev);
		prev = xml;
	    }
	  else if (contrast->gamma)
	    {
		xml =
		    sqlite3_mprintf ("%s<GammaValue>%1.2f</GammaValue>", prev,
				     contrast->gamma_value);
		sqlite3_free (prev);
		prev = xml;
	    }
	  else
	    {
		xml = sqlite3_mprintf ("%s<Normalize/>", prev);
		sqlite3_free (prev);
		prev = xml;
	    }
	  xml = sqlite3_mprintf ("%s</ContrastEnhancement>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
    if (raster->color_ramp != NULL)
      {
	  rl2MapColorRampPtr ramp = raster->color_ramp;
	  xml = sqlite3_mprintf ("%s<ColorMap>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<Interpolate fallbackValue=\"%s\">", prev,
			       ramp->min_color);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<LookupValue>Rasterdata</LookupValue>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Data>%1.8f</Data>", prev, lyr->min_value);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Value>%s</Value>", prev, ramp->min_color);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<InterpolationPoint>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Data>%1.8f</Data>", prev, lyr->max_value);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s<Value>%s</Value>", prev, ramp->max_color);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</InterpolationPoint>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</Interpolate>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</ColorMap>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
  shaded_relief:
    if (raster->shaded_relief)
      {
	  xml = sqlite3_mprintf ("%s<ShadedRelief>", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s<ReliefFactor>%1.2f</ReliefFactor>", prev,
			       raster->relief_factor);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s</ShadedRelief>", prev);
	  sqlite3_free (prev);
	  prev = xml;
      }
  foot:
    xml = sqlite3_mprintf ("%s</RasterSymbolizer>", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s</Rule></CoverageStyle>", prev);
    sqlite3_free (prev);

    lyr->xml_style = xml;
}

RL2_PRIVATE void
rl2_create_xml_quick_style (rl2PrivMapLayerPtr lyr)
{
/* creating an XML Quick Style */
    rl2MapLayerPtr layer;
    int is_multi = 0;

    if (lyr == NULL)
	return;
    if (lyr->xml_style != NULL)
      {
	  sqlite3_free (lyr->xml_style);
	  lyr->xml_style = NULL;
      }

    layer = lyr->layer;
    if (layer == NULL)
	return;

    if (layer->raster_style_internal_name != NULL)
      {
	  /* an internal style is required */
	  do_set_style_name (lyr, layer->raster_style_internal_name);
	  return;
      }
    if (layer->vector_style_internal_name != NULL)
      {
	  /* an internal style is required */
	  do_set_style_name (lyr, layer->vector_style_internal_name);
	  return;
      }
    if (layer->raster_style != NULL)
      {
	  /* Raster Style */
	  do_create_coverage_xml (lyr);
      }
    if (layer->vector_style != NULL)
      {
	  /* Vector Style */
	  rl2MapLineSymbolizerPtr line;
	  rl2MapVectorLayerStylePtr vector = layer->vector_style;
	  int count = 0;
	  if (vector->point_sym != NULL)
	      count++;
	  line = vector->first_line_sym;
	  while (line != NULL)
	    {
		count++;
		line = line->next;
	    }
	  if (vector->polygon_sym != NULL)
	      count++;
	  if (vector->text_sym != NULL)
	      count++;
	  if (count == 0)
	      return;
	  if (count > 1)
	      is_multi = 1;
	  if (layer->ok_min_scale || layer->ok_max_scale || is_multi)
	      do_create_feature_type_xml (lyr);
	  else
	      do_create_vector_symbolizer_xml (lyr);
      }

    if (layer->topology_style != NULL)
      {
	  /* Topology Style */
	  do_create_topo_style_xml (lyr);
      }
    if (layer->network_style != NULL)
      {
	  /* Network Style */
	  do_create_net_style_xml (lyr);
      }
}
