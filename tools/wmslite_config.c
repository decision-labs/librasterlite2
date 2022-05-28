/* 
/ wmslite_config
/
/ a light-weight WMS server / GCI supporting RasterLite2 DataSources
/ parsing the working configuration 
/
/ version 2.0, 2021 March 2
/
/ Author: Sandro Furieri a.furieri@lqt.it
/
/ Copyright (C) 2021  Alessandro Furieri
/
/    This program is free software: you can redistribute it and/or modify
/    it under the terms of the GNU General Public License as published by
/    the Free Software Foundation, either version 3 of the License, or
/    (at your option) any later version.
/
/    This program is distributed in the hope that it will be useful,
/    but WITHOUT ANY WARRANTY; without even the implied warranty of
/    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/    GNU General Public License for more details.
/
/    You should have received a copy of the GNU General Public License
/    along with this program.  If not, see <http://www.gnu.org/licenses/>.
/
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "wmslite.h"

#include <libxml/parser.h>

static void
destroy_wmslite_child (WmsLiteChildLayerPtr child)
{
/* memory cleanup - freeing a WmsLite Child Layer reference */
    if (child->ChildAliasName != NULL)
	free (child->ChildAliasName);
    free (child);
}

static WmsLiteKeywordPtr
add_wmslite_keyword (WmsLiteConfigPtr list, const char *keyword)
{
/* creating a WmsLite Keyword */
    int len = strlen (keyword);
    WmsLiteKeywordPtr key = malloc (sizeof (WmsLiteKeyword));
    key->Keyword = malloc (len + 1);
    strcpy (key->Keyword, keyword);
    key->Next = NULL;
    if (list->KeyFirst == NULL)
	list->KeyFirst = key;
    if (list->KeyLast != NULL)
	list->KeyLast->Next = key;
    list->KeyLast = key;
    return key;
}

static void
destroy_wmslite_keyword (WmsLiteKeywordPtr key)
{
/* memory cleanup - freeing a WmsLite Keyword */
    if (key->Keyword != NULL)
	free (key->Keyword);
    free (key);
}

extern WmsLiteBBoxPtr
create_wmslite_bbox (double minx, double miny, double maxx, double maxy)
{
/* creating a WmsLite BBox */
    WmsLiteBBoxPtr bbox = malloc (sizeof (WmsLiteBBox));
    bbox->MinX = minx;
    bbox->MinY = miny;
    bbox->MaxX = maxx;
    bbox->MaxY = maxy;
    return bbox;
}

static void
destroy_wmslite_crs (WmsLiteCrsPtr crs)
{
/* memory cleanup - freeing a WmsLite CRS */
    if (crs->BBox != NULL)
	free (crs->BBox);
    free (crs);
}

static void
destroy_wmslite_wms_crs (WmsLiteWmsCrsPtr crs)
{
/* memory cleanup - freeing a WmsLite WMS CRS */
    if (crs->Srs != NULL)
	free (crs->Srs);
    if (crs->BBox != NULL)
	free (crs->BBox);
    free (crs);
}

static void
destroy_wmslite_style (WmsLiteStylePtr style)
{
/* memory cleanup - freeing a WmsLite Style */
    if (style->Name != NULL)
	free (style->Name);
    if (style->Title != NULL)
	free (style->Title);
    if (style->Abstract != NULL)
	free (style->Abstract);
    free (style);
}

extern WmsLiteCascadedWmsPtr
create_wmslite_cascaded_wms (int id, const char *title, const char *abstract,
			     int is_transparent, int is_queryable, int cascaded)
{
/* creating a WmsLite CascadedWms Layer */
    int len;
    WmsLiteCascadedWmsPtr lyr = malloc (sizeof (WmsLiteCascadedWms));
    lyr->Id = id;
    if (title == NULL)
	lyr->Title = NULL;
    else
      {
	  len = strlen (title);
	  lyr->Title = malloc (len + 1);
	  strcpy (lyr->Title, title);
      }
    if (abstract == NULL)
	lyr->Abstract = NULL;
    else
      {
	  len = strlen (abstract);
	  lyr->Abstract = malloc (len + 1);
	  strcpy (lyr->Abstract, abstract);
      }
    lyr->GeographicBBox = NULL;
    lyr->IsQueryable = is_queryable;
    lyr->IsTransparent = is_transparent;
    lyr->Cascaded = cascaded;
    lyr->KeyFirst = NULL;
    lyr->KeyLast = NULL;
    lyr->CrsFirst = NULL;
    lyr->CrsLast = NULL;
    lyr->StyleFirst = NULL;
    lyr->StyleLast = NULL;
    return lyr;
}

extern void
destroy_wmslite_cascaded_wms (WmsLiteCascadedWmsPtr lyr)
{
/* memory cleanup - freeing a WmsLite Cascaded WMS Layer */
    WmsLiteKeywordPtr pK;
    WmsLiteKeywordPtr pKn;
    WmsLiteWmsCrsPtr pC;
    WmsLiteWmsCrsPtr pCn;
    WmsLiteStylePtr pS;
    WmsLiteStylePtr pSn;
    if (lyr->Title != NULL)
	free (lyr->Title);
    if (lyr->Abstract != NULL)
	free (lyr->Abstract);
    if (lyr->GeographicBBox != NULL)
	free (lyr->GeographicBBox);
    pK = lyr->KeyFirst;
    while (pK != NULL)
      {
	  pKn = pK->Next;
	  destroy_wmslite_keyword (pK);
	  pK = pKn;
      }
    pC = lyr->CrsFirst;
    while (pC != NULL)
      {
	  pCn = pC->Next;
	  destroy_wmslite_wms_crs (pC);
	  pC = pCn;
      }
    pS = lyr->StyleFirst;
    while (pS != NULL)
      {
	  pSn = pS->Next;
	  destroy_wmslite_style (pS);
	  pS = pSn;
      }
    free (lyr);
}

extern WmsLiteRasterPtr
create_wmslite_raster (const char *title, const char *abstract, int srid,
		       int is_queryable)
{
/* creating a WmsLite Raster Layer */
    int len;
    WmsLiteRasterPtr lyr = malloc (sizeof (WmsLiteRaster));
    if (title == NULL)
	lyr->Title = NULL;
    else
      {
	  len = strlen (title);
	  lyr->Title = malloc (len + 1);
	  strcpy (lyr->Title, title);
      }
    if (abstract == NULL)
	lyr->Abstract = NULL;
    else
      {
	  len = strlen (abstract);
	  lyr->Abstract = malloc (len + 1);
	  strcpy (lyr->Abstract, abstract);
      }
    lyr->Srid = srid;
    lyr->GeographicBBox = NULL;
    lyr->IsQueryable = is_queryable;
    lyr->KeyFirst = NULL;
    lyr->KeyLast = NULL;
    lyr->CrsFirst = NULL;
    lyr->CrsLast = NULL;
    lyr->StyleFirst = NULL;
    lyr->StyleLast = NULL;
    return lyr;
}

extern void
destroy_wmslite_raster (WmsLiteRasterPtr lyr)
{
/* memory cleanup - freeing a WmsLite RasterLayer */
    WmsLiteKeywordPtr pK;
    WmsLiteKeywordPtr pKn;
    WmsLiteCrsPtr pC;
    WmsLiteCrsPtr pCn;
    WmsLiteStylePtr pS;
    WmsLiteStylePtr pSn;
    if (lyr->Title != NULL)
	free (lyr->Title);
    if (lyr->Abstract != NULL)
	free (lyr->Abstract);
    if (lyr->GeographicBBox != NULL)
	free (lyr->GeographicBBox);
    pK = lyr->KeyFirst;
    while (pK != NULL)
      {
	  pKn = pK->Next;
	  destroy_wmslite_keyword (pK);
	  pK = pKn;
      }
    pC = lyr->CrsFirst;
    while (pC != NULL)
      {
	  pCn = pC->Next;
	  destroy_wmslite_crs (pC);
	  pC = pCn;
      }
    pS = lyr->StyleFirst;
    while (pS != NULL)
      {
	  pSn = pS->Next;
	  destroy_wmslite_style (pS);
	  pS = pSn;
      }
    free (lyr);
}

extern WmsLiteCrsPtr
add_raster_crs (WmsLiteRasterPtr raster, int srid, double minx, double miny,
		double maxx, double maxy)
{
/* adding a WmsLite CRS definition to some Raster Layer */
    WmsLiteCrsPtr crs = malloc (sizeof (WmsLiteCrs));
    crs->Srid = srid;
    crs->BBox = malloc (sizeof (WmsLiteBBox));
    crs->BBox->MinX = minx;
    crs->BBox->MinY = miny;
    crs->BBox->MaxX = maxx;
    crs->BBox->MaxY = maxy;
    crs->Next = NULL;
    if (raster->CrsFirst == NULL)
	raster->CrsFirst = crs;
    if (raster->CrsLast != NULL)
	raster->CrsLast->Next = crs;
    raster->CrsLast = crs;
    return crs;
}

extern WmsLiteKeywordPtr
add_raster_keyword (WmsLiteRasterPtr raster, const char *keyword)
{
/* adding a Keyword to some Raster Layer */
    int len;
    WmsLiteKeywordPtr key = malloc (sizeof (WmsLiteKeyword));
    len = strlen (keyword);
    key->Keyword = malloc (len + 1);
    strcpy (key->Keyword, keyword);
    key->Next = NULL;
    if (raster->KeyFirst == NULL)
	raster->KeyFirst = key;
    if (raster->KeyLast != NULL)
	raster->KeyLast->Next = key;
    raster->KeyLast = key;
    return key;
}

extern WmsLiteStylePtr
add_raster_style (WmsLiteRasterPtr raster, const char *name, const char *title,
		  const char *abstract)
{
/* adding an SLD/SE Style to some Raster Layer */
    int len;
    WmsLiteStylePtr style = malloc (sizeof (WmsLiteStyle));
    len = strlen (name);
    style->Name = malloc (len + 1);
    strcpy (style->Name, name);
    if (title == NULL)
	style->Title = NULL;
    else
      {
	  len = strlen (title);
	  style->Title = malloc (len + 1);
	  strcpy (style->Title, title);
      }
    if (abstract == NULL)
	style->Abstract = NULL;
    else
      {
	  len = strlen (abstract);
	  style->Abstract = malloc (len + 1);
	  strcpy (style->Abstract, abstract);
      }
    style->Next = NULL;
    if (raster->StyleFirst == NULL)
	raster->StyleFirst = style;
    if (raster->StyleLast != NULL)
	raster->StyleLast->Next = style;
    raster->StyleLast = style;
    return style;
}

extern WmsLiteVectorPtr
create_wmslite_vector (const char *f_table_name, const char *f_geometry_column,
		       const char *view_name, const char *view_geometry,
		       const char *virt_name, const char *virt_geometry,
		       const char *topology_name, const char *network_name,
		       const char *title, const char *abstract,
		       int is_queryable)
{
/* creating a WmsLite Vector Layer */
    int len;
    WmsLiteVectorPtr lyr = malloc (sizeof (WmsLiteVector));
    if (f_table_name == NULL)
	lyr->FTableName = NULL;
    else
      {
	  len = strlen (f_table_name);
	  lyr->FTableName = malloc (len + 1);
	  strcpy (lyr->FTableName, f_table_name);
      }
    if (f_geometry_column == NULL)
	lyr->FGeometryColumn = NULL;
    else
      {
	  len = strlen (f_geometry_column);
	  lyr->FGeometryColumn = malloc (len + 1);
	  strcpy (lyr->FGeometryColumn, f_geometry_column);
      }
    if (view_name == NULL)
	lyr->ViewName = NULL;
    else
      {
	  len = strlen (view_name);
	  lyr->ViewName = malloc (len + 1);
	  strcpy (lyr->ViewName, view_name);
      }
    if (view_geometry == NULL)
	lyr->ViewGeometry = NULL;
    else
      {
	  len = strlen (view_geometry);
	  lyr->ViewGeometry = malloc (len + 1);
	  strcpy (lyr->ViewGeometry, view_geometry);
      }
    if (virt_name == NULL)
	lyr->VirtName = NULL;
    else
      {
	  len = strlen (virt_name);
	  lyr->VirtName = malloc (len + 1);
	  strcpy (lyr->VirtName, virt_name);
      }
    if (virt_geometry == NULL)
	lyr->VirtGeometry = NULL;
    else
      {
	  len = strlen (virt_geometry);
	  lyr->VirtGeometry = malloc (len + 1);
	  strcpy (lyr->ViewGeometry, view_geometry);
      }
    if (topology_name == NULL)
	lyr->TopologyName = NULL;
    else
      {
	  len = strlen (topology_name);
	  lyr->TopologyName = malloc (len + 1);
	  strcpy (lyr->TopologyName, topology_name);
      }
    if (network_name == NULL)
	lyr->NetworkName = NULL;
    else
      {
	  len = strlen (network_name);
	  lyr->NetworkName = malloc (len + 1);
	  strcpy (lyr->NetworkName, network_name);
      }
    if (title == NULL)
	lyr->Title = NULL;
    else
      {
	  len = strlen (title);
	  lyr->Title = malloc (len + 1);
	  strcpy (lyr->Title, title);
      }
    if (abstract == NULL)
	lyr->Abstract = NULL;
    else
      {
	  len = strlen (abstract);
	  lyr->Abstract = malloc (len + 1);
	  strcpy (lyr->Abstract, abstract);
      }
    lyr->Srid = -1;
    lyr->GeographicBBox = NULL;
    lyr->IsQueryable = is_queryable;
    lyr->KeyFirst = NULL;
    lyr->KeyLast = NULL;
    lyr->CrsFirst = NULL;
    lyr->CrsLast = NULL;
    lyr->StyleFirst = NULL;
    lyr->StyleLast = NULL;
    return lyr;
}

extern void
destroy_wmslite_vector (WmsLiteVectorPtr lyr)
{
/* memory cleanup - freeing a WmsLite VectorLayer */
    WmsLiteKeywordPtr pK;
    WmsLiteKeywordPtr pKn;
    WmsLiteCrsPtr pC;
    WmsLiteCrsPtr pCn;
    WmsLiteStylePtr pS;
    WmsLiteStylePtr pSn;
    if (lyr->FTableName != NULL)
	free (lyr->FTableName);
    if (lyr->FGeometryColumn != NULL)
	free (lyr->FGeometryColumn);
    if (lyr->ViewName != NULL)
	free (lyr->ViewName);
    if (lyr->ViewGeometry != NULL)
	free (lyr->ViewGeometry);
    if (lyr->VirtName != NULL)
	free (lyr->VirtName);
    if (lyr->VirtGeometry != NULL)
	free (lyr->VirtGeometry);
    if (lyr->TopologyName != NULL)
	free (lyr->TopologyName);
    if (lyr->NetworkName != NULL)
	free (lyr->NetworkName);
    if (lyr->Title != NULL)
	free (lyr->Title);
    if (lyr->Abstract != NULL)
	free (lyr->Abstract);
    if (lyr->GeographicBBox != NULL)
	free (lyr->GeographicBBox);
    pK = lyr->KeyFirst;
    while (pK != NULL)
      {
	  pKn = pK->Next;
	  destroy_wmslite_keyword (pK);
	  pK = pKn;
      }
    pC = lyr->CrsFirst;
    while (pC != NULL)
      {
	  pCn = pC->Next;
	  destroy_wmslite_crs (pC);
	  pC = pCn;
      }
    pS = lyr->StyleFirst;
    while (pS != NULL)
      {
	  pSn = pS->Next;
	  destroy_wmslite_style (pS);
	  pS = pSn;
      }
    free (lyr);
}

extern WmsLiteCrsPtr
add_vector_crs (WmsLiteVectorPtr vector, int srid, double minx, double miny,
		double maxx, double maxy)
{
/* adding a WmsLite CRS definition to some Vector Layer */
    WmsLiteCrsPtr crs = malloc (sizeof (WmsLiteCrs));
    crs->Srid = srid;
    crs->BBox = malloc (sizeof (WmsLiteBBox));
    crs->BBox->MinX = minx;
    crs->BBox->MinY = miny;
    crs->BBox->MaxX = maxx;
    crs->BBox->MaxY = maxy;
    crs->Next = NULL;
    if (vector->CrsFirst == NULL)
	vector->CrsFirst = crs;
    if (vector->CrsLast != NULL)
	vector->CrsLast->Next = crs;
    vector->CrsLast = crs;
    return crs;
}

extern WmsLiteKeywordPtr
add_vector_keyword (WmsLiteVectorPtr vector, const char *keyword)
{
/* adding a Keyword to some Vector Layer */
    int len;
    WmsLiteKeywordPtr key = malloc (sizeof (WmsLiteKeyword));
    len = strlen (keyword);
    key->Keyword = malloc (len + 1);
    strcpy (key->Keyword, keyword);
    key->Next = NULL;
    if (vector->KeyFirst == NULL)
	vector->KeyFirst = key;
    if (vector->KeyLast != NULL)
	vector->KeyLast->Next = key;
    vector->KeyLast = key;
    return key;
}

extern WmsLiteStylePtr
add_vector_style (WmsLiteVectorPtr vector, const char *name, const char *title,
		  const char *abstract)
{
/* adding an SLD/SE Style to some Vector Layer */
    int len;
    WmsLiteStylePtr style = malloc (sizeof (WmsLiteStyle));
    len = strlen (name);
    style->Name = malloc (len + 1);
    strcpy (style->Name, name);
    if (title == NULL)
	style->Title = NULL;
    else
      {
	  len = strlen (title);
	  style->Title = malloc (len + 1);
	  strcpy (style->Title, title);
      }
    if (abstract == NULL)
	style->Abstract = NULL;
    else
      {
	  len = strlen (abstract);
	  style->Abstract = malloc (len + 1);
	  strcpy (style->Abstract, abstract);
      }
    style->Next = NULL;
    if (vector->StyleFirst == NULL)
	vector->StyleFirst = style;
    if (vector->StyleLast != NULL)
	vector->StyleLast->Next = style;
    vector->StyleLast = style;
    return style;
}

extern WmsLiteMapConfigMultiSRIDPtr
create_map_config_multi_srid (int srid)
{
/* creating a MapConfig MultiSRID */
    WmsLiteMapConfigMultiSRIDPtr multi =
	malloc (sizeof (WmsLiteMapConfigMultiSRID));
    multi->Srid = srid;
    multi->GeographicBBox = NULL;
    multi->First = NULL;
    multi->Last = NULL;
    return multi;
}

extern void
destroy_map_config_multi_srid (WmsLiteMapConfigMultiSRIDPtr multi)
{
/* memory cleanup - freeing a MapConfig MultiSRID */
    WmsLiteMapConfigSRIDPtr pS;
    WmsLiteMapConfigSRIDPtr pSn;
    if (multi->GeographicBBox != NULL)
	free (multi->GeographicBBox);
    pS = multi->First;
    while (pS != NULL)
      {
	  pSn = pS->Next;
	  if (pS->BBox != NULL)
	      free (pS->BBox);
	  free (pS);
	  pS = pSn;
      }
    free (multi);
}

static WmsLiteLayerPtr
create_wmslite_layer (char type, const char *alias, const char *name, int child)
{
/* creating a WmsLite Layer */
    int len;
    WmsLiteLayerPtr lyr;

    switch (type)
      {
      case WMS_LAYER_RASTER:
      case WMS_LAYER_VECTOR:
      case WMS_LAYER_CASCADED_WMS:
      case WMS_LAYER_MAP_CONFIG:
	  break;
      default:
	  return NULL;
      };
    if (alias == NULL || name == NULL)
	return NULL;

    lyr = malloc (sizeof (WmsLiteLayer));
    if (alias == NULL)
	lyr->AliasName = NULL;
    else
      {
	  len = strlen (alias);
	  lyr->AliasName = malloc (len + 1);
	  strcpy (lyr->AliasName, alias);
      }
    lyr->Type = type;
    if (name == NULL)
	lyr->Name = NULL;
    else
      {
	  len = strlen (name);
	  lyr->Name = malloc (len + 1);
	  strcpy (lyr->Name, name);
      }
    lyr->MapConfig = NULL;
    lyr->MapConfigSrids = NULL;
    lyr->CascadedWMS = NULL;
    lyr->Raster = NULL;
    lyr->Vector = NULL;
    lyr->First = NULL;
    lyr->Last = NULL;
    lyr->MinScaleDenominator = -1.0;
    lyr->MaxScaleDenominator = -1.0;
    lyr->ChildLayer = child;
    lyr->Next = NULL;
    return lyr;
}

static void
destroy_wmslite_layer (WmsLiteLayerPtr lyr)
{
/* memory cleanup - freeing a WmsLite layer */
    WmsLiteChildLayerPtr pC;
    WmsLiteChildLayerPtr pCn;
    if (lyr->AliasName != NULL)
	free (lyr->AliasName);
    if (lyr->Name != NULL)
	free (lyr->Name);
    if (lyr->MapConfig != NULL)
	rl2_destroy_map_config (lyr->MapConfig);
    if (lyr->MapConfigSrids != NULL)
	destroy_map_config_multi_srid (lyr->MapConfigSrids);
    if (lyr->CascadedWMS != NULL)
	destroy_wmslite_cascaded_wms (lyr->CascadedWMS);
    if (lyr->Raster != NULL)
	destroy_wmslite_raster (lyr->Raster);
    if (lyr->Vector != NULL)
	destroy_wmslite_vector (lyr->Vector);
    pC = lyr->First;
    while (pC != NULL)
      {
	  pCn = pC->Next;
	  destroy_wmslite_child (pC);
	  pC = pCn;
      }
    free (lyr);
}

extern WmsLiteWmsCrsPtr
add_cascaded_wms_crs (WmsLiteCascadedWmsPtr wms, const char *srs, double minx,
		      double miny, double maxx, double maxy, int is_default)
{
/* adding a WmsLite CRS definition to some Cascaded WMS Layer */
    int len;
    WmsLiteWmsCrsPtr crs = malloc (sizeof (WmsLiteWmsCrs));
    len = strlen (srs);
    crs->Srs = malloc (len + 1);
    strcpy (crs->Srs, srs);
    crs->BBox = malloc (sizeof (WmsLiteBBox));
    crs->BBox->MinX = minx;
    crs->BBox->MinY = miny;
    crs->BBox->MaxX = maxx;
    crs->BBox->MaxY = maxy;
    crs->IsDefault = is_default;
    crs->Next = NULL;
    if (wms->CrsFirst == NULL)
	wms->CrsFirst = crs;
    if (wms->CrsLast != NULL)
	wms->CrsLast->Next = crs;
    wms->CrsLast = crs;
    return crs;
}

extern WmsLiteStylePtr
add_cascaded_wms_style (WmsLiteCascadedWmsPtr wms, const char *name,
			const char *title, const char *abstract)
{
/* adding an SLD/SE Style to some Cascaded WMS Layer */
    int len;
    WmsLiteStylePtr style = malloc (sizeof (WmsLiteStyle));
    len = strlen (name);
    style->Name = malloc (len + 1);
    strcpy (style->Name, name);
    len = strlen (title);
    style->Title = malloc (len + 1);
    strcpy (style->Title, title);
    if (abstract == NULL)
	style->Abstract = NULL;
    else
      {
	  len = strlen (abstract);
	  style->Abstract = malloc (len + 1);
	  strcpy (style->Abstract, abstract);
      }
    style->Next = NULL;
    if (wms->StyleFirst == NULL)
	wms->StyleFirst = style;
    if (wms->StyleLast != NULL)
	wms->StyleLast->Next = style;
    wms->StyleLast = style;
    return style;
}

static WmsLiteAttachedLayerPtr
create_wmslite_attached_layer (char type, const char *alias, const char *name,
			       int child)
{
/* creating a WmsLite AttachedLayer */
    int len;
    WmsLiteAttachedLayerPtr lyr;

    switch (type)
      {
      case WMS_LAYER_RASTER:
      case WMS_LAYER_VECTOR:
      case WMS_LAYER_CASCADED_WMS:
	  break;
      default:
	  return NULL;
      };

    lyr = malloc (sizeof (WmsLiteLayer));
    if (alias == NULL)
	lyr->AliasName = NULL;
    else
      {
	  len = strlen (alias);
	  lyr->AliasName = malloc (len + 1);
	  strcpy (lyr->AliasName, alias);
      }
    lyr->Type = type;
    if (name == NULL)
	lyr->Name = NULL;
    else
      {
	  len = strlen (name);
	  lyr->Name = malloc (len + 1);
	  strcpy (lyr->Name, name);
      }
    lyr->CascadedWMS = NULL;
    lyr->Raster = NULL;
    lyr->Vector = NULL;
    lyr->MinScaleDenominator = -1.0;
    lyr->MaxScaleDenominator = -1.0;
    lyr->ChildLayer = child;
    lyr->Next = NULL;
    return lyr;
}

static void
destroy_wmslite_attached_layer (WmsLiteAttachedLayerPtr lyr)
{
/* memory cleanup - freeing a WmsLite AttachedLayer */
    if (lyr->AliasName != NULL)
	free (lyr->AliasName);
    if (lyr->Name != NULL)
	free (lyr->Name);
    if (lyr->CascadedWMS != NULL)
	destroy_wmslite_cascaded_wms (lyr->CascadedWMS);
    if (lyr->Raster != NULL)
	destroy_wmslite_raster (lyr->Raster);
    if (lyr->Vector != NULL)
	destroy_wmslite_vector (lyr->Vector);
    free (lyr);
}

static WmsLiteAttachedPtr
create_wmslite_attached (const char *prefix, const char *path)
{
/* creating a WmsLite ATTACHed DB */
    int len;
    WmsLiteAttachedPtr db;
    if (prefix == NULL || path == NULL)
	return NULL;

    db = malloc (sizeof (WmsLiteAttached));
    if (prefix == NULL)
	db->DbPrefix = NULL;
    else
      {
	  len = strlen (prefix);
	  db->DbPrefix = malloc (len + 1);
	  strcpy (db->DbPrefix, prefix);
      }
    if (path == NULL)
	db->Path = NULL;
    else
      {
	  len = strlen (path);
	  db->Path = malloc (len + 1);
	  strcpy (db->Path, path);
      }
    db->Valid = 0;
    db->First = NULL;
    db->Last = NULL;
    db->Next = NULL;
    return db;
}

static void
destroy_wmslite_attached (WmsLiteAttachedPtr db)
{
/* memory cleanup - freeing an ATTACHed DB */
    WmsLiteAttachedLayerPtr pL;
    WmsLiteAttachedLayerPtr pLn;
    if (db->DbPrefix != NULL)
	free (db->DbPrefix);
    if (db->Path != NULL)
	free (db->Path);
    pL = db->First;
    while (pL != NULL)
      {
	  pLn = pL->Next;
	  destroy_wmslite_attached_layer (pL);
	  pL = pLn;
      }
    free (db);
}

static WmsLiteConfigPtr
create_wmslite_config (const char *path)
{
/* creating an empty WmsLite configuration */
    int len = strlen (path);
    WmsLiteConfigPtr config = malloc (sizeof (WmsLiteConfig));
    config->IsMiniServer = 0;
    config->PendingShutdown = 0;
    config->MultithreadEnabled = 0;
    config->MaxThreads = 1;
    config->WmsMaxRetries = 5;
    config->WmsPause = 1000;
    config->BackgroundRed = 255;
    config->BackgroundGreen = 255;
    config->BackgroundBlue = 255;
    config->Transparent = 0;
    config->LabelAntiCollision = 0;
    config->LabelWrapText = 0;
    config->LabelAutoRotate = 0;
    config->LabelShiftPosition = 0;
    config->Name = NULL;
    config->Title = NULL;
    config->Abstract = NULL;
    config->OnlineResource = NULL;
    config->ContactPerson = NULL;
    config->ContactOrganization = NULL;
    config->ContactPosition = NULL;
    config->AddressType = NULL;
    config->Address = NULL;
    config->City = NULL;
    config->State = NULL;
    config->PostCode = NULL;
    config->Country = NULL;
    config->ContactEMail = NULL;
    config->Fees = NULL;
    config->AccessConstraints = NULL;
    config->LayerLimit = 1;
    config->MaxWidth = 5000;
    config->MaxHeight = 5000;
    config->TotalLayersCount = 0;
    config->TopLayerName = NULL;
    config->TopLayerTitle = NULL;
    config->TopLayerAbstract = NULL;
    config->TopLayerRef = NULL;
    config->TopAttachedLayerRef = NULL;
    config->TopLayerMinScale = -1.0;
    config->TopLayerMaxScale = -1.0;
    config->EnableLegendURL = 0;
    config->LegendWidth = 128;
    config->LegendHeight = 32;
    config->LegendFormat = NULL;
    config->LegendFontName = NULL;
    config->LegendFontSize = 10.0;
    config->LegendFontItalic = 0;
    config->LegendFontBold = 0;
    config->LegendFontColor = NULL;
    config->Path = malloc (len + 1);
    strcpy (config->Path, path);
    config->MainDbPath = NULL;
    config->Connection.handle = NULL;
    config->Connection.splite_privdata = NULL;
    config->Connection.rl2_privdata = NULL;
    config->KeyFirst = NULL;
    config->KeyLast = NULL;
    config->MainFirst = NULL;
    config->MainLast = NULL;
    config->DbFirst = NULL;
    config->DbLast = NULL;
    config->Capabilities_100 = NULL;
    config->Capabilities_110 = NULL;
    config->Capabilities_111 = NULL;
    config->Capabilities_130 = NULL;
    config->Capabilities_100_Length = 0;
    config->Capabilities_110_Length = 0;
    config->Capabilities_111_Length = 0;
    config->Capabilities_130_Length = 0;
    return config;
}

extern void
destroy_wmslite_config (WmsLiteConfigPtr config)
{
/* memory cleanup - freeing a WmsLite configuration */
    WmsLiteKeywordPtr pK;
    WmsLiteKeywordPtr pKn;
    WmsLiteLayerPtr pL;
    WmsLiteLayerPtr pLn;
    WmsLiteAttachedPtr pA;
    WmsLiteAttachedPtr pAn;
    if (config->Name != NULL)
	free (config->Name);
    if (config->Title != NULL)
	free (config->Title);
    if (config->Abstract != NULL)
	free (config->Abstract);
    if (config->OnlineResource != NULL)
	free (config->OnlineResource);
    if (config->ContactPerson != NULL)
	free (config->ContactPerson);
    if (config->ContactOrganization != NULL)
	free (config->ContactOrganization);
    if (config->ContactPosition != NULL)
	free (config->ContactPosition);
    if (config->AddressType != NULL)
	free (config->AddressType);
    if (config->Address != NULL)
	free (config->Address);
    if (config->City != NULL)
	free (config->City);
    if (config->State != NULL)
	free (config->State);
    if (config->PostCode != NULL)
	free (config->PostCode);
    if (config->Country != NULL)
	free (config->Country);
    if (config->ContactEMail != NULL)
	free (config->ContactEMail);
    if (config->Fees != NULL)
	free (config->Fees);
    if (config->AccessConstraints != NULL)
	free (config->AccessConstraints);
    if (config->TopLayerName != NULL)
	free (config->TopLayerName);
    if (config->TopLayerTitle != NULL)
	free (config->TopLayerTitle);
    if (config->TopLayerAbstract != NULL)
	free (config->TopLayerAbstract);
    if (config->LegendFormat != NULL)
	free (config->LegendFormat);
    if (config->LegendFontName != NULL)
	free (config->LegendFontName);
    if (config->LegendFontColor != NULL)
	free (config->LegendFontColor);
    if (config->Path != NULL)
	free (config->Path);
    if (config->MainDbPath != NULL)
	free (config->MainDbPath);
    if (config->Connection.handle != NULL)
	sqlite3_close (config->Connection.handle);
    if (config->Connection.rl2_privdata != NULL)
	rl2_cleanup_private (config->Connection.rl2_privdata);
    if (config->Connection.splite_privdata != NULL)
	spatialite_cleanup_ex (config->Connection.splite_privdata);
    spatialite_shutdown ();
    if (config->Capabilities_100 != NULL)
	sqlite3_free (config->Capabilities_100);
    if (config->Capabilities_110 != NULL)
	sqlite3_free (config->Capabilities_110);
    if (config->Capabilities_111 != NULL)
	sqlite3_free (config->Capabilities_111);
    if (config->Capabilities_130 != NULL)
	sqlite3_free (config->Capabilities_130);
    pK = config->KeyFirst;
    while (pK != NULL)
      {
	  pKn = pK->Next;
	  destroy_wmslite_keyword (pK);
	  pK = pKn;
      }
    pL = config->MainFirst;
    while (pL != NULL)
      {
	  pLn = pL->Next;
	  destroy_wmslite_layer (pL);
	  pL = pLn;
      }
    pA = config->DbFirst;
    while (pA != NULL)
      {
	  pAn = pA->Next;
	  destroy_wmslite_attached (pA);
	  pA = pAn;
      }
    free (config);
}

static WmsLiteAttachedPtr
add_wmslite_attached_db (WmsLiteConfigPtr list, const char *prefix,
			 const char *path)
{
/* adding some ATTACHed DB into the configuration */
    WmsLiteAttachedPtr db = create_wmslite_attached (prefix, path);
    if (list->DbFirst == NULL)
	list->DbFirst = db;
    if (list->DbLast != NULL)
	list->DbLast->Next = db;
    list->DbLast = db;
    return db;
}

static WmsLiteLayerPtr
add_wmslite_layer (WmsLiteConfigPtr list, char type, const char *alias,
		   const char *name, int child)
{
/* adding a Layer into the configuration from the MAIN DB */
    WmsLiteLayerPtr lyr = create_wmslite_layer (type, alias, name, child);
    if (lyr == NULL)
	return NULL;
    if (list->MainFirst == NULL)
	list->MainFirst = lyr;
    if (list->MainLast != NULL)
	list->MainLast->Next = lyr;
    list->MainLast = lyr;
    return lyr;
}

static WmsLiteChildLayerPtr
add_wmslite_child_layer (WmsLiteLayerPtr lyr, const char *child)
{
/* adding a ChildLayer to its Parent */
    int len;
    WmsLiteChildLayerPtr cld = malloc (sizeof (WmsLiteChildLayer));
    len = strlen (child);
    cld->ChildAliasName = malloc (len + 1);
    strcpy (cld->ChildAliasName, child);
    cld->Next = NULL;
    if (lyr->First == NULL)
	lyr->First = cld;
    if (lyr->Last != NULL)
	lyr->Last->Next = cld;
    lyr->Last = cld;
    return cld;
}

static WmsLiteAttachedLayerPtr
add_wmslite_attached_layer (WmsLiteAttachedPtr db, char type, const char *alias,
			    const char *name, int child)
{
/* adding an AttachedLayer into its ATTACHed DB */
    WmsLiteAttachedLayerPtr lyr =
	create_wmslite_attached_layer (type, alias, name, child);
    if (lyr == NULL)
	return NULL;
    if (db->First == NULL)
	db->First = lyr;
    if (db->Last != NULL)
	db->Last->Next = lyr;
    db->Last = lyr;
    return lyr;
}

static int
wmslite_parse_hex (unsigned char hi, unsigned char lo, unsigned char *val)
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
wmslite_parse_hex_color (const char *color, unsigned char *red,
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
    if (!wmslite_parse_hex (*(color + 1), *(color + 2), &r))
	return 0;
    if (!wmslite_parse_hex (*(color + 3), *(color + 4), &g))
	return 0;
    if (!wmslite_parse_hex (*(color + 5), *(color + 6), &b))
	return 0;
    *red = r;
    *green = g;
    *blue = b;
    return 1;
}

static void
WmsLiteSilentError (void *ctx, const char *msg, ...)
{
/* shutting up XML Errors */
    if (ctx != NULL)
	ctx = NULL;		/* suppressing stupid compiler warnings (unused args) */
    if (msg != NULL)
	ctx = NULL;		/* suppressing stupid compiler warnings (unused args) */
}

static void
parse_wmslite_name (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><Name> tag */
    const char *value;
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
					if (list->Name != NULL)
					    free (list->Name);
					list->Name = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->Name != NULL)
					    free (list->Name);
					list->Name = malloc (len + 1);
					strcpy (list->Name, value);
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
parse_wmslite_title (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><Title> tag */
    const char *value;
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
					if (list->Title != NULL)
					    free (list->Title);
					list->Title = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->Title != NULL)
					    free (list->Title);
					list->Title = malloc (len + 1);
					strcpy (list->Title, value);
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
parse_wmslite_abstract (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><Abstract> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
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
					if (list->Abstract != NULL)
					    free (list->Abstract);
					list->Abstract = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->Abstract != NULL)
					    free (list->Abstract);
					list->Abstract = malloc (len + 1);
					strcpy (list->Abstract, value);
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
parse_wmslite_resource (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><OnlineResource> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "OnlineResource") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->OnlineResource != NULL)
					    free (list->OnlineResource);
					list->Fees = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->OnlineResource != NULL)
					    free (list->OnlineResource);
					list->OnlineResource = malloc (len + 1);
					strcpy (list->OnlineResource, value);
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
parse_wmslite_fees (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><Fees> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Fees") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->Fees != NULL)
					    free (list->Fees);
					list->Fees = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->Fees != NULL)
					    free (list->Fees);
					list->Fees = malloc (len + 1);
					strcpy (list->Fees, value);
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
parse_wmslite_constraints (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><AccessConstraints> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "AccessConstraints") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->AccessConstraints != NULL)
					    free (list->AccessConstraints);
					list->AccessConstraints = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->AccessConstraints != NULL)
					    free (list->AccessConstraints);
					list->AccessConstraints =
					    malloc (len + 1);
					strcpy (list->AccessConstraints, value);
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
parse_wmslite_layer_limit (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><LayerLimit> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "LayerLimit") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      list->LayerLimit = 1;
				  else
				      list->LayerLimit = atoi (value);
				  if (list->LayerLimit < 1)
				      list->LayerLimit = 1;
			      }
			    text = text->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_max_width (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><MaxWidth> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MaxWidth") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      list->MaxWidth = 5000;
				  else
				      list->MaxWidth = atoi (value);
				  if (list->MaxWidth < 512)
				      list->MaxWidth = 512;
				  if (list->MaxWidth > 5000)
				      list->MaxWidth = 5000;
			      }
			    text = text->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_max_height (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><MaxHeight> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MaxHeight") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      list->MaxHeight = 5000;
				  else
				      list->MaxHeight = atoi (value);
				  if (list->MaxHeight < 512)
				      list->MaxHeight = 512;
				  if (list->MaxHeight > 5000)
				      list->MaxHeight = 5000;
			      }
			    text = text->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_person (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation><ContactPersonPrimary><ContactPerson> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ContactPerson") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->ContactPerson != NULL)
					    free (list->ContactPerson);
					list->ContactPerson = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->ContactPerson != NULL)
					    free (list->ContactPerson);
					list->ContactPerson = malloc (len + 1);
					strcpy (list->ContactPerson, value);
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
parse_wmslite_organization (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation><ContactPersonPrimary><ContactOrganization> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ContactOrganization") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->ContactPosition != NULL)
					    free (list->ContactPosition);
					list->ContactPosition = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->ContactPosition != NULL)
					    free (list->ContactPosition);
					list->ContactPosition =
					    malloc (len + 1);
					strcpy (list->ContactPosition, value);
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
parse_wmslite_primary (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation><ContactPersonPrimary> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ContactPerson") == 0)
		    parse_wmslite_person (node, list);
		if (strcmp (name, "ContactOrganization") == 0)
		    parse_wmslite_organization (node, list);
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_position (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation><ContactPosition> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ContactPosition") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->ContactPosition != NULL)
					    free (list->ContactPosition);
					list->ContactPosition = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->ContactPosition != NULL)
					    free (list->ContactPosition);
					list->ContactPosition =
					    malloc (len + 1);
					strcpy (list->ContactPosition, value);
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
parse_wmslite_email (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation><ContactElectronicMailAddress> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ContactElectronicMailAddress") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->ContactEMail != NULL)
					    free (list->ContactEMail);
					list->ContactEMail = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->ContactEMail != NULL)
					    free (list->ContactEMail);
					list->ContactEMail = malloc (len + 1);
					strcpy (list->ContactEMail, value);
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
parse_wmslite_address_type (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation><ContactAddress><AddressType> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "AddressType") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->AddressType != NULL)
					    free (list->AddressType);
					list->AddressType = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->AddressType != NULL)
					    free (list->AddressType);
					list->AddressType = malloc (len + 1);
					strcpy (list->AddressType, value);
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
parse_wmslite_address (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation><ContactAddress><Address> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Address") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->Address != NULL)
					    free (list->Address);
					list->Address = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->Address != NULL)
					    free (list->Address);
					list->Address = malloc (len + 1);
					strcpy (list->Address, value);
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
parse_wmslite_city (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation><ContactAddress><City> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "City") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->City != NULL)
					    free (list->City);
					list->City = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->City != NULL)
					    free (list->City);
					list->City = malloc (len + 1);
					strcpy (list->City, value);
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
parse_wmslite_state (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation><ContactAddress><StateOrProvince> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "StateOrProvince") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->State != NULL)
					    free (list->State);
					list->State = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->State != NULL)
					    free (list->State);
					list->State = malloc (len + 1);
					strcpy (list->State, value);
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
parse_wmslite_post_code (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation><ContactAddress><PostCode> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "PostCode") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->PostCode != NULL)
					    free (list->PostCode);
					list->PostCode = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->PostCode != NULL)
					    free (list->PostCode);
					list->PostCode = malloc (len + 1);
					strcpy (list->PostCode, value);
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
parse_wmslite_country (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation><ContactAddress><Country> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Country") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				    {
					if (list->Country != NULL)
					    free (list->Country);
					list->Country = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->Country != NULL)
					    free (list->Country);
					list->Country = malloc (len + 1);
					strcpy (list->Country, value);
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
parse_wmslite_keyword (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><KeywordList><Keyword> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Keyword") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				      add_wmslite_keyword (list, value);
			      }
			    text = text->next;
			}
		  }
		return;
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_contact_address (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation><ContactAddress> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "AddressType") == 0)
		    parse_wmslite_address_type (node, list);
		if (strcmp (name, "Address") == 0)
		    parse_wmslite_address (node, list);
		if (strcmp (name, "City") == 0)
		    parse_wmslite_city (node, list);
		if (strcmp (name, "StateOrProvince") == 0)
		    parse_wmslite_state (node, list);
		if (strcmp (name, "PostCode") == 0)
		    parse_wmslite_post_code (node, list);
		if (strcmp (name, "Country") == 0)
		    parse_wmslite_country (node, list);
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_contact (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <Service><ContactInformation> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ContactPersonPrimary") == 0)
		    parse_wmslite_primary (node->children, list);
		if (strcmp (name, "ContactPosition") == 0)
		    parse_wmslite_position (node, list);
		if (strcmp (name, "ContactAddress") == 0)
		    parse_wmslite_contact_address (node->children, list);
		if (strcmp (name, "ContactElectronicMailAddress") == 0)
		    parse_wmslite_email (node, list);
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_key_config (xmlNodePtr node, WmsLiteConfigPtr config)
{
/* parsing a <Service><KeywordList> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Keyword") == 0)
		    parse_wmslite_keyword (node, config);
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_service (xmlNodePtr node, WmsLiteConfigPtr config)
{
/* parsing a <Service> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Name") == 0)
		    parse_wmslite_name (node, config);
		if (strcmp (name, "Title") == 0)
		    parse_wmslite_title (node, config);
		if (strcmp (name, "Abstract") == 0)
		    parse_wmslite_abstract (node, config);
		if (strcmp (name, "KeywordList") == 0)
		    parse_wmslite_key_config (node->children, config);
		if (strcmp (name, "OnlineResource") == 0)
		    parse_wmslite_resource (node, config);
		if (strcmp (name, "ContactInformation") == 0)
		    parse_wmslite_contact (node->children, config);
		if (strcmp (name, "Fees") == 0)
		    parse_wmslite_fees (node, config);
		if (strcmp (name, "AccessConstraints") == 0)
		    parse_wmslite_constraints (node, config);
		if (strcmp (name, "LayerLimit") == 0)
		    parse_wmslite_layer_limit (node, config);
		if (strcmp (name, "MaxWidth") == 0)
		    parse_wmslite_max_width (node, config);
		if (strcmp (name, "MaxHeight") == 0)
		    parse_wmslite_max_height (node, config);
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_multi_threading (xmlNodePtr node, WmsLiteConfigPtr config)
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
		      config->MultithreadEnabled = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    config->MultithreadEnabled = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "MaxThreads") == 0)
		  {
		      xmlNode *text = attr->children;
		      config->MaxThreads = 1;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				      config->MaxThreads = atoi (value);
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_wmslite_background (xmlNodePtr node, WmsLiteConfigPtr config)
{
/* parsing a <Background> tag */
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
		      config->BackgroundRed = 255;
		      config->BackgroundGreen = 255;
		      config->BackgroundBlue = 255;
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
					if (wmslite_parse_hex_color
					    (value, &red, &green, &blue))
					  {
					      config->BackgroundRed = red;
					      config->BackgroundGreen = green;
					      config->BackgroundBlue = blue;
					  }
				    }
			      }
			}
		  }
		if (strcmp (name, "Transparent") == 0)
		  {
		      xmlNode *text = attr->children;
		      config->Transparent = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    config->Transparent = 1;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_wmslite_wms (xmlNodePtr node, WmsLiteConfigPtr config)
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
		      config->WmsMaxRetries = 5;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					config->WmsMaxRetries = atoi (value);
					if (config->WmsMaxRetries < 1)
					    config->WmsMaxRetries = 1;
					if (config->WmsMaxRetries > 10)
					    config->WmsMaxRetries = 10;
				    }
			      }
			}
		  }
		if (strcmp (name, "Pause") == 0)
		  {
		      xmlNode *text = attr->children;
		      config->WmsPause = 1000;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					config->WmsPause = atoi (value);
					if (config->WmsPause < 1)
					    config->WmsPause = 1;
					if (config->WmsPause > 30000)
					    config->WmsPause = 30000;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_wmslite_label_advanced_options (xmlNodePtr node, WmsLiteConfigPtr config)
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
		      config->LabelAntiCollision = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    config->LabelAntiCollision = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "WrapTextEnabled") == 0)
		  {
		      xmlNode *text = attr->children;
		      config->LabelWrapText = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    config->LabelWrapText = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "AutoRotateEnabled") == 0)
		  {
		      xmlNode *text = attr->children;
		      config->LabelAutoRotate = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    config->LabelAutoRotate = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "ShiftPositionEnabled") == 0)
		  {
		      xmlNode *text = attr->children;
		      config->LabelShiftPosition = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcmp (value, "true") == 0)
					    config->LabelShiftPosition = 1;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_wmslite_legend_url (xmlNodePtr node, WmsLiteConfigPtr config)
{
/* parsing a <LegendURL> tag */
    const char *value;
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
		      config->LegendWidth = 128;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					config->LegendWidth = atoi (value);
					if (config->LegendWidth < 16)
					    config->LegendWidth = 16;
					if (config->LegendWidth > 512)
					    config->LegendWidth = 512;
					config->EnableLegendURL = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "Height") == 0)
		  {
		      xmlNode *text = attr->children;
		      config->LegendHeight = 32;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					config->LegendHeight = atoi (value);
					if (config->LegendHeight < 16)
					    config->LegendHeight = 16;
					if (config->LegendHeight > 512)
					    config->LegendHeight = 512;
					config->EnableLegendURL = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "Format") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (config->LegendFormat != NULL)
			  free (config->LegendFormat);
		      config->LegendFormat = NULL;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					int len;
					const char *fmt;
					if (strcmp (value, "image/jpeg") == 0)
					    fmt = "image/jpeg";
					else
					    fmt = "image/png";
					len = strlen (fmt);
					config->LegendFormat = malloc (len + 1);
					strcpy (config->LegendFormat, fmt);
					config->EnableLegendURL = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "FontName") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (config->LegendFontName != NULL)
			  free (config->LegendFontName);
		      config->LegendFontName = NULL;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					int len;
					len = strlen (value);
					config->LegendFontName =
					    malloc (len + 1);
					strcpy (config->LegendFontName, value);
					config->EnableLegendURL = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "FontSize") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					config->LegendFontSize = atof (value);
					config->EnableLegendURL = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "FontItalic") == 0)
		  {
		      xmlNode *text = attr->children;
		      config->LegendFontItalic = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcasecmp (value, "true") == 0)
					    config->LegendFontItalic = 1;
					config->EnableLegendURL = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "FontBold") == 0)
		  {
		      xmlNode *text = attr->children;
		      config->LegendFontBold = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					if (strcasecmp (value, "true") == 0)
					    config->LegendFontBold = 1;
					config->EnableLegendURL = 1;
				    }
			      }
			}
		  }
		if (strcmp (name, "FontColor") == 0)
		  {
		      xmlNode *text = attr->children;
		      if (config->LegendFontColor != NULL)
			  free (config->LegendFontColor);
		      config->LegendFontColor = NULL;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				    {
					int len;
					len = strlen (value);
					config->LegendFontColor =
					    malloc (len + 1);
					strcpy (config->LegendFontColor, value);
					config->EnableLegendURL = 1;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
}

static void
parse_wmslite_options (xmlNodePtr node, WmsLiteConfigPtr config)
{
/* parsing a <GeneralOptions> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MultiThreading") == 0)
		    parse_wmslite_multi_threading (node, config);
		if (strcmp (name, "WMS") == 0)
		    parse_wmslite_wms (node, config);
		if (strcmp (name, "Background") == 0)
		    parse_wmslite_background (node, config);
		if (strcmp (name, "LabelAdvancedOptions") == 0)
		    parse_wmslite_label_advanced_options (node, config);
		if (strcmp (name, "LegendURL") == 0)
		    parse_wmslite_legend_url (node, config);
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_child_layer (xmlNodePtr node, WmsLiteLayerPtr lyr)
{
/* parsing a <WmsLayers><MainDB><Layer><ChildLayer> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ChildLayer") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value != NULL)
				      add_wmslite_child_layer (lyr, value);
			      }
			    text = text->next;
			}
		  }
		return;
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_layer (xmlNodePtr node, WmsLiteConfigPtr config)
{
/* parsing a <WmsLayers><MainDB><Layer> tag */
    const char *value;
    const char *alias = NULL;
    const char *lyr_name = NULL;
    char type;
    int child = 0;
    WmsLiteLayerPtr lyr;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "Alias") == 0)
		  {
		      xmlNode *text = attr->children;
		      alias = NULL;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      alias = NULL;
				  else
				      alias = value;
			      }
			}
		  }
		if (strcmp (name, "Name") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr_name = NULL;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      lyr_name = NULL;
				  else
				      lyr_name = value;
			      }
			}
		  }
		if (strcmp (name, "Type") == 0)
		  {
		      xmlNode *text = attr->children;
		      type = WMS_LAYER_UNKNOWN;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      type = WMS_LAYER_UNKNOWN;
				  else
				    {
					if (strcasecmp (value, "RasterCoverage")
					    == 0)
					    type = WMS_LAYER_RASTER;
					else if (strcasecmp
						 (value, "VectorCoverage") == 0)
					    type = WMS_LAYER_VECTOR;
					else if (strcasecmp
						 (value, "CascadedWMS") == 0)
					    type = WMS_LAYER_CASCADED_WMS;
					else if (strcasecmp
						 (value,
						  "MapConfiguration") == 0)
					    type = WMS_LAYER_MAP_CONFIG;
					else
					    type = WMS_LAYER_UNKNOWN;
				    }
			      }
			}
		  }
		if (strcmp (name, "Child") == 0)
		  {
		      xmlNode *text = attr->children;
		      child = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      child = 0;
				  else
				    {
					if (strcasecmp (value, "true") == 0)
					    child = 1;
					else
					    child = 0;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
    lyr = add_wmslite_layer (config, type, alias, lyr_name, child);
    if (lyr == NULL)
	return;

    node = node->children;
    while (node)
      {
	  /* Children (ChildLayers) */
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "ChildLayer") == 0)
		    parse_wmslite_child_layer (node, lyr);
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_attached_layer (xmlNodePtr node, WmsLiteAttachedPtr db)
{
/* parsing a <WmsLayers><AttachedDB><AttachedLayer> tag */
    const char *value;
    const char *alias = NULL;
    const char *lyr_name = NULL;
    char type;
    int child = 0;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "Alias") == 0)
		  {
		      xmlNode *text = attr->children;
		      alias = NULL;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      alias = NULL;
				  else
				      alias = value;
			      }
			}
		  }
		if (strcmp (name, "Name") == 0)
		  {
		      xmlNode *text = attr->children;
		      lyr_name = NULL;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      lyr_name = NULL;
				  else
				      lyr_name = value;
			      }
			}
		  }
		if (strcmp (name, "Type") == 0)
		  {
		      xmlNode *text = attr->children;
		      type = WMS_LAYER_UNKNOWN;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      type = WMS_LAYER_UNKNOWN;
				  else
				    {
					if (strcasecmp (value, "RasterCoverage")
					    == 0)
					    type = WMS_LAYER_RASTER;
					else if (strcasecmp
						 (value, "VectorCoverage") == 0)
					    type = WMS_LAYER_VECTOR;
					else if (strcasecmp
						 (value, "CascadedWMS") == 0)
					    type = WMS_LAYER_CASCADED_WMS;
					else
					    type = WMS_LAYER_UNKNOWN;
				    }
			      }
			}
		  }
		if (strcmp (name, "Child") == 0)
		  {
		      xmlNode *text = attr->children;
		      child = 0;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      child = 0;
				  else
				    {
					if (strcasecmp (value, "true") == 0)
					    child = 1;
					else
					    child = 0;
				    }
			      }
			}
		  }
		attr = attr->next;
	    }
      }
    add_wmslite_attached_layer (db, type, alias, lyr_name, child);
}

static void
parse_wmslite_main_db (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <WmsLayers><MainDB> tag */
    const char *value;
    const char *path = NULL;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "Path") == 0)
		  {
		      xmlNode *text = attr->children;
		      path = NULL;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      path = NULL;
				  else
				      path = value;
			      }
			}
		  }
		attr = attr->next;
	    }
      }
    if (list->MainDbPath != NULL)
	free (list->MainDbPath);
    if (path == NULL)
	list->MainDbPath = NULL;
    else
      {
	  int len = strlen (path);
	  list->MainDbPath = malloc (len + 1);
	  strcpy (list->MainDbPath, path);
      }

    node = node->children;
    while (node)
      {
	  /* Children (Layers) */
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Layer") == 0)
		    parse_wmslite_layer (node, list);
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_attached_db (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <WmsLayers><AttachedDB> tag */
    WmsLiteAttachedPtr db;
    const char *value;
    const char *prefix = NULL;
    const char *path = NULL;
    struct _xmlAttr *attr = node->properties;
    while (attr != NULL)
      {
	  /* attributes */
	  if (attr->type == XML_ATTRIBUTE_NODE)
	    {
		const char *name = (const char *) (attr->name);
		if (strcmp (name, "Prefix") == 0)
		  {
		      xmlNode *text = attr->children;
		      prefix = NULL;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      prefix = NULL;
				  else
				      prefix = value;
			      }
			}
		  }
		if (strcmp (name, "Path") == 0)
		  {
		      xmlNode *text = attr->children;
		      path = NULL;
		      if (text != NULL)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      path = NULL;
				  else
				      path = value;
			      }
			}
		  }
		attr = attr->next;
	    }
      }
    db = add_wmslite_attached_db (list, prefix, path);
    if (db == NULL)
	return;

    node = node->children;
    while (node)
      {
	  /* Children (AttachedLayers) */
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "AttachedLayer") == 0)
		    parse_wmslite_attached_layer (node, db);
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_toplevel_name (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <WmsLayers><TopLevelLayer><Name> tag */
    const char *value;
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
					if (list->TopLayerName != NULL)
					    free (list->TopLayerName);
					list->TopLayerName = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->TopLayerName != NULL)
					    free (list->TopLayerName);
					list->TopLayerName = malloc (len + 1);
					strcpy (list->TopLayerName, value);
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
parse_wmslite_toplevel_title (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <WmsLayers><TopLayerLayer><Title> tag */
    const char *value;
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
					if (list->TopLayerTitle != NULL)
					    free (list->TopLayerTitle);
					list->TopLayerTitle = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->TopLayerTitle != NULL)
					    free (list->TopLayerTitle);
					list->TopLayerTitle = malloc (len + 1);
					strcpy (list->TopLayerTitle, value);
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
parse_wmslite_toplevel_abstract (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <WmsLayers><TopLayerLayer><Abstract> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
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
					if (list->TopLayerAbstract != NULL)
					    free (list->TopLayerAbstract);
					list->TopLayerAbstract = NULL;
				    }
				  else
				    {
					int len = strlen (value);
					if (list->TopLayerAbstract != NULL)
					    free (list->TopLayerAbstract);
					list->TopLayerAbstract =
					    malloc (len + 1);
					strcpy (list->TopLayerAbstract, value);
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
parse_wmslite_toplevel_min_scale (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <WmsLayers><TopLayerLayer><MinScaleDenominator> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MinScaleDenominator") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      list->TopLayerMinScale = -1.0;
				  else
				      list->TopLayerMinScale = atof (value);
			      }
			    text = text->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_toplevel_max_scale (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <WmsLayers><TopLayerLayer><MaxScaleDenominator> tag */
    const char *value;
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "MaxScaleDenominator") == 0)
		  {
		      xmlNode *text = node->children;
		      while (text)
			{
			    if (text->type == XML_TEXT_NODE)
			      {
				  value = (const char *) (text->content);
				  if (value == NULL)
				      list->TopLayerMaxScale = -1.0;
				  else
				      list->TopLayerMaxScale = atof (value);
			      }
			    text = text->next;
			}
		  }
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_toplevel (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <WmsLayers><TopLevelLayer> tag */
    while (node)
      {
	  /* Children (Layers) */
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Name") == 0)
		    parse_wmslite_toplevel_name (node, list);
		if (strcmp (name, "Title") == 0)
		    parse_wmslite_toplevel_title (node, list);
		if (strcmp (name, "Abstract") == 0)
		    parse_wmslite_toplevel_abstract (node, list);
		if (strcmp (name, "MinScaleDenominator") == 0)
		    parse_wmslite_toplevel_min_scale (node, list);
		if (strcmp (name, "MaxScaleDenominator") == 0)
		    parse_wmslite_toplevel_max_scale (node, list);
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_layers (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <WmsLayers> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "TopLevelLayer") == 0)
		    parse_wmslite_toplevel (node->children, list);
		if (strcmp (name, "MainDB") == 0)
		    parse_wmslite_main_db (node, list);
		if (strcmp (name, "AttachedDB") == 0)
		    parse_wmslite_attached_db (node, list);
	    }
	  node = node->next;
      }
}

static void
parse_wmslite_config (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* parsing a <WmsLiteConfig> tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "Service") == 0)
		    parse_wmslite_service (node->children, list);
		if (strcmp (name, "GeneralOptions") == 0)
		    parse_wmslite_options (node->children, list);
		if (strcmp (name, "WmsLayers") == 0)
		    parse_wmslite_layers (node->children, list);
	    }
	  node = node->next;
      }
}

static int
find_wmslite_config (xmlNodePtr node, WmsLiteConfigPtr list)
{
/* searching a WmsLiteConfig tag */
    while (node)
      {
	  if (node->type == XML_ELEMENT_NODE)
	    {
		const char *name = (const char *) (node->name);
		if (strcmp (name, "WmsLiteConfig") == 0)
		  {
		      parse_wmslite_config (node->children, list);
		      return 1;
		  }
	    }
	  node = node->next;
      }
    return 0;
}

extern WmsLiteConfigPtr
wmslite_parse_config (const char *path)
{
/* attempting to build a WmsLite configuration from an XML definition */
    WmsLiteConfigPtr list = NULL;
    xmlDocPtr xml_doc = NULL;
    xmlNodePtr root;
    xmlGenericErrorFunc silentError = (xmlGenericErrorFunc) WmsLiteSilentError;

    list = create_wmslite_config (path);
    if (list == NULL)
	return NULL;

/* parsing the XML document */
    xmlSetGenericErrorFunc (NULL, silentError);
    xml_doc = xmlReadFile (path, NULL, XML_PARSE_BIG_LINES);
    if (xml_doc == NULL)
      {
	  /* parsing error; not a well-formed XML */
	  goto error;
      }
    root = xmlDocGetRootElement (xml_doc);
    if (root == NULL)
	goto error;
    if (!find_wmslite_config (root, list))
	goto error;

    xmlFreeDoc (xml_doc);
    return list;

  error:
    if (xml_doc != NULL)
	xmlFreeDoc (xml_doc);
    if (list != NULL)
	destroy_wmslite_config (list);
    return NULL;
}

extern int
is_valid_wmslite_layer (WmsLiteLayerPtr lyr)
{
/* checking if a WmsLite Layer is a valid one */
    switch (lyr->Type)
      {
      case WMS_LAYER_RASTER:
	  if (lyr->Raster == NULL)
	      return 0;
	  if (lyr->Raster->GeographicBBox == NULL)
	      return 0;
	  return 1;
      case WMS_LAYER_VECTOR:
	  if (lyr->Vector == NULL)
	      return 0;
	  if (lyr->Vector->GeographicBBox == NULL)
	      return 0;
	  return 1;
      case WMS_LAYER_CASCADED_WMS:
	  if (lyr->CascadedWMS == NULL)
	      return 0;
	  if (lyr->CascadedWMS->Srs == NULL)
	      return 0;
	  if (lyr->CascadedWMS->GeographicBBox == NULL)
	      return 0;
	  return 1;
      case WMS_LAYER_MAP_CONFIG:
	  if (lyr->MapConfig == NULL)
	      return 0;
	  if (lyr->MapConfig->valid == 0)
	      return 0;
	  return 1;
      default:
	  break;
      };
    return 0;
}

extern int
is_valid_wmslite_attached_layer (WmsLiteAttachedLayerPtr lyr)
{
/* checking if a WmsLite AttachedLayer is a valid one */
    switch (lyr->Type)
      {
      case WMS_LAYER_RASTER:
	  if (lyr->Raster == NULL)
	      return 0;
	  if (lyr->Raster->GeographicBBox == NULL)
	      return 0;
	  return 1;
      case WMS_LAYER_VECTOR:
	  if (lyr->Vector == NULL)
	      return 0;
	  if (lyr->Vector->GeographicBBox == NULL)
	      return 0;
	  return 1;
      case WMS_LAYER_CASCADED_WMS:
	  if (lyr->CascadedWMS == NULL)
	      return 0;
	  if (lyr->CascadedWMS->Srs == NULL)
	      return 0;
	  if (lyr->CascadedWMS->GeographicBBox == NULL)
	      return 0;
	  return 1;
      default:
	  break;
      };
    return 0;
}

extern int
is_queryable_wmslite_layer (WmsLiteLayerPtr lyr)
{
/* checking if a WmsLite Layer is Queryable or not */
    switch (lyr->Type)
      {
      case WMS_LAYER_RASTER:
	  if (lyr->Raster == NULL)
	      return 0;
	  return lyr->Raster->IsQueryable;
      case WMS_LAYER_VECTOR:
	  if (lyr->Vector == NULL)
	      return 0;
	  return lyr->Vector->IsQueryable;
      case WMS_LAYER_CASCADED_WMS:
	  if (lyr->CascadedWMS == NULL)
	      return 0;
	  return lyr->CascadedWMS->IsQueryable;
      default:
	  break;
      };
    return 0;
}

extern int
is_queryable_wmslite_attached_layer (WmsLiteAttachedLayerPtr lyr)
{
/* checking if a WmsLite AttachedLayer is Queryable or not */
    switch (lyr->Type)
      {
      case WMS_LAYER_RASTER:
	  if (lyr->Raster == NULL)
	      return 0;
	  return lyr->Raster->IsQueryable;
      case WMS_LAYER_VECTOR:
	  if (lyr->Vector == NULL)
	      return 0;
	  return lyr->Vector->IsQueryable;
      case WMS_LAYER_CASCADED_WMS:
	  if (lyr->CascadedWMS == NULL)
	      return 0;
	  return lyr->CascadedWMS->IsQueryable;
      default:
	  break;
      };
    return 0;
}
