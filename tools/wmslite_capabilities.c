/* 
/ wmslite_capabilities
/
/ a light-weight WMS server / GCI supporting RasterLite2 DataSources
/ creating the GetCapabilities XML file 
/
/ version 2.0, 2021 March 3
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


static char *
clean_xml (const char *dirty)
{
/* cleaning an XML string by masking illegal chars */
    int count = 0;
    const char *in = dirty;
    char *out;
    char *clean = NULL;
    if (dirty == NULL)
	return NULL;
    while (*in != '\0')
      {
	  /* counting how many illegal chars */
	  switch (*in)
	    {
	    case '<':
		count += 4;
		break;
	    case '>':
		count += 4;
		break;
	    case '&':
		count += 5;
		break;
	    case '"':
		count += 6;
		break;
	    default:
		count++;
		break;
	    };
	  in++;
      }

    clean = malloc (count + 1);
    out = clean;
    in = dirty;
    while (*in != '\0')
      {
	  /* masking illegal chars */
	  switch (*in)
	    {
	    case '<':
		*out++ = '&';
		*out++ = 'l';
		*out++ = 't';
		*out++ = ';';
		break;
	    case '>':
		*out++ = '&';
		*out++ = 'g';
		*out++ = 't';
		*out++ = ';';
		break;
	    case '&':
		*out++ = '&';
		*out++ = 'a';
		*out++ = 'm';
		*out++ = 'p';
		*out++ = ';';
		break;
	    case '"':
		*out++ = '&';
		*out++ = 'q';
		*out++ = 'u';
		*out++ = 'o';
		*out++ = 't';
		*out++ = ';';
		break;
	    default:
		*out++ = *in;
		break;
	    };
	  in++;
      }
    *out = '\0';
    return clean;
}

static char *
print_raster_layer (const char *alias_name, WmsLiteConfigPtr list,
		    WmsLiteRasterPtr lyr, double min_scale, double max_scale,
		    const char *tabpad, int version)
{
/* preparing the XML fragment for a single WmsLite Raster Layer */
    char *xml;
    char *prev;
    char *cleared;
    const char *dirty;
    WmsLiteKeywordPtr key;
    WmsLiteCrsPtr crs;
    WmsLiteStylePtr style;

    dirty = alias_name;
    cleared = clean_xml (dirty);
    xml = sqlite3_mprintf ("%s\t\t<Name>%s</Name>\r\n", tabpad, cleared);
    free (cleared);
    if (lyr->Title != NULL)
      {
	  prev = xml;
	  dirty = lyr->Title;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s%s\t\t<Title>%s</Title>\r\n", prev, tabpad,
			       cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }
    if (lyr->Abstract != NULL)
      {
	  prev = xml;
	  dirty = lyr->Abstract;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s%s\t\t<Abstract>%s</Abstract>\r\n", prev,
			       tabpad, cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }
    if (lyr->KeyFirst != NULL)
      {
	  prev = xml;
	  if (version == WMS_VERSION_100)
	      xml = sqlite3_mprintf ("%s%s\t\t<Keywords>\r\n", prev, tabpad);
	  else
	      xml = sqlite3_mprintf ("%s%s\t\t<KeywordList>\r\n", prev, tabpad);
	  sqlite3_free (prev);
	  key = lyr->KeyFirst;
	  while (key != NULL)
	    {
		prev = xml;
		if (key->Keyword == NULL)
		    dirty = "unknown";
		else
		    dirty = key->Keyword;
		cleared = clean_xml (dirty);
		if (version == WMS_VERSION_100)
		    xml = sqlite3_mprintf ("%s%s ", prev, cleared);
		else
		    xml =
			sqlite3_mprintf ("%s%s\t\t\t<Keyword>%s</Keyword>\r\n",
					 prev, tabpad, cleared);
		free (cleared);
		sqlite3_free (prev);
		key = key->Next;
	    }
	  prev = xml;
	  if (version == WMS_VERSION_100)
	      xml = sqlite3_mprintf ("%s%s\t\t</Keywords>\r\n", prev, tabpad);
	  else
	      xml =
		  sqlite3_mprintf ("%s%s\t\t</KeywordList>\r\n", prev, tabpad);
	  sqlite3_free (prev);
      }
    prev = xml;
    if (version == WMS_VERSION_130)
	xml =
	    sqlite3_mprintf ("%s%s\t\t<CRS>EPSG:%d</CRS>\r\n", prev, tabpad,
			     lyr->Srid);
    else
	xml =
	    sqlite3_mprintf ("%s%s\t\t<SRS>EPSG:%d</SRS>\r\n", prev, tabpad,
			     lyr->Srid);
    sqlite3_free (prev);
    if (version == WMS_VERSION_130)
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s%s\t\t<EX_GeographicBoundingBox>\r\n", prev,
			       tabpad);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t\t<westBoundLongitude>%1.2f</westBoundLongitude>\r\n",
	       prev, tabpad, lyr->GeographicBBox->MinX);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t\t<eastBoundLongitude>%1.2f</eastBoundLongitude>\r\n",
	       prev, tabpad, lyr->GeographicBBox->MaxX);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t\t<southBoundLatitude>%1.2f</southBoundLatitude>\r\n",
	       prev, tabpad, lyr->GeographicBBox->MinY);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t\t<northBoundLatitude>%1.2f</northBoundLatitude>\r\n",
	       prev, tabpad, lyr->GeographicBBox->MaxY);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s%s\t\t</EX_GeographicBoundingBox>\r\n", prev,
			       tabpad);
	  sqlite3_free (prev);
      }
    else
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t<LatLonBoundingBox minx=\"%1.2f\" miny=\"%1.2f\" maxx=\"%1.2f\" maxy=\"%1.2f\" />\r\n",
	       prev, tabpad, lyr->GeographicBBox->MinX,
	       lyr->GeographicBBox->MinY, lyr->GeographicBBox->MaxX,
	       lyr->GeographicBBox->MaxY);
	  sqlite3_free (prev);
      }
    crs = lyr->CrsFirst;
    while (crs != NULL)
      {
	  if (crs->BBox == NULL)
	    {
		/* skipping any undefined BBox */
		crs = crs->Next;
		continue;
	    }
	  prev = xml;
	  if (version == WMS_VERSION_130)
	      xml =
		  sqlite3_mprintf
		  ("%s%s\t\t<BoundingBox CRS=\"EPSG:%d\" minx=\"%1.5f\" "
		   "miny=\"%1.5f\" maxx=\"%1.5f\" maxy=\"%1.5f\" />\r\n", prev,
		   tabpad, crs->Srid, crs->BBox->MinX, crs->BBox->MinY,
		   crs->BBox->MaxX, crs->BBox->MaxY);
	  else
	      xml =
		  sqlite3_mprintf
		  ("%s%s\t\t<BoundingBox SRS=\"EPSG:%d\" minx=\"%1.5f\" "
		   "miny=\"%1.5f\" maxx=\"%1.5f\" maxy=\"%1.5f\" />\r\n", prev,
		   tabpad, crs->Srid, crs->BBox->MinX, crs->BBox->MinY,
		   crs->BBox->MaxX, crs->BBox->MaxY);
	  sqlite3_free (prev);
	  crs = crs->Next;
      }
    style = lyr->StyleFirst;
    while (style != NULL)
      {
	  if (style->Name == NULL)
	    {
		/* skipping any unnamed style */
		style = style->Next;
		continue;
	    }
	  prev = xml;
	  xml = sqlite3_mprintf ("%s%s\t\t<Style>\r\n", prev, tabpad);
	  sqlite3_free (prev);
	  prev = xml;
	  dirty = style->Name;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s%s\t\t\t<Name>%s</Name>\r\n", prev, tabpad,
			       cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  if (style->Title != NULL)
	    {
		prev = xml;
		dirty = style->Title;
		cleared = clean_xml (dirty);
		xml =
		    sqlite3_mprintf ("%s%s\t\t\t<Title>%s</Title>\r\n", prev,
				     tabpad, cleared);
		free (cleared);
		sqlite3_free (prev);
	    }
	  if (style->Abstract != NULL)
	    {
		prev = xml;
		dirty = style->Abstract;
		cleared = clean_xml (dirty);
		xml =
		    sqlite3_mprintf ("%s%s\t\t\t<Abstract>%s</Abstract>\r\n",
				     prev, tabpad, cleared);
		free (cleared);
		sqlite3_free (prev);
	    }
	  if (list->EnableLegendURL)
	    {
		const char *fmt;
		char *resource;
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s%s\t\t\t<LegendURL width=\"%d\" height=\"%d\">\r\n",
		     prev, tabpad, list->LegendWidth, list->LegendHeight);
		sqlite3_free (prev);
		fmt = list->LegendFormat;
		if (fmt == NULL)
		    fmt = "image/png";
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s%s\t\t\t\t<Format>%s</Format>\r\n",
				     prev, tabpad, fmt);
		sqlite3_free (prev);
		resource =
		    sqlite3_mprintf
		    ("%sservice=WMS&version=1.3.0&request=GetLegendGraphic&layer=%s&format=%s&style=%s",
		     list->OnlineResource, alias_name, fmt, style->Name);
		cleared = clean_xml (resource);
		sqlite3_free (resource);
		prev = xml;
		if (version == WMS_VERSION_100)
		    xml =
			sqlite3_mprintf
			("%s%s\t\t\t\t<OnlineResource>%s</OnlineResource>\r\n",
			 prev, tabpad, cleared);
		else
		    xml =
			sqlite3_mprintf
			("%s%s\t\t\t\t<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
			 "xlink:type=\"simple\" xlink:href=\"%s\" />\r\n", prev,
			 tabpad, cleared);
		free (cleared);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s%s\t\t\t</LegendURL>\r\n", prev,
				     tabpad);
		sqlite3_free (prev);
	    }
	  prev = xml;
	  xml = sqlite3_mprintf ("%s%s\t\t</Style>\r\n", prev, tabpad);
	  sqlite3_free (prev);
	  style = style->Next;
      }
    if (min_scale >= 0.0)
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t<MinScaleDenominator>%1.0f</MinScaleDenominator>\r\n",
	       prev, tabpad, min_scale);
	  sqlite3_free (prev);
      }
    if (max_scale >= 0.0)
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t<MaxScaleDenominator>%1.0f</MaxScaleDenominator>\r\n",
	       prev, tabpad, max_scale);
	  sqlite3_free (prev);
      }
    return xml;
}

static char *
print_vector_layer (const char *alias_name, WmsLiteConfigPtr list,
		    WmsLiteVectorPtr lyr, double min_scale, double max_scale,
		    const char *tabpad, int version)
{
/* preparing the XML fragment for a single WmsLite Vector Layer */
    char *xml;
    char *prev;
    char *cleared;
    const char *dirty;
    WmsLiteKeywordPtr key;
    WmsLiteCrsPtr crs;
    WmsLiteStylePtr style;

    dirty = alias_name;
    cleared = clean_xml (dirty);
    xml = sqlite3_mprintf ("%s\t\t<Name>%s</Name>\r\n", tabpad, cleared);
    free (cleared);
    if (lyr->Title != NULL)
      {
	  prev = xml;
	  dirty = lyr->Title;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s%s\t\t<Title>%s</Title>\r\n", prev, tabpad,
			       cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }
    if (lyr->Abstract != NULL)
      {
	  prev = xml;
	  dirty = lyr->Abstract;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s%s\t\t<Abstract>%s</Abstract>\r\n", prev,
			       tabpad, cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }
    if (lyr->KeyFirst != NULL)
      {
	  prev = xml;
	  if (version == WMS_VERSION_100)
	      xml = sqlite3_mprintf ("%s%s\t\t<Keywords>\r\n", prev, tabpad);
	  else
	      xml = sqlite3_mprintf ("%s%s\t\t<KeywordList>\r\n", prev, tabpad);
	  sqlite3_free (prev);
	  key = lyr->KeyFirst;
	  while (key != NULL)
	    {
		prev = xml;
		if (key->Keyword == NULL)
		    dirty = "unknown";
		else
		    dirty = key->Keyword;
		cleared = clean_xml (dirty);
		if (version == WMS_VERSION_100)
		    xml = sqlite3_mprintf ("%s%s ", prev, cleared);
		else
		    xml =
			sqlite3_mprintf ("%s%s\t\t\t<Keyword>%s</Keyword>\r\n",
					 prev, tabpad, cleared);
		free (cleared);
		sqlite3_free (prev);
		key = key->Next;
	    }
	  prev = xml;
	  if (version == WMS_VERSION_100)
	      xml = sqlite3_mprintf ("%s%s\t\t</Keywords>\r\n", prev, tabpad);
	  else
	      xml =
		  sqlite3_mprintf ("%s%s\t\t</KeywordList>\r\n", prev, tabpad);
	  sqlite3_free (prev);
      }
    prev = xml;
    if (version == WMS_VERSION_130)
	xml =
	    sqlite3_mprintf ("%s%s\t\t<CRS>EPSG:%d</CRS>\r\n", prev, tabpad,
			     lyr->Srid);
    else
	xml =
	    sqlite3_mprintf ("%s%s\t\t<SRS>EPSG:%d</SRS>\r\n", prev, tabpad,
			     lyr->Srid);
    sqlite3_free (prev);
    if (version == WMS_VERSION_130)
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s%s\t\t<EX_GeographicBoundingBox>\r\n", prev,
			       tabpad);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t\t<westBoundLongitude>%1.2f</westBoundLongitude>\r\n",
	       prev, tabpad, lyr->GeographicBBox->MinX);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t\t<eastBoundLongitude>%1.2f</eastBoundLongitude>\r\n",
	       prev, tabpad, lyr->GeographicBBox->MaxX);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t\t<southBoundLatitude>%1.2f</southBoundLatitude>\r\n",
	       prev, tabpad, lyr->GeographicBBox->MinY);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t\t<northBoundLatitude>%1.2f</northBoundLatitude>\r\n",
	       prev, tabpad, lyr->GeographicBBox->MaxY);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s%s\t\t</EX_GeographicBoundingBox>\r\n", prev,
			       tabpad);
	  sqlite3_free (prev);
      }
    else
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t<LatLonBoundingBox minx=\"%1.2f\" miny=\"%1.2f\" maxx=\"%1.2f\" maxy=\"%1.2f\" />\r\n",
	       prev, tabpad, lyr->GeographicBBox->MinX,
	       lyr->GeographicBBox->MinY, lyr->GeographicBBox->MaxX,
	       lyr->GeographicBBox->MaxY);
	  sqlite3_free (prev);
      }
    crs = lyr->CrsFirst;
    while (crs != NULL)
      {
	  if (crs->BBox == NULL)
	    {
		/* skipping any undefined BBox */
		crs = crs->Next;
		continue;
	    }
	  prev = xml;
	  if (version == WMS_VERSION_130)
	      xml =
		  sqlite3_mprintf
		  ("%s%s\t\t<BoundingBox CRS=\"EPSG:%d\" minx=\"%1.5f\" "
		   "miny=\"%1.5f\" maxx=\"%1.5f\" maxy=\"%1.5f\" />\r\n", prev,
		   tabpad, crs->Srid, crs->BBox->MinX, crs->BBox->MinY,
		   crs->BBox->MaxX, crs->BBox->MaxY);
	  else
	      xml =
		  sqlite3_mprintf
		  ("%s%s\t\t<BoundingBox SRS=\"EPSG:%d\" minx=\"%1.5f\" "
		   "miny=\"%1.5f\" maxx=\"%1.5f\" maxy=\"%1.5f\" />\r\n", prev,
		   tabpad, crs->Srid, crs->BBox->MinX, crs->BBox->MinY,
		   crs->BBox->MaxX, crs->BBox->MaxY);
	  sqlite3_free (prev);
	  crs = crs->Next;
      }
    style = lyr->StyleFirst;
    while (style != NULL)
      {
	  if (style->Name == NULL)
	    {
		/* skipping any unnamed style */
		style = style->Next;
		continue;
	    }
	  prev = xml;
	  xml = sqlite3_mprintf ("%s%s\t\t<Style>\r\n", prev, tabpad);
	  sqlite3_free (prev);
	  prev = xml;
	  dirty = style->Name;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s%s\t\t\t<Name>%s</Name>\r\n", prev, tabpad,
			       cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  if (style->Title != NULL)
	    {
		prev = xml;
		dirty = style->Title;
		cleared = clean_xml (dirty);
		xml =
		    sqlite3_mprintf ("%s%s\t\t\t<Title>%s</Title>\r\n", prev,
				     tabpad, cleared);
		free (cleared);
		sqlite3_free (prev);
	    }
	  if (style->Abstract != NULL)
	    {
		prev = xml;
		dirty = style->Abstract;
		cleared = clean_xml (dirty);
		xml =
		    sqlite3_mprintf ("%s%s\t\t\t<Abstract>%s</Abstract>\r\n",
				     prev, tabpad, cleared);
		free (cleared);
		sqlite3_free (prev);
	    }
	  if (list->EnableLegendURL)
	    {
		const char *fmt;
		char *resource;
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s%s\t\t\t<LegendURL width=\"%d\" height=\"%d\">\r\n",
		     prev, tabpad, list->LegendWidth, list->LegendHeight);
		sqlite3_free (prev);
		fmt = list->LegendFormat;
		if (fmt == NULL)
		    fmt = "image/png";
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s%s\t\t\t\t<Format>%s</Format>\r\n",
				     prev, tabpad, fmt);
		sqlite3_free (prev);
		resource =
		    sqlite3_mprintf
		    ("%sservice=WMS&version=1.3.0&request=GetLegendGraphic&layer=%s&format=%s&style=%s",
		     list->OnlineResource, alias_name, fmt, style->Name);
		cleared = clean_xml (resource);
		sqlite3_free (resource);
		prev = xml;
		if (version == WMS_VERSION_100)
		    xml =
			sqlite3_mprintf
			("%s%s\t\t\t\t<OnlineResource>%s</OnlineResource>\r\n",
			 prev, tabpad, cleared);
		else
		    xml =
			sqlite3_mprintf
			("%s%s\t\t\t\t<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
			 "xlink:type=\"simple\" xlink:href=\"%s\" />\r\n", prev,
			 tabpad, cleared);
		free (cleared);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s%s\t\t\t</LegendURL>\r\n", prev,
				     tabpad);
		sqlite3_free (prev);
	    }
	  prev = xml;
	  xml = sqlite3_mprintf ("%s%s\t\t</Style>\r\n", prev, tabpad);
	  sqlite3_free (prev);
	  if (min_scale >= 0.0)
	    {
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s%s\t\t<MinScaleDenominator>%1.0f</MinScaleDenominator>\r\n",
		     prev, tabpad, min_scale);
		sqlite3_free (prev);
	    }
	  if (max_scale >= 0.0)
	    {
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s%s\t\t<MaxScaleDenominator>%1.0f</MaxScaleDenominator>\r\n",
		     prev, tabpad, max_scale);
		sqlite3_free (prev);
	    }
	  style = style->Next;
      }
    return xml;
}

static char *
print_cascaded_wms_layer (const char *alias_name,
			  WmsLiteCascadedWmsPtr lyr, double min_scale,
			  double max_scale, const char *tabpad, int version)
{
/* preparing the XML fragment for a single WmsLite Raster Layer */
    char *xml;
    char *prev;
    char *cleared;
    const char *dirty;
    WmsLiteKeywordPtr key;
    WmsLiteWmsCrsPtr crs;
    WmsLiteStylePtr style;

    dirty = alias_name;
    cleared = clean_xml (dirty);
    xml = sqlite3_mprintf ("%s\t\t<Name>%s</Name>\r\n", tabpad, cleared);
    free (cleared);
    if (lyr->Title != NULL)
      {
	  prev = xml;
	  dirty = lyr->Title;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s%s\t\t<Title>%s</Title>\r\n", prev, tabpad,
			       cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }
    if (lyr->Abstract != NULL)
      {
	  prev = xml;
	  dirty = lyr->Abstract;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s%s\t\t<Abstract>%s</Abstract>\r\n", prev,
			       tabpad, cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }
    if (lyr->KeyFirst != NULL)
      {
	  /* NOTE: Keywords are not supported on Cascaded WMS */
	  prev = xml;
	  if (version == WMS_VERSION_100)
	      xml = sqlite3_mprintf ("%s%s\t\t<Keywords>\r\n", prev, tabpad);
	  else
	      xml = sqlite3_mprintf ("%s%s\t\t<KeywordList>\r\n", prev, tabpad);
	  sqlite3_free (prev);
	  key = lyr->KeyFirst;
	  while (key != NULL)
	    {
		prev = xml;
		if (key->Keyword == NULL)
		    dirty = "unknown";
		else
		    dirty = key->Keyword;
		cleared = clean_xml (dirty);
		if (version == WMS_VERSION_100)
		    xml = sqlite3_mprintf ("%s%s ", prev, cleared);
		else
		    xml =
			sqlite3_mprintf ("%s%s\t\t\t<Keyword>%s</Keyword>\r\n",
					 prev, tabpad, cleared);
		free (cleared);
		sqlite3_free (prev);
		key = key->Next;
	    }
	  prev = xml;
	  if (version == WMS_VERSION_100)
	      xml = sqlite3_mprintf ("%s%s\t\t</Keywords>\r\n", prev, tabpad);
	  else
	      xml =
		  sqlite3_mprintf ("%s%s\t\t</KeywordList>\r\n", prev, tabpad);
	  sqlite3_free (prev);
      }
    if (lyr->Srs != NULL)
      {
	  prev = xml;
	  if (version == WMS_VERSION_130)
	      xml =
		  sqlite3_mprintf ("%s%s\t\t<CRS>%s</CRS>\r\n", prev, tabpad,
				   lyr->Srs);
	  else
	      xml =
		  sqlite3_mprintf ("%s%s\t\t<SRS>%s</SRS>\r\n", prev, tabpad,
				   lyr->Srs);
	  sqlite3_free (prev);
      }
    if (lyr->GeographicBBox != NULL)
      {
	  if (version == WMS_VERSION_130)
	    {
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s%s\t\t<EX_GeographicBoundingBox>\r\n",
				     prev, tabpad);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s%s\t\t\t<westBoundLongitude>%1.2f</westBoundLongitude>\r\n",
		     prev, tabpad, lyr->GeographicBBox->MinX);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s%s\t\t\t<eastBoundLongitude>%1.2f</eastBoundLongitude>\r\n",
		     prev, tabpad, lyr->GeographicBBox->MaxX);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s%s\t\t\t<southBoundLatitude>%1.2f</southBoundLatitude>\r\n",
		     prev, tabpad, lyr->GeographicBBox->MinY);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s%s\t\t\t<northBoundLatitude>%1.2f</northBoundLatitude>\r\n",
		     prev, tabpad, lyr->GeographicBBox->MaxY);
		sqlite3_free (prev);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s%s\t\t</EX_GeographicBoundingBox>\r\n",
				     prev, tabpad);
		sqlite3_free (prev);
	    }
	  else
	    {
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s%s\t\t<LatLonBoundingBox minx=\"%1.2f\" miny=\"%1.2f\" maxx=\"%1.2f\" maxy=\"%1.2f\" />\r\n",
		     prev, tabpad, lyr->GeographicBBox->MinX,
		     lyr->GeographicBBox->MinY, lyr->GeographicBBox->MaxX,
		     lyr->GeographicBBox->MaxY);
		sqlite3_free (prev);
	    }
      }
    crs = lyr->CrsFirst;
    while (crs != NULL)
      {
	  if (crs->BBox == NULL)
	    {
		/* skipping any undefined BBox */
		crs = crs->Next;
		continue;
	    }
	  prev = xml;
	  if (version == WMS_VERSION_130)
	      xml =
		  sqlite3_mprintf
		  ("%s%s\t\t<BoundingBox CRS=\"%s\" minx=\"%1.5f\" "
		   "miny=\"%1.5f\" maxx=\"%1.5f\" maxy=\"%1.5f\" />\r\n", prev,
		   tabpad, crs->Srs, crs->BBox->MinX, crs->BBox->MinY,
		   crs->BBox->MaxX, crs->BBox->MaxY);
	  else
	      xml =
		  sqlite3_mprintf
		  ("%s%s\t\t<BoundingBox SRS=\"%s\" minx=\"%1.5f\" "
		   "miny=\"%1.5f\" maxx=\"%1.5f\" maxy=\"%1.5f\" />\r\n", prev,
		   tabpad, crs->Srs, crs->BBox->MinX, crs->BBox->MinY,
		   crs->BBox->MaxX, crs->BBox->MaxY);
	  sqlite3_free (prev);
	  crs = crs->Next;
      }
    style = lyr->StyleFirst;
    while (style != NULL)
      {
	  if (style->Name == NULL)
	    {
		/* skipping any unnamed style */
		style = style->Next;
		continue;
	    }
	  prev = xml;
	  xml = sqlite3_mprintf ("%s%s\t\t<Style>\r\n", prev, tabpad);
	  sqlite3_free (prev);
	  prev = xml;
	  dirty = style->Name;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s%s\t\t\t<Name>%s</Name>\r\n", prev, tabpad,
			       cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  if (style->Title != NULL)
	    {
		prev = xml;
		dirty = style->Title;
		cleared = clean_xml (dirty);
		xml =
		    sqlite3_mprintf ("%s%s\t\t\t<Title>%s</Title>\r\n", prev,
				     tabpad, cleared);
		free (cleared);
		sqlite3_free (prev);
	    }
	  if (style->Abstract != NULL)
	    {
		prev = xml;
		dirty = style->Abstract;
		cleared = clean_xml (dirty);
		xml =
		    sqlite3_mprintf ("%s%s\t\t\t<Abstract>%s</Abstract>\r\n",
				     prev, tabpad, cleared);
		free (cleared);
		sqlite3_free (prev);
	    }
/* note: Legend URL is unsupported on Cascaded WMS */
	  prev = xml;
	  xml = sqlite3_mprintf ("%s%s\t\t</Style>\r\n", prev, tabpad);
	  sqlite3_free (prev);
	  if (min_scale >= 0.0)
	    {
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s%s\t\t<MinScaleDenominator>%1.0f</MinScaleDenominator>\r\n",
		     prev, tabpad, min_scale);
		sqlite3_free (prev);
	    }
	  if (max_scale >= 0.0)
	    {
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s%s\t\t<MaxScaleDenominator>%1.0f</MaxScaleDenominator>\r\n",
		     prev, tabpad, max_scale);
		sqlite3_free (prev);
	    }
	  style = style->Next;
      }
    return xml;
}

static char *
print_map_config_main_child_layer (WmsLiteLayerPtr lyr,
				   WmsLiteConfigPtr list,
				   const char *tabpad, int version)
{
/* printing a MapConfig's Child Layer on MAIN DB */
    char *prev;
    char *xml;
    char *xml2;

    if (lyr->Type == WMS_LAYER_CASCADED_WMS && lyr->CascadedWMS != NULL)
      {
	  int opaque;
	  if (lyr->CascadedWMS->IsTransparent)
	      opaque = 0;
	  else
	      opaque = 1;
	  if (lyr->CascadedWMS->Cascaded >= 0)
	      xml =
		  sqlite3_mprintf
		  ("%s\t<Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"%d\">\r\n",
		   tabpad, is_queryable_wmslite_layer (lyr), opaque,
		   lyr->CascadedWMS->Cascaded + 1);
	  else
	      xml =
		  sqlite3_mprintf
		  ("%s\t<Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"1\">\r\n",
		   tabpad, is_queryable_wmslite_layer (lyr), opaque);
      }
    else
	xml =
	    sqlite3_mprintf ("%s\t<Layer queryable=\"%d\">\r\n",
			     tabpad, is_queryable_wmslite_layer (lyr));
    xml2 = NULL;
    switch (lyr->Type)
      {
      case WMS_LAYER_CASCADED_WMS:
	  xml2 =
	      print_cascaded_wms_layer (lyr->AliasName, lyr->CascadedWMS,
					lyr->MinScaleDenominator,
					lyr->MaxScaleDenominator, tabpad,
					version);
	  break;
      case WMS_LAYER_RASTER:
	  xml2 =
	      print_raster_layer (lyr->AliasName, list, lyr->Raster,
				  lyr->MinScaleDenominator,
				  lyr->MaxScaleDenominator, tabpad, version);
	  break;
      case WMS_LAYER_VECTOR:
	  xml2 =
	      print_vector_layer (lyr->AliasName, list, lyr->Vector,
				  lyr->MinScaleDenominator,
				  lyr->MaxScaleDenominator, tabpad, version);
	  break;
      };
    if (xml2 != NULL)
      {
	  prev = xml;
	  xml = sqlite3_mprintf ("%s%s", prev, xml2);
	  sqlite3_free (prev);
	  sqlite3_free (xml2);
      }
    prev = xml;
    xml = sqlite3_mprintf ("%s%s\t</Layer>\r\n", prev, tabpad);
    sqlite3_free (prev);
    return xml;
}

static char *
print_map_config_attached_child_layer (WmsLiteAttachedLayerPtr lyr,
				       WmsLiteConfigPtr list,
				       const char *tabpad, int version)
{
/* printing a MapConfig's Child Layer on some ATTACHED DB */
    char *prev;
    char *xml;
    char *xml2;

    if (lyr->Type == WMS_LAYER_CASCADED_WMS && lyr->CascadedWMS != NULL)
      {
	  int opaque;
	  if (lyr->CascadedWMS->IsTransparent)
	      opaque = 0;
	  else
	      opaque = 1;
	  if (lyr->CascadedWMS->Cascaded >= 0)
	      xml =
		  sqlite3_mprintf
		  ("%s\t<Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"%d\">\r\n",
		   tabpad, is_queryable_wmslite_attached_layer (lyr), opaque,
		   lyr->CascadedWMS->Cascaded + 1);
	  else
	      xml =
		  sqlite3_mprintf
		  ("%s\t<Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"1\">\r\n",
		   tabpad, is_queryable_wmslite_attached_layer (lyr), opaque);
      }
    else
	xml =
	    sqlite3_mprintf ("%s\t<Layer queryable=\"%d\">\r\n",
			     tabpad, is_queryable_wmslite_attached_layer (lyr));
    xml2 = NULL;
    switch (lyr->Type)
      {
      case WMS_LAYER_CASCADED_WMS:
	  xml2 =
	      print_cascaded_wms_layer (lyr->AliasName, lyr->CascadedWMS,
					lyr->MinScaleDenominator,
					lyr->MaxScaleDenominator, tabpad,
					version);
	  break;
      case WMS_LAYER_RASTER:
	  xml2 =
	      print_raster_layer (lyr->AliasName, list, lyr->Raster,
				  lyr->MinScaleDenominator,
				  lyr->MaxScaleDenominator, tabpad, version);
	  break;
      case WMS_LAYER_VECTOR:
	  xml2 =
	      print_vector_layer (lyr->AliasName, list, lyr->Vector,
				  lyr->MinScaleDenominator,
				  lyr->MaxScaleDenominator, tabpad, version);
	  break;
      };
    if (xml2 != NULL)
      {
	  prev = xml;
	  xml = sqlite3_mprintf ("%s%s", prev, xml2);
	  sqlite3_free (prev);
	  sqlite3_free (xml2);
      }
    if (lyr->MinScaleDenominator >= 0.0)
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t<MinScaleDenominator>%1.0f</MinScaleDenominator>\r\n",
	       prev, tabpad, lyr->MinScaleDenominator);
	  sqlite3_free (prev);
      }
    if (lyr->MaxScaleDenominator >= 0.0)
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t<MaxScaleDenominator>%1.0f</MaxScaleDenominator>\r\n",
	       prev, tabpad, lyr->MaxScaleDenominator);
	  sqlite3_free (prev);
      }
    prev = xml;
    xml = sqlite3_mprintf ("%s%s\t</Layer>\r\n", prev, tabpad);
    sqlite3_free (prev);
    return xml;
}

static char *
print_map_config_child_layer (const char *alias_name,
			      WmsLiteConfigPtr list, const char *tabpad,
			      int version)
{
/* printing a MapConfig's Child Layer */
    WmsLiteAttachedPtr db;
    WmsLiteAttachedLayerPtr attLyr;
    WmsLiteLayerPtr lyr;
    lyr = list->MainFirst;
    while (lyr != NULL)
      {
	  /* searching for a Child Layer on MAIN DB */
	  if (!is_valid_wmslite_layer (lyr) || !(lyr->ChildLayer))
	    {
		/* skipping any invalid or non-Child Layer */
		lyr = lyr->Next;
		continue;
	    }
	  if (strcasecmp (alias_name, lyr->AliasName) == 0)
	      return print_map_config_main_child_layer (lyr, list, tabpad,
							version);
	  lyr = lyr->Next;
      }
    db = list->DbFirst;
    while (db != NULL)
      {
	  /* searching for a Child Layer on some ATTACHED DB */
	  if (db->Valid == 0)
	    {
		/* skipping any invalid ATTACHED DB */
		db = db->Next;
		continue;
	    }
	  attLyr = db->First;
	  while (attLyr != NULL)
	    {
		if (!is_valid_wmslite_attached_layer (attLyr)
		    || !(attLyr->ChildLayer))
		  {
		      /* skipping any invalid or non-Child Layer */
		      attLyr = attLyr->Next;
		      continue;
		  }
		if (strcasecmp (alias_name, attLyr->AliasName) == 0)
		    return print_map_config_attached_child_layer (attLyr, list,
								  tabpad,
								  version);
		attLyr = attLyr->Next;
	    }
	  db = db->Next;
      }
    return NULL;
}

static char *
print_map_config_layer (const char *alias_name, WmsLiteConfigPtr list,
			rl2MapConfigPtr lyr, WmsLiteMapConfigMultiSRIDPtr srids,
			WmsLiteChildLayerPtr children, double min_scale,
			double max_scale, const char *tabpad, int version)
{
/* preparing the XML fragment for a single WmsLite MapConfig Layer */
    char *xml;
    char *prev;
    char *cleared;
    const char *dirty;
    WmsLiteMapConfigSRIDPtr crs;

    dirty = alias_name;
    cleared = clean_xml (dirty);
    xml = sqlite3_mprintf ("%s\t\t<Name>%s</Name>\r\n", tabpad, cleared);
    free (cleared);
    if (lyr->title != NULL)
      {
	  prev = xml;
	  dirty = lyr->title;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s%s\t\t<Title>%s</Title>\r\n", prev, tabpad,
			       cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }
    if (lyr->abstract != NULL)
      {
	  prev = xml;
	  dirty = lyr->abstract;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s%s\t\t<Abstract>%s</Abstract>\r\n", prev,
			       tabpad, cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }

/* Map Configuration has no Keywords */

    prev = xml;
    if (version == WMS_VERSION_130)
	xml =
	    sqlite3_mprintf ("%s%s\t\t<CRS>EPSG:%d</CRS>\r\n", prev, tabpad,
			     srids->Srid);
    else
	xml =
	    sqlite3_mprintf ("%s%s\t\t<SRS>EPSG:%d</SRS>\r\n", prev, tabpad,
			     srids->Srid);
    sqlite3_free (prev);
    if (version == WMS_VERSION_130)
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s%s\t\t<EX_GeographicBoundingBox>\r\n", prev,
			       tabpad);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t\t<westBoundLongitude>%1.2f</westBoundLongitude>\r\n",
	       prev, tabpad, srids->GeographicBBox->MinX);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t\t<eastBoundLongitude>%1.2f</eastBoundLongitude>\r\n",
	       prev, tabpad, srids->GeographicBBox->MaxX);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t\t<southBoundLatitude>%1.2f</southBoundLatitude>\r\n",
	       prev, tabpad, srids->GeographicBBox->MinY);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t\t<northBoundLatitude>%1.2f</northBoundLatitude>\r\n",
	       prev, tabpad, srids->GeographicBBox->MaxY);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s%s\t\t</EX_GeographicBoundingBox>\r\n", prev,
			       tabpad);
	  sqlite3_free (prev);
      }
    else
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t<LatLonBoundingBox minx=\"%1.2f\" miny=\"%1.2f\" maxx=\"%1.2f\" maxy=\"%1.2f\" />\r\n",
	       prev, tabpad, srids->GeographicBBox->MinX,
	       srids->GeographicBBox->MinY, srids->GeographicBBox->MaxX,
	       srids->GeographicBBox->MaxY);
	  sqlite3_free (prev);
      }
    crs = srids->First;
    while (crs != NULL)
      {
	  prev = xml;
	  if (version == WMS_VERSION_130)
	      xml =
		  sqlite3_mprintf
		  ("%s%s\t\t<BoundingBox CRS=\"EPSG:%d\" minx=\"%1.5f\" "
		   "miny=\"%1.5f\" maxx=\"%1.5f\" maxy=\"%1.5f\" />\r\n", prev,
		   tabpad, crs->Srid, crs->BBox->MinX, crs->BBox->MinY,
		   crs->BBox->MaxX, crs->BBox->MaxY);
	  else
	      xml =
		  sqlite3_mprintf
		  ("%s%s\t\t<BoundingBox SRS=\"EPSG:%d\" minx=\"%1.5f\" "
		   "miny=\"%1.5f\" maxx=\"%1.5f\" maxy=\"%1.5f\" />\r\n", prev,
		   tabpad, crs->Srid, crs->BBox->MinX, crs->BBox->MinY,
		   crs->BBox->MaxX, crs->BBox->MaxY);
	  sqlite3_free (prev);
	  crs = crs->Next;
      }

/* Map Configuration has no Styles */
    if (min_scale >= 0.0)
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t<MinScaleDenominator>%1.0f</MinScaleDenominator>\r\n",
	       prev, tabpad, min_scale);
	  sqlite3_free (prev);
      }
    if (max_scale >= 0.0)
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s%s\t\t<MaxScaleDenominator>%1.0f</MaxScaleDenominator>\r\n",
	       prev, tabpad, max_scale);
	  sqlite3_free (prev);
      }

    if (children != NULL)
      {
	  /* printing all children Layers of MapConfig */
	  WmsLiteChildLayerPtr child = children;
	  while (child != NULL)
	    {
		char *xml2;
		char tabpad2[64];
		sprintf (tabpad2, "%s\t", tabpad);
		xml2 =
		    print_map_config_child_layer (child->ChildAliasName, list,
						  tabpad2, version);
		if (xml2 != NULL)
		  {
		      prev = xml;
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (xml2);
		      sqlite3_free (prev);
		  }
		child = child->Next;
	    }
      }

    return xml;
}

extern char *
wmslite_create_getcapabilities (WmsLiteConfigPtr list, int version)
{
/* preparing the GetCapabilities XML document */
    char *xml;
    char *xml2;
    char *prev;
    char *cleared;
    const char *dirty;
    WmsLiteLayerPtr lyr;
    WmsLiteAttachedPtr db;
    WmsLiteAttachedLayerPtr attLyr;
    const char *tabpad;

    xml = sqlite3_mprintf ("<?xml version='1.0' encoding=\"UTF-8\" ?>\r\n");
    prev = xml;
    if (version == WMS_VERSION_100)
      {
	  xml = sqlite3_mprintf ("%s<!DOCTYPE WMT_MS_Capabilities SYSTEM "
				 "\"http://schemas.opengeospatial.net/wms/1.0.0/capabilities_1_0_0.dtd\">\r\n"
				 "<WMT_MS_Capabilities version=\"1.0.0\" updateSequence=\"0\">\r\n",
				 prev);
      }
    else if (version == WMS_VERSION_110)
      {
	  xml = sqlite3_mprintf ("%s<!DOCTYPE WMT_MS_Capabilities SYSTEM "
				 "\"http://schemas.opengeospatial.net/wms/1.1.0/capabilities_1_1_0.dtd\">\r\n"
				 "<WMT_MS_Capabilities version=\"1.1.0\" updateSequence=\"0\">\r\n",
				 prev);
      }
    else if (version == WMS_VERSION_111)
      {
	  xml = sqlite3_mprintf ("%s<!DOCTYPE WMT_MS_Capabilities SYSTEM "
				 "\"http://schemas.opengeospatial.net/wms/1.1.1/capabilities_1_1_1.dtd\">\r\n"
				 "<WMT_MS_Capabilities version=\"1.1.1\" updateSequence=\"0\">\r\n",
				 prev);
      }
    else
      {
	  /* assuming by default version 1.3.0 */
	  xml = sqlite3_mprintf ("%s<WMS_Capabilities version=\"1.3.0\" "
				 "xmlns=\"http://www.opengis.net/wms\" "
				 "xmlns:xlink=\"http://www.w3.org/1999/xlink\" "
				 "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
				 "xsi:schemaLocation=\"http://www.opengis.net/wms "
				 "http://schemas.opengis.net/wms/1.3.0/capabilities_1_3_0.xsd\">\r\n",
				 prev);
      }
    sqlite3_free (prev);
/* printing the <Service> section */
    prev = xml;
    xml = sqlite3_mprintf ("%s\t<Service>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    if (list->Name == NULL)
	dirty = "unknown";
    else
	dirty = list->Name;
    cleared = clean_xml (dirty);
    xml = sqlite3_mprintf ("%s\t\t<Name>%s</Name>\r\n", prev, cleared);
    free (cleared);
    sqlite3_free (prev);
    prev = xml;
    if (list->Title == NULL)
	dirty = "unknown";
    else
	dirty = list->Title;
    cleared = clean_xml (dirty);
    xml = sqlite3_mprintf ("%s\t\t<Title>%s</Title>\r\n", prev, cleared);
    free (cleared);
    sqlite3_free (prev);
    prev = xml;
    if (list->Abstract == NULL)
	dirty = "unknown";
    else
	dirty = list->Abstract;
    cleared = clean_xml (dirty);
    xml = sqlite3_mprintf ("%s\t\t<Abstract>%s</Abstract>\r\n", prev, cleared);
    free (cleared);
    sqlite3_free (prev);
    if (list->KeyFirst != NULL)
      {
	  /* printing the <KeywordList> sub-section */
	  WmsLiteKeywordPtr key;
	  prev = xml;
	  if (version == WMS_VERSION_100)
	      xml = sqlite3_mprintf ("%s\t\t<Keywords>", prev);
	  else
	      xml = sqlite3_mprintf ("%s\t\t<KeywordList>\r\n", prev);
	  sqlite3_free (prev);
	  key = list->KeyFirst;
	  while (key != NULL)
	    {
		prev = xml;
		if (key->Keyword == NULL)
		    dirty = "unknown";
		else
		    dirty = key->Keyword;
		cleared = clean_xml (dirty);
		if (version == WMS_VERSION_100)
		    xml = sqlite3_mprintf ("%s%s ", prev, cleared);
		else
		    xml =
			sqlite3_mprintf ("%s\t\t\t<Keyword>%s</Keyword>\r\n",
					 prev, cleared);
		free (cleared);
		sqlite3_free (prev);
		key = key->Next;
	    }
	  prev = xml;
	  if (version == WMS_VERSION_100)
	      xml = sqlite3_mprintf ("%s\t\t</Keywords>\r\n", prev);
	  else
	      xml = sqlite3_mprintf ("%s\t\t</KeywordList>\r\n", prev);
	  sqlite3_free (prev);
      }
    if (version == WMS_VERSION_100)
      {
	  prev = xml;
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  if (version == WMS_VERSION_100)
	      xml =
		  sqlite3_mprintf
		  ("%s\t\t<OnlineResource>%s</OnlineResource>\r\n",
		   prev, cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }
    else
      {
	  prev = xml;
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s\" />\r\n",
	       prev, cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t<ContactInformation>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t<ContactPersonPrimary>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->ContactPerson == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->ContactPerson;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t<ContactPerson>%s</ContactPerson>\r\n", prev,
	       cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->ContactOrganization == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->ContactOrganization;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t<ContactOrganization>%s</ContactOrganization>\r\n",
	       prev, cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t</ContactPersonPrimary>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->ContactPosition == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->ContactPosition;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t<ContactPosition>%s</ContactPosition>\r\n", prev,
	       cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t<ContactAddress>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->AddressType == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->AddressType;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<AddressType>%s</AddressType>\r\n",
			       prev, cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->Address == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->Address;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<Address>%s</Address>\r\n", prev,
			       cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->City == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->City;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<City>%s</City>\r\n", prev, cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->State == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->State;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t<StateOrProvince>%s</StateOrProvince>\r\n", prev,
	       cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->PostCode == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->PostCode;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<PostCode>%s</PostCode>\r\n", prev,
			       cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->Country == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->Country;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<Country>%s</Country>\r\n", prev,
			       cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t</ContactAddress>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->ContactEMail == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->ContactEMail;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t<ContactElectronicMailAddress>%s</ContactElectronicMailAddress>\r\n",
	       prev, cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t</ContactInformation>\r\n", prev);
	  sqlite3_free (prev);
      }
    prev = xml;
    if (list->Fees == NULL)
	dirty = "unknown";
    else
	dirty = list->Fees;
    cleared = clean_xml (dirty);
    xml = sqlite3_mprintf ("%s\t\t<Fees>%s</Fees>\r\n", prev, cleared);
    free (cleared);
    sqlite3_free (prev);
    prev = xml;
    if (list->AccessConstraints == NULL)
	dirty = "unknown";
    else
	dirty = list->AccessConstraints;
    cleared = clean_xml (dirty);
    xml =
	sqlite3_mprintf ("%s\t\t<AccessConstraints>%s</AccessConstraints>\r\n",
			 prev, cleared);
    free (cleared);
    sqlite3_free (prev);
    if (version == WMS_VERSION_130)
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t<LayerLimit>%d</LayerLimit>\r\n", prev,
			       list->LayerLimit);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t<MaxWidth>%d</MaxWidth>\r\n", prev,
			       list->MaxWidth);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t<MaxHeight>%d</MaxHeight>\r\n", prev,
			       list->MaxHeight);
	  sqlite3_free (prev);
      }
    prev = xml;
    xml = sqlite3_mprintf ("%s\t</Service>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s\t<Capability>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s\t\t<Request>\r\n", prev);
    sqlite3_free (prev);
    if (version != WMS_VERSION_100)
      {
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t<GetCapabilities>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<Format>text/xml</Format>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t<DCPType>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t<HTTP>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t\t<Get>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t\t\t\t<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s\" />\r\n",
	       prev, cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t\t</Get>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t\t<Post>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t\t\t\t<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s\" />\r\n",
	       prev, cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t\t</Post>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t</HTTP>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t</DCPType>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t</GetCapabilities>\r\n", prev);
	  sqlite3_free (prev);
      }
    prev = xml;
    if (version == WMS_VERSION_100)
	xml = sqlite3_mprintf ("%s\t\t\t<Map>\r\n", prev);
    else
	xml = sqlite3_mprintf ("%s\t\t\t<GetMap>\r\n", prev);
    sqlite3_free (prev);
    if (version == WMS_VERSION_100)
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t<Format><PNG /><JPEG /><TIFF /></Format>\r\n", prev);
	  sqlite3_free (prev);
      }
    else
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<Format>image/png</Format>\r\n",
			       prev);
	  sqlite3_free (prev);
#ifndef OMIT_LEPTONICA
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<Format>image/png8</Format>\r\n",
			       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<Format>image/gif</Format>\r\n",
			       prev);
	  sqlite3_free (prev);
#endif
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<Format>image/jpeg</Format>\r\n",
			       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<Format>image/tiff</Format>\r\n",
			       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<Format>image/geotiff</Format>\r\n",
			       prev);
	  sqlite3_free (prev);
#ifndef OMIT_LEPTONICA
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<Format>image/tiff8</Format>\r\n",
			       prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<Format>image/geotiff8</Format>\r\n",
			       prev);
	  sqlite3_free (prev);
#endif
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t<Format>application/pdf</Format>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t<Format>application/x-pdf</Format>\r\n", prev);
	  sqlite3_free (prev);
      }
    prev = xml;
    xml = sqlite3_mprintf ("%s\t\t\t\t<DCPType>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s\t\t\t\t\t<HTTP>\r\n", prev);
    sqlite3_free (prev);
    if (version == WMS_VERSION_100)
      {
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t\t\t<Get onlineResource=\"%s\" />\r\n", prev,
	       cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }
    else
      {
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t\t<Get>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t\t\t\t<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s\" />\r\n",
	       prev, cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t\t</Get>\r\n", prev);
	  sqlite3_free (prev);
      }
    if (version == WMS_VERSION_100)
      {
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t\t\t<Post onlineResource=\"%s\" />\r\n", prev,
	       cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }
    else
      {
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t\t<Post>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t\t\t\t<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s\" />\r\n",
	       prev, cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t\t</Post>\r\n", prev);
	  sqlite3_free (prev);
      }
    prev = xml;
    xml = sqlite3_mprintf ("%s\t\t\t\t\t</HTTP>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s\t\t\t\t</DCPType>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    if (version == WMS_VERSION_100)
	xml = sqlite3_mprintf ("%s\t\t\t</Map>\r\n", prev);
    else
	xml = sqlite3_mprintf ("%s\t\t\t</GetMap>\r\n", prev);
    sqlite3_free (prev);
    if (version == WMS_VERSION_100)
      {
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t<Capabilities>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml =
	      sqlite3_mprintf ("%s\t\t\t\t<Format>text/xml</Format>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t<DCPType>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t<HTTP>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t\t\t<Get onlineResource=\"%s\" />\r\n", prev,
	       cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t\t\t<Post onlineResource=\"%s\" />\r\n", prev,
	       cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t</HTTP>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t</DCPType>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t</Capabilities>\r\n", prev);
	  sqlite3_free (prev);
      }
    prev = xml;
    if (version == WMS_VERSION_100)
	xml = sqlite3_mprintf ("%s\t\t\t<FeatureInfo>\r\n", prev);
    else
	xml = sqlite3_mprintf ("%s\t\t\t<GetFeatureInfo>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s\t\t\t\t<Format>text/xml</Format>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s\t\t\t\t<DCPType>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s\t\t\t\t\t<HTTP>\r\n", prev);
    sqlite3_free (prev);
    if (version == WMS_VERSION_100)
      {
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t\t\t<Get onlineResource=\"%s\" />\r\n", prev,
	       cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }
    else
      {
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t\t<Get>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t\t\t\t<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s\" />\r\n",
	       prev, cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t\t</Get>\r\n", prev);
	  sqlite3_free (prev);
      }
    if (version == WMS_VERSION_100)
      {
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t\t\t<Post onlineResource=\"%s\" />\r\n", prev,
	       cleared);
	  free (cleared);
	  sqlite3_free (prev);
      }
    else
      {
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t\t<Post>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  if (list->OnlineResource == NULL)
	      dirty = "unknown";
	  else
	      dirty = list->OnlineResource;
	  cleared = clean_xml (dirty);
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t\t\t\t\t\t<OnlineResource xmlns:xlink=\"http://www.w3.org/1999/xlink\" xlink:type=\"simple\" xlink:href=\"%s\" />\r\n",
	       prev, cleared);
	  free (cleared);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t\t\t\t</Post>\r\n", prev);
	  sqlite3_free (prev);
      }
    prev = xml;
    xml = sqlite3_mprintf ("%s\t\t\t\t\t</HTTP>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s\t\t\t\t</DCPType>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    if (version == WMS_VERSION_100)
	xml = sqlite3_mprintf ("%s\t\t\t</FeatureInfo>\r\n", prev);
    else
	xml = sqlite3_mprintf ("%s\t\t\t</GetFeatureInfo>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    xml = sqlite3_mprintf ("%s\t\t</Request>\r\n", prev);
    sqlite3_free (prev);
    if (version == WMS_VERSION_100)
      {
	  prev = xml;
	  xml =
	      sqlite3_mprintf
	      ("%s\t\t<Exception><Format><INIMAGE /><WMS_XML /><BLANK /></Format></Exception>\r\n",
	       prev);
	  sqlite3_free (prev);
      }
    else
      {
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t<Exception>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t<Format>XML</Format>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t<Format>INIMAGE</Format>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t\t<Format>BLANK</Format>\r\n", prev);
	  sqlite3_free (prev);
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t</Exception>\r\n", prev);
	  sqlite3_free (prev);
      }
    if (list->TotalLayersCount > 1)
      {
	  if (list->TopLayerRef != NULL)
	    {
		prev = xml;
		if (list->TopLayerName == NULL)
		    xml = sqlite3_mprintf ("%s\t\t<Layer>\r\n", prev);
		else
		  {
		      if (list->TopLayerRef->Type == WMS_LAYER_CASCADED_WMS
			  && list->TopLayerRef->CascadedWMS != NULL)
			{
			    int opaque;
			    if (list->TopLayerRef->CascadedWMS->IsTransparent)
				opaque = 0;
			    else
				opaque = 1;
			    if (list->TopLayerRef->CascadedWMS->Cascaded >= 0)
				xml =
				    sqlite3_mprintf
				    ("%s\t\t<Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"%d\">\r\n",
				     prev,
				     is_queryable_wmslite_layer
				     (list->TopLayerRef), opaque,
				     list->TopLayerRef->CascadedWMS->Cascaded +
				     1);
			    else
				xml =
				    sqlite3_mprintf
				    ("%s%s\t<Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"1\">\r\n",
				     prev,
				     is_queryable_wmslite_layer
				     (list->TopLayerRef), opaque);
			}
		      else
			  xml =
			      sqlite3_mprintf
			      ("%s\t\t<Layer queryable=\"%d\">\r\n", prev,
			       is_queryable_wmslite_layer (list->TopLayerRef));
		  }
	    }
	  else if (list->TopAttachedLayerRef != NULL)
	    {
		prev = xml;
		if (list->TopLayerName == NULL)
		    xml = sqlite3_mprintf ("%s\t\t<Layer>\r\n", prev);
		else
		  {
		      if (list->TopLayerRef->Type == WMS_LAYER_CASCADED_WMS
			  && list->TopLayerRef->CascadedWMS != NULL)
			{
			    int opaque;
			    if (list->TopLayerRef->CascadedWMS->IsTransparent)
				opaque = 0;
			    else
				opaque = 1;
			    if (list->TopLayerRef->CascadedWMS->Cascaded >= 0)
				xml =
				    sqlite3_mprintf
				    ("%s\t\t<Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"%d\">\r\n",
				     prev,
				     is_queryable_wmslite_layer
				     (list->TopLayerRef), opaque,
				     list->TopLayerRef->CascadedWMS->Cascaded +
				     1);
			    else
				xml =
				    sqlite3_mprintf
				    ("%s%s\t<Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"1\">\r\n",
				     prev,
				     is_queryable_wmslite_layer
				     (list->TopLayerRef), opaque);
			}
		      else
			  xml =
			      sqlite3_mprintf
			      ("%s\t\t<Layer queryable=\"%d\">\r\n", prev,
			       is_queryable_wmslite_layer (list->TopLayerRef));
		  }
	    }
	  sqlite3_free (prev);
	  if (list->TopLayerName != NULL)
	    {
		cleared = clean_xml (list->TopLayerName);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s\t\t\t<Name>%s</Name>\r\n", prev,
				     cleared);
		free (cleared);
		sqlite3_free (prev);
	    }
	  if (list->TopLayerTitle != NULL)
	    {
		cleared = clean_xml (list->TopLayerTitle);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s\t\t\t<Title>%s</Title>\r\n", prev,
				     cleared);
		free (cleared);
		sqlite3_free (prev);
	    }
	  if (list->TopLayerAbstract != NULL)
	    {
		cleared = clean_xml (list->TopLayerAbstract);
		prev = xml;
		xml =
		    sqlite3_mprintf ("%s\t\t\t<Abstract>%s</Abstract>\r\n",
				     prev, cleared);
		free (cleared);
		sqlite3_free (prev);
	    }
	  if (list->TopLayerMinScale >= 0.0)
	    {
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s\t\t\t<MinScaleDenominator>%1.0f</MinScaleDenominator>\r\n",
		     prev, list->TopLayerMinScale);
		sqlite3_free (prev);
	    }
	  if (list->TopLayerMaxScale >= 0.0)
	    {
		prev = xml;
		xml =
		    sqlite3_mprintf
		    ("%s\t\t\t<MaxScaleDenominator>%1.0f</MaxScaleDenominator>\r\n",
		     prev, list->TopLayerMaxScale);
		sqlite3_free (prev);
	    }
	  tabpad = "\t\t";
      }
    else
	tabpad = "\t";
    lyr = list->MainFirst;
    while (lyr != NULL)
      {
	  /* printing all Layers from MAIN */
	  if (!is_valid_wmslite_layer (lyr) || lyr->ChildLayer)
	    {
		/* skipping any invalid or Child Layers */
		lyr = lyr->Next;
		continue;
	    }
	  prev = xml;
	  if (lyr->Type == WMS_LAYER_CASCADED_WMS && lyr->CascadedWMS != NULL)
	    {
		int opaque;
		if (lyr->CascadedWMS->IsTransparent)
		    opaque = 0;
		else
		    opaque = 1;
		if (lyr->CascadedWMS->Cascaded >= 0)
		    xml =
			sqlite3_mprintf
			("%s%s\t<Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"%d\">\r\n",
			 prev, tabpad, is_queryable_wmslite_layer (lyr), opaque,
			 lyr->CascadedWMS->Cascaded + 1);
		else
		    xml =
			sqlite3_mprintf
			("%s%s\t<Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"1\">\r\n",
			 prev, tabpad, is_queryable_wmslite_layer (lyr),
			 opaque);
	    }
	  else
	      xml =
		  sqlite3_mprintf ("%s%s\t<Layer queryable=\"%d\">\r\n", prev,
				   tabpad, is_queryable_wmslite_layer (lyr));
	  sqlite3_free (prev);
	  xml2 = NULL;
	  switch (lyr->Type)
	    {
	    case WMS_LAYER_CASCADED_WMS:
		xml2 =
		    print_cascaded_wms_layer (lyr->AliasName, lyr->CascadedWMS,
					      lyr->MinScaleDenominator,
					      lyr->MaxScaleDenominator, tabpad,
					      version);
		break;
	    case WMS_LAYER_RASTER:
		xml2 =
		    print_raster_layer (lyr->AliasName, list, lyr->Raster,
					lyr->MinScaleDenominator,
					lyr->MaxScaleDenominator, tabpad,
					version);
		break;
	    case WMS_LAYER_VECTOR:
		xml2 =
		    print_vector_layer (lyr->AliasName, list, lyr->Vector,
					lyr->MinScaleDenominator,
					lyr->MaxScaleDenominator, tabpad,
					version);
		break;
	    case WMS_LAYER_MAP_CONFIG:
		xml2 =
		    print_map_config_layer (lyr->AliasName, list,
					    lyr->MapConfig, lyr->MapConfigSrids,
					    lyr->First,
					    lyr->MinScaleDenominator,
					    lyr->MaxScaleDenominator, tabpad,
					    version);
		break;
	    };
	  if (xml2 != NULL)
	    {
		prev = xml;
		xml = sqlite3_mprintf ("%s%s", prev, xml2);
		sqlite3_free (prev);
		sqlite3_free (xml2);
	    }
	  prev = xml;
	  xml = sqlite3_mprintf ("%s%s\t</Layer>\r\n", prev, tabpad);
	  sqlite3_free (prev);
	  lyr = lyr->Next;
      }
    db = list->DbFirst;
    while (db != NULL)
      {
	  if (db->Valid == 0)
	    {
		/* skipping any invalid ATTACHED DB */
		db = db->Next;
		continue;
	    }
	  attLyr = db->First;
	  while (attLyr != NULL)
	    {
		/* printing all Attached Layers */

		if (!is_valid_wmslite_attached_layer (attLyr)
		    || attLyr->ChildLayer)
		  {
		      /* skipping any invalid or Child Layers */
		      attLyr = attLyr->Next;
		      continue;
		  }
		prev = xml;
		if (attLyr->Type == WMS_LAYER_CASCADED_WMS
		    && attLyr->CascadedWMS != NULL)
		  {
		      int opaque;
		      if (attLyr->CascadedWMS->IsTransparent)
			  opaque = 0;
		      else
			  opaque = 1;
		      if (attLyr->CascadedWMS->Cascaded >= 0)
			  xml =
			      sqlite3_mprintf
			      ("%s%s\t<Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"%d\">\r\n",
			       prev, tabpad,
			       is_queryable_wmslite_attached_layer (attLyr),
			       opaque, attLyr->CascadedWMS->Cascaded + 1);
		      else
			  xml =
			      sqlite3_mprintf
			      ("%s%s\t<Layer queryable=\"%d\" opaque=\"%d\" cascaded=\"1\">\r\n",
			       prev, tabpad,
			       is_queryable_wmslite_attached_layer (attLyr),
			       opaque);
		  }
		else
		    xml =
			sqlite3_mprintf ("%s%s\t<Layer queryable=\"%d\">\r\n",
					 prev, tabpad,
					 is_queryable_wmslite_attached_layer
					 (attLyr));
		sqlite3_free (prev);
		xml2 = NULL;
		switch (attLyr->Type)
		  {
		  case WMS_LAYER_CASCADED_WMS:
		      xml2 =
			  print_cascaded_wms_layer (attLyr->AliasName,
						    attLyr->CascadedWMS,
						    attLyr->MinScaleDenominator,
						    attLyr->MaxScaleDenominator,
						    tabpad, version);
		      break;
		  case WMS_LAYER_RASTER:
		      xml2 =
			  print_raster_layer (attLyr->AliasName, list,
					      attLyr->Raster,
					      attLyr->MinScaleDenominator,
					      attLyr->MaxScaleDenominator,
					      tabpad, version);
		      break;
		  case WMS_LAYER_VECTOR:
		      xml2 =
			  print_vector_layer (attLyr->AliasName, list,
					      attLyr->Vector,
					      attLyr->MinScaleDenominator,
					      attLyr->MaxScaleDenominator,
					      tabpad, version);
		      break;
		  };
		if (xml2 != NULL)
		  {
		      prev = xml;
		      xml = sqlite3_mprintf ("%s%s", prev, xml2);
		      sqlite3_free (prev);
		      sqlite3_free (xml2);
		  }
		prev = xml;
		xml = sqlite3_mprintf ("%s%s\t</Layer>\r\n", prev, tabpad);
		sqlite3_free (prev);
		attLyr = attLyr->Next;
	    }
	  db = db->Next;
      }
    if (list->TotalLayersCount > 1)
      {
	  prev = xml;
	  xml = sqlite3_mprintf ("%s\t\t</Layer>\r\n", prev);
	  sqlite3_free (prev);
      }
    prev = xml;
    xml = sqlite3_mprintf ("%s\t</Capability>\r\n", prev);
    sqlite3_free (prev);
    prev = xml;
    if (version == WMS_VERSION_130)
	xml = sqlite3_mprintf ("%s</WMS_Capabilities>\r\n", prev);
    else
	xml = sqlite3_mprintf ("%s</WMT_MS_Capabilities>\r\n", prev);
    sqlite3_free (prev);

    return xml;
}
