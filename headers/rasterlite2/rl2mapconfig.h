/*

 rl2mapconfig -- the RL2MapConfig struct

 version 0.1, 2020 July 4

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

/**
 \file rl2mapconfig.h

 RL2MapConfig header file
 */

#ifndef _RL2MAPCONFIG_H
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _RL2MAPCONFIG_H
#endif


#ifdef __cplusplus
extern "C"
{
#endif

#define EXT_QUICK_STYLE_SOLID_LINE		1
#define EXT_QUICK_STYLE_DOT_LINE		2
#define EXT_QUICK_STYLE_DASH_LINE		3
#define EXT_QUICK_STYLE_DASH_DOT_LINE	4

/**
 Container for ATTACHED DB
 */
    typedef struct rl2_map_attached_db
    {
/** DB Prefix (NULL = 'MAIN') */
	char *prefix;
/** DB Path */
	char *path;
/** pointer to next item [linked list] */
	struct rl2_map_attached_db *next;
    } rl2MapAttachedDb;
/**
 Typedef for ATTACHED DB

 \sa rl2MapAttachedDb
 */
    typedef rl2MapAttachedDb *rl2MapAttachedDbPtr;

/**
 Container for RGB Color
 */
    typedef struct rl2_map_color
    {
/** RED component */
	unsigned char red;
/** GREEN component */
	unsigned char green;
/** BLUE component */
	unsigned char blue;
    } rl2MapColor;
/**
 Typedef for RGB Color

 \sa rl2MapColor
 */
    typedef rl2MapColor *rl2MapColorPtr;

/**
 Container for Channel Selection
 */
    typedef struct rl2_map_channel_selection
    {
/** RGB type */
	int rgb;
/** Red channel */
	int red_channel;
/** Green channel */
	int green_channel;
/** Blue channel */
	int blue_channel;
/** Gray channel */
	int gray_channel;
    } rl2MapChannelSelection;
/**
 Typedef for Channel Selection

 \sa rl2MapChannelSelection
 */
    typedef rl2MapChannelSelection *rl2MapChannelSelectionPtr;

/**
 Container for Contrast Enhancement
 */
    typedef struct rl2_map_contrast_enhancement
    {
/** Normalize method */
	int normalize;
/** Histogram method */
	int histogram;
/** Gamma Value method */
	int gamma;
/** Gamma Value factor */
	double gamma_value;
    } rl2MapContrastEnhancement;
/**
 Typedef for Contrast Enhancement

 \sa rl2MapContrastEnhancement
 */
    typedef rl2MapContrastEnhancement *rl2MapContrastEnhancementPtr;

/**
 Container for Color Ramp
 */
    typedef struct rl2_map_color_ramp
    {
/** Minimum Value */
	double min_value;
/** Maximum Value */
	double max_value;
/** Minimum Color */
	char *min_color;
/** Maximum Color */
	char *max_color;
    } rl2MapColorRamp;
/**
 Typedef for Color Ramp

 \sa rl2MapColorRamp
 */
    typedef rl2MapColorRamp *rl2MapColorRampPtr;

/** 
 Container for Raster Layer Style
 */
    typedef struct rl2_map_raster_layer_style
    {
/** Opacity */
	double opacity;
/** Channel Selection */
	rl2MapChannelSelectionPtr channel_selection;
/** Color Map Name */
	char *color_map_name;
/** Color Ramp */
	rl2MapColorRampPtr color_ramp;
/** Contrast Enhancement */
	rl2MapContrastEnhancementPtr contrast_enhancement;
/** Shaded Relief Enabled */
	int shaded_relief;
/** Shaded Relief Factor */
	double relief_factor;
    } rl2MapRasterLayerStyle;
/**
 Typedef for Raster Layer Style
 */
    typedef rl2MapRasterLayerStyle *rl2MapRasterLayerStylePtr;

/**
 Container for Graphic Fill
 */
    typedef struct rl2_map_graphic_fill
    {
/** resource pseudo-URL */
	char *resource;
/** MIME format */
	char *format;
/** Remap Color */
	rl2MapColorPtr color;
    } rl2MapGraphicFill;
/**
 Typedef for Graphic Fill

 \sa rl2MapGraphicFill
 */
    typedef rl2MapGraphicFill *rl2MapGraphicFillPtr;

/**
 Container for Fill
 */
    typedef struct rl2_map_fill
    {
/** Graphic Fill (may be NULL) */
	rl2MapGraphicFillPtr graphic;
/** RED component */
	unsigned char red;
/** GREEN component */
	unsigned char green;
/** BLUE component */
	unsigned char blue;
/** Opacity */
	double opacity;
    } rl2MapFill;
/**
 Typedef for Fill

 \sa rl2MapFill
 */
    typedef rl2MapFill *rl2MapFillPtr;

/**
 Container for Stroke
 */
    typedef struct rl2_map_stroke
    {
/** RED component */
	unsigned char red;
/** GREEN component */
	unsigned char green;
/** BLUE component */
	unsigned char blue;
/** Opacity */
	double opacity;
/** Pen Width */
	double width;
/** Dot Style */
	int dot_style;
    } rl2MapStroke;
/**
 Typedef for Stroke

 \sa rl2MapStroke
 */
    typedef rl2MapStroke *rl2MapStrokePtr;

/**
 Container for Mark
 */
    typedef struct rl2_map_mark
    {
/** Well Known Name */
	int type;
/** Fill (may be NULL) */
	rl2MapFillPtr fill;
/** Stroke (may be NULL) */
	rl2MapStrokePtr stroke;
    } rl2MapMark;
/**
 Typedef for Mark

 \sa rl2MapMark
 */
    typedef rl2MapMark *rl2MapMarkPtr;

/**
 Container for Point Symbolizer
 */
    typedef struct rl2_map_point_symbolizer
    {
/** Mark (may be NULL) */
	rl2MapMarkPtr mark;
/** ExternalGraphic (may be NULL) */
	rl2MapGraphicFillPtr graphic;
/** Opacity */
	double opacity;
/** Symbol Size */
	double size;
/** Symbol Anchor X */
	double anchor_x;
/** Symbol Anchor Y */
	double anchor_y;
/** Symbol Displacement X */
	double displacement_x;
/** Symbol Displacement Y */
	double displacement_y;
/** Symbol Rotation */
	double rotation;
    } rl2MapPointSymbolizer;
/**
 Typedef for Point Symbolizer

 \sa rl2MapPointSymbolizer
 */
    typedef rl2MapPointSymbolizer *rl2MapPointSymbolizerPtr;

/**
 Container for Line Symbolizer
 */
    typedef struct rl2_map_line_symbolizer
    {
/** Stroke (may be NULL) */
	rl2MapStrokePtr stroke;
/** Perpendicular offset */
	double perpendicular_offset;
	struct rl2_map_line_symbolizer *next;
    } rl2MapLineSymbolizer;
/**
 Typedef for Line Symbolizer

 \sa rl2MapLineSymbolizer
 */
    typedef rl2MapLineSymbolizer *rl2MapLineSymbolizerPtr;

/**
 Container for Polygon Symbolizer
 */
    typedef struct rl2_map_polygon_symbolizer
    {
/** Fill (may be NULL) */
	rl2MapFillPtr fill;
/** Stroke (may be NULL) */
	rl2MapStrokePtr stroke;
/** X Displacement */
	double displacement_x;
/** Y Displacement */
	double displacement_y;
/** Perpendicular offset */
	double perpendicular_offset;
    } rl2MapPolygonSymbolizer;
/**
 Typedef for Polygon Symbolizer

 \sa rl2MapPolygonSymbolizer
 */
    typedef rl2MapPolygonSymbolizer *rl2MapPolygonSymbolizerPtr;

/**
 Container for Font
 */
    typedef struct rl2_map_font
    {
/** Font family name */
	char *family;
/** Font style */
	int style;
/** Font Weight */
	int weight;
/** Font size */
	double size;
    } rl2MapFont;
/**
 Typedef for Font

 \sa rl2MapFont
 */
    typedef rl2MapFont *rl2MapFontPtr;

/**
 Container for Point Placement
 */
    typedef struct rl2_map_point_placement
    {
/** Label Anchor X */
	double anchor_x;
/** Label Anchor Y */
	double anchor_y;
/** Label Displacement X */
	double displacement_x;
/** Label Displacement Y */
	double displacement_y;
/** Label Rotation */
	double rotation;
    } rl2MapPointPlacement;
/**
 Typedef for Point Placement

 \sa rl2MapPointPlacement
 */
    typedef rl2MapPointPlacement *rl2MapPointPlacementPtr;

/**
 Container for Line Placement
 */
    typedef struct rl2_map_line_placement
    {
/** Perpendicular offset */
	double perpendicular_offset;
/** Repeated Label */
	int repeated;
/** Label Initial Gap */
	double initial_gap;
/** Label Gap */
	double gap;
/** Label is Aligned */
	int aligned;
/** Label Generalize Line */
	int generalize;
    } rl2MapLinePlacement;
/**
 Typedef for Line Placement

 \sa rl2MapLinePlacement
 */
    typedef rl2MapLinePlacement *rl2MapLinePlacementPtr;

/**
 Container for Label Placement
 */
    typedef struct rl2_map_placement
    {
/** Point Placement */
	rl2MapPointPlacementPtr point;
/** Line Placement */
	rl2MapLinePlacementPtr line;
    } rl2MapPlacement;
/**
 Typedef for Label Placement

 \sa rl2MapPlacement
 */
    typedef rl2MapPlacement *rl2MapPlacementPtr;

/**
 Container for Halo
 */
    typedef struct rl2_map_halo
    {
/** Halo */
	double radius;
/** Fill */
	rl2MapFillPtr fill;
    } rl2MapHalo;
/**
 Typedef for Halo

 \sa rl2MapHalo
 */
    typedef rl2MapHalo *rl2MapHaloPtr;

/**
 Container for Text Symbolizer
 */
    typedef struct rl2_map_text_symbolizer
    {
/** Label */
	char *label;
/** Font */
	rl2MapFontPtr font;
/** Label Placement */
	rl2MapPlacementPtr placement;
/** Halo (may be NULL) */
	rl2MapHaloPtr halo;
/** Fill */
	rl2MapFillPtr fill;
/** Text Symbolizer alone (ignore Geometry) */
	int alone;
    } rl2MapTextSymbolizer;
/**
 Typedef for Text Symbolizer

 \sa rl2MapTextSymbolizer
 */
    typedef rl2MapTextSymbolizer *rl2MapTextSymbolizerPtr;

/** 
 Container for Vector Layer Style
 */
    typedef struct rl2_map_vector_layer_style
    {
/** Point Symbolizer (may be NULL) */
	rl2MapPointSymbolizerPtr point_sym;
/** pointer to first Line Symbolizer [linked list]; may be NULL */
	rl2MapLineSymbolizerPtr first_line_sym;
/** pointer to last Line Symbolizer [linked list]; may be NULL */
	rl2MapLineSymbolizerPtr last_line_sym;
/** Polygon Symbolizer (may be NULL) */
	rl2MapPolygonSymbolizerPtr polygon_sym;
/** Text Symbolizer (may be NULL) */
	rl2MapTextSymbolizerPtr text_sym;
    } rl2MapVectorLayerStyle;
/**
 Typedef for Vector Layer Style
 */
    typedef rl2MapVectorLayerStyle *rl2MapVectorLayerStylePtr;

/** 
 Container for Topology Layer Style
 */
    typedef struct rl2_map_topology_layer_style
    {
/** Faces Visibility */
	int show_faces;
/** Edges Visibility */
	int show_edges;
/** Nodes Visibility */
	int show_nodes;
/** Edge Seeds Visibility */
	int show_edge_seeds;
/** Face Seeds Visibility */
	int show_face_seeds;
/** Faces Symbolizer [Polygon] (may be NULL) */
	rl2MapPolygonSymbolizerPtr faces_sym;
/** Edges Symbolizer [Line] (may be NULL) */
	rl2MapLineSymbolizerPtr edges_sym;
/** Nodes Symbolizer [Point] (may be NULL) */
	rl2MapPointSymbolizerPtr nodes_sym;
/** Edge Seeds Symbolizer [Point] (may be NULL) */
	rl2MapPointSymbolizerPtr edge_seeds_sym;
/** Face Seeds Symbolizer [Point] (may be NULL) */
	rl2MapPointSymbolizerPtr face_seeds_sym;
    } rl2MapTopologyLayerStyle;
/**
 Typedef for Topology Layer Style
 */
    typedef rl2MapTopologyLayerStyle *rl2MapTopologyLayerStylePtr;

/** 
 Container for Topology Layer Internal Style
 */
    typedef struct rl2_map_topology_layer_internal_style
    {
/** Layer Internal Style Name */
	char *style_internal_name;
/** Faces Visibility */
	int show_faces;
/** Edges Visibility */
	int show_edges;
/** Nodes Visibility */
	int show_nodes;
/** Edge Seeds Visibility */
	int show_edge_seeds;
/** Face Seeds Visibility */
	int show_face_seeds;
    } rl2MapTopologyLayerInternalStyle;
/**
 Typedef for Topology Layer Internal Style
 */
    typedef rl2MapTopologyLayerInternalStyle
	* rl2MapTopologyLayerInternalStylePtr;

/** 
 Container for Network Layer Style
 */
    typedef struct rl2_map_network_layer_style
    {
/** Links Visibility */
	int show_links;
/** Nodes Visibility */
	int show_nodes;
/** Link Seeds Visibility */
	int show_link_seeds;
/** Links Symbolizer [Line] (may be NULL) */
	rl2MapLineSymbolizerPtr links_sym;
/** Nodes Symbolizer [Point] (may be NULL) */
	rl2MapPointSymbolizerPtr nodes_sym;
/** Link Seeds Symbolizer [Point] (may be NULL) */
	rl2MapPointSymbolizerPtr link_seeds_sym;
    } rl2MapNetworkLayerStyle;
/**
 Typedef for Network Layer Style
 */
    typedef rl2MapNetworkLayerStyle *rl2MapNetworkLayerStylePtr;

/** 
 Container for Network Layer Internal Style
 */
    typedef struct rl2_map_network_layer_internal_style
    {
/** Layer Internal Style Name */
	char *style_internal_name;
/** Links Visibility */
	int show_links;
/** Nodes Visibility */
	int show_nodes;
/** Link Seeds Visibility */
	int show_link_seeds;
    } rl2MapNetworkLayerInternalStyle;
/**
 Typedef for Network Layer Internal Style
 */
    typedef rl2MapNetworkLayerInternalStyle *rl2MapNetworkLayerInternalStylePtr;

/** 
 Container for WMS Layer Style
 */
    typedef struct rl2_map_wms_layer_style
    {
/** GetMap URL */
	char *get_map_url;
/** GetFeatureInfo URL */
	char *get_feature_info_url;
/** WMS Protocol */
	char *wms_protocol;
/** CRS */
	char *crs;
/** Swap XY axes */
	int swap_xy;
/** Style */
	char *style;
/** MIME Image Format */
	char *image_format;
/** Opaque image */
	int opaque;
/** Background color */
	char *background_color;
/** Tiled */
	int is_tiled;
/** Tile Width */
	int tile_width;
/** Tile Height */
	int tile_height;
    } rl2MapWmsLayerStyle;
/**
 Typedef for WMS Layer Style
 */
    typedef rl2MapWmsLayerStyle *rl2MapWmsLayerStylePtr;

/**
 Container for Map Layer
 */
    typedef struct rl2_map_layer
    {
/** Layer type */
	int type;
/** DB Prefix (NULL = 'MAIN') */
	char *prefix;
/** Coverage name */
	char *name;
/** Visible / Hidden */
	int visible;
/** MinScale Enabled */
	int ok_min_scale;
/** MinScale Denominator */
	double min_scale;
/** MaxScale Enabled */
	int ok_max_scale;
/** MaxScale Denominator */
	double max_scale;
/** Raster Layer Internal Style Name */
	char *raster_style_internal_name;
/** Vector Layer Internal Style Name */
	char *vector_style_internal_name;
/** Raster Layer Style (may be NULL) */
	rl2MapRasterLayerStylePtr raster_style;
/** Vector Layer Style (may be NULL) */
	rl2MapVectorLayerStylePtr vector_style;
/** Topology Layer Style (may be NULL) */
	rl2MapTopologyLayerStylePtr topology_style;
/** Topology Layer Internal Style (may be NULL) */
	rl2MapTopologyLayerInternalStylePtr topology_internal_style;
/** Network Layer Style (may be NULL) */
	rl2MapNetworkLayerStylePtr network_style;
/** Network Layer Internal Style (may be NULL) */
	rl2MapNetworkLayerInternalStylePtr network_internal_style;
/** WMS Layer Style (may be NULL) */
	rl2MapWmsLayerStylePtr wms_style;
/** pointer to next item [linked list] */
	struct rl2_map_layer *next;
    } rl2MapLayer;
/**
 Typedef for Map Layer

 \sa rl2MapLayer
 */
    typedef rl2MapLayer *rl2MapLayerPtr;

/**
 Container for Map Config
 */
    typedef struct rl2_map_config
    {
/** Configuration Name */
	char *name;
/** Title */
	char *title;
/** Abstract */
	char *abstract;
/** Multithread rendering enabled */
	int multithread_enabled;
/** Number of concurrent threads */
	int max_threads;
/** Map SRID */
	int srid;
/** Auto Transform enabled */
	int autotransform_enabled;
/** DMS or DD geographic coordinates */
	int dms;
/** Map background - RED component */
	unsigned char map_background_red;
/** Map background - GREEN component */
	unsigned char map_background_green;
/** Map background - BLUE component */
	unsigned char map_background_blue;
/** Map background is Transparent */
	int map_background_transparent;
/** Raster/WMS auto-switch is enabled */
	int raster_wms_auto_switch;
/** Label Anti Collision is enabled */
	int label_anti_collision;
/** Label Wrap Text is enabled */
	int label_wrap_text;
/** Label Auto Rotate is enabled */
	int label_auto_rotate;
/** Label Shift Position is enabled */
	int label_shift_position;
/** pointer to first ATTACHED DB [linked list]; may be NULL */
	rl2MapAttachedDbPtr first_db;
/** pointer to last ATTACHED DB [linked list]; may be NULL */
	rl2MapAttachedDbPtr last_db;
/** pointer to first MAP LAYER [linked list]; may be NULL */
	rl2MapLayerPtr first_lyr;
/** pointer to last MAP LAYER [linked list]; may be NULL */
	rl2MapLayerPtr last_lyr;
    } rl2MapConfig;
/**
 Typedef for MapC onfig

 \sa rl2MapLayer
 */
    typedef rl2MapConfig *rl2MapConfigPtr;

    RL2_DECLARE rl2MapConfigPtr rl2_parse_map_config_xml (const unsigned char
							  *xml);

    RL2_DECLARE void rl2_destroy_map_config (rl2MapConfigPtr map_config);

#ifdef __cplusplus
}
#endif

#endif				/* _RL2MAPCONFIG_H */
