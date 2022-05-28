/* 
/ wmslite_sql
/
/ a light-weight WMS server / GCI supporting RasterLite2 DataSources
/ SQL and Database operations 
/
/ version 2.0, 2021 March 5
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

#ifdef _WIN23
#include <io.h>
#else
#include <unistd.h>
#endif

static int
parse_epsg_srs (const char *srs)
{
/* parsing an EPSG:SRID expression */
    int len;
    int i;
    int x = -1;

    if (srs == NULL)
	return -1;

    len = strlen (srs);
    for (i = len - 1; i >= 0; i--)
      {
	  if (srs[i] == ':')
	    {
		x = i + 1;
		break;
	    }
      }
    if (x < 0)
	return -1;
    return atoi (srs + x);
}

static int
compute_geographic_bbox (sqlite3 * sqlite, int srid, double *minx, double *miny,
			 double *maxx, double *maxy)
{
/* computing a Geographic BBOX */
    const char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;

    sql = "SELECT MbrMinX(g.geom), MbrMinY(g.geom), MbrMaxX(g.geom), "
	"MbrMaxY(g.geom) FROM "
	"(SELECT ST_Transform(BuildMbr(?, ?, ?, ?, ?), 4326) AS geom) AS g";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_double (stmt, 1, *minx);
    sqlite3_bind_double (stmt, 2, *miny);
    sqlite3_bind_double (stmt, 3, *maxx);
    sqlite3_bind_double (stmt, 4, *maxy);
    sqlite3_bind_int (stmt, 5, srid);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		*minx = sqlite3_column_double (stmt, 0);
		*miny = sqlite3_column_double (stmt, 1);
		*maxx = sqlite3_column_double (stmt, 2);
		*maxy = sqlite3_column_double (stmt, 3);
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return 1;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return 0;
}

static WmsLiteSridListPtr
create_wmslite_srid_list ()
{
/* creating a list of WmsLite Srids */
    WmsLiteSridListPtr srids = malloc (sizeof (WmsLiteSridList));
    srids->First = NULL;
    srids->Last = NULL;
    return srids;
}

static void
destroy_wmslite_srid_list (WmsLiteSridListPtr srids)
{
/* memory cleanup - freeing a list of WmsLite Srids */
    WmsLiteSridPtr pS;
    WmsLiteSridPtr pSn;
    pS = srids->First;
    while (pS != NULL)
      {
	  pSn = pS->Next;
	  free (pS);
	  pS = pSn;
      }
    free (srids);
}

static void
fetch_raster_alt_srids (sqlite3 * sqlite, const char *alias_name,
			const char *prefix, const char *name,
			WmsLiteRasterPtr raster)
{
/* fetching all alternative SRIDs related to some Raster Layer */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *xprefix;

    if (name == NULL || raster == NULL)
	return;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT srid, extent_minx, extent_miny, extent_maxx, extent_maxy "
	 "FROM \"%s\".raster_coverages WHERE coverage_name = %Q UNION "
	 "SELECT srid, extent_minx, extent_miny, extent_maxx, extent_maxy "
	 "FROM \"%s\".raster_coverages_srid WHERE coverage_name = %Q",
	 xprefix, name, xprefix, name);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		int null_minx = 0;
		int null_miny = 0;
		int null_maxx = 0;
		int null_maxy = 0;
		double minx;
		double miny;
		double maxx;
		double maxy;
		int srid = sqlite3_column_int (stmt, 0);
		if (sqlite3_column_type (stmt, 1) == SQLITE_NULL)
		    null_minx = 1;
		else
		    minx = sqlite3_column_double (stmt, 1);
		if (sqlite3_column_type (stmt, 2) == SQLITE_NULL)
		    null_miny = 1;
		else
		    miny = sqlite3_column_double (stmt, 2);
		if (sqlite3_column_type (stmt, 3) == SQLITE_NULL)
		    null_maxx = 1;
		else
		    maxx = sqlite3_column_double (stmt, 3);
		if (sqlite3_column_type (stmt, 4) == SQLITE_NULL)
		    null_maxy = 1;
		else
		    maxy = sqlite3_column_double (stmt, 4);
		if (null_minx || null_miny || null_maxx || null_maxy)
		  {
		      fprintf (stderr,
			       "WmsLite: RasterLayer %s SRID=%d: undefined Bounding Box\n",
			       alias_name, srid);
		      fflush (stderr);
		  }
		else
		    add_raster_crs (raster, srid, minx, miny, maxx, maxy);
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return;
}

static void
fetch_raster_keywords (sqlite3 * sqlite, const char *prefix, const char *name,
		       WmsLiteRasterPtr raster)
{
/* fetching all Keywords related to some Raster Layer */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *xprefix;

    if (name == NULL || raster == NULL)
	return;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT keyword FROM \"%s\".raster_coverages_keyword WHERE coverage_name = %Q",
	 xprefix, name);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE3_TEXT)
		  {
		      const char *keyword =
			  (const char *) sqlite3_column_text (stmt, 0);
		      add_raster_keyword (raster, keyword);
		  }
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return;
}

static void
fetch_raster_styles (sqlite3 * sqlite, const char *prefix, const char *name,
		     WmsLiteRasterPtr raster)
{
/* fetching all Styles available for some Raster Layer */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *xprefix;

    if (name == NULL || raster == NULL)
	return;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT name, title, abstract FROM \"%s\".SE_raster_styled_layers_view "
	 "WHERE coverage_name = %Q", xprefix, name);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const char *name = NULL;
		const char *title = NULL;
		const char *abstract = NULL;
		if (sqlite3_column_type (stmt, 0) != SQLITE_NULL)
		    name = (const char *) sqlite3_column_text (stmt, 0);
		if (sqlite3_column_type (stmt, 1) != SQLITE_NULL)
		    title = (const char *) sqlite3_column_text (stmt, 1);
		if (sqlite3_column_type (stmt, 2) != SQLITE_NULL)
		    abstract = (const char *) sqlite3_column_text (stmt, 2);
		if (name != NULL)
		    add_raster_style (raster, name, title, abstract);
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return;
}

static WmsLiteRasterPtr
verify_raster (sqlite3 * sqlite, const char *alias_name, const char *prefix,
	       const char *name, double *min_scale_denominator,
	       double *max_scale_denominator)
{
/* attempting to verify a Raster Layer */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    WmsLiteRasterPtr raster = NULL;
    char *xprefix;

    if (name == NULL)
	return NULL;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT title, abstract, srid, geo_minx, geo_miny, geo_maxx, geo_maxy, is_queryable, min_scale, max_scale "
	 "FROM \"%s\".raster_coverages WHERE coverage_name = %Q", xprefix,
	 name);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		double min_scale = -1.0;
		double max_scale = -1.0;
		double geo_minx;
		double geo_miny;
		double geo_maxx;
		double geo_maxy;
		int null_minx = 0;
		int null_miny = 0;
		int null_maxx = 0;
		int null_maxy = 0;
		const char *title =
		    (const char *) sqlite3_column_text (stmt, 0);
		const char *abstract =
		    (const char *) sqlite3_column_text (stmt, 1);
		int srid = sqlite3_column_int (stmt, 2);
		int is_queryable = sqlite3_column_int (stmt, 7);
		if (sqlite3_column_type (stmt, 3) == SQLITE_NULL)
		    null_minx = 1;
		else
		    geo_minx = sqlite3_column_double (stmt, 3);
		if (sqlite3_column_type (stmt, 4) == SQLITE_NULL)
		    null_miny = 1;
		else
		    geo_miny = sqlite3_column_double (stmt, 4);
		if (sqlite3_column_type (stmt, 5) == SQLITE_NULL)
		    null_maxx = 1;
		else
		    geo_maxx = sqlite3_column_double (stmt, 5);
		if (sqlite3_column_type (stmt, 6) == SQLITE_NULL)
		    null_maxy = 1;
		else
		    geo_maxy = sqlite3_column_double (stmt, 6);
		if (sqlite3_column_type (stmt, 8) == SQLITE_FLOAT)
		    min_scale = sqlite3_column_double (stmt, 8);
		if (sqlite3_column_type (stmt, 9) == SQLITE_FLOAT)
		    max_scale = sqlite3_column_double (stmt, 9);
		if (raster != NULL)
		    destroy_wmslite_raster (raster);
		raster =
		    create_wmslite_raster (title, abstract, srid, is_queryable);
		if (null_minx || null_miny || null_maxx || null_maxy)
		  {
		      fprintf (stderr,
			       "WmsLite: RasterLayer %s: undefined Geographic Bounding Box\n",
			       alias_name);
		      fflush (stderr);
		  }
		else
		    raster->GeographicBBox =
			create_wmslite_bbox (geo_minx, geo_miny, geo_maxx,
					     geo_maxy);
		fetch_raster_alt_srids (sqlite, alias_name, prefix, name,
					raster);
		fetch_raster_keywords (sqlite, prefix, name, raster);
		fetch_raster_styles (sqlite, prefix, name, raster);
		*min_scale_denominator = min_scale;
		*max_scale_denominator = max_scale;
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return raster;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return NULL;
}

static int
fetch_vector_table_srid (sqlite3 * sqlite, const char *prefix, const char *name,
			 const char *geometry, int *srid)
{
/* attempting to return the SRID for some Vector Coverage  of the Table type */
    int ret;
    char *sql;
    int ok = 0;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT srid FROM \"%s\".geometry_columns "
	 "WHERE f_table_name = %Q AND f_geometry_column = %Q", xprefix, name,
	 geometry);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		*srid = atoi (results[(i * columns) + 0]);
		ok = 1;
	    }
      }
    sqlite3_free_table (results);
    return ok;
}

static int
fetch_vector_view_srid (sqlite3 * sqlite, const char *prefix, const char *name,
			const char *geometry, int *srid)
{
/* attempting to return the SRID for some Vector Coverage  of the View type */
    int ret;
    char *sql;
    int ok = 0;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf ("SELECT g.srid FROM \"%s\".views_geometry_columns AS "
			 "v JOIN \"%s\".geometry_columns AS g ON "
			 "(Upper(v.f_table_name) = Upper(g.f_table_name) AND Upper(v.f_geometry_column) = Upper(g.f_geometry_column)) "
			 "WHERE v.view_name = %Q AND v.view_geometry = %Q",
			 xprefix, xprefix, name, geometry);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		*srid = atoi (results[(i * columns) + 0]);
		ok = 1;
	    }
      }
    sqlite3_free_table (results);
    return ok;
}

static int
fetch_vector_virt_srid (sqlite3 * sqlite, const char *prefix, const char *name,
			const char *geometry, int *srid)
{
/* attempting to return the SRID for some Vector Coverage  of the Virt type */
    int ret;
    char *sql;
    int ok = 0;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql = sqlite3_mprintf ("SELECT srid FROM \"%s\".virts_geometry_columns "
			   "WHERE virt_name = %Q AND virt_geometry = %Q",
			   xprefix, name, geometry);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		*srid = atoi (results[(i * columns) + 0]);
		ok = 1;
	    }
      }
    sqlite3_free_table (results);
    return ok;
}

static int
fetch_vector_topology_srid (sqlite3 * sqlite, const char *prefix,
			    const char *name, int *srid)
{
/* attempting to return the SRID for some Vector Coverage  of the Topology type */
    int ret;
    char *sql;
    int ok = 0;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT srid FROM \"%s\".topologies WHERE network_name = %Q", xprefix,
	 name);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		*srid = atoi (results[(i * columns) + 0]);
		ok = 1;
	    }
      }
    sqlite3_free_table (results);
    return ok;
}

static int
fetch_vector_network_srid (sqlite3 * sqlite, const char *prefix,
			   const char *name, int *srid)
{
/* attempting to return the SRID for some Vector Coverage  of the Network type */
    int ret;
    char *sql;
    int ok = 0;
    int i;
    char **results;
    int rows;
    int columns;
    char *xprefix;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT srid FROM \"%s\".networks WHERE network_name = %Q", xprefix,
	 name);
    free (xprefix);
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		*srid = atoi (results[(i * columns) + 0]);
		ok = 1;
	    }
      }
    sqlite3_free_table (results);
    return ok;
}

static int
fetch_vector_srid (sqlite3 * sqlite, const char *prefix, WmsLiteVectorPtr lyr,
		   int *srid)
{
/* attempting to return the SRID for some Vector Coverage */
    if (lyr->FTableName != NULL && lyr->FGeometryColumn != NULL)
	return fetch_vector_table_srid (sqlite, prefix, lyr->FTableName,
					lyr->FGeometryColumn, srid);
    if (lyr->ViewName != NULL && lyr->ViewGeometry != NULL)
	return fetch_vector_view_srid (sqlite, prefix, lyr->ViewName,
				       lyr->ViewGeometry, srid);
    if (lyr->VirtName != NULL && lyr->VirtGeometry != NULL)
	return fetch_vector_virt_srid (sqlite, prefix, lyr->ViewName,
				       lyr->ViewGeometry, srid);
    if (lyr->TopologyName != NULL)
	return fetch_vector_topology_srid (sqlite, prefix, lyr->TopologyName,
					   srid);
    if (lyr->NetworkName != NULL)
	return fetch_vector_network_srid (sqlite, prefix, lyr->NetworkName,
					  srid);
    return 0;
}

static void
fetch_vector_alt_srids (sqlite3 * sqlite, int srid, const char *alias_name,
			const char *prefix, const char *name,
			WmsLiteVectorPtr vector)
{
/* fetching all alternative SRIDs related to some Vector Layer */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *xprefix;

    if (name == NULL || vector == NULL)
	return;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT %d, extent_minx, extent_miny, extent_maxx, extent_maxy "
	 "FROM \"%s\".vector_coverages WHERE coverage_name = %Q UNION "
	 "SELECT srid, extent_minx, extent_miny, extent_maxx, extent_maxy "
	 "FROM \"%s\".vector_coverages_srid WHERE coverage_name = %Q",
	 srid, xprefix, name, xprefix, name);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		int null_minx = 0;
		int null_miny = 0;
		int null_maxx = 0;
		int null_maxy = 0;
		double minx;
		double miny;
		double maxx;
		double maxy;
		int srid = sqlite3_column_int (stmt, 0);
		if (sqlite3_column_type (stmt, 1) == SQLITE_NULL)
		    null_minx = 1;
		else
		    minx = sqlite3_column_double (stmt, 1);
		if (sqlite3_column_type (stmt, 2) == SQLITE_NULL)
		    null_miny = 1;
		else
		    miny = sqlite3_column_double (stmt, 2);
		if (sqlite3_column_type (stmt, 3) == SQLITE_NULL)
		    null_maxx = 1;
		else
		    maxx = sqlite3_column_double (stmt, 3);
		if (sqlite3_column_type (stmt, 4) == SQLITE_NULL)
		    null_maxy = 1;
		else
		    maxy = sqlite3_column_double (stmt, 4);
		if (null_minx || null_miny || null_maxx || null_maxy)
		  {
		      fprintf (stderr,
			       "WmsLite: VectorLayer %s SRID=%d: undefined Bounding Box\n",
			       alias_name, srid);
		      fflush (stderr);
		  }
		else
		    add_vector_crs (vector, srid, minx, miny, maxx, maxy);
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return;
}

static void
fetch_vector_keywords (sqlite3 * sqlite, const char *prefix, const char *name,
		       WmsLiteVectorPtr vector)
{
/* fetching all Keywords related to some Vector Layer */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *xprefix;

    if (name == NULL || vector == NULL)
	return;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT keyword FROM \"%s\".vector_coverages_keyword WHERE coverage_name = %Q",
	 xprefix, name);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE3_TEXT)
		  {
		      const char *keyword =
			  (const char *) sqlite3_column_text (stmt, 0);
		      add_vector_keyword (vector, keyword);
		  }
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return;
}

static void
fetch_vector_styles (sqlite3 * sqlite, const char *prefix, const char *name,
		     WmsLiteVectorPtr vector)
{
/* fetching all Styles available for some vector Layer */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *xprefix;

    if (name == NULL || vector == NULL)
	return;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT name, title, abstract FROM \"%s\".SE_vector_styled_layers_view "
	 "WHERE coverage_name = %Q", xprefix, name);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const char *name = NULL;
		const char *title = NULL;
		const char *abstract = NULL;
		if (sqlite3_column_type (stmt, 0) != SQLITE_NULL)
		    name = (const char *) sqlite3_column_text (stmt, 0);
		if (sqlite3_column_type (stmt, 1) != SQLITE_NULL)
		    title = (const char *) sqlite3_column_text (stmt, 1);
		if (sqlite3_column_type (stmt, 2) != SQLITE_NULL)
		    abstract = (const char *) sqlite3_column_text (stmt, 2);
		if (name != NULL)
		    add_vector_style (vector, name, title, abstract);
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return;
}

static WmsLiteVectorPtr
verify_vector (sqlite3 * sqlite, const char *alias_name, const char *prefix,
	       const char *name, double *min_scale_denominator,
	       double *max_scale_denominator)
{
/* attempting to verify a Vector Layer */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *xprefix;
    WmsLiteVectorPtr vector = NULL;

    if (name == NULL)
	return NULL;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT f_table_name, f_geometry_column, view_name, view_geometry, "
	 "virt_name, virt_geometry, topology_name, network_name, title, abstract, "
	 "geo_minx, geo_miny, geo_maxx, geo_maxy, is_queryable, min_scale, max_scale "
	 "FROM \"%s\".vector_coverages WHERE coverage_name = %Q", xprefix,
	 name);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		double min_scale = -1.0;
		double max_scale = -1.0;
		int srid;
		double geo_minx;
		double geo_miny;
		double geo_maxx;
		double geo_maxy;
		int null_minx = 0;
		int null_miny = 0;
		int null_maxx = 0;
		int null_maxy = 0;
		const char *f_table_name = NULL;
		const char *f_geometry_column = NULL;
		const char *view_name = NULL;
		const char *view_geometry = NULL;
		const char *virt_name = NULL;
		const char *virt_geometry = NULL;
		const char *topology_name = NULL;
		const char *network_name = NULL;
		const char *title =
		    (const char *) sqlite3_column_text (stmt, 8);
		const char *abstract =
		    (const char *) sqlite3_column_text (stmt, 9);
		int is_queryable = sqlite3_column_int (stmt, 14);
		if (sqlite3_column_type (stmt, 0) == SQLITE_TEXT)
		    f_table_name = (const char *) sqlite3_column_text (stmt, 0);
		if (sqlite3_column_type (stmt, 1) == SQLITE_TEXT)
		    f_geometry_column =
			(const char *) sqlite3_column_text (stmt, 1);
		if (sqlite3_column_type (stmt, 2) == SQLITE_TEXT)
		    view_name = (const char *) sqlite3_column_text (stmt, 2);
		if (sqlite3_column_type (stmt, 3) == SQLITE_TEXT)
		    view_geometry =
			(const char *) sqlite3_column_text (stmt, 3);
		if (sqlite3_column_type (stmt, 4) == SQLITE_TEXT)
		    virt_name = (const char *) sqlite3_column_text (stmt, 4);
		if (sqlite3_column_type (stmt, 5) == SQLITE_TEXT)
		    virt_geometry =
			(const char *) sqlite3_column_text (stmt, 5);
		if (sqlite3_column_type (stmt, 6) == SQLITE_TEXT)
		    topology_name =
			(const char *) sqlite3_column_text (stmt, 6);
		if (sqlite3_column_type (stmt, 7) == SQLITE_TEXT)
		    network_name = (const char *) sqlite3_column_text (stmt, 7);
		if (sqlite3_column_type (stmt, 10) == SQLITE_NULL)
		    null_minx = 1;
		else
		    geo_minx = sqlite3_column_double (stmt, 10);
		if (sqlite3_column_type (stmt, 11) == SQLITE_NULL)
		    null_miny = 1;
		else
		    geo_miny = sqlite3_column_double (stmt, 11);
		if (sqlite3_column_type (stmt, 12) == SQLITE_NULL)
		    null_maxx = 1;
		else
		    geo_maxx = sqlite3_column_double (stmt, 12);
		if (sqlite3_column_type (stmt, 13) == SQLITE_NULL)
		    null_maxy = 1;
		else
		    geo_maxy = sqlite3_column_double (stmt, 13);
		if (sqlite3_column_type (stmt, 15) == SQLITE_FLOAT)
		    min_scale = sqlite3_column_double (stmt, 15);
		if (sqlite3_column_type (stmt, 16) == SQLITE_FLOAT)
		    max_scale = sqlite3_column_double (stmt, 16);
		vector =
		    create_wmslite_vector (f_table_name, f_geometry_column,
					   view_name, view_geometry, virt_name,
					   virt_geometry, topology_name,
					   network_name, title, abstract,
					   is_queryable);
		if (fetch_vector_srid (sqlite, prefix, vector, &srid))
		    vector->Srid = srid;
		else
		  {
		      fprintf (stderr,
			       "WmsLite: VectorLayer %s: undefined SRID\n",
			       alias_name);
		      fflush (stderr);
		  }
		if (null_minx || null_miny || null_maxx || null_maxy)
		  {
		      fprintf (stderr,
			       "WmsLite: VectorLayer %s: undefined Geographic Bounding Box\n",
			       alias_name);
		      fflush (stderr);
		  }
		else
		    vector->GeographicBBox =
			create_wmslite_bbox (geo_minx, geo_miny, geo_maxx,
					     geo_maxy);
		fetch_vector_alt_srids (sqlite, srid, alias_name, prefix, name,
					vector);
		fetch_vector_keywords (sqlite, prefix, name, vector);
		fetch_vector_styles (sqlite, prefix, name, vector);
		*min_scale_denominator = min_scale;
		*max_scale_denominator = max_scale;
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return vector;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return NULL;
}

static void
fetch_cascaded_wms_alt_srids (sqlite3 * sqlite, const char *alias_name,
			      const char *prefix, int id,
			      WmsLiteCascadedWmsPtr wms)
{
/* fetching all alternative SRIDs related to some Cascaded WMS Layer */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *xprefix;

    if (wms == NULL)
	return;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT srs, minx, miny, maxx, maxy, is_default "
	 "FROM \"%s\".wms_ref_sys WHERE parent_id = %d", xprefix, id);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		int null_minx = 0;
		int null_miny = 0;
		int null_maxx = 0;
		int null_maxy = 0;
		double minx;
		double miny;
		double maxx;
		double maxy;
		const char *srs = (const char *) sqlite3_column_text (stmt, 0);
		int is_default = sqlite3_column_int (stmt, 5);
		if (sqlite3_column_type (stmt, 1) == SQLITE_NULL)
		    null_minx = 1;
		else
		    minx = sqlite3_column_double (stmt, 1);
		if (sqlite3_column_type (stmt, 2) == SQLITE_NULL)
		    null_miny = 1;
		else
		    miny = sqlite3_column_double (stmt, 2);
		if (sqlite3_column_type (stmt, 3) == SQLITE_NULL)
		    null_maxx = 1;
		else
		    maxx = sqlite3_column_double (stmt, 3);
		if (sqlite3_column_type (stmt, 4) == SQLITE_NULL)
		    null_maxy = 1;
		else
		    maxy = sqlite3_column_double (stmt, 4);
		if (null_minx || null_miny || null_maxx || null_maxy)
		  {
		      fprintf (stderr,
			       "WmsLite: CascadedWMS Layer %s SRS=%s: undefined Bounding Box\n",
			       alias_name, srs);
		      fflush (stderr);
		  }
		else
		    add_cascaded_wms_crs (wms, srs, minx, miny, maxx, maxy,
					  is_default);
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return;
}

static void
fetch_cascaded_wms_styles (sqlite3 * sqlite, const char *prefix, int id,
			   WmsLiteCascadedWmsPtr wms)
{
/* fetching all Styles available for some Cascaded WMS Layer */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    char *xprefix;

    if (wms == NULL)
	return;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT value, style_title, style_abstract FROM \"%s\".wms_settings "
	 "WHERE parent_id = %d AND key = 'style'", xprefix, id);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const char *name = NULL;
		const char *title = NULL;
		const char *abstract = NULL;
		if (sqlite3_column_type (stmt, 0) != SQLITE_NULL)
		    name = (const char *) sqlite3_column_text (stmt, 0);
		if (sqlite3_column_type (stmt, 1) != SQLITE_NULL)
		    title = (const char *) sqlite3_column_text (stmt, 1);
		if (sqlite3_column_type (stmt, 2) != SQLITE_NULL)
		    abstract = (const char *) sqlite3_column_text (stmt, 2);
		if (name != NULL && title != NULL)
		    add_cascaded_wms_style (wms, name, title, abstract);
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return;
}

extern void
do_set_cascaded_wms_srs (sqlite3 * sqlite, WmsLiteCascadedWmsPtr lyr)
{
/* attempting to retrieve the default SRS and building the Geographic BBOX */
    WmsLiteWmsCrsPtr pC;
    WmsLiteWmsCrsPtr pSRS = NULL;
    WmsLiteWmsCrsPtr pWGS84 = NULL;

    if (lyr->GeographicBBox != NULL)
	free (lyr->GeographicBBox);
    lyr->GeographicBBox = NULL;

    pC = lyr->CrsFirst;
    while (pC != NULL)
      {
	  if (pC->IsDefault)
	      pSRS = pC;
	  if (strcasecmp (pC->Srs, "EPSG:4326") == 0)
	      pWGS84 = pC;
	  pC = pC->Next;
      }
    if (pSRS == NULL)
	return;
    lyr->Srs = pSRS->Srs;
    if (pWGS84 != NULL)
      {
	  lyr->GeographicBBox =
	      create_wmslite_bbox (pWGS84->BBox->MinX, pWGS84->BBox->MinY,
				   pWGS84->BBox->MaxX, pWGS84->BBox->MaxY);
      }
    else
      {
	  /* calculating the Geographic BBox from the Main SRID */
	  int srid = parse_epsg_srs (pSRS->Srs);
	  double minx = pSRS->BBox->MinX;
	  double miny = pSRS->BBox->MinY;
	  double maxx = pSRS->BBox->MaxX;
	  double maxy = pSRS->BBox->MaxY;
	  if (compute_geographic_bbox
	      (sqlite, srid, &minx, &miny, &maxx, &maxy))
	      lyr->GeographicBBox =
		  create_wmslite_bbox (minx, miny, maxx, maxy);
      }
}

static WmsLiteCascadedWmsPtr
verify_cascaded_WMS (sqlite3 * sqlite, const char *alias_name,
		     const char *prefix, const char *name,
		     double *min_scale_denominator,
		     double *max_scale_denominator)
{
/* attempting to verify a Cascaded WMS Layer */
    char *sql;
    int ret;
    sqlite3_stmt *stmt = NULL;
    WmsLiteCascadedWmsPtr wms = NULL;
    char *xprefix;

    if (name == NULL)
	return NULL;

    if (prefix == NULL)
	xprefix = gaiaDoubleQuotedSql ("MAIN");
    else
	xprefix = gaiaDoubleQuotedSql (prefix);
    sql =
	sqlite3_mprintf
	("SELECT id, title, abstract, transparent, is_queryable, "
	 "cascaded, min_scale, max_scale "
	 "FROM \"%s\".wms_getmap " "WHERE layer_name = %Q", xprefix, name);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		int cascaded = -1;
		double min_scale = -1.0;
		double max_scale = -1.0;
		int id = sqlite3_column_int (stmt, 0);
		const char *title =
		    (const char *) sqlite3_column_text (stmt, 1);
		const char *abstract =
		    (const char *) sqlite3_column_text (stmt, 2);
		int is_transparent = sqlite3_column_int (stmt, 3);
		int is_queryable = sqlite3_column_int (stmt, 4);
		if (sqlite3_column_type (stmt, 5) == SQLITE_INTEGER)
		    cascaded = sqlite3_column_int (stmt, 5);
		if (sqlite3_column_type (stmt, 6) == SQLITE_FLOAT)
		    min_scale = sqlite3_column_double (stmt, 6);
		if (sqlite3_column_type (stmt, 7) == SQLITE_FLOAT)
		    max_scale = sqlite3_column_double (stmt, 7);
		if (wms != NULL)
		    destroy_wmslite_cascaded_wms (wms);
		wms =
		    create_wmslite_cascaded_wms (id, title, abstract,
						 is_transparent, is_queryable,
						 cascaded);
		fetch_cascaded_wms_alt_srids (sqlite, alias_name, prefix, id,
					      wms);
		do_set_cascaded_wms_srs (sqlite, wms);
		fetch_cascaded_wms_styles (sqlite, prefix, id, wms);
		*min_scale_denominator = min_scale;
		*max_scale_denominator = max_scale;
	    }
	  else
	      goto error;
      }
    sqlite3_finalize (stmt);
    return wms;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    return NULL;
}

extern void
attach_database (sqlite3 * sqlite, WmsLiteAttachedPtr db)
{
/* attempting to ATTACH a further database */
    int i;
    char *xprefix;
    char *xpath;
    char *sql;
    int ret;
    char **results;
    int rows;
    int columns;
    int exists = 0;
    int confirmed = 0;

    if (db->DbPrefix == NULL || db->Path == NULL)
	return;
#ifdef _WIN32
    if (_access (db->Path, 0) == 0)
	exists = 1;
#else
    if (access (db->Path, R_OK) == 0)
	exists = 1;
#endif
    if (!exists)
	return;

    xprefix = gaiaDoubleQuotedSql (db->DbPrefix);
    xpath = gaiaDoubleQuotedSql (db->Path);
    sql = sqlite3_mprintf ("ATTACH DATABASE \"%s\" AS \"%s\"", xpath, xprefix);
    free (xprefix);
    free (xpath);
    sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    sqlite3_free (sql);

/* testing if the database has really been attached */
    sql = "PRAGMA database_list";
    ret = sqlite3_get_table (sqlite, sql, &results, &rows, &columns, NULL);
    if (ret != SQLITE_OK)
	return;
    if (rows < 1)
	;
    else
      {
	  for (i = 1; i <= rows; i++)
	    {
		const char *prefix = results[(i * columns) + 1];
		const char *path = results[(i * columns) + 2];
		if (strcasecmp (prefix, db->DbPrefix) == 0
		    && strcasecmp (path, db->Path) == 0)
		    confirmed = 1;
	    }
      }
    sqlite3_free_table (results);
    if (confirmed)
	db->Valid = 1;
}

static void
add_layer_srid (WmsLiteSridListPtr srids, int srid)
{
/* adding some SRID to a WMS Layer */
    WmsLiteSridPtr crs = srids->First;
    while (crs != NULL)
      {
	  if (crs->Srid == srid)
	    {
		/* skipping: already inserted */
		return;
	    }
	  crs = crs->Next;
      }
    crs = malloc (sizeof (WmsLiteSrid));
    crs->Srid = srid;
    crs->Next = NULL;
    if (srids->First == NULL)
	srids->First = crs;
    if (srids->Last != NULL)
	srids->Last->Next = crs;
    srids->Last = crs;
}

static int
search_raster_layer_srids (WmsLiteSridListPtr srids, WmsLiteLayerPtr layer,
			   int srid)
{
/* searching for Raster Layer SRIDs */
    WmsLiteCrsPtr crs;
    WmsLiteCrsPtr mainCrs = NULL;
    if (layer->Raster == NULL)
	return 0;
    crs = layer->Raster->CrsFirst;
    while (crs != NULL)
      {
	  /* searching for the MapConfig SRID */
	  if (crs->Srid == srid)
	      mainCrs = crs;
	  crs = crs->Next;
      }
    if (mainCrs == NULL)
	return 0;

    crs = layer->Raster->CrsFirst;
    while (crs != NULL)
      {
	  /* updating the list of SRIDs */
	  add_layer_srid (srids, crs->Srid);
	  crs = crs->Next;
      }
    return 1;
}

static int
search_vector_layer_srids (WmsLiteSridListPtr srids, WmsLiteLayerPtr layer,
			   int srid)
{
/* searching for Vector Layer SRIDs */
    WmsLiteCrsPtr crs;
    WmsLiteCrsPtr mainCrs = NULL;
    if (layer->Vector == NULL)
	return 0;
    crs = layer->Vector->CrsFirst;
    while (crs != NULL)
      {
	  /* searching for the MapConfig SRID */
	  if (crs->Srid == srid)
	      mainCrs = crs;
	  crs = crs->Next;
      }
    if (mainCrs == NULL)
	return 0;

    crs = layer->Vector->CrsFirst;
    while (crs != NULL)
      {
	  /* updating the list of SRIDs */
	  add_layer_srid (srids, crs->Srid);
	  crs = crs->Next;
      }
    return 1;
}

static int
search_cascaded_wmw_layer_srids (WmsLiteSridListPtr srids,
				 WmsLiteLayerPtr layer, int srid)
{
/* searching for Cascaded WMS Layer SRIDs */
    WmsLiteWmsCrsPtr crs;
    WmsLiteWmsCrsPtr mainCrs = NULL;
    char *srs;
    if (layer->CascadedWMS == NULL)
	return 0;
    srs = sqlite3_mprintf ("EPSG:%d", srid);
    crs = layer->CascadedWMS->CrsFirst;
    while (crs != NULL)
      {
	  /* searching for the MapConfig SRID */
	  if (strcasecmp (srs, crs->Srs) == 0)
	      mainCrs = crs;
	  crs = crs->Next;
      }
    sqlite3_free (srs);
    if (mainCrs == NULL)
	return 0;

    crs = layer->CascadedWMS->CrsFirst;
    while (crs != NULL)
      {
	  /* updating the list of SRIDs */
	  int sridval = parse_epsg_srs (crs->Srs);
	  add_layer_srid (srids, sridval);
	  crs = crs->Next;
      }
    return 1;
}

static int
search_raster_attached_srids (WmsLiteSridListPtr srids,
			      WmsLiteAttachedLayerPtr layer, int srid)
{
/* searching for Raster AttachedLayer SRIDs */
    WmsLiteCrsPtr crs;
    WmsLiteCrsPtr mainCrs = NULL;
    if (layer->Raster == NULL)
	return 0;
    crs = layer->Raster->CrsFirst;
    while (crs != NULL)
      {
	  /* searching for the MapConfig SRID */
	  if (crs->Srid == srid)
	      mainCrs = crs;
	  crs = crs->Next;
      }
    if (mainCrs == NULL)
	return 0;

    crs = layer->Raster->CrsFirst;
    while (crs != NULL)
      {
	  /* updating the list of SRIDs */
	  add_layer_srid (srids, crs->Srid);
	  crs = crs->Next;
      }
    return 1;
}

static int
search_vector_attached_srids (WmsLiteSridListPtr srids,
			      WmsLiteAttachedLayerPtr layer, int srid)
{
/* searching for Vector AttachedLayer SRIDs */
    WmsLiteCrsPtr crs;
    WmsLiteCrsPtr mainCrs = NULL;
    if (layer->Vector == NULL)
	return 0;
    crs = layer->Vector->CrsFirst;
    while (crs != NULL)
      {
	  /* searching for the MapConfig SRID */
	  if (crs->Srid == srid)
	      mainCrs = crs;
	  crs = crs->Next;
      }
    if (mainCrs == NULL)
	return 0;

    crs = layer->Vector->CrsFirst;
    while (crs != NULL)
      {
	  /* updating the list of SRIDs */
	  add_layer_srid (srids, crs->Srid);
	  crs = crs->Next;
      }
    return 1;
}

static int
search_cascaded_wmw_attached_srids (WmsLiteSridListPtr srids,
				    WmsLiteAttachedLayerPtr layer, int srid)
{
/* searching for Cascaded WMS AttachedLayer SRIDs */
    WmsLiteWmsCrsPtr crs;
    WmsLiteWmsCrsPtr mainCrs = NULL;
    char *srs;
    if (layer->CascadedWMS == NULL)
	return 0;
    srs = sqlite3_mprintf ("EPSG:%d", srid);
    crs = layer->CascadedWMS->CrsFirst;
    while (crs != NULL)
      {
	  /* searching for the MapConfig SRID */
	  if (strcasecmp (srs, crs->Srs) == 0)
	      mainCrs = crs;
	  crs = crs->Next;
      }
    sqlite3_free (srs);
    if (mainCrs == NULL)
	return 0;

    crs = layer->CascadedWMS->CrsFirst;
    while (crs != NULL)
      {
	  /* updating the list of SRIDs */
	  int sridval = parse_epsg_srs (crs->Srs);
	  add_layer_srid (srids, sridval);
	  crs = crs->Next;
      }
    return 1;
}

static int
search_valid_layer (WmsLiteConfigPtr config, WmsLiteSridListPtr srids,
		    rl2MapLayerPtr lyr, int srid)
{
/* searching for a valid Layer */
    WmsLiteAttachedPtr db;
    int main = 0;
    const char *prefix = lyr->prefix;
    const char *name = lyr->name;

    if (prefix == NULL)
	main = 1;
    if (strcasecmp (prefix, "MAIN") == 0)
	main = 1;
    if (main)
      {
	  /* searching on MAIN DB */
	  WmsLiteLayerPtr layer = config->MainFirst;
	  while (layer != NULL)
	    {
		if (strcasecmp (name, layer->Name) == 0)
		  {
		      switch (layer->Type)
			{
			case WMS_LAYER_RASTER:
			    if (layer->Raster != NULL)
			      {
				  if (search_raster_layer_srids
				      (srids, layer, srid))
				      return 1;
			      }
			    break;
			case WMS_LAYER_VECTOR:
			    if (layer->Vector != NULL)
			      {
				  if (search_vector_layer_srids
				      (srids, layer, srid))
				      return 1;
			      }
			    break;
			case WMS_LAYER_CASCADED_WMS:
			    if (layer->CascadedWMS != NULL)
			      {
				  if (search_cascaded_wmw_layer_srids
				      (srids, layer, srid))
				      return 1;
			      }
			    break;
			};
		  }
		layer = layer->Next;
	    }
	  return 0;
      }

/* searching on some ATTACHED DB */
    db = config->DbFirst;
    while (db != NULL)
      {
	  if (db->Valid == 0)
	    {
		/* skipping invalids ATTACHED DB */
		db = db->Next;
		continue;
	    }
	  if (strcasecmp (prefix, db->DbPrefix) == 0)
	    {
		WmsLiteAttachedLayerPtr attLyr = db->First;
		while (attLyr != NULL)
		  {
		      if (strcasecmp (name, attLyr->Name) == 0)
			{
			    switch (attLyr->Type)
			      {
			      case WMS_LAYER_RASTER:
				  if (attLyr->Raster != NULL)
				    {
					if (search_raster_attached_srids
					    (srids, attLyr, srid))
					    return 1;
				    }
				  break;
			      case WMS_LAYER_VECTOR:
				  if (attLyr->Vector != NULL)
				    {
					if (search_vector_attached_srids
					    (srids, attLyr, srid))
					    return 1;
				    }
				  break;
			      case WMS_LAYER_CASCADED_WMS:
				  if (attLyr->CascadedWMS != NULL)
				    {
					if (search_cascaded_wmw_attached_srids
					    (srids, attLyr, srid))
					    return 1;
				    }
				  break;
			      };
			}
		      attLyr = attLyr->Next;
		  }
	    }
	  db = db->Next;
      }
    return 0;
}

static int
check_raster_layer_srid (WmsLiteLayerPtr layer, int srid)
{
/* checking if a Raster Layer supports a given srid */
    if (layer->Raster != NULL)
      {
	  WmsLiteCrsPtr crs = layer->Raster->CrsFirst;
	  while (crs != NULL)
	    {
		if (srid == crs->Srid)
		    return 1;
		crs = crs->Next;
	    }
      }
    return 0;
}

static int
check_vector_layer_srid (WmsLiteLayerPtr layer, int srid)
{
/* checking if a Vector Layer supports a given srid */
    if (layer->Vector != NULL)
      {
	  WmsLiteCrsPtr crs = layer->Vector->CrsFirst;
	  while (crs != NULL)
	    {
		if (srid == crs->Srid)
		    return 1;
		crs = crs->Next;
	    }
      }
    return 0;
}

static int
check_cascaded_wms_layer_srid (WmsLiteLayerPtr layer, int srid)
{
/* checking if a Cascaded WMS Layer supports a given srid */
    if (layer->CascadedWMS != NULL)
      {
	  WmsLiteWmsCrsPtr crs = layer->CascadedWMS->CrsFirst;
	  while (crs != NULL)
	    {
		int sridval = parse_epsg_srs (crs->Srs);
		if (srid == sridval)
		    return 1;
		crs = crs->Next;
	    }
      }
    return 0;
}

static int
check_raster_attached_srid (WmsLiteAttachedLayerPtr layer, int srid)
{
/* checking if a Raster AttachedLayer supports a given srid */
    if (layer->Raster != NULL)
      {
	  WmsLiteCrsPtr crs = layer->Raster->CrsFirst;
	  while (crs != NULL)
	    {
		if (srid == crs->Srid)
		    return 1;
		crs = crs->Next;
	    }
      }
    return 0;
}

static int
check_vector_attached_srid (WmsLiteAttachedLayerPtr layer, int srid)
{
/* checking if a Vector AttachedLayer supports a given srid */
    if (layer->Vector != NULL)
      {
	  WmsLiteCrsPtr crs = layer->Vector->CrsFirst;
	  while (crs != NULL)
	    {
		if (srid == crs->Srid)
		    return 1;
		crs = crs->Next;
	    }
      }
    return 0;
}

static int
check_cascaded_wms_attached_srid (WmsLiteAttachedLayerPtr layer, int srid)
{
/* checking if a Cascaded WMS AttachedLayer supports a given srid */
    if (layer->CascadedWMS != NULL)
      {
	  WmsLiteWmsCrsPtr crs = layer->CascadedWMS->CrsFirst;
	  while (crs != NULL)
	    {
		int sridval = parse_epsg_srs (crs->Srs);
		if (srid == sridval)
		    return 1;
		crs = crs->Next;
	    }
      }
    return 0;
}

static int
search_valid_layer_srid (WmsLiteConfigPtr config, rl2MapLayerPtr lyr, int srid)
{
/* searching for a valid Layer */
    WmsLiteAttachedPtr db;
    int main = 0;
    const char *prefix = lyr->prefix;
    const char *name = lyr->name;

    if (prefix == NULL)
	main = 1;
    if (strcasecmp (prefix, "MAIN") == 0)
	main = 1;
    if (main)
      {
	  /* searching on MAIN DB */
	  WmsLiteLayerPtr layer = config->MainFirst;
	  while (layer != NULL)
	    {
		if (strcasecmp (name, layer->Name) == 0)
		  {
		      switch (layer->Type)
			{
			case WMS_LAYER_RASTER:
			    if (layer->Raster != NULL)
			      {
				  if (check_raster_layer_srid (layer, srid))
				      return 1;
			      }
			    break;
			case WMS_LAYER_VECTOR:
			    if (layer->Vector != NULL)
			      {
				  if (check_vector_layer_srid (layer, srid))
				      return 1;
			      }
			    break;
			case WMS_LAYER_CASCADED_WMS:
			    if (layer->CascadedWMS != NULL)
			      {
				  if (check_cascaded_wms_layer_srid
				      (layer, srid))
				      return 1;
			      }
			    break;
			};
		  }
		layer = layer->Next;
	    }
	  return 0;
      }

/* searching on some ATTACHED DB */
    db = config->DbFirst;
    while (db != NULL)
      {
	  if (db->Valid == 0)
	    {
		/* skipping invalids ATTACHED DB */
		db = db->Next;
		continue;
	    }
	  if (strcasecmp (prefix, db->DbPrefix) == 0)
	    {
		WmsLiteAttachedLayerPtr attLyr = db->First;
		while (attLyr != NULL)
		  {
		      if (strcasecmp (name, attLyr->Name) == 0)
			{
			    switch (attLyr->Type)
			      {
			      case WMS_LAYER_RASTER:
				  if (attLyr->Raster != NULL)
				    {
					if (check_raster_attached_srid
					    (attLyr, srid))
					    return 1;
				    }
				  break;
			      case WMS_LAYER_VECTOR:
				  if (attLyr->Vector != NULL)
				    {
					if (check_vector_attached_srid
					    (attLyr, srid))
					    return 1;
				    }
				  break;
			      case WMS_LAYER_CASCADED_WMS:
				  if (attLyr->CascadedWMS != NULL)
				    {
					if (check_cascaded_wms_attached_srid
					    (attLyr, srid))
					    return 1;
				    }
				  break;
			      };
			}
		      attLyr = attLyr->Next;
		  }
	    }
	  db = db->Next;
      }
    return 0;
}

static int
get_raster_layer_srid (WmsLiteLayerPtr layer, int srid, double *minx,
		       double *miny, double *maxx, double *maxy)
{
/* attempting to get some BBOX from a Raster Layer */
    if (layer->Raster != NULL)
      {
	  WmsLiteCrsPtr crs = layer->Raster->CrsFirst;
	  while (crs != NULL)
	    {
		if (srid == crs->Srid)
		  {
		      *minx = crs->BBox->MinX;
		      *miny = crs->BBox->MinY;
		      *maxx = crs->BBox->MaxX;
		      *maxy = crs->BBox->MaxY;
		      return 1;
		  }
		crs = crs->Next;
	    }
      }
    return 0;
}

static int
get_vector_layer_srid (WmsLiteLayerPtr layer, int srid, double *minx,
		       double *miny, double *maxx, double *maxy)
{
/* attempting to get some BBOX from a Vector Layer */
    if (layer->Vector != NULL)
      {
	  WmsLiteCrsPtr crs = layer->Vector->CrsFirst;
	  while (crs != NULL)
	    {
		if (srid == crs->Srid)
		  {
		      *minx = crs->BBox->MinX;
		      *miny = crs->BBox->MinY;
		      *maxx = crs->BBox->MaxX;
		      *maxy = crs->BBox->MaxY;
		      return 1;
		  }
		crs = crs->Next;
	    }
      }
    return 0;
}

static int
get_cascaded_wms_layer_srid (WmsLiteLayerPtr layer, int srid, double *minx,
			     double *miny, double *maxx, double *maxy)
{
/* attempting to get some BBOX from a Cascaded WMS Layer */
    if (layer->CascadedWMS != NULL)
      {
	  WmsLiteWmsCrsPtr crs = layer->CascadedWMS->CrsFirst;
	  while (crs != NULL)
	    {
		int sridval = parse_epsg_srs (crs->Srs);
		if (srid == sridval)
		  {
		      *minx = crs->BBox->MinX;
		      *miny = crs->BBox->MinY;
		      *maxx = crs->BBox->MaxX;
		      *maxy = crs->BBox->MaxY;
		      return 1;
		  }
		crs = crs->Next;
	    }
      }
    return 0;
}

static int
get_raster_attached_srid (WmsLiteAttachedLayerPtr layer, int srid, double *minx,
			  double *miny, double *maxx, double *maxy)
{
/* attempting to get some BBOX from a Raster AttachedLayer */
    if (layer->Raster != NULL)
      {
	  WmsLiteCrsPtr crs = layer->Raster->CrsFirst;
	  while (crs != NULL)
	    {
		if (srid == crs->Srid)
		  {
		      *minx = crs->BBox->MinX;
		      *miny = crs->BBox->MinY;
		      *maxx = crs->BBox->MaxX;
		      *maxy = crs->BBox->MaxY;
		      return 1;
		  }
		crs = crs->Next;
	    }
      }
    return 0;
}

static int
get_vector_attached_srid (WmsLiteAttachedLayerPtr layer, int srid, double *minx,
			  double *miny, double *maxx, double *maxy)
{
/* attempting to get some BBOX from a Vector AttachedLayer */
    if (layer->Vector != NULL)
      {
	  WmsLiteCrsPtr crs = layer->Vector->CrsFirst;
	  while (crs != NULL)
	    {
		if (srid == crs->Srid)
		  {
		      *minx = crs->BBox->MinX;
		      *miny = crs->BBox->MinY;
		      *maxx = crs->BBox->MaxX;
		      *maxy = crs->BBox->MaxY;
		      return 1;
		  }
		crs = crs->Next;
	    }
      }
    return 0;
}

static int
get_cascaded_wms_attached_srid (WmsLiteAttachedLayerPtr layer, int srid,
				double *minx, double *miny, double *maxx,
				double *maxy)
{
/* attempting to get some BBOX from a Cascaded WMS AttachedLayer */
    if (layer->CascadedWMS != NULL)
      {
	  WmsLiteWmsCrsPtr crs = layer->CascadedWMS->CrsFirst;
	  while (crs != NULL)
	    {
		int sridval = parse_epsg_srs (crs->Srs);
		if (srid == sridval)
		  {
		      *minx = crs->BBox->MinX;
		      *miny = crs->BBox->MinY;
		      *maxx = crs->BBox->MaxX;
		      *maxy = crs->BBox->MaxY;
		      return 1;
		  }
		crs = crs->Next;
	    }
      }
    return 0;
}

static int
get_srid_bbox (WmsLiteConfigPtr config, rl2MapLayerPtr lyr, int srid,
	       double *minx, double *miny, double *maxx, double *maxy)
{
/* attempting to get some BBOX */
    WmsLiteAttachedPtr db;
    int main = 0;
    const char *prefix = lyr->prefix;
    const char *name = lyr->name;

    if (prefix == NULL)
	main = 1;
    if (strcasecmp (prefix, "MAIN") == 0)
	main = 1;
    if (main)
      {
	  /* searching on MAIN DB */
	  WmsLiteLayerPtr layer = config->MainFirst;
	  while (layer != NULL)
	    {
		if (strcasecmp (name, layer->Name) == 0)
		  {
		      switch (layer->Type)
			{
			case WMS_LAYER_RASTER:
			    if (layer->Raster != NULL)
			      {
				  if (get_raster_layer_srid
				      (layer, srid, minx, miny, maxx, maxy))
				      return 1;
			      }
			    break;
			case WMS_LAYER_VECTOR:
			    if (layer->Vector != NULL)
			      {
				  if (get_vector_layer_srid
				      (layer, srid, minx, miny, maxx, maxy))
				      return 1;
			      }
			    break;
			case WMS_LAYER_CASCADED_WMS:
			    if (layer->CascadedWMS != NULL)
			      {
				  if (get_cascaded_wms_layer_srid
				      (layer, srid, minx, miny, maxx, maxy))
				      return 1;
			      }
			    break;
			};
		  }
		layer = layer->Next;
	    }
	  return 0;
      }

/* searching on some ATTACHED DB */
    db = config->DbFirst;
    while (db != NULL)
      {
	  if (db->Valid == 0)
	    {
		/* skipping invalids ATTACHED DB */
		db = db->Next;
		continue;
	    }
	  if (strcasecmp (prefix, db->DbPrefix) == 0)
	    {
		WmsLiteAttachedLayerPtr attLyr = db->First;
		while (attLyr != NULL)
		  {
		      if (strcasecmp (name, attLyr->Name) == 0)
			{
			    switch (attLyr->Type)
			      {
			      case WMS_LAYER_RASTER:
				  if (attLyr->Raster != NULL)
				    {
					if (get_raster_attached_srid
					    (attLyr, srid, minx, miny, maxx,
					     maxy))
					    return 1;
				    }
				  break;
			      case WMS_LAYER_VECTOR:
				  if (attLyr->Vector != NULL)
				    {
					if (get_vector_attached_srid
					    (attLyr, srid, minx, miny, maxx,
					     maxy))
					    return 1;
				    }
				  break;
			      case WMS_LAYER_CASCADED_WMS:
				  if (attLyr->CascadedWMS != NULL)
				    {
					if (get_cascaded_wms_attached_srid
					    (attLyr, srid, minx, miny, maxx,
					     maxy))
					    return 1;
				    }
				  break;
			      };
			}
		      attLyr = attLyr->Next;
		  }
	    }
	  db = db->Next;
      }
    return 0;
}

static void
add_map_config_layer_srid (WmsLiteConfigPtr config,
			   WmsLiteLayerPtr maplyr, rl2MapLayerPtr lyr, int srid)
{
/* adding some alternative SRID to a MapConfig */
    WmsLiteMapConfigSRIDPtr crs = NULL;
    double minx;
    double miny;
    double maxx;
    double maxy;

    if (!get_srid_bbox (config, lyr, srid, &minx, &miny, &maxx, &maxy))
	return;
    if (maplyr->MapConfigSrids == NULL)
	return;

    crs = maplyr->MapConfigSrids->First;
    while (crs != NULL)
      {
	  if (srid == crs->Srid)
	    {
		/* updating an already inserted CRS */
		if (crs->BBox->MinX < minx)
		    crs->BBox->MinX = minx;
		if (crs->BBox->MinY < miny)
		    crs->BBox->MinY = miny;
		if (crs->BBox->MaxX > maxx)
		    crs->BBox->MaxX = maxx;
		if (crs->BBox->MaxY > maxy)
		    crs->BBox->MaxY = maxy;
		return;
	    }
	  crs = crs->Next;
      }

/* adding a new CRS */
    crs = malloc (sizeof (WmsLiteCrs));
    crs->Srid = srid;
    crs->BBox = malloc (sizeof (WmsLiteBBox));
    crs->BBox->MinX = minx;
    crs->BBox->MinY = miny;
    crs->BBox->MaxX = maxx;
    crs->BBox->MaxY = maxy;
    crs->Next = NULL;
    if (maplyr->MapConfigSrids->First == NULL)
	maplyr->MapConfigSrids->First = crs;
    if (maplyr->MapConfigSrids->Last != NULL)
	maplyr->MapConfigSrids->Last->Next = crs;
    maplyr->MapConfigSrids->Last = crs;
}

static void
add_map_config_main_srid (WmsLiteLayerPtr layer, int srid)
{
/* adding the MapConfig MultiSRID */
    if (layer->MapConfigSrids != NULL)
	destroy_map_config_multi_srid (layer->MapConfigSrids);
    layer->MapConfigSrids = create_map_config_multi_srid (srid);
}

static int
verify_map_config (rl2MapConfigPtr map, WmsLiteLayerPtr maplyr,
		   WmsLiteConfigPtr config)
{
/* checking a MapConfiguration for validity */
    int count = 0;
    rl2MapLayerPtr lyr;
    WmsLiteSridListPtr srids = create_wmslite_srid_list ();

    lyr = map->first_lyr;
    while (lyr != NULL)
      {
	  if (search_valid_layer (config, srids, lyr, map->srid))
	      count++;
	  lyr = lyr->next;
      }
    if (count > 0)
      {
	  /* completing the config of supported SRIDs */
	  WmsLiteSridPtr crs;
	  add_map_config_main_srid (maplyr, map->srid);
	  crs = srids->First;
	  while (crs != NULL)
	    {
		/* checking if a SRID is supported by all MapcConfig Layers */
		int cnt = 0;
		lyr = map->first_lyr;
		while (lyr != NULL)
		  {
		      if (search_valid_layer_srid (config, lyr, crs->Srid))
			  cnt++;
		      lyr = lyr->next;
		  }
		if (cnt == count)
		  {
		      /* adding the MapConfig SRID */
		      lyr = map->first_lyr;
		      while (lyr != NULL)
			{
			    add_map_config_layer_srid (config, maplyr, lyr,
						       crs->Srid);
			    lyr = lyr->next;
			}
		  }
		crs = crs->Next;
	    }
      }
    if (srids != NULL)
	destroy_wmslite_srid_list (srids);
    if (count > 0)
      {
	  /* determining the Geographic Extent */
	  WmsLiteMapConfigSRIDPtr mainCrs = NULL;
	  WmsLiteMapConfigSRIDPtr crs;

	  if (maplyr->MapConfigSrids->GeographicBBox != NULL)
	      free (maplyr->MapConfigSrids->GeographicBBox);

	  crs = maplyr->MapConfigSrids->First;
	  while (crs != NULL)
	    {
		if (crs->Srid == maplyr->MapConfigSrids->Srid)
		    mainCrs = crs;
		if (crs->Srid == 4326)
		  {
		      /* assuming the WGS84 BBox as the Geographic BBox */
		      maplyr->MapConfigSrids->GeographicBBox =
			  create_wmslite_bbox (crs->BBox->MinX, crs->BBox->MinY,
					       crs->BBox->MaxX,
					       crs->BBox->MaxY);
		  }
		crs = crs->Next;
	    }
	  if (maplyr->MapConfigSrids->GeographicBBox == NULL && mainCrs != NULL)
	    {
		/* calculating the Geographic BBox from the Main SRID */
		double minx = mainCrs->BBox->MinX;
		double miny = mainCrs->BBox->MinY;
		double maxx = mainCrs->BBox->MaxX;
		double maxy = mainCrs->BBox->MaxY;
		if (compute_geographic_bbox
		    (config->Connection.handle, maplyr->MapConfigSrids->Srid,
		     &minx, &miny, &maxx, &maxy))
		    maplyr->MapConfigSrids->GeographicBBox =
			create_wmslite_bbox (minx, miny, maxx, maxy);
	    }
      }
    return count;
}

extern int
wmslite_validate_config (WmsLiteConfigPtr config)
{
/* attempting to validate the WmsLite Configuration */
    WmsLiteAttachedPtr db;
    WmsLiteLayerPtr lyr;
    WmsLiteAttachedLayerPtr attLyr;
    double min = -1.0;
    double max = -1.0;

/* opening the MAIN DB */
    connection_init (&(config->Connection), config);
    if (config->Connection.handle == NULL)
	goto error;

    db = config->DbFirst;
    while (db != NULL)
      {
	  /* ATTACHing any required database */
	  attach_database (config->Connection.handle, db);
	  db = db->Next;
      }
    lyr = config->MainFirst;
    while (lyr != NULL)
      {
	  /* verifying all Layers from the MAIN DB */
	  switch (lyr->Type)
	    {
	    case WMS_LAYER_CASCADED_WMS:
		if (lyr->CascadedWMS != NULL)
		    destroy_wmslite_cascaded_wms (lyr->CascadedWMS);
		lyr->CascadedWMS =
		    verify_cascaded_WMS (config->Connection.handle,
					 lyr->AliasName, NULL, lyr->Name, &min,
					 &max);
		if (lyr->CascadedWMS != NULL)
		  {
		      lyr->MinScaleDenominator = min;
		      lyr->MaxScaleDenominator = max;
		      config->TopLayerRef = lyr;
		      if (lyr->ChildLayer == 0)
			  config->TotalLayersCount += 1;
		  }
		break;
	    case WMS_LAYER_RASTER:
		if (lyr->Raster != NULL)
		    destroy_wmslite_raster (lyr->Raster);
		lyr->Raster =
		    verify_raster (config->Connection.handle, lyr->AliasName,
				   NULL, lyr->Name, &min, &max);
		if (lyr->Raster != NULL)
		  {
		      lyr->MinScaleDenominator = min;
		      lyr->MaxScaleDenominator = max;
		      config->TopLayerRef = lyr;
		      if (lyr->ChildLayer == 0)
			  config->TotalLayersCount += 1;
		  }
		break;
	    case WMS_LAYER_VECTOR:
		if (lyr->Vector != NULL)
		    destroy_wmslite_vector (lyr->Vector);
		lyr->Vector =
		    verify_vector (config->Connection.handle, lyr->AliasName,
				   NULL, lyr->Name, &min, &max);
		if (lyr->Vector != NULL)
		  {
		      lyr->MinScaleDenominator = min;
		      lyr->MaxScaleDenominator = max;
		      config->TopLayerRef = lyr;
		      if (lyr->ChildLayer == 0)
			  config->TotalLayersCount += 1;
		  }
		break;
	    };
	  min = -1.0;
	  max = -1.0;
	  if (lyr->MinScaleDenominator >= 0.0)
	      min = lyr->MinScaleDenominator;
	  if (lyr->MaxScaleDenominator >= 0.0)
	      max = lyr->MaxScaleDenominator;
	  if (min >= 0 && max >= 0 && max < min)
	    {
		/* swapping min and max scale denominators */
		lyr->MinScaleDenominator = lyr->MaxScaleDenominator;
		lyr->MaxScaleDenominator = min;
	    }
	  lyr = lyr->Next;
      }
    db = config->DbFirst;
    while (db != NULL)
      {
	  /* verifiying all Layers from any ATTACHed DB */
	  attLyr = db->First;
	  while (attLyr != NULL)
	    {
		switch (attLyr->Type)
		  {
		  case WMS_LAYER_CASCADED_WMS:
		      if (attLyr->CascadedWMS != NULL)
			  destroy_wmslite_cascaded_wms (attLyr->CascadedWMS);
		      attLyr->CascadedWMS =
			  verify_cascaded_WMS (config->Connection.handle,
					       attLyr->AliasName, db->DbPrefix,
					       attLyr->Name, &min, &max);
		      if (attLyr->CascadedWMS != NULL)
			{
			    attLyr->MinScaleDenominator = min;
			    attLyr->MaxScaleDenominator = max;
			    config->TopAttachedLayerRef = attLyr;
			    if (attLyr->ChildLayer == 0)
				config->TotalLayersCount += 1;
			}
		      break;
		  case WMS_LAYER_RASTER:
		      if (attLyr->Raster != NULL)
			  destroy_wmslite_raster (attLyr->Raster);
		      attLyr->Raster =
			  verify_raster (config->Connection.handle,
					 attLyr->AliasName, db->DbPrefix,
					 attLyr->Name, &min, &max);
		      if (attLyr->Raster != NULL)
			{
			    attLyr->MinScaleDenominator = min;
			    attLyr->MaxScaleDenominator = max;
			    config->TopAttachedLayerRef = attLyr;
			    if (attLyr->ChildLayer == 0)
				config->TotalLayersCount += 1;
			}
		      break;
		  case WMS_LAYER_VECTOR:
		      if (attLyr->Vector != NULL)
			  destroy_wmslite_vector (attLyr->Vector);
		      attLyr->Vector =
			  verify_vector (config->Connection.handle,
					 attLyr->AliasName, db->DbPrefix,
					 attLyr->Name, &min, &max);
		      if (attLyr->Vector != NULL)
			{
			    attLyr->MinScaleDenominator = min;
			    attLyr->MaxScaleDenominator = max;
			    config->TopAttachedLayerRef = attLyr;
			    if (attLyr->ChildLayer == 0)
				config->TotalLayersCount += 1;
			}
		      break;
		  };
		attLyr = attLyr->Next;
	    }
	  db = db->Next;
      }
    lyr = config->MainFirst;
    while (lyr != NULL)
      {
	  /* verifying all Map Configurations */
	  if (lyr->Type == WMS_LAYER_MAP_CONFIG)
	    {
		unsigned char *xml =
		    rl2_xml_from_registered_map_config (config->
							Connection.handle,
							lyr->Name);
		if (xml != NULL)
		  {
		      lyr->MapConfig = rl2_parse_map_config_xml (xml);
		      free (xml);
		  }
		if (lyr->MapConfig != NULL)
		  {
		      if (verify_map_config (lyr->MapConfig, lyr, config) <= 0)
			  lyr->MapConfig->valid = 0;
		      else
			{
			    lyr->MinScaleDenominator =
				lyr->MapConfig->min_scale;
			    lyr->MaxScaleDenominator =
				lyr->MapConfig->max_scale;
			    config->TopLayerRef = lyr;
			    config->TotalLayersCount += 1;	/* MapConfig can't never be a child layer */
			}
		  }
	    }
	  lyr = lyr->Next;
      }
    min = -1.0;
    max = -1.0;
    if (config->TopLayerMinScale >= 0.0)
	min = config->TopLayerMinScale;
    if (config->TopLayerMaxScale >= 0.0)
	max = config->TopLayerMaxScale;
    if (min >= 0 && max >= 0 && max < min)
      {
	  /* swapping min and max scale denominators */
	  config->TopLayerMinScale = config->TopLayerMaxScale;
	  config->TopLayerMaxScale = min;
      }

    return 1;

  error:
    destroy_wmslite_config (config);
    return 0;
}
