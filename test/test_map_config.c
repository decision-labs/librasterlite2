/*

 test_map_config.c -- RasterLite-2 Test Case

 Author: Sandro Furieri <a.furieri@lqt.it>

 ------------------------------------------------------------------------------
 
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

Contributor(s):

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
#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"
#include "spatialite.h"

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2/rl2mapconfig.h"
#include "rasterlite2_private.h"

static int
test_map_config (const unsigned char *xml, int *retcode)
{
/* testing a Map Configuration */
    int raster_1 = 0;
    int raster_2 = 0;
    int raster_3 = 0;
    int raster_4 = 0;
    int raster_5 = 0;
    int raster_6 = 0;
    int wms = 0;
    int vector_1 = 0;
    int vector_2 = 0;
    int vector_3 = 0;
    int topology_1 = 0;
    int network_1 = 0;
    int point = 0;
    int line = 0;
    int polygon_1 = 0;
    int polygon_2 = 0;
    int text_point = 0;
    int text_line = 0;
    int ret = 0;
    rl2MapLayerPtr lyr;
    rl2MapConfigPtr clone;
    rl2MapConfigPtr map = rl2_parse_map_config_xml (xml);
    if (map == NULL)
      {
	  fprintf (stderr, "Unable to parse the XML document\n");
	  ret = -1;
	  goto stop;
      }

/* testing Layers */
    lyr = map->first_lyr;
    while (lyr != NULL)
      {
	  if (strcmp (lyr->prefix, "a") == 0
	      && strcmp (lyr->name, "raster_1") == 0)
	    {
		/* testing Raster_1 */
		raster_1 = 1;
		if (lyr->type != RL2_MAP_LAYER_RASTER)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> is not of the Raster Type !!\n",
			       lyr->prefix, lyr->name);
		      ret = -2;
		      goto stop;
		  }
		if (lyr->raster_style_internal_name == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Internal Style Name !!\n",
			       lyr->prefix, lyr->name);
		      ret = -3;
		      goto stop;
		  }
		if (strcmp (lyr->raster_style_internal_name, "raster_style_1")
		    != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected Internal Style Name \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->raster_style_internal_name);
		      ret = -4;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "raster_2") == 0)
	    {
		/* testing Raster_2 */
		raster_2 = 1;
		if (lyr->type != RL2_MAP_LAYER_RASTER)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> is not of the Raster Type !!\n",
			       lyr->prefix, lyr->name);
		      ret = -5;
		      goto stop;
		  }
		if (lyr->raster_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Raster Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -6;
		      goto stop;
		  }
		if (lyr->raster_style->opacity != 0.73)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected Opacity %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->raster_style->opacity);
		      ret = -7;
		      goto stop;
		  }
		if (strcmp (lyr->raster_style->color_map_name, "srtm") != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected ColorMap Name \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->raster_style->color_map_name);
		      ret = -8;
		      goto stop;
		  }
		if (lyr->raster_style->relief_factor != 23.14)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected Relief Factor %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->raster_style->relief_factor);
		      ret = -9;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "a") == 0
	      && strcmp (lyr->name, "raster_4") == 0)
	    {
		/* testing Raster_3 */
		raster_3 = 1;
		if (lyr->type != RL2_MAP_LAYER_RASTER)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> is not of the Raster Type !!\n",
			       lyr->prefix, lyr->name);
		      ret = -10;
		      goto stop;
		  }
		if (lyr->raster_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Raster Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -11;
		      goto stop;
		  }
		if (lyr->raster_style->color_ramp == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Color Ramp !!\n",
			       lyr->prefix, lyr->name);
		      ret = -12;
		      goto stop;
		  }
		if (lyr->raster_style->color_ramp->min_value != -5.38527298)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected ColorRamp MinValue %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->raster_style->color_ramp->min_value);
		      ret = -13;
		      goto stop;
		  }
		if (strcmp (lyr->raster_style->color_ramp->min_color, "#1020ff")
		    != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected ColorRamp MinColor \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->raster_style->color_ramp->min_color);
		      ret = -14;
		      goto stop;
		  }
		if (lyr->raster_style->color_ramp->max_value != 2053.62207031)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected ColorRamp MaxValue %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->raster_style->color_ramp->max_value);
		      ret = -15;
		      goto stop;
		  }
		if (strcmp (lyr->raster_style->color_ramp->max_color, "#ff080e")
		    != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected ColorRamp MaxColor \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->raster_style->color_ramp->max_color);
		      ret = -15;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "raster_7") == 0)
	    {
		/* testing Raster_4 */
		raster_4 = 1;
		if (lyr->type != RL2_MAP_LAYER_RASTER)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> is not of the Raster Type !!\n",
			       lyr->prefix, lyr->name);
		      ret = -16;
		      goto stop;
		  }
		if (lyr->raster_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Raster Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -17;
		      goto stop;
		  }
		if (lyr->raster_style->contrast_enhancement == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL ContrastEnhancement !!\n",
			       lyr->prefix, lyr->name);
		      ret = -18;
		      goto stop;
		  }
		if (lyr->raster_style->contrast_enhancement->gamma_value !=
		    1.57)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected ContrastEnhancement GammaValue %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->raster_style->
			       contrast_enhancement->gamma_value);
		      ret = -19;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "raster_8") == 0)
	    {
		/* testing Raster_5 */
		raster_5 = 1;
		if (lyr->type != RL2_MAP_LAYER_RASTER)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> is not of the Raster Type !!\n",
			       lyr->prefix, lyr->name);
		      ret = -20;
		      goto stop;
		  }
		if (lyr->raster_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Raster Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -21;
		      goto stop;
		  }
		if (lyr->raster_style->channel_selection == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL ChannelSelection !!\n",
			       lyr->prefix, lyr->name);
		      ret = -22;
		      goto stop;
		  }
		if (lyr->raster_style->channel_selection->rgb == 0
		    || lyr->raster_style->channel_selection->green_channel != 1)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected ChannelSelection RGB=%d Green=%d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->raster_style->channel_selection->rgb,
			       lyr->raster_style->
			       channel_selection->green_channel);
		      ret = -23;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "raster_9") == 0)
	    {
		/* testing Raster_6 */
		raster_6 = 1;
		if (lyr->type != RL2_MAP_LAYER_RASTER)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> is not of the Raster Type !!\n",
			       lyr->prefix, lyr->name);
		      ret = -24;
		      goto stop;
		  }
		if (lyr->raster_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Raster Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -25;
		      goto stop;
		  }
		if (lyr->raster_style->channel_selection == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL ChannelSelection !!\n",
			       lyr->prefix, lyr->name);
		      ret = -26;
		      goto stop;
		  }
		if (lyr->raster_style->channel_selection->rgb != 0
		    || lyr->raster_style->channel_selection->gray_channel != 4)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected ChannelSelection RGB=%d Green=%d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->raster_style->channel_selection->rgb,
			       lyr->raster_style->
			       channel_selection->gray_channel);
		      ret = -27;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "wms_layer_2") == 0)
	    {
		/* testing WMS */
		wms = 1;
		if (lyr->type != RL2_MAP_LAYER_WMS)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> is not of the WMS Type !!\n",
			       lyr->prefix, lyr->name);
		      ret = -28;
		      goto stop;
		  }
		if (lyr->wms_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL WMS Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -29;
		      goto stop;
		  }
		if (lyr->wms_style->get_map_url == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL WMS GetMapURL !!\n",
			       lyr->prefix, lyr->name);
		      ret = -30;
		      goto stop;
		  }
		if (strcmp
		    (lyr->wms_style->get_map_url,
		     "http://www.something.com") != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected WMS GetMapURL \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->wms_style->get_map_url);
		      ret = -31;
		      goto stop;
		  }
		if (lyr->wms_style->get_feature_info_url == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL WMS GetFeatureInfoURL !!\n",
			       lyr->prefix, lyr->name);
		      ret = -32;
		      goto stop;
		  }
		if (strcmp
		    (lyr->wms_style->get_feature_info_url,
		     "http://www.sample.com") != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected WMS GetFeatureInfoURL \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->wms_style->get_feature_info_url);
		      ret = -33;
		      goto stop;
		  }
		if (lyr->wms_style->wms_protocol == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL WMS Protocol !!\n",
			       lyr->prefix, lyr->name);
		      ret = -34;
		      goto stop;
		  }
		if (strcmp (lyr->wms_style->wms_protocol, "1.3.0") != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected WMS Protocol \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->wms_style->wms_protocol);
		      ret = -35;
		      goto stop;
		  }
		if (lyr->wms_style->crs == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL WMS CRS !!\n",
			       lyr->prefix, lyr->name);
		      ret = -36;
		      goto stop;
		  }
		if (strcmp (lyr->wms_style->crs, "EPSG:4326") != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected WMS CRS \"%s\" !!\n",
			       lyr->prefix, lyr->name, lyr->wms_style->crs);
		      ret = -37;
		      goto stop;
		  }
		if (lyr->wms_style->swap_xy != 1)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected WMS SwapXY %d !!\n",
			       lyr->prefix, lyr->name, lyr->wms_style->swap_xy);
		      ret = -38;
		      goto stop;
		  }
		if (lyr->wms_style->style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL WMS Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -39;
		      goto stop;
		  }
		if (strcmp (lyr->wms_style->style, "wms_style") != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected WMS Style \"%s\" !!\n",
			       lyr->prefix, lyr->name, lyr->wms_style->style);
		      ret = -40;
		      goto stop;
		  }
		if (lyr->wms_style->image_format == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL ImageFormat !!\n",
			       lyr->prefix, lyr->name);
		      ret = -41;
		      goto stop;
		  }
		if (strcmp (lyr->wms_style->image_format, "image/jpeg") != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected WMS ImageFormat \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->wms_style->image_format);
		      ret = -42;
		      goto stop;
		  }
		if (lyr->wms_style->background_color == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL BgColor !!\n",
			       lyr->prefix, lyr->name);
		      ret = -43;
		      goto stop;
		  }
		if (strcmp (lyr->wms_style->background_color, "#ffff00") != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected WMS BgColor \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->wms_style->background_color);
		      ret = -44;
		      goto stop;
		  }
		if (lyr->wms_style->opaque != 1)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected WMS Opaque %d !!\n",
			       lyr->prefix, lyr->name, lyr->wms_style->opaque);
		      ret = -45;
		      goto stop;
		  }
		if (lyr->wms_style->is_tiled == 0
		    || lyr->wms_style->tile_width != 511)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected WMS Tiled=%d TileWidth=%d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->wms_style->is_tiled,
			       lyr->wms_style->tile_width);
		      ret = -46;
		      goto stop;
		  }
		if (lyr->wms_style->is_tiled == 0
		    || lyr->wms_style->tile_height != 513)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected WMS Tiled=%d TileHeight=%d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->wms_style->is_tiled,
			       lyr->wms_style->tile_height);
		      ret = -47;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "vector_1") == 0)
	    {
		/* testing Vector_1 */
		vector_1 = 1;
		if (lyr->type != RL2_MAP_LAYER_VECTOR_VIEW)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> is not of the VectorView Type !!\n",
			       lyr->prefix, lyr->name);
		      ret = -48;
		      goto stop;
		  }
		if (lyr->vector_style_internal_name == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Internal Style Name !!\n",
			       lyr->prefix, lyr->name);
		      ret = -49;
		      goto stop;
		  }
		if (strcmp (lyr->vector_style_internal_name, "vector_style_1")
		    != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected Internal Style Name \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style_internal_name);
		      ret = -50;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "vector_2") == 0)
	    {
		/* testing Vector_2 */
		vector_2 = 1;
		if (lyr->type != RL2_MAP_LAYER_VECTOR_VIRTUAL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> is not of the VectorVirtual Type !!\n",
			       lyr->prefix, lyr->name);
		      ret = -51;
		      goto stop;
		  }
		if (lyr->vector_style_internal_name == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Internal Style Name !!\n",
			       lyr->prefix, lyr->name);
		      ret = -52;
		      goto stop;
		  }
		if (strcmp (lyr->vector_style_internal_name, "vector_style_2")
		    != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected Internal Style Name \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style_internal_name);
		      ret = -53;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "vector_3") == 0)
	    {
		/* testing Vector_3 */
		vector_3 = 1;
		if (lyr->type != RL2_MAP_LAYER_VECTOR)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> is not of the Vector Type !!\n",
			       lyr->prefix, lyr->name);
		      ret = -51;
		      goto stop;
		  }
		if (lyr->vector_style_internal_name == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Internal Style Name !!\n",
			       lyr->prefix, lyr->name);
		      ret = -52;
		      goto stop;
		  }
		if (strcmp (lyr->vector_style_internal_name, "vector_style_3")
		    != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected Internal Style Name \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style_internal_name);
		      ret = -53;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "topology_1") == 0)
	    {
		/* testing Topology_1 */
		topology_1 = 1;
		if (lyr->type != RL2_MAP_LAYER_TOPOLOGY)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> is not of the Topology Type !!\n",
			       lyr->prefix, lyr->name);
		      ret = -54;
		      goto stop;
		  }
		if (lyr->topology_internal_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Internal Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -55;
		      goto stop;
		  }
		if (lyr->topology_internal_style->style_internal_name == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Internal Style Name !!\n",
			       lyr->prefix, lyr->name);
		      ret = -56;
		      goto stop;
		  }
		if (strcmp
		    (lyr->topology_internal_style->style_internal_name,
		     "topology_style_1") != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected Internal Style Name \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->
			       topology_internal_style->style_internal_name);
		      ret = -57;
		      goto stop;
		  }
		if (lyr->topology_internal_style->show_faces == 0
		    || lyr->topology_internal_style->show_edges == 0
		    || lyr->topology_internal_style->show_nodes == 0
		    || lyr->topology_internal_style->show_edge_seeds == 0
		    || lyr->topology_internal_style->show_face_seeds == 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected Flags Faces=%d Edges=%d Nodes=%d EdgeSeeds=%d FaceSeeds=%d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->topology_internal_style->show_faces,
			       lyr->topology_internal_style->show_edges,
			       lyr->topology_internal_style->show_nodes,
			       lyr->topology_internal_style->show_edge_seeds,
			       lyr->topology_internal_style->show_face_seeds);
		      ret = -58;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "network_1") == 0)
	    {
		/* testing Network_1 */
		network_1 = 1;
		if (lyr->type != RL2_MAP_LAYER_NETWORK)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> is not of the Network Type !!\n",
			       lyr->prefix, lyr->name);
		      ret = -59;
		      goto stop;
		  }
		if (lyr->network_internal_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Internal Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -60;
		      goto stop;
		  }
		if (lyr->network_internal_style->style_internal_name == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Internal Style Name !!\n",
			       lyr->prefix, lyr->name);
		      ret = -61;
		      goto stop;
		  }
		if (strcmp
		    (lyr->network_internal_style->style_internal_name,
		     "network_style_1") != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected Internal Style Name \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->
			       network_internal_style->style_internal_name);
		      ret = -62;
		      goto stop;
		  }
		if (lyr->network_internal_style->show_links == 0 ||
		    lyr->network_internal_style->show_nodes == 0
		    || lyr->network_internal_style->show_link_seeds == 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected Flags Links=%d Nodes=%d LinkSeeds=%d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->network_internal_style->show_links,
			       lyr->network_internal_style->show_nodes,
			       lyr->network_internal_style->show_link_seeds);
		      ret = -63;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "vector_6") == 0)
	    {
		/* testing PointSymbolizer */
		point = 1;
		if (lyr->vector_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Vector Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -64;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL PointSymbolizer !!\n",
			       lyr->prefix, lyr->name);
		      ret = -65;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->mark == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL PointSymbolizerMark !!\n",
			       lyr->prefix, lyr->name);
		      ret = -66;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->mark->type !=
		    RL2_GRAPHIC_MARK_SQUARE)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PointSymbolizerMarkType %d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->point_sym->mark->type);
		      ret = -67;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->mark->fill == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL PointSymbolizerMarkFill !!\n",
			       lyr->prefix, lyr->name);
		      ret = -68;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->mark->fill->red != 0xff ||
		    lyr->vector_style->point_sym->mark->fill->green != 0x01 ||
		    lyr->vector_style->point_sym->mark->fill->blue != 0x05)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PointSymbolizerMarkFill #%02x%02x%02x !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->point_sym->mark->fill->red,
			       lyr->vector_style->point_sym->mark->fill->green,
			       lyr->vector_style->point_sym->mark->fill->blue);
		      ret = -69;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->mark->stroke == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL PointSymbolizerMarkStroke !!\n",
			       lyr->prefix, lyr->name);
		      ret = -70;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->mark->stroke->red != 0x06 ||
		    lyr->vector_style->point_sym->mark->stroke->green != 0x03 ||
		    lyr->vector_style->point_sym->mark->stroke->blue != 0xfa)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PointSymbolizerMarkStroke #%02x%02x%02x !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->point_sym->mark->stroke->red,
			       lyr->vector_style->point_sym->mark->
			       stroke->green,
			       lyr->vector_style->point_sym->mark->
			       stroke->blue);
		      ret = -71;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->opacity != 0.75)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PointSymbolizerOpacity %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->point_sym->opacity);
		      ret = -72;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->size != 8.05)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PointSymbolizerSize %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->point_sym->size);
		      ret = -73;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->rotation != 45.1)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PointSymbolizerRotation %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->point_sym->rotation);
		      ret = -74;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->anchor_x != 5.2)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PointSymbolizerAnchorX %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->point_sym->anchor_x);
		      ret = -75;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->anchor_y != 7.5)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PointSymbolizerAnchorY %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->point_sym->anchor_y);
		      ret = -76;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->displacement_x != 12.3)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PointSymbolizerDisplacementX %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->point_sym->displacement_x);
		      ret = -77;
		      goto stop;
		  }
		if (lyr->vector_style->point_sym->displacement_y != 13.5)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PointSymbolizerDisplacementY %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->point_sym->displacement_y);
		      ret = -77;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "vector_5") == 0)
	    {
		/* testing LineSymbolizer */
		line = 1;
		if (lyr->vector_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Vector Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -78;
		      goto stop;
		  }
		if (lyr->vector_style->first_line_sym == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL LineSymbolizer !!\n",
			       lyr->prefix, lyr->name);
		      ret = -79;
		      goto stop;
		  }
		if (lyr->vector_style->first_line_sym->stroke == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL LineSymbolizerStroke !!\n",
			       lyr->prefix, lyr->name);
		      ret = -80;
		      goto stop;
		  }
		if (lyr->vector_style->first_line_sym->stroke->red != 0xff ||
		    lyr->vector_style->first_line_sym->stroke->green != 0xa5 ||
		    lyr->vector_style->first_line_sym->stroke->blue != 0x1d)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected LineSymbolizerStroke #%02x%02x%02x !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->first_line_sym->stroke->red,
			       lyr->vector_style->first_line_sym->stroke->green,
			       lyr->vector_style->first_line_sym->stroke->blue);
		      ret = -81;
		      goto stop;
		  }
		if (lyr->vector_style->first_line_sym->stroke->opacity != 0.66)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected LineSymbolizerStrokeOpacity %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->first_line_sym->
			       stroke->opacity);
		      ret = -82;
		      goto stop;
		  }
		if (lyr->vector_style->first_line_sym->stroke->width != 8.52)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected LineSymbolizerStrokeWidth %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->first_line_sym->
			       stroke->width);
		      ret = -83;
		      goto stop;
		  }
		if (lyr->vector_style->first_line_sym->stroke->dot_style != 4)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected LineSymbolizerStrokeDotStyle %d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->first_line_sym->
			       stroke->dot_style);
		      ret = -84;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "topology_2") == 0)
	    {
		/* testing PolygonSymbolizer_1 */
		polygon_1 = 1;
		if (lyr->topology_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Topology Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -85;
		      goto stop;
		  }
		if (lyr->topology_style->faces_sym == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TopologyFacesSymbolizer !!\n",
			       lyr->prefix, lyr->name);
		      ret = -86;
		      goto stop;
		  }
		if (lyr->topology_style->faces_sym->fill == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TopologyFacesSymbolizerFill !!\n",
			       lyr->prefix, lyr->name);
		      ret = -87;
		      goto stop;
		  }
		if (lyr->topology_style->faces_sym->fill->red != 0xfc ||
		    lyr->topology_style->faces_sym->fill->green != 0x67 ||
		    lyr->topology_style->faces_sym->fill->blue != 0x3e)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TopologyFacesSymbolizerFill #%02x%02x%02x !!\n",
			       lyr->prefix, lyr->name,
			       lyr->topology_style->faces_sym->fill->red,
			       lyr->topology_style->faces_sym->fill->green,
			       lyr->topology_style->faces_sym->fill->blue);
		      ret = -88;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "vector_4") == 0)
	    {
		/* testing PolygonSymbolizer_2 */
		polygon_2 = 1;
		if (lyr->vector_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Vector Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -89;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL PolygonSymbolizer !!\n",
			       lyr->prefix, lyr->name);
		      ret = -90;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->fill == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL PolygonSymbolizerFill !!\n",
			       lyr->prefix, lyr->name);
		      ret = -91;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->fill->graphic == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL PolygonSymbolizerFillGraphic !!\n",
			       lyr->prefix, lyr->name);
		      ret = -92;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->fill->graphic->resource ==
		    NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL PolygonSymbolizerFillGraphicResource !!\n",
			       lyr->prefix, lyr->name);
		      ret = -93;
		      goto stop;
		  }
		if (strcmp
		    (lyr->vector_style->polygon_sym->fill->graphic->resource,
		     "http://www.utopia.gov/stdbrush_crossdiag.png") != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PolygonSymbolizerFillGraphicResource \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->polygon_sym->fill->graphic->
			       resource);
		      ret = -94;
		      goto stop;
		  }
		if (strcmp
		    (lyr->vector_style->polygon_sym->fill->graphic->format,
		     "image/png") != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PolygonSymbolizerFillGraphicFormat \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->polygon_sym->fill->graphic->
			       format);
		      ret = -95;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->fill->graphic->color ==
		    NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL PolygonSymbolizerFillGraphicRemapColor !!\n",
			       lyr->prefix, lyr->name);
		      ret = -96;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->fill->graphic->color->red !=
		    0xfb
		    || lyr->vector_style->polygon_sym->fill->graphic->color->
		    green != 0xe3
		    || lyr->vector_style->polygon_sym->fill->graphic->color->
		    blue != 0x46)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PolygonSymbolizerFillGraphicRemapColor #%02x%02x%02x !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->polygon_sym->fill->graphic->
			       color->red,
			       lyr->vector_style->polygon_sym->fill->graphic->
			       color->green,
			       lyr->vector_style->polygon_sym->fill->graphic->
			       color->blue);
		      ret = -97;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->stroke == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL PolygonSymbolizerStroke !!\n",
			       lyr->prefix, lyr->name);
		      ret = -98;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->stroke->red != 0x12 ||
		    lyr->vector_style->polygon_sym->stroke->green != 0x31 ||
		    lyr->vector_style->polygon_sym->stroke->blue != 0xfa)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PolygonSymbolizerStroke #%02x%02x%02x !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->polygon_sym->stroke->red,
			       lyr->vector_style->polygon_sym->stroke->green,
			       lyr->vector_style->polygon_sym->stroke->blue);
		      ret = -99;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->stroke->opacity != 0.8)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PolygonSymbolizerStrokeOpacity %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->polygon_sym->stroke->opacity);
		      ret = -100;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->stroke->width != 1.84)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PolygonSymbolizerStrokeWidth %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->polygon_sym->stroke->width);
		      ret = -101;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->stroke->dot_style != 2)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PolygonSymbolizerStrokeDotStyle %d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->polygon_sym->
			       stroke->dot_style);
		      ret = -102;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->displacement_x != 5.3)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PolygonSymbolizerDisplacementX %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->polygon_sym->displacement_x);
		      ret = -103;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->displacement_y != 15.6)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PolygonSymbolizerDisplacementY %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->polygon_sym->displacement_y);
		      ret = -104;
		      goto stop;
		  }
		if (lyr->vector_style->polygon_sym->perpendicular_offset !=
		    10.25)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected PolygonSymbolizerPerpendicularOffset %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->polygon_sym->
			       perpendicular_offset);
		      ret = -104;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "vector_6") == 0)
	    {
		/* testing TextSymbolizer_Point */
		text_point = 1;
		if (lyr->vector_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Vector Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -105;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TextSymbolizer !!\n",
			       lyr->prefix, lyr->name);
		      ret = -106;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->label == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TextSymbolizerLabel !!\n",
			       lyr->prefix, lyr->name);
		      ret = -107;
		      goto stop;
		  }
		if (strcmp (lyr->vector_style->text_sym->label, "@den_uff@") !=
		    0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerLabel \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->label);
		      ret = -108;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->font == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TextSymbolizerFont !!\n",
			       lyr->prefix, lyr->name);
		      ret = -109;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->font->family == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TextSymbolizerFontFamily !!\n",
			       lyr->prefix, lyr->name);
		      ret = -110;
		      goto stop;
		  }
		if (strcmp (lyr->vector_style->text_sym->font->family, "serif")
		    != 0)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerFontFamily \"%s\" !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->font->family);
		      ret = -111;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->font->style != 5102)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerFontStyle %d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->font->style);
		      ret = -112;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->font->weight != 5202)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerFontWeight %d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->font->weight);
		      ret = -113;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->font->size != 12.53)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerFontSize %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->font->size);
		      ret = -114;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TextSymbolizerPlacement !!\n",
			       lyr->prefix, lyr->name);
		      ret = -115;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->point == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TextSymbolizerPointPlacement !!\n",
			       lyr->prefix, lyr->name);
		      ret = -116;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->point->rotation !=
		    45.2)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerPointPlacementRotation %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->placement->
			       point->rotation);
		      ret = -117;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->point->anchor_x !=
		    0.3)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerPointPlacementAnchorX %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->placement->
			       point->anchor_x);
		      ret = -118;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->point->anchor_y !=
		    2.5)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerPointPlacementAnchorY %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->placement->
			       point->anchor_y);
		      ret = -119;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->
		    point->displacement_x != 10.1)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerPointPlacementDisplacementX %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->placement->
			       point->displacement_x);
		      ret = -120;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->
		    point->displacement_y != 0.32)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerPointPlacementDisplacementY %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->placement->
			       point->displacement_y);
		      ret = -121;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->halo == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TextSymbolizerHalo !!\n",
			       lyr->prefix, lyr->name);
		      ret = -122;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->halo->radius != 0.75)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerHaloRadius %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->halo->radius);
		      ret = -123;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->halo->fill == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TextSymbolizerHaloFill !!\n",
			       lyr->prefix, lyr->name);
		      ret = -124;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->halo->fill->red != 0xfa ||
		    lyr->vector_style->text_sym->halo->fill->green != 0xfb ||
		    lyr->vector_style->text_sym->halo->fill->blue != 0xfc)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerHaloFill #%02x%02x%02x !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->halo->fill->red,
			       lyr->vector_style->text_sym->halo->fill->green,
			       lyr->vector_style->text_sym->halo->fill->blue);
		      ret = -125;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->halo->fill->opacity != 0.33)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerHaloFillOpacity %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->halo->
			       fill->opacity);
		      ret = -126;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->fill == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TextSymbolizerFill !!\n",
			       lyr->prefix, lyr->name);
		      ret = -127;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->fill->red != 0x0a ||
		    lyr->vector_style->text_sym->fill->green != 0x0b ||
		    lyr->vector_style->text_sym->fill->blue != 0x0c)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerFill #%02x%02x%02x !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->fill->red,
			       lyr->vector_style->text_sym->fill->green,
			       lyr->vector_style->text_sym->fill->blue);
		      ret = -128;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->fill->opacity != 0.80)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerFillOpacity %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->fill->opacity);
		      ret = -129;
		      goto stop;
		  }
	    }
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "line_label_1") == 0)
	    {
		/* testing TextSymbolizer_Line */
		text_line = 1;
		if (lyr->vector_style == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL Vector Style !!\n",
			       lyr->prefix, lyr->name);
		      ret = -130;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TextSymbolizer !!\n",
			       lyr->prefix, lyr->name);
		      ret = -131;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TextSymbolizerPlacement !!\n",
			       lyr->prefix, lyr->name);
		      ret = -132;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->line == NULL)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected NULL TextSymbolizerLinePlacement !!\n",
			       lyr->prefix, lyr->name);
		      ret = -133;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->line->
		    perpendicular_offset != 1.32)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerLinePlacementPerpendicularOffset %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->placement->line->
			       perpendicular_offset);
		      ret = -134;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->line->repeated != 1)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerLinePlacementIsRepeated %d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->placement->line->
			       repeated);
		      ret = -135;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->line->initial_gap !=
		    5.25)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerLinePlacementInitialGap %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->placement->line->
			       initial_gap);
		      ret = -136;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->line->gap != 10.5)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerLinePlacementGap %1.2f !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->placement->line->
			       gap);
		      ret = -137;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->line->aligned != 1)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerLinePlacementIsAligned %d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->placement->line->
			       aligned);
		      ret = -138;
		      goto stop;
		  }
		if (lyr->vector_style->text_sym->placement->line->generalize !=
		    1)
		  {
		      fprintf (stderr,
			       "Layer <%s><%s> unexpected TextSymbolizerLinePlacementGeneralizeLine %d !!\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_sym->placement->line->
			       generalize);
		      ret = -139;
		      goto stop;
		  }
	    }
	  lyr = lyr->next;
      }

/* testing ATTACHED DATABASES */
    if (map->last_db == NULL)
      {
	  fprintf (stderr, "AttachedDatabases unexpected Last=NULL !!\n");
	  ret = -140;
	  goto stop;
      }
    if (map->last_db->prefix == NULL)
      {
	  fprintf (stderr, "AttachedDatabases unexpected Prefix=NULL !!\n");
	  ret = -141;
	  goto stop;
      }
    if (strcmp (map->last_db->prefix, "b") != 0)
      {
	  fprintf (stderr, "AttachedDatabases unexpected Prefix=\"%s\" !!\n",
		   map->last_db->prefix);
	  ret = -142;
	  goto stop;
      }
    if (map->last_db->path == NULL)
      {
	  fprintf (stderr, "AttachedDatabases unexpected Path=NULL !!\n");
	  ret = -143;
	  goto stop;
      }
    if (strcmp (map->last_db->path, "./test2.sqlite") != 0)
      {
	  fprintf (stderr, "AttachedDatabases unexpected Path=\"%s\" !!\n",
		   map->last_db->path);
	  ret = -144;
	  goto stop;
      }

/* testing MAP OPTIONS */
    if (map->multithread_enabled == 0)
      {
	  fprintf (stderr, "MapOptions unexpected MultithreadEnabled=%d !!\n",
		   map->multithread_enabled);
	  ret = -145;
	  goto stop;
      }
    if (map->max_threads != 8)
      {
	  fprintf (stderr, "MapOptions unexpected MaxThreads=%d !!\n",
		   map->max_threads);
	  ret = -145;
	  goto stop;
      }
    if (map->srid != 3003)
      {
	  fprintf (stderr, "MapOptions unexpected SRID=%d !!\n", map->srid);
	  ret = -146;
	  goto stop;
      }
    if (map->autotransform_enabled == 0)
      {
	  fprintf (stderr, "MapOptions unexpected AutoTransformEnabled=%d !!\n",
		   map->autotransform_enabled);
	  ret = -147;
	  goto stop;
      }
    if (map->dms != 0)
      {
	  fprintf (stderr, "MapOptions unexpected AutoTransformEnabled=%d !!\n",
		   map->dms);
	  ret = -148;
	  goto stop;
      }
    if (map->map_background_red != 0xfa || map->map_background_green != 0xec
	|| map->map_background_blue != 0xa8)
      {
	  fprintf (stderr,
		   "MapOptions unexpected MapBackground=\"#%02x%02x%02x\" !!\n",
		   map->map_background_red, map->map_background_green,
		   map->map_background_blue);
	  ret = -149;
	  goto stop;
      }
    if (map->map_background_transparent == 0)
      {
	  fprintf (stderr,
		   "MapOptions unexpected TransparentBackground=%d !!\n",
		   map->map_background_transparent);
	  ret = -150;
	  goto stop;
      }
    if (map->raster_wms_auto_switch != 0)
      {
	  fprintf (stderr,
		   "MapOptions unexpected Raster/WMS AutoSwitch=%d !!\n",
		   map->raster_wms_auto_switch);
	  ret = -151;
	  goto stop;
      }
    if (map->label_anti_collision != 0)
      {
	  fprintf (stderr, "MapOptions unexpected LabelAntiCollision=%d !!\n",
		   map->label_anti_collision);
	  ret = -152;
	  goto stop;
      }
    if (map->label_wrap_text == 0)
      {
	  fprintf (stderr, "MapOptions unexpected LabelWrapText=%d !!\n",
		   map->label_wrap_text);
	  ret = -153;
	  goto stop;
      }
    if (map->label_auto_rotate != 0)
      {
	  fprintf (stderr, "MapOptions unexpected LabelAutoRotate=%d !!\n",
		   map->label_auto_rotate);
	  ret = -154;
	  goto stop;
      }
    if (map->label_shift_position == 0)
      {
	  fprintf (stderr, "MapOptions unexpected LabelShiftPosition=%d !!\n",
		   map->label_shift_position);
	  ret = -155;
	  goto stop;
      }

/* testing MapBoundingBox */
    if (map->bbox == NULL)
      {
	  fprintf (stderr, "MapBoundingBox unexpected NULL !!\n");
	  ret = -156;
	  goto stop;
      }
    if (map->bbox->minx != 10.05)
      {
	  fprintf (stderr, "MapBoundingBox unexpected MinX=%1.6f\n",
		   map->bbox->minx);
	  ret = -157;
	  goto stop;
      }
    if (map->bbox->miny != 20.75)
      {
	  fprintf (stderr, "MapBoundingBox unexpected MinY=%1.6f\n",
		   map->bbox->miny);
	  ret = -158;
	  goto stop;
      }
    if (map->bbox->maxx != 15.12)
      {
	  fprintf (stderr, "MapBoundingBox unexpected MaxX=%1.6f\n",
		   map->bbox->maxx);
	  ret = -159;
	  goto stop;
      }
    if (map->bbox->maxy != 23.43)
      {
	  fprintf (stderr, "MapBoundingBox unexpected MaxY=%1.6f\n",
		   map->bbox->maxy);
	  ret = -160;
	  goto stop;
      }

/* testing TextSymbolizerAlone */
    lyr = map->first_lyr;
    while (lyr != NULL)
      {
	  int expected = 0;
	  if (strcmp (lyr->prefix, "main") == 0
	      && strcmp (lyr->name, "vector_4") == 0)
	      expected = 1;
	  if (lyr->vector_style != NULL)
	    {
		if (lyr->vector_style->text_alone != expected)
		  {
		      fprintf (stderr,
			       "VectorLayerStyle \"%s\".\"%s\" unexpected TextSymbolizerAlone=%d\n",
			       lyr->prefix, lyr->name,
			       lyr->vector_style->text_alone);
		      ret = -161;
		      goto stop;
		  }
	    }
	  lyr = lyr->next;
      }

/* testing Clone Map Config */
    clone = rl2_clone_map_config (map);
    if (clone == NULL)
      {
	  fprintf (stderr, "CloneMapConfig unexpected NULL\n");
	  ret = -162;
	  goto stop;
      }
    if (rl2_compare_map_configs (map, clone) == 0)
      {
	  fprintf (stderr, "CompareMapConfigs unexpected result: 0\n");
	  ret = -163;
	  goto stop;
      }
    rl2_destroy_map_config (clone);


  stop:
    if (map != NULL)
	rl2_destroy_map_config (map);

    if (!raster_1)
      {
	  fprintf (stderr, "Layer \"raster_1\" not found !!\n");
	  ret = -170;
      }
    if (!raster_2)
      {
	  fprintf (stderr, "Layer \"raster_2\" not found !!\n");
	  ret = -171;
      }
    if (!raster_3)
      {
	  fprintf (stderr, "Layer \"raster_3\" not found !!\n");
	  ret = -172;
      }
    if (!raster_4)
      {
	  fprintf (stderr, "Layer \"raster_4\" not found !!\n");
	  ret = -173;
      }
    if (!raster_5)
      {
	  fprintf (stderr, "Layer \"raster_5\" not found !!\n");
	  ret = -174;
      }
    if (!raster_6)
      {
	  fprintf (stderr, "Layer \"raster_6\" not found !!\n");
	  ret = -175;
      }
    if (!wms)
      {
	  fprintf (stderr, "Layer \"wms_layer_2\" not found !!\n");
	  ret = -176;
      }
    if (!vector_1)
      {
	  fprintf (stderr, "Layer \"vector_1\" not found !!\n");
	  ret = -177;
      }
    if (!vector_2)
      {
	  fprintf (stderr, "Layer \"vector_2\" not found !!\n");
	  ret = -178;
      }
    if (!vector_3)
      {
	  fprintf (stderr, "Layer \"vector_3\" not found !!\n");
	  ret = -179;
      }
    if (!topology_1)
      {
	  fprintf (stderr, "Layer \"topology_1\" not found !!\n");
	  ret = -180;
      }
    if (!network_1)
      {
	  fprintf (stderr, "Layer \"network_1\" not found !!\n");
	  ret = -181;
      }
    if (!point)
      {
	  fprintf (stderr, "Layer \"vector_6\" not found !!\n");
	  ret = -182;
      }
    if (!line)
      {
	  fprintf (stderr, "Layer \"vector_5\" not found !!\n");
	  ret = -183;
      }
    if (!polygon_1)
      {
	  fprintf (stderr, "Layer \"topology_2\" not found !!\n");
	  ret = -184;
      }
    if (!polygon_2)
      {
	  fprintf (stderr, "Layer \"vector_4\" not found !!\n");
	  ret = -185;
      }
    if (!text_point)
      {
	  fprintf (stderr, "Layer \"vector_6\" not found !!\n");
	  ret = -186;
      }
    if (!text_line)
      {
	  fprintf (stderr, "Layer \"line_label_1\" not found !!\n");
	  ret = -187;
      }

    *retcode = ret;
    return ret;
}

static unsigned char *
do_load_xml ()
{
/* loading the XML in-memory */
    const char *path = "./map_config.xml";
    unsigned char *xml = NULL;
    int n_bytes;
    int rd;
    FILE *in = NULL;
    in = fopen (path, "rb");
    if (in == NULL)
      {
	  fprintf (stderr, "Unable to open \"%s\"\n", path);
	  goto error;
      }
    if (fseek (in, 0, SEEK_END) < 0)
      {
	  fprintf (stderr, "Seek error\n");
	  goto error;
      }
    n_bytes = ftell (in);
    rewind (in);
    xml = malloc (n_bytes + 1);
    memset (xml, '\0', n_bytes + 1);
    rd = fread (xml, 1, n_bytes, in);
    fclose (in);
    in = NULL;
    if (rd != n_bytes)
      {
	  fprintf (stderr, "Read error\n");
	  goto error;
      }
    return xml;

  error:
    if (xml != NULL)
	free (xml);
    if (in != NULL)
	fclose (in);
    return NULL;
}

int
main (int argc, char *argv[])
{
    int result = 0;
    int ret;
    sqlite3 *db_handle;
    void *cache = spatialite_alloc_connection ();
    void *priv_data = rl2_alloc_private ();
    unsigned char *xml = NULL;

    if (argc > 1 || argv[0] == NULL)
	argc = 1;		/* silencing stupid compiler warnings */

/* opening and initializing the "memory" test DB */
    ret = sqlite3_open_v2 (":memory:", &db_handle, SQLITE_OPEN_READONLY, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "sqlite3_open_v2() error: %s\n",
		   sqlite3_errmsg (db_handle));
	  return -1;
      }
    spatialite_init_ex (db_handle, cache, 0);
    rl2_init (db_handle, priv_data, 0);

/* loading the XML document */
    xml = do_load_xml ();
    if (xml != NULL)
      {
	  /* test */
	  ret = test_map_config (xml, &ret);
	  if (ret < 0)
	      result = ret;
      }

/* closing the DB */
    sqlite3_close (db_handle);
    spatialite_cleanup_ex (cache);
    rl2_cleanup_private (priv_data);
    spatialite_shutdown ();
    if (xml != NULL)
	free (xml);
    return result;
}
