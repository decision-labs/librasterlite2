/*

 rl2draping -- Drape Geometries

 version 1.1.0, 2018 September 5

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
 
Portions created by the Initial Developer are Copyright (C) 2018
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

#include "config.h"

#include <sqlite3.h>

#include "rasterlite2/rasterlite2.h"
#include "rasterlite2_private.h"

#include <spatialite/gg_const.h>

static int
check_raster (sqlite3 * sqlite, const char *db_prefix, const char *coverage,
	      int *srid, int *datagrid, int *strict, double *horz_res,
	      double *vert_res, int *has_no_data, double *no_data)
{
/* testing a Raster Coverage for validity */
    char *sql;
    char *xprefix;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int ok = 0;

    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = rl2_double_quoted_sql (db_prefix);
    sql =
	sqlite3_mprintf ("SELECT pixel_type, srid, strict_resolution, "
			 "horz_resolution, vert_resolution, sample_type, nodata_pixel "
			 "FROM \"%s\".raster_coverages "
			 "WHERE Lower(coverage_name) = Lower(?)", xprefix);
    free (xprefix);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage, strlen (coverage), SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const char *type = (const char *) sqlite3_column_text (stmt, 0);
		*srid = sqlite3_column_int (stmt, 1);
		*strict = sqlite3_column_int (stmt, 2);
		*horz_res = sqlite3_column_double (stmt, 3);
		*vert_res = sqlite3_column_double (stmt, 4);
		const char *sample_type =
		    (const char *) sqlite3_column_text (stmt, 5);
		if (sqlite3_column_type (stmt, 6) == SQLITE_BLOB)
		  {
		      const unsigned char *blob = sqlite3_column_blob (stmt, 6);
		      int blob_sz = sqlite3_column_bytes (stmt, 6);
		      rl2PixelPtr nodata_pixel =
			  rl2_deserialize_dbms_pixel (blob, blob_sz);
		      if (nodata_pixel != NULL)
			{
			    if (rl2_is_pixel_none (nodata_pixel) == RL2_FALSE)
			      {
				  unsigned char band;
				  rl2PrivSamplePtr sample;
				  rl2PrivPixelPtr noData =
				      (rl2PrivPixelPtr) nodata_pixel;
				  for (band = 0; band < noData->nBands; band++)
				    {
					sample = noData->Samples + band;
					if (strcasecmp (sample_type, "INT8") ==
					    0)
					  {
					      *no_data = sample->int8;
					      *has_no_data = 1;
					  }
					else if (strcasecmp
						 (sample_type, "UINT8") == 0)
					  {
					      *no_data = sample->uint8;
					      *has_no_data = 1;
					  }
					else if (strcasecmp
						 (sample_type, "INT16") == 0)
					  {
					      *no_data = sample->int16;
					      *has_no_data = 1;
					  }
					else if (strcasecmp
						 (sample_type, "UINT16") == 0)
					  {
					      *no_data = sample->uint16;
					      *has_no_data = 1;
					  }
					else if (strcasecmp
						 (sample_type, "INT32") == 0)
					  {
					      *no_data = sample->int32;
					      *has_no_data = 1;
					  }
					else if (strcasecmp
						 (sample_type, "UINT32") == 0)
					  {
					      *no_data = sample->uint32;
					      *has_no_data = 1;
					  }
					else if (strcasecmp
						 (sample_type, "FLOAT") == 0)
					  {
					      *no_data = sample->float32;
					      *has_no_data = 1;
					  }
					else if (strcasecmp
						 (sample_type, "DOUBLE") == 0)
					  {
					      *no_data = sample->float64;
					      *has_no_data = 1;
					  }
					else
					  {
					      *has_no_data = 0;
					      *no_data = 0.0;
					  }
				    }
			      }
			    else
			      {
				  *has_no_data = 0;
				  *no_data = 0.0;
			      }
			    rl2_destroy_pixel (nodata_pixel);
			}
		      else
			{
			    *has_no_data = 0;
			    *no_data = 0.0;
			}
		  }
		else
		  {
		      *has_no_data = 0;
		      *no_data = 0.0;
		  }
		if (strcasecmp (type, "DATAGRID") == 0)
		    *datagrid = 1;
		else
		    *datagrid = 0;
		ok = 1;
	    }
      }
    sqlite3_finalize (stmt);
    return ok;
}

static int
do_check_raster_coverage (sqlite3 * sqlite, const char *db_prefix,
			  const char *raster_coverage, int *srid, char **msg)
{
/* checking a Raster Coverage for validity */
    int rst_srid;
    double horz_res;
    double vert_res;
    int strict_resolution;
    int datagrid;
    int has_no_data;
    double no_data;
    const char *prefix = db_prefix;

    *msg = NULL;
    if (prefix == NULL)
	prefix = "main";
    if (!check_raster
	(sqlite, prefix, raster_coverage, &rst_srid, &datagrid,
	 &strict_resolution, &horz_res, &vert_res, &has_no_data, &no_data))
      {
	  *msg =
	      sqlite3_mprintf ("Raster Coverage %s.%s does not exists.", prefix,
			       raster_coverage);
	  goto error;
      }
    if (!datagrid)
      {
	  *msg =
	      sqlite3_mprintf ("Raster Coverage %s.%s is not a DATAGRID.",
			       prefix, raster_coverage);
	  goto error;
      }
    if (!strict_resolution)
      {
	  *msg =
	      sqlite3_mprintf ("Raster Coverage %s.%s has no StrictResolution.",
			       prefix, raster_coverage);
	  goto error;
      }
    *srid = rst_srid;
    return 1;

  error:
    return 0;
}

static int
check_coverage_list (sqlite3 * sqlite, const char *coverage_list_table,
		     int *srid, char **msg)
{
/* checking a Coverage List for validity */
    const char *sql;
    char *sql2;
    char *xname;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int ok_progr = 0;
    int ok_prefix = 0;
    int ok_coverage = 0;
    int ok_table = 0;
    int count = 0;
    int ref_srid;

    *msg = NULL;
/* testing for existence and conformity */
    sql = "PRAGMA table_info(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, coverage_list_table,
		       strlen (coverage_list_table), SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const char *name = (const char *) sqlite3_column_text (stmt, 1);
		if (strcasecmp (name, "progr") == 0)
		    ok_progr = 1;
		if (strcasecmp (name, "db_prefix") == 0)
		    ok_prefix = 1;
		if (strcasecmp (name, "coverage_name") == 0)
		    ok_coverage = 1;
		ok_table = 1;
	    }
      }
    sqlite3_finalize (stmt);
    if (!ok_table)
      {
	  *msg =
	      sqlite3_mprintf ("Coverage List Table %s does not exists",
			       coverage_list_table);
	  goto error;
      }
    if (ok_prefix)
      {
	  *msg =
	      sqlite3_mprintf
	      ("Table %s already has no Column named \"db_prefix\"",
	       coverage_list_table);
	  goto error;
      }
    if (ok_coverage)
      {
	  *msg =
	      sqlite3_mprintf
	      ("Table %s already has no Column named \"coverage_name\"",
	       coverage_list_table);
	  goto error;
      }
    if (!ok_progr)
      {
	  *msg =
	      sqlite3_mprintf ("Table %s has no Column named \"progr\"",
			       coverage_list_table);
	  goto error;
      }

/* testing for validity */
    xname = rl2_double_quoted_sql (coverage_list_table);
    sql2 =
	sqlite3_mprintf
	("SELECT db_prefix, coverage_name FROM main.\"%s\" ORDER BY progr",
	 xname);
    free (xname);
    ret = sqlite3_prepare_v2 (sqlite, sql2, strlen (sql2), &stmt, NULL);
    sqlite3_free (sql2);
    if (ret != SQLITE_OK)
	goto error;
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		int srid;
		const char *prefix =
		    (const char *) sqlite3_column_text (stmt, 0);
		const char *coverage =
		    (const char *) sqlite3_column_text (stmt, 1);
		if (!do_check_raster_coverage
		    (sqlite, prefix, coverage, &srid, msg))
		    goto error;
		if (count == 0)
		    ref_srid = srid;
		if (srid != ref_srid)
		  {
		      *msg =
			  sqlite3_mprintf
			  ("Coverage List contains mismatching SRIDs (%d/%d)",
			   ref_srid, srid);
		      goto error;
		  }
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count < 1)
      {
	  *msg =
	      sqlite3_mprintf ("Table %s contains an empty Coverage List",
			       coverage_list_table);
	  goto error;
      }
    *srid = ref_srid;
    return 1;

  error:
    return 0;
}

static int
check_spatial_table (sqlite3 * sqlite, const char *spatial_table,
		     const char *old_geometry_column,
		     const char *new_geometry_column,
		     int *geom_srid, int *gtype, char **msg)
{
/* testing a Spatial Table/Geometry for validity */
    const char *sql;
    char *sql2;
    char *xtable;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int count = 0;
    int srid;
    int geom_type = 0;
    int ok_old = 0;
    int ok_new = 0;
    int ok_table = 0;

    *msg = NULL;
/* testing for existence */
    xtable = rl2_double_quoted_sql (spatial_table);
    sql2 = sqlite3_mprintf ("PRAGMA table_info(\"%s\")", xtable);
    free (xtable);
    ret = sqlite3_prepare_v2 (sqlite, sql2, strlen (sql2), &stmt, NULL);
    sqlite3_free (sql2);
    if (ret != SQLITE_OK)
	goto error;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, spatial_table, strlen (spatial_table),
		       SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		const char *name = (const char *) sqlite3_column_text (stmt, 1);
		if (strcasecmp (name, old_geometry_column) == 0)
		    ok_old = 1;
		if (strcasecmp (name, new_geometry_column) == 0)
		    ok_new = 1;
		ok_table = 1;
	    }
      }
    sqlite3_finalize (stmt);
    if (!ok_table)
      {
	  *msg =
	      sqlite3_mprintf ("Spatial Table \"%s\" does not exists",
			       spatial_table);
	  goto error;
      }
    if (!ok_old)
      {
	  *msg =
	      sqlite3_mprintf ("Table %s has no Column named \"%s\"",
			       spatial_table, old_geometry_column);
	  goto error;
      }
    if (ok_new)
      {
	  *msg =
	      sqlite3_mprintf ("Table %s already has a Column named \"%s\"",
			       spatial_table, new_geometry_column);
	  goto error;
      }

/* testing Old Geometry */
    sql =
	"SELECT geometry_type, srid "
	"FROM MAIN.geometry_columns "
	"WHERE Lower(f_table_name) = Lower(?) "
	"AND Lower(f_geometry_column) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, spatial_table, strlen (spatial_table),
		       SQLITE_STATIC);
    sqlite3_bind_text (stmt, 2, old_geometry_column,
		       strlen (old_geometry_column), SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		geom_type = sqlite3_column_int (stmt, 0);
		srid = sqlite3_column_int (stmt, 1);
		count++;
	    }
      }
    sqlite3_finalize (stmt);
    if (count != 1)
      {
	  *msg =
	      sqlite3_mprintf ("%s.%s: not a valid Geometry column",
			       spatial_table, old_geometry_column);
	  goto error;
      }

    *geom_srid = srid;
    *gtype = geom_type;
    return 1;

  error:
    return 0;
}

static double
compute_distance (double x0, double y0, double x1, double y1)
{
/* computing a 2D distance */
    return sqrt (((x0 - x1) * (x0 - x1)) + ((y0 - y1) * (y0 - y1)));
}

static void
densify_segmentZM (rl2DynLinePtr dyn, double dist, double x, double y, double z,
		   double m, double densify_dist, double no_data_value,
		   double update_m)
{
/* interpolating XYZM Vertices */
    int i;
    double x_step;
    double y_step;
    double z_step;
    double m_step;
    int pts = dist / densify_dist;

    if (((double) pts * densify_dist) < dist)
	pts++;
    x_step = (x - dyn->last->x) / (double) pts;
    y_step = (y - dyn->last->y) / (double) pts;
    z_step = (z - dyn->last->z) / (double) pts;
    m_step = (m - dyn->last->m) / (double) pts;
    x = dyn->last->x;
    y = dyn->last->y;
    z = dyn->last->z;
    m = dyn->last->m;

    for (i = 1; i < pts; i++)
      {
	  x += x_step;
	  y += y_step;
	  z += z_step;
	  m += m_step;
	  if (update_m)
	      m = no_data_value;
	  else
	      z = no_data_value;
	  rl2AddDynPointZM (dyn, x, y, z, m);
      }
}

static void
densify_segmentZ (rl2DynLinePtr dyn, double dist, double x, double y, double z,
		  double densify_dist, double no_data_value, double update_m)
{
/* interpolating XYZ Vertices */
    int i;
    double x_step;
    double y_step;
    double z_step;
    int pts = dist / densify_dist;

    if (((double) pts * densify_dist) < dist)
	pts++;
    x_step = (x - dyn->last->x) / (double) pts;
    y_step = (y - dyn->last->y) / (double) pts;
    z_step = (z - dyn->last->z) / (double) pts;
    x = dyn->last->x;
    y = dyn->last->y;
    z = dyn->last->z;

    for (i = 1; i < pts; i++)
      {
	  x += x_step;
	  y += y_step;
	  z += z_step;
	  if (!update_m)
	      z = no_data_value;
	  rl2AddDynPointZ (dyn, x, y, z);
      }
}

static void
densify_segmentM (rl2DynLinePtr dyn, double dist, double x, double y, double m,
		  double densify_dist, double no_data_value, double update_m)
{
/* interpolating XYM Vertices */
    int i;
    double x_step;
    double y_step;
    double m_step;
    int pts = dist / densify_dist;

    if (((double) pts * densify_dist) < dist)
	pts++;
    x_step = (x - dyn->last->x) / (double) pts;
    y_step = (y - dyn->last->y) / (double) pts;
    m_step = (m - dyn->last->m) / (double) pts;
    x = dyn->last->x;
    y = dyn->last->y;
    m = dyn->last->m;

    for (i = 1; i < pts; i++)
      {
	  x += x_step;
	  y += y_step;
	  m += m_step;
	  if (update_m)
	      m = no_data_value;
	  rl2AddDynPointM (dyn, x, y, m);
      }
}

static void
copyDynLinestring (rl2DynLinePtr dyn, rl2LinestringPtr ln)
{
/* copying all Vertices from a DynLine to a Linestring */
    int iv = 0;
    rl2PointPtr pt = dyn->first;
    while (pt != NULL)
      {
	  if (ln->dims == GAIA_XY_Z_M)
	    {
		rl2SetPointZM (ln->coords, iv, pt->x, pt->y, pt->z, pt->m);
	    }
	  else if (ln->dims == GAIA_XY_Z)
	    {
		rl2SetPointZ (ln->coords, iv, pt->x, pt->y, pt->z);
	    }
	  else if (ln->dims == GAIA_XY_M)
	    {
		rl2SetPointM (ln->coords, iv, pt->x, pt->y, pt->m);
	    }
	  else
	    {
		rl2SetPoint (ln->coords, iv, pt->x, pt->y);
	    }
	  pt = pt->next;
	  iv++;
      }
}

static void
copyDynRing (rl2DynLinePtr dyn, rl2RingPtr rng)
{
/* copying all Vertices from a DynLine to a Polygon Ring */
    int iv = 0;
    rl2PointPtr pt = dyn->first;
    while (pt != NULL)
      {
	  if (rng->dims == GAIA_XY_Z_M)
	    {
		rl2SetPointZM (rng->coords, iv, pt->x, pt->y, pt->z, pt->m);
	    }
	  else if (rng->dims == GAIA_XY_Z)
	    {
		rl2SetPointZ (rng->coords, iv, pt->x, pt->y, pt->z);
	    }
	  else if (rng->dims == GAIA_XY_M)
	    {
		rl2SetPointM (rng->coords, iv, pt->x, pt->y, pt->m);
	    }
	  else
	    {
		rl2SetPoint (rng->coords, iv, pt->x, pt->y);
	    }
	  pt = pt->next;
	  iv++;
      }
}

static rl2GeometryPtr
densify_geometry (rl2GeometryPtr g1, double densify_dist,
		  double no_data_value, int update_m)
{
/* densifying Linestring/Polygon Ring Vertices */
    rl2PointPtr pt;
    rl2LinestringPtr ln;
    rl2PolygonPtr pg;
    rl2RingPtr rng;
    rl2DynLinePtr dyn;
    int iv;
    int ib;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    double dist;
    rl2GeometryPtr g2 = rl2CreateGeometry (g1->dims, g1->type);
    g2->srid = g1->srid;

    pt = g1->first_point;
    while (pt != NULL)
      {
	  /* copying POINTs as they are */
	  if (pt->dims == GAIA_XY_Z_M)
	      rl2AddPointXYZMToGeometry (g2, pt->x, pt->y, pt->z, pt->m);
	  else if (pt->dims == GAIA_XY_Z)
	      rl2AddPointXYZToGeometry (g2, pt->x, pt->y, pt->z);
	  else if (pt->dims == GAIA_XY_M)
	      rl2AddPointXYMToGeometry (g2, pt->x, pt->y, pt->m);
	  pt = pt->next;
      }

    ln = g1->first_linestring;
    while (ln != NULL)
      {
	  /* processing LINESTRINGs */
	  rl2LinestringPtr ln2;
	  dyn = rl2CreateDynLine ();
	  for (iv = 0; iv < ln->points; iv++)
	    {
		switch (ln->dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2GetPointZM (ln->coords, iv, &x, &y, &z, &m);
		      break;
		  case GAIA_XY_Z:
		      rl2GetPointZ (ln->coords, iv, &x, &y, &z);
		      break;
		  case GAIA_XY_M:
		      rl2GetPointM (ln->coords, iv, &x, &y, &m);
		      break;
		  };
		if (dyn->last != NULL)
		  {
		      switch (ln->dims)
			{
			case GAIA_XY_Z_M:
			    dist =
				compute_distance (x, y, dyn->last->x,
						  dyn->last->y);
			    if (dist > densify_dist)
				densify_segmentZM (dyn, dist, x, y, z, m,
						   densify_dist, no_data_value,
						   update_m);
			    break;
			case GAIA_XY_Z:
			    dist =
				compute_distance (x, y, dyn->last->x,
						  dyn->last->y);
			    if (dist > densify_dist)
				densify_segmentZ (dyn, dist, x, y, z,
						  densify_dist, no_data_value,
						  update_m);
			    break;
			case GAIA_XY_M:
			    dist =
				compute_distance (x, y, dyn->last->x,
						  dyn->last->y);
			    if (dist > densify_dist)
				densify_segmentM (dyn, dist, x, y, m,
						  densify_dist, no_data_value,
						  update_m);
			    break;
			};
		  }
		switch (ln->dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2AddDynPointZM (dyn, x, y, z, m);
		      break;
		  case GAIA_XY_Z:
		      rl2AddDynPointZ (dyn, x, y, z);
		      break;
		  case GAIA_XY_M:
		      rl2AddDynPointM (dyn, x, y, m);
		      break;
		  };
	    }
	  ln2 = rl2AddLinestringToGeometry (g2, rl2CountDynLinePoints (dyn));
	  copyDynLinestring (dyn, ln2);
	  rl2DestroyDynLine (dyn);
	  ln = ln->next;
      }

    pg = g1->first_polygon;
    while (pg != NULL)
      {
	  /* processing POLYGONs */
	  rl2PolygonPtr pg2;
	  rl2RingPtr rng2;
	  dyn = rl2CreateDynLine ();
	  rng = pg->exterior;
	  for (iv = 0; iv < rng->points; iv++)
	    {
		/* exterior Ring */
		switch (rng->dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2GetPointZM (rng->coords, iv, &x, &y, &z, &m);
		      break;
		  case GAIA_XY_Z:
		      rl2GetPointZ (rng->coords, iv, &x, &y, &z);
		      break;
		  case GAIA_XY_M:
		      rl2GetPointM (rng->coords, iv, &x, &y, &m);
		      break;
		  };
		if (dyn->last != NULL)
		  {
		      switch (rng->dims)
			{
			case GAIA_XY_Z_M:
			    dist =
				compute_distance (x, y, dyn->last->x,
						  dyn->last->y);
			    if (dist > densify_dist)
				densify_segmentZM (dyn, dist, x, y, z, m,
						   densify_dist, no_data_value,
						   update_m);
			    break;
			case GAIA_XY_Z:
			    dist =
				compute_distance (x, y, dyn->last->x,
						  dyn->last->y);
			    if (dist > densify_dist)
				densify_segmentZ (dyn, dist, x, y, z,
						  densify_dist, no_data_value,
						  update_m);
			    break;
			case GAIA_XY_M:
			    dist =
				compute_distance (x, y, dyn->last->x,
						  dyn->last->y);
			    if (dist > densify_dist)
				densify_segmentM (dyn, dist, x, y, m,
						  densify_dist, no_data_value,
						  update_m);
			    break;
			};
		  }
		switch (rng->dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2AddDynPointZM (dyn, x, y, z, m);
		      break;
		  case GAIA_XY_Z:
		      rl2AddDynPointZ (dyn, x, y, z);
		      break;
		  case GAIA_XY_M:
		      rl2AddDynPointM (dyn, x, y, m);
		      break;
		  };
	    }
	  pg2 =
	      rl2AddPolygonToGeometry (g2, rl2CountDynLinePoints (dyn),
				       pg->num_interiors);
	  rng2 = pg2->exterior;
	  copyDynRing (dyn, rng2);
	  rl2DestroyDynLine (dyn);
	  for (ib = 0; ib < pg->num_interiors; ib++)
	    {
		/* interior Rings */
		rng = pg->interiors + ib;
		dyn = rl2CreateDynLine ();
		for (iv = 0; iv < rng->points; iv++)
		  {
		      /* exterior Ring */
		      switch (rng->dims)
			{
			case GAIA_XY_Z_M:
			    rl2GetPointZM (rng->coords, iv, &x, &y, &z, &m);
			    break;
			case GAIA_XY_Z:
			    rl2GetPointZ (rng->coords, iv, &x, &y, &z);
			    break;
			case GAIA_XY_M:
			    rl2GetPointM (rng->coords, iv, &x, &y, &m);
			    break;
			};
		      if (dyn->last != NULL)
			{
			    switch (rng->dims)
			      {
			      case GAIA_XY_Z_M:
				  dist =
				      compute_distance (x, y, dyn->last->x,
							dyn->last->y);
				  if (dist > densify_dist)
				      densify_segmentZM (dyn, dist, x, y, z, m,
							 densify_dist,
							 no_data_value,
							 update_m);
				  break;
			      case GAIA_XY_Z:
				  dist =
				      compute_distance (x, y, dyn->last->x,
							dyn->last->y);
				  if (dist > densify_dist)
				      densify_segmentZ (dyn, dist, x, y, z,
							densify_dist,
							no_data_value,
							update_m);
				  break;
			      case GAIA_XY_M:
				  dist =
				      compute_distance (x, y, dyn->last->x,
							dyn->last->y);
				  if (dist > densify_dist)
				      densify_segmentM (dyn, dist, x, y, m,
							densify_dist,
							no_data_value,
							update_m);
				  break;
			      };
			}
		      switch (rng->dims)
			{
			case GAIA_XY_Z_M:
			    rl2AddDynPointZM (dyn, x, y, z, m);
			    break;
			case GAIA_XY_Z:
			    rl2AddDynPointZ (dyn, x, y, z);
			    break;
			case GAIA_XY_M:
			    rl2AddDynPointM (dyn, x, y, m);
			    break;
			};
		  }
		rng2 =
		    rl2AddInteriorRing (pg2, ib, rl2CountDynLinePoints (dyn));
		copyDynRing (dyn, rng2);
		rl2DestroyDynLine (dyn);
	    }
	  pg = pg->next;
      }

    return g2;
}

static int
copy_densify_geometry (sqlite3_stmt * stmt,
		       sqlite3_int64 rowid, rl2GeometryPtr geom,
		       double densify_dist, double no_data_value, int update_m,
		       int target_dims, int target_type)
{
/* copying and densifying points for a single Geometry */
    rl2GeometryPtr geom2;
    rl2PointPtr pt;
    rl2LinestringPtr ln;
    rl2PolygonPtr pg;
    rl2RingPtr rng;
    int iv;
    int ib;
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double m = 0.0;
    unsigned char *blob;
    int blob_size;
    int ret;
    int densify = 0;

/* creating the output Geometry */
    geom2 = rl2CreateGeometry (target_dims, target_type);
    if (geom2 == NULL)
	return 0;
    geom2->srid = geom->srid;

    pt = geom->first_point;
    while (pt != NULL)
      {
	  /* Point */
	  if (pt->dims == GAIA_XY_Z_M)
	    {
		x = pt->x;
		y = pt->y;
		z = pt->z;
		m = pt->m;
		if (update_m)
		    m = no_data_value;
		else
		    z = no_data_value;
	    }
	  else if (pt->dims == GAIA_XY_Z)
	    {
		x = pt->x;
		y = pt->y;
		z = pt->z;
		m = 0.0;
		if (update_m)
		    m = no_data_value;
		else
		    z = no_data_value;
	    }
	  else if (pt->dims == GAIA_XY_M)
	    {
		x = pt->x;
		y = pt->y;
		z = 0.0;
		m = pt->m;
		if (update_m)
		    m = no_data_value;
		else
		    z = no_data_value;
	    }
	  else
	    {
		x = pt->x;
		y = pt->y;
		z = 0.0;
		m = 0.0;
		if (update_m)
		    m = no_data_value;
		else
		    z = no_data_value;
	    }
	  switch (target_dims)
	    {
	    case GAIA_XY_Z_M:
		rl2AddPointXYZMToGeometry (geom2, x, y, z, m);
		break;
	    case GAIA_XY_Z:
		rl2AddPointXYZToGeometry (geom2, x, y, z);
		break;
	    case GAIA_XY_M:
		rl2AddPointXYMToGeometry (geom2, x, y, m);
		break;
	    };
	  pt = pt->next;
      }
    ln = geom->first_linestring;
    while (ln != NULL)
      {
	  /* Linestrings */
	  rl2LinestringPtr ln2 = rl2AddLinestringToGeometry (geom2, ln->points);
	  for (iv = 0; iv < ln->points; iv++)
	    {
		switch (ln->dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2GetPointZM (ln->coords, iv, &x, &y, &z, &m);
		      if (update_m)
			  m = no_data_value;
		      else
			  z = no_data_value;
		      break;
		  case GAIA_XY_Z:
		      rl2GetPointZ (ln->coords, iv, &x, &y, &z);
		      m = 0.0;
		      if (update_m)
			  m = no_data_value;
		      else
			  z = no_data_value;
		      break;
		  case GAIA_XY_M:
		      rl2GetPointM (ln->coords, iv, &x, &y, &m);
		      m = 0.0;
		      if (update_m)
			  m = no_data_value;
		      else
			  z = no_data_value;
		      break;
		  case GAIA_XY:
		  default:
		      rl2GetPoint (ln->coords, iv, &x, &y);
		      z = 0.0;
		      m = 0.0;
		      if (update_m)
			  m = no_data_value;
		      else
			  z = no_data_value;
		      break;
		  };
		switch (target_dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2SetPointZM (ln2->coords, iv, x, y, z, m);
		      break;
		  case GAIA_XY_Z:
		      rl2SetPointZ (ln2->coords, iv, x, y, z);
		      break;
		  case GAIA_XY_M:
		      rl2SetPointM (ln2->coords, iv, x, y, m);
		      break;
		  };
	    }
	  densify = 1;
	  ln = ln->next;
      }
    pg = geom->first_polygon;
    while (pg != NULL)
      {
	  /* Polygons */
	  rl2PolygonPtr pg2;
	  rl2RingPtr rng2;
	  rng = pg->exterior;
	  pg2 = rl2AddPolygonToGeometry (geom2, rng->points, pg->num_interiors);
	  rng2 = pg2->exterior;
	  for (iv = 0; iv < rng->points; iv++)
	    {
		switch (rng->dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2GetPointZM (rng->coords, iv, &x, &y, &z, &m);
		      if (update_m)
			  m = no_data_value;
		      else
			  z = no_data_value;
		      break;
		  case GAIA_XY_Z:
		      rl2GetPointZ (rng->coords, iv, &x, &y, &z);
		      m = 0.0;
		      if (update_m)
			  m = no_data_value;
		      else
			  z = no_data_value;
		      break;
		  case GAIA_XY_M:
		      rl2GetPointM (rng->coords, iv, &x, &y, &m);
		      m = 0.0;
		      if (update_m)
			  m = no_data_value;
		      else
			  z = no_data_value;
		      break;
		  case GAIA_XY:
		  default:
		      rl2GetPoint (rng->coords, iv, &x, &y);
		      z = 0.0;
		      m = 0.0;
		      if (update_m)
			  m = no_data_value;
		      else
			  z = no_data_value;
		      break;
		  };
		switch (target_dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2SetPointZM (rng2->coords, iv, x, y, z, m);
		      break;
		  case GAIA_XY_Z:
		      rl2SetPointZ (rng2->coords, iv, x, y, z);
		      break;
		  case GAIA_XY_M:
		      rl2SetPointM (rng2->coords, iv, x, y, m);
		      break;
		  };
	    }
	  for (ib = 0; ib < pg->num_interiors; ib++)
	    {
		rng = pg->interiors + ib;
		rng2 = rl2AddInteriorRing (pg2, ib, rng->points);
		for (iv = 0; iv < rng->points; iv++)
		  {
		      switch (rng->dims)
			{
			case GAIA_XY_Z_M:
			    rl2GetPointZM (rng->coords, iv, &x, &y, &z, &m);
			    if (update_m)
				m = no_data_value;
			    else
				z = no_data_value;
			    break;
			case GAIA_XY_Z:
			    rl2GetPointZ (rng->coords, iv, &x, &y, &z);
			    m = 0.0;
			    if (update_m)
				m = no_data_value;
			    else
				z = no_data_value;
			    break;
			case GAIA_XY_M:
			    rl2GetPointM (rng->coords, iv, &x, &y, &m);
			    m = 0.0;
			    if (update_m)
				m = no_data_value;
			    else
				z = no_data_value;
			    break;
			case GAIA_XY:
			default:
			    rl2GetPoint (rng->coords, iv, &x, &y);
			    z = 0.0;
			    m = 0.0;
			    if (update_m)
				m = no_data_value;
			    else
				z = no_data_value;
			    break;
			};
		      switch (target_dims)
			{
			case GAIA_XY_Z_M:
			    rl2SetPointZM (rng2->coords, iv, x, y, z, m);
			    break;
			case GAIA_XY_Z:
			    rl2SetPointZ (rng2->coords, iv, x, y, z);
			    break;
			case GAIA_XY_M:
			    rl2SetPointM (rng2->coords, iv, x, y, m);
			    break;
			};
		  }
	    }
	  densify = 1;
	  pg = pg->next;
      }

    if (densify && densify_dist > 0.0)
      {
	  /* densifying Linestrings and Polygon Rings */
	  rl2GeometryPtr geom3 =
	      densify_geometry (geom2, densify_dist, no_data_value, update_m);
	  if (geom3 != NULL)
	    {
		rl2_destroy_geometry (geom2);
		geom2 = geom3;
	    }
      }

/* updating the output Geometry */
    if (!rl2_geometry_to_blob (geom2, &blob, &blob_size))
      {
	  rl2_destroy_geometry (geom2);
	  return 0;
      }
    rl2_destroy_geometry (geom2);
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_size, free);
    sqlite3_bind_int64 (stmt, 2, rowid);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	return 0;
    return 1;
}

static int
copy_densify (sqlite3 * sqlite, const char *table, const char *old_geom,
	      const char *new_geom, double densify_dist, double no_data_value,
	      int update_m, int target_dims, int target_type)
{
/* densifying the Points */
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_upd = NULL;
    int ret;
    char *xtable;
    char *xgeom;
    char *sql;

/* preparing the update statement */
    xtable = rl2_double_quoted_sql (table);
    xgeom = rl2_double_quoted_sql (new_geom);
    sql =
	sqlite3_mprintf ("UPDATE main.\"%s\" SET \"%s\" = ? WHERE rowid = ?",
			 xtable, xgeom);
    free (xtable);
    free (xgeom);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_upd, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

/* preparing the intput statement */
    xtable = rl2_double_quoted_sql (table);
    xgeom = rl2_double_quoted_sql (old_geom);
    sql =
	sqlite3_mprintf ("SELECT rowid, \"%s\" FROM main.\"%s\"", xgeom,
			 xtable);
    free (xtable);
    free (xgeom);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;
/* processing all input Geometries */
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *blob = NULL;
		int sz;
		rl2GeometryPtr g;
		sqlite3_int64 rowid = sqlite3_column_int64 (stmt_in, 0);
		if (sqlite3_column_type (stmt_in, 1) == SQLITE_BLOB)
		  {
		      blob =
			  (const unsigned char *)
			  sqlite3_column_blob (stmt_in, 1);
		      sz = sqlite3_column_bytes (stmt_in, 1);
		      g = rl2_geometry_from_blob (blob, sz);
		      if (g != NULL)
			{
			    if (!copy_densify_geometry
				(stmt_upd, rowid, g, densify_dist,
				 no_data_value, update_m, target_dims,
				 target_type))
			      {
				  rl2_destroy_geometry (g);
				  goto error;
			      }
			    rl2_destroy_geometry (g);
			}
		  }
	    }
      }
    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_upd);
    return 1;

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_upd != NULL)
	sqlite3_finalize (stmt_upd);
    return 0;
}

static int
create_output_geom (sqlite3 * sqlite, const char *table, const char *geom,
		    int srid, int gtype, int update_m, int *target_dims)
{
/* attempting to create the output Geometry */
    char *sql;
    int ret;
    const char *geom_type;
    const char *dims;

    switch (gtype)
      {
      case GAIA_POINT:
      case GAIA_POINTZ:
      case GAIA_POINTM:
      case GAIA_POINTZM:
	  geom_type = "POINT";
	  break;
      case GAIA_LINESTRING:
      case GAIA_LINESTRINGZ:
      case GAIA_LINESTRINGM:
      case GAIA_LINESTRINGZM:
	  geom_type = "LINESTRING";
	  break;
      case GAIA_POLYGON:
      case GAIA_POLYGONZ:
      case GAIA_POLYGONM:
      case GAIA_POLYGONZM:
	  geom_type = "POLYGON";
	  break;
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
	  geom_type = "MULTIPOINT";
	  break;
      case GAIA_MULTILINESTRING:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTILINESTRINGZM:
	  geom_type = "MULTILINESTRING";
	  break;
      case GAIA_MULTIPOLYGON:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_MULTIPOLYGONM:
      case GAIA_MULTIPOLYGONZM:
	  geom_type = "MULTIPOLYGON";
	  break;
      case GAIA_GEOMETRYCOLLECTION:
      case GAIA_GEOMETRYCOLLECTIONZ:
      case GAIA_GEOMETRYCOLLECTIONM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  geom_type = "GEOMETRYCOLLECTION";
	  break;
      default:
	  geom_type = "GEOMETRY";
	  break;
      }
    switch (gtype)
      {
      case GAIA_POINTZ:
      case GAIA_LINESTRINGZ:
      case GAIA_POLYGONZ:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTILINESTRINGZ:
      case GAIA_MULTIPOLYGONZ:
      case GAIA_GEOMETRYCOLLECTIONZ:
	  if (update_m)
	    {
		dims = "XYZM";
		*target_dims = GAIA_XY_Z_M;
	    }
	  else
	    {
		dims = "XYZ";
		*target_dims = GAIA_XY_Z;
	    }
	  break;
      case GAIA_POINTM:
      case GAIA_LINESTRINGM:
      case GAIA_POLYGONM:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTILINESTRINGM:
      case GAIA_MULTIPOLYGONM:
      case GAIA_GEOMETRYCOLLECTIONM:
	  if (update_m)
	    {
		dims = "XYM";
		*target_dims = GAIA_XY_M;
	    }
	  else
	    {
		dims = "XYZM";
		*target_dims = GAIA_XY_Z_M;
	    }
	  break;
      case GAIA_POINTZM:
      case GAIA_LINESTRINGZM:
      case GAIA_POLYGONZM:
      case GAIA_MULTIPOINTZM:
      case GAIA_MULTILINESTRINGZM:
      case GAIA_MULTIPOLYGONZM:
      case GAIA_GEOMETRYCOLLECTIONZM:
	  dims = "XYZM";
	  *target_dims = GAIA_XY_Z_M;
	  break;
      case GAIA_POINT:
      case GAIA_LINESTRING:
      case GAIA_POLYGON:
      case GAIA_MULTIPOINT:
      case GAIA_MULTILINESTRING:
      case GAIA_MULTIPOLYGON:
      case GAIA_GEOMETRYCOLLECTION:
      default:
	  if (update_m)
	    {
		dims = "XYM";
		*target_dims = GAIA_XY_M;
	    }
	  else
	    {
		dims = "XYZ";
		*target_dims = GAIA_XY_Z;
	    }
	  break;
      }

    sql =
	sqlite3_mprintf ("SELECT AddGeometryColumn(%Q, %Q, %d, %Q, %Q)", table,
			 geom, srid, geom_type, dims);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    return 1;
}

static int
create_output_geom_rtree (sqlite3 * sqlite, const char *table, const char *geom)
{
/* attempting to create a Spatial Index supporting the output Geometry */
    char *sql;
    int ret;

    sql = sqlite3_mprintf ("SELECT CreateSpatialIndex(%Q, %Q)", table, geom);
    ret = sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	return 0;
    return 1;
}

static int
check_table (sqlite3 * sqlite, const char *spatial_table,
	     const char *geometry_column, int *geom_has_z, int *geom_has_m)
{
/* testing a Spatial Table/Geometry for validity */
    const char *sql;
    sqlite3_stmt *stmt = NULL;
    int ret;
    int count = 0;
    int geom_type;
    int has_z = 0;
    int has_m = 0;

    sql =
	"SELECT geometry_type "
	"FROM MAIN.geometry_columns "
	"WHERE Lower(f_table_name) = Lower(?) "
	"AND Lower(f_geometry_column) = Lower(?)";
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
	goto error;

    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_text (stmt, 1, spatial_table, strlen (spatial_table),
		       SQLITE_STATIC);
    if (geometry_column != NULL)
	sqlite3_bind_text (stmt, 2, geometry_column, strlen (geometry_column),
			   SQLITE_STATIC);
    while (1)
      {
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;
	  if (ret == SQLITE_ROW)
	    {
		geom_type = sqlite3_column_int (stmt, 0);
		count++;
	    }
      }
    sqlite3_finalize (stmt);

    if (count != 1)
	goto error;
    switch (geom_type)
      {
      case 1001:
      case 1002:
      case 1003:
      case 1004:
      case 1005:
      case 1006:
      case 1007:
	  /* ok, the Z dimension is supported */
	  has_z = 1;
	  break;
      case 2001:
      case 2002:
      case 2003:
      case 2004:
      case 2005:
      case 2006:
      case 2007:
	  /* ok, the M dimension is supported */
	  has_m = 1;
	  break;
      case 3001:
      case 3002:
      case 3003:
      case 3004:
      case 3005:
      case 3006:
      case 3007:
	  /* ok, both the Z and M dimensions are supported */
	  has_z = 1;
	  has_m = 1;
	  break;
      };
    *geom_has_z = has_z;
    *geom_has_m = has_m;
    return 1;

  error:
    return 0;
}

static int
check_matching_bboxes (double tile_minx, double tile_maxx, double tile_miny,
		       double tile_maxy, double geom_minx, double geom_maxx,
		       double geom_miny, double geom_maxy)
{
/* checking if the BBOXes of the Raster Tile and of the Geometry do really match */
    if (geom_maxx < tile_minx)
	return 0;
    if (geom_minx > tile_maxx)
	return 0;
    if (geom_maxy < tile_miny)
	return 0;
    if (geom_miny > tile_maxy)
	return 0;
    return 1;
}

static int
check_matching_point (double tile_minx, double tile_maxx, double tile_miny,
		      double tile_maxy, double x, double y)
{
/* checking if a 2D point matches the BBOXes of the Raster Tile */
    if (x < tile_minx)
	return 0;
    if (x > tile_maxx)
	return 0;
    if (y < tile_miny)
	return 0;
    if (y > tile_maxy)
	return 0;
    return 1;
}

static int
do_get_mask_pixel (rl2PrivRasterPtr raster, unsigned int row, unsigned int col)
{
/* fetching a MASK pixel from a raster tile */
    unsigned char *ptr = (unsigned char *) (raster->maskBuffer);
    if (raster->maskBuffer == NULL)
	return 1;

    ptr += (row * raster->width);
    ptr += col;
    if (*ptr == 0)
	return 1;
    else
	return 0;
}

static double
do_get_int8_pixel (rl2PrivRasterPtr raster, unsigned int row, unsigned int col,
		   int *opaque)
{
/* fetching a pixel from an INT8 raster tile */
    double value;
    char *ptr = (char *) (raster->rasterBuffer);

    ptr += (row * raster->width);
    ptr += col;
    value = *ptr;
    *opaque = do_get_mask_pixel (raster, row, col);
    return value;
}

static double
do_get_uint8_pixel (rl2PrivRasterPtr raster, unsigned int row, unsigned int col,
		    int *opaque)
{
/* fetching a pixel from an UINT8 raster tile */
    double value;
    unsigned char *ptr = (unsigned char *) (raster->rasterBuffer);

    ptr += (row * raster->width);
    ptr += col;
    value = *ptr;
    *opaque = do_get_mask_pixel (raster, row, col);
    return value;
}

static double
do_get_int16_pixel (rl2PrivRasterPtr raster, unsigned int row, unsigned int col,
		    int *opaque)
{
/* fetching a pixel from an INT16 raster tile */
    double value;
    short *ptr = (short *) (raster->rasterBuffer);

    ptr += (row * raster->width);
    ptr += col;
    value = *ptr;
    *opaque = do_get_mask_pixel (raster, row, col);
    return value;
}

static double
do_get_uint16_pixel (rl2PrivRasterPtr raster, unsigned int row,
		     unsigned int col, int *opaque)
{
/* fetching a pixel from an UINT16 raster tile */
    double value;
    unsigned short *ptr = (unsigned short *) (raster->rasterBuffer);

    ptr += (row * raster->width);
    ptr += col;
    value = *ptr;
    *opaque = do_get_mask_pixel (raster, row, col);
    return value;
}

static double
do_get_int32_pixel (rl2PrivRasterPtr raster, unsigned int row, unsigned int col,
		    int *opaque)
{
/* fetching a pixel from an INT32 raster tile */
    double value;
    int *ptr = (int *) (raster->rasterBuffer);

    ptr += (row * raster->width);
    ptr += col;
    value = *ptr;
    *opaque = do_get_mask_pixel (raster, row, col);
    return value;
}

static double
do_get_uint32_pixel (rl2PrivRasterPtr raster, unsigned int row,
		     unsigned int col, int *opaque)
{
/* fetching a pixel from an UINT32 raster tile */
    double value;
    unsigned int *ptr = (unsigned int *) (raster->rasterBuffer);

    ptr += (row * raster->width);
    ptr += col;
    value = *ptr;
    *opaque = do_get_mask_pixel (raster, row, col);
    return value;
}

static double
do_get_float_pixel (rl2PrivRasterPtr raster, unsigned int row, unsigned int col,
		    int *opaque)
{
/* fetching a pixel from a FLOAT raster tile */
    double value;
    float *ptr = (float *) (raster->rasterBuffer);

    ptr += (row * raster->width);
    ptr += col;
    value = *ptr;
    *opaque = do_get_mask_pixel (raster, row, col);
    return value;
}

static double
do_get_double_pixel (rl2PrivRasterPtr raster, unsigned int row,
		     unsigned int col, int *opaque)
{
/* fetching a pixel from a DOUBLE raster tile */
    double value;
    double *ptr = (double *) (raster->rasterBuffer);

    ptr += (row * raster->width);
    ptr += col;
    value = *ptr;
    *opaque = do_get_mask_pixel (raster, row, col);
    return value;
}

static double
do_get_pixel_value (rl2PrivRasterPtr raster, double x, double y,
		    rl2AuxDrapeGeometriesPtr aux, int *transparent)
{
/* fetching a pixel value from the Raster Tile */
    int has_no_data = aux->raster_has_no_data;
    double no_data = aux->raster_no_data;
    unsigned int row;
    unsigned int col;
    double value;

    row = (aux->tile_maxy - y) / aux->vert_res;
    col = (x - aux->tile_minx) / aux->horz_res;
    if (col < raster->width && row < raster->height)
      {
	  int opaque;
	  switch (raster->sampleType)
	    {
	    case RL2_SAMPLE_INT8:
		value = do_get_int8_pixel (raster, row, col, &opaque);
		break;
	    case RL2_SAMPLE_UINT8:
		value = do_get_uint8_pixel (raster, row, col, &opaque);
		break;
	    case RL2_SAMPLE_INT16:
		value = do_get_int16_pixel (raster, row, col, &opaque);
		break;
	    case RL2_SAMPLE_UINT16:
		value = do_get_uint16_pixel (raster, row, col, &opaque);
		break;
	    case RL2_SAMPLE_INT32:
		value = do_get_int32_pixel (raster, row, col, &opaque);
		break;
	    case RL2_SAMPLE_UINT32:
		value = do_get_uint32_pixel (raster, row, col, &opaque);
		break;
	    case RL2_SAMPLE_FLOAT:
		value = do_get_float_pixel (raster, row, col, &opaque);
		break;
	    case RL2_SAMPLE_DOUBLE:
		value = do_get_double_pixel (raster, row, col, &opaque);
		break;
	    default:
		opaque = 0;
		break;
	    };
	  if (opaque)
	    {
		if (has_no_data && value == no_data)
		    *transparent = 1;
		else
		    *transparent = 0;
	    }
	  else
	      *transparent = 1;
      }
    else
      {
	  *transparent = 1;
	  return no_data;
      }
    return value;
}

static int
do_drape_one_geom (rl2AuxDrapeGeometriesPtr aux)
{
/* processing the current Geometry */
    sqlite3_stmt *stmt = aux->stmt_upd;
    sqlite3_int64 rowid = aux->geom_rowid;
    rl2UpdatableGeometryPtr geom = aux->geometry;
    rl2PrivRasterPtr raster = aux->raster;
    rl2CoordSeqPtr pCS;
    int transparent;
    int to_be_updated = 0;

    pCS = geom->first;
    while (pCS != NULL)
      {
	  /* looping on Coordinate Sequences */
	  if (check_matching_bboxes
	      (aux->tile_minx, aux->tile_maxy, aux->tile_miny, aux->tile_maxy,
	       geom->minx, geom->maxx, geom->miny, geom->maxy))
	    {
		int iv;
		for (iv = 0; iv < pCS->points; iv++)
		  {
		      double x;
		      double y;
		      double zm;
		      x = rl2_get_coord_seq_value (pCS, iv, 'x');
		      y = rl2_get_coord_seq_value (pCS, iv, 'y');
		      if (!check_matching_point
			  (aux->tile_minx, aux->tile_maxx, aux->tile_miny,
			   aux->tile_maxy, x, y))
			  continue;
		      if (aux->update_m)
			  zm = rl2_get_coord_seq_value (pCS, iv, 'm');
		      else
			  zm = rl2_get_coord_seq_value (pCS, iv, 'z');
		      if (zm != aux->geom_no_data)
			  continue;	/* preserving already set values */
		      zm = do_get_pixel_value (raster, x, y, aux, &transparent);
		      if (!transparent)
			{
			    if (aux->update_m)
				rl2_set_coord_seq_value (zm, pCS, iv, 'm');
			    else
				rl2_set_coord_seq_value (zm, pCS, iv, 'z');
			    to_be_updated = 1;
			}
		  }
	    }
	  pCS = pCS->next;
      }

    if (to_be_updated)
      {
	  /* updating the Spatial Feature */
	  int ret;
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_blob (stmt, 1, geom->blob, geom->size, SQLITE_STATIC);
	  sqlite3_bind_int64 (stmt, 2, rowid);
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	      aux->update_count += 1;
	  else
	    {
		if (aux->message != NULL)
		    sqlite3_free (aux->message);
		aux->message =
		    sqlite3_mprintf
		    ("DrapeGeometries: error when updating geometry (feature rowid=%lld)\n",
		     rowid);
		return 0;
	    }
      }

    return 1;
}

static int
do_drape_geoms (rl2AuxDrapeGeometriesPtr aux)
{
/* processing the current Raster Tile */
    int ret;
    const char *table = aux->spatial_table;
    const char *geom_column = aux->geometry_column;
    sqlite3_stmt *stmt_geom = aux->stmt_geom;
    const unsigned char *geom_blob = aux->geom_blob;
    int geom_sz = aux->geom_sz;
    const unsigned char *odd_blob = aux->odd_blob;
    int odd_sz = aux->odd_sz;
    const unsigned char *even_blob = aux->even_blob;
    int even_sz = aux->even_sz;
    rl2RasterPtr raster = NULL;

    sqlite3_reset (stmt_geom);
    sqlite3_clear_bindings (stmt_geom);
    sqlite3_bind_blob (stmt_geom, 1, geom_blob, geom_sz, SQLITE_STATIC);
    sqlite3_bind_text (stmt_geom, 2, table, strlen (table), SQLITE_STATIC);
    sqlite3_bind_text (stmt_geom, 3, geom_column, strlen (geom_column),
		       SQLITE_STATIC);
    sqlite3_bind_blob (stmt_geom, 4, geom_blob, geom_sz, SQLITE_STATIC);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_geom);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		rl2UpdatableGeometryPtr geom;
		const unsigned char *blob = NULL;
		int size;
		sqlite3_int64 rowid = sqlite3_column_int64 (stmt_geom, 0);
		if (raster == NULL)
		  {
		      raster =
			  rl2_raster_decode (RL2_SCALE_1, odd_blob, odd_sz,
					     even_blob, even_sz, NULL);
		      if (raster == NULL)
			{
			    if (aux->message != NULL)
				sqlite3_free (aux->message);
			    aux->message =
				sqlite3_mprintf
				("DrapeGeometries: Invalid Raster Tile (tile_id=%lld)\n",
				 aux->tile_id);
			    return 0;
			}
		      aux->raster = (rl2PrivRasterPtr) raster;
		  }
		if (sqlite3_column_type (stmt_geom, 1) == SQLITE_BLOB)
		  {
		      blob =
			  (const unsigned char *)
			  sqlite3_column_blob (stmt_geom, 1);
		      size = sqlite3_column_bytes (stmt_geom, 1);
		      geom = rl2_create_updatable_geometry (blob, size);
		      if (geom != NULL)
			{
			    aux->geom_rowid = rowid;
			    aux->geometry = geom;
			    if (!do_drape_one_geom (aux))
				return 0;
			    rl2_destroy_updatable_geometry (geom);
			}
		  }
	    }
	  else
	    {
		if (aux->message != NULL)
		    sqlite3_free (aux->message);
		aux->message =
		    sqlite3_mprintf ("DrapeGeometries: Geometry read error\n");
		if (raster != NULL)
		    rl2_destroy_raster (raster);
		return 0;
	    }
      }
    if (raster != NULL)
	rl2_destroy_raster (raster);
    return 1;
}

static int
do_drape_geometries (sqlite3 * sqlite, const char *db_prefix,
		     const char *raster_coverage, const char *spatial_table,
		     const char *geom_name, double no_data_value, int update_m,
		     char **msg)
{
/* Draping Geometries over a Raster DEM */
    char *xprefix;
    char *tiles;
    char *xtiles;
    char *data;
    char *xdata;
    char *xtable;
    char *xgeom;
    char *sql;
    sqlite3_stmt *stmt_tile = NULL;
    sqlite3_stmt *stmt_geom = NULL;
    sqlite3_stmt *stmt_upd = NULL;
    int ret;
    int has_no_data;
    double no_data;
    double horz_res;
    double vert_res;
    int has_z;
    int has_m;
    int rst_srid;
    int strict_resolution;
    int datagrid;
    int tot_tile_count = 0;
    int tot_update_count = 0;

    *msg = NULL;

/* checking the Raster Coverage for validity */
    if (!check_raster
	(sqlite, db_prefix, raster_coverage, &rst_srid, &datagrid,
	 &strict_resolution, &horz_res, &vert_res, &has_no_data, &no_data))
	goto error;
/* checking the Spatial Table for validity */
    if (!check_table (sqlite, spatial_table, geom_name, &has_z, &has_m))
	goto error;

/* preparing the SQL query - raster tiles */
    if (db_prefix == NULL)
	db_prefix = "MAIN";
    xprefix = rl2_double_quoted_sql (db_prefix);
    tiles = sqlite3_mprintf ("%s_tiles", raster_coverage);
    xtiles = rl2_double_quoted_sql (tiles);
    sqlite3_free (tiles);
    data = sqlite3_mprintf ("%s_tile_data", raster_coverage);
    xdata = rl2_double_quoted_sql (data);
    sqlite3_free (data);
    sql =
	sqlite3_mprintf
	("SELECT t.tile_id, t.geometry, d.tile_data_odd, d.tile_data_even "
	 "FROM \"%s\".\"%s\" AS t "
	 "JOIN \"%s\".\"%s\" AS d ON (t.tile_id = d.tile_id) "
	 "WHERE t.pyramid_level = 0", xprefix, xtiles, xprefix, xdata);
    free (xprefix);
    free (xtiles);
    free (xdata);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_tile, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

/* preparing the SQL query - geometries intersecting tile */
    xgeom = rl2_double_quoted_sql (geom_name);
    xtable = rl2_double_quoted_sql (spatial_table);
    sql = sqlite3_mprintf ("SELECT rowid, \"%s\" FROM MAIN.\"%s\" "
			   "WHERE ST_Intersects(\"%s\", ?) = 1 AND rowid IN ( "
			   "SELECT rowid FROM SpatialIndex WHERE f_table_name = ? "
			   "AND f_geometry_column = ? AND search_frame = ?)",
			   xgeom, xtable, xgeom);
    free (xtable);
    free (xgeom);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_geom, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

/* preparing the UPDATE statement - geometries */
    xgeom = rl2_double_quoted_sql (geom_name);
    xtable = rl2_double_quoted_sql (spatial_table);
    sql =
	sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET \"%s\" = ? WHERE rowid = ?",
			 xtable, xgeom);
    free (xtable);
    free (xgeom);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_upd, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

/* processing all DTM tiles */
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_tile);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		const unsigned char *geom_blob = NULL;
		int geom_sz = 0;
		const unsigned char *odd_blob = NULL;
		int odd_sz = 0;
		const unsigned char *even_blob = NULL;
		int even_sz = 0;
		sqlite3_int64 tile_id = sqlite3_column_int64 (stmt_tile, 0);
		if (sqlite3_column_type (stmt_tile, 1) == SQLITE_BLOB)
		  {
		      geom_blob =
			  (const unsigned char *)
			  sqlite3_column_blob (stmt_tile, 1);
		      geom_sz = sqlite3_column_bytes (stmt_tile, 1);
		  }
		if (sqlite3_column_type (stmt_tile, 2) == SQLITE_BLOB)
		  {
		      odd_blob =
			  (const unsigned char *)
			  sqlite3_column_blob (stmt_tile, 2);
		      odd_sz = sqlite3_column_bytes (stmt_tile, 2);
		  }
		if (sqlite3_column_type (stmt_tile, 3) == SQLITE_BLOB)
		  {
		      even_blob =
			  (const unsigned char *)
			  sqlite3_column_blob (stmt_tile, 3);
		      even_sz = sqlite3_column_bytes (stmt_tile, 3);
		  }
		if (geom_blob != NULL && odd_blob != NULL)
		  {
		      rl2AuxDrapeGeometries aux;
		      rl2GeometryPtr geom =
			  rl2_geometry_from_blob (geom_blob, geom_sz);
		      if (geom != NULL)
			{
			    aux.sqlite = sqlite;
			    aux.spatial_table = spatial_table;
			    aux.geometry_column = geom_name;
			    aux.stmt_geom = stmt_geom;
			    aux.stmt_upd = stmt_upd;
			    aux.tile_id = tile_id;
			    aux.update_count = 0;
			    aux.tile_minx = geom->minx;
			    aux.tile_maxx = geom->maxx;
			    aux.tile_miny = geom->miny;
			    aux.tile_maxy = geom->maxy;
			    aux.raster_has_no_data = has_no_data;
			    aux.raster_no_data = no_data;
			    aux.geom_blob = geom_blob;
			    aux.geom_sz = geom_sz;
			    aux.odd_blob = odd_blob;
			    aux.odd_sz = odd_sz;
			    aux.even_blob = even_blob;
			    aux.even_sz = even_sz;
			    aux.horz_res = horz_res;
			    aux.vert_res = vert_res;
			    aux.update_m = update_m;
			    aux.geom_no_data = no_data_value;
			    aux.has_z = has_z;
			    aux.has_m = has_m;
			    aux.message = NULL;
			    rl2_destroy_geometry (geom);
			    if (!do_drape_geoms (&aux))
			      {
				  *msg = aux.message;
				  goto error;
			      }
			    tot_tile_count++;
			    tot_update_count += aux.update_count;
			}
		  }
		else
		  {
		      *msg =
			  sqlite3_mprintf
			  ("DrapeGeometries: invalid Geometry\n");
		      goto error;
		  }
	    }
      }

    sqlite3_finalize (stmt_tile);
    sqlite3_finalize (stmt_geom);
    sqlite3_finalize (stmt_upd);
    return 1;

  error:
    if (stmt_tile != NULL)
	sqlite3_finalize (stmt_tile);
    if (stmt_geom != NULL)
	sqlite3_finalize (stmt_geom);
    if (stmt_upd != NULL)
	sqlite3_finalize (stmt_upd);
    return 0;
}

static rl2DouglasPeuckerSeqPtr
alloc_douglas_peucker_seq (int count)
{
/* allocating an empty Douglas-Peucker list */
    int i;
    rl2DouglasPeuckerSeqPtr dps = malloc (sizeof (rl2DouglasPeuckerSeq));
    dps->count = count;
    dps->points = malloc (sizeof (rl2DouglasPeuckerPoint) * count);
    for (i = 0; i < dps->count; i++)
      {
	  rl2DouglasPeuckerPointPtr pt = dps->points + i;
	  pt->no_data = 1;
	  pt->confirmed = 0;
      }
    dps->valid_count = 0;
    dps->valid_points = NULL;
    return dps;
}

static void
insert_into_douglas_peuker (rl2DouglasPeuckerSeqPtr dps, int iv, double x,
			    double y, double z, double m, int dims,
			    double no_data_value, int update_m)
{
/* inserting a point into the Dougles-Peuker list */
    rl2DouglasPeuckerPointPtr pt = dps->points + iv;
    pt->x = x;
    pt->y = y;
    switch (dims)
      {
      case GAIA_XY_Z_M:
	  pt->z = z;
	  pt->m = m;
	  if (update_m)
	    {
		if (m != no_data_value)
		    pt->no_data = 0;
	    }
	  else
	    {
		if (z != no_data_value)
		    pt->no_data = 0;
	    }
	  break;
      case GAIA_XY_Z:
	  pt->z = z;
	  if (!update_m)
	    {
		if (z != no_data_value)
		    pt->no_data = 0;
	    }
	  break;
      case GAIA_XY_M:
	  pt->m = m;
	  if (update_m)
	    {
		if (m != no_data_value)
		    pt->no_data = 0;
	    }
	  break;
      };
}

static void
do_complete_douglas_peucker (rl2DouglasPeuckerSeqPtr dps)
{
/* completing the Dougles-Peuker list */
    int iv;
    double lx;
    double ly;
    double progr_dist = 0.0;
    for (iv = 0; iv < dps->count; iv++)
      {
	  /* computing the progressive distance for each Vertex */
	  rl2DouglasPeuckerPointPtr pt = dps->points + iv;
	  if (iv == 0)
	    {
		pt->progr_dist = 0.0;
		lx = pt->x;
		ly = pt->y;
		continue;
	    }
	  progr_dist +=
	      sqrt (((pt->x - lx) * (pt->x - lx)) +
		    ((pt->y - ly) * (pt->y - ly)));
	  lx = pt->x;
	  ly = pt->y;
	  pt->progr_dist = progr_dist;
      }
}

static int
confirm_douglas_peuker_original_vertex (rl2DouglasPeuckerSeqPtr dps, double x,
					double y, int *start)
{
/* checking the first matching Vertex */
    int iv;
    for (iv = *start; iv < dps->count; iv++)
      {
	  rl2DouglasPeuckerPointPtr pt = dps->points + iv;
	  if (pt->x == x && pt->y == y)
	    {
		pt->confirmed = 1;
		*start = iv + 1;
		return 1;
	    }
      }
    return 0;
}

static void
do_prepare_douglas_peucker (rl2DouglasPeuckerSeqPtr dps)
{
/* preparing the Dougls-Peucker object for final simplification */
    int iv;
    int iref;
    int count = 0;
    double base_dist;
    rl2DouglasPeuckerPointPtr pt;
    rl2DouglasPeuckerPointRefPtr ref;
    for (iv = 0; iv < dps->count; iv++)
      {
	  pt = dps->points + iv;
	  if (pt->no_data == 0)
	      count++;
      }
    if (dps->valid_points != NULL)
	free (dps->valid_points);
    dps->valid_points = NULL;
    dps->valid_count = count;
    if (count > 0)
      {
	  dps->valid_points =
	      malloc (sizeof (rl2DouglasPeuckerPointRef) * count);
	  iref = 0;
	  for (iv = 0; iv < dps->count; iv++)
	    {
		pt = dps->points + iv;
		if (pt->no_data == 0)
		  {
		      ref = dps->valid_points + iref;
		      ref->point = pt;
		      ref->index = iv;
		      if (iref == 0)
			{
			    ref->progr_dist = 0.0;
			    base_dist = pt->progr_dist;
			}
		      else
			  ref->progr_dist = pt->progr_dist - base_dist;
		      iref++;
		  }
	    }
      }
}

static double
douglas_peucker_dist (sqlite3_stmt * stmt, rl2GeometryPtr line,
		      rl2GeometryPtr point)
{
/* computing the perpendicular distance between a Point and a Line */
    int ret;
    double dist = 0.0;
    unsigned char *line_blob = NULL;
    int line_size;
    unsigned char *point_blob = NULL;
    int point_size;

    if (!rl2_geometry_to_blob (line, &line_blob, &line_size))
	goto error;
    if (!rl2_geometry_to_blob (point, &point_blob, &point_size))
	goto error;
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, line_blob, line_size, free);
    sqlite3_bind_blob (stmt, 2, point_blob, point_size, free);
    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		if (sqlite3_column_type (stmt, 0) == SQLITE_FLOAT)
		    dist = sqlite3_column_double (stmt, 0);
	    }
	  else
	      goto error;
      }
    return dist;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
    if (line_blob != NULL)
	free (line_blob);
    if (point_blob != NULL)
	free (point_blob);
    return 0.0;
}

static void
do_compute_douglas_peucker (sqlite3_stmt * stmt, rl2DouglasPeuckerSeqPtr dps,
			    int start, int end, double z_simplify_dist,
			    int update_m)
{
/* computing the Dougles-Peucker simplification */
    int iv;
    double d;
    double zm;
    double sd;
    double szm;
    double ed;
    double ezm;
    double base_dist;
    double max_dist = 0.0;
    int max_iv = -1;
    rl2DouglasPeuckerPointRefPtr ref;
    rl2GeometryPtr point;
    rl2GeometryPtr line;
    rl2LinestringPtr ln;

    if (start < 0 || start >= dps->valid_count)
	return;
    if (end < (start + 1) || end >= dps->valid_count)
	return;
    if (start >= end)
	return;
/* preparing the start Point of the BaseLine */
    ref = dps->valid_points + start;
    sd = 0.0;
    base_dist = ref->progr_dist;
    if (update_m)
	szm = ref->point->m;
    else
	szm = ref->point->z;
/* preparing the end Point of the BaseLine */
    ref = dps->valid_points + end;
    ed = ref->progr_dist - base_dist;
    if (update_m)
	ezm = ref->point->m;
    else
	ezm = ref->point->z;
/* preparing the BaseLine Geometry */
    line = rl2CreateGeometry (GAIA_XY, GAIA_LINESTRING);
    line->srid = -1;
    ln = rl2AddLinestringToGeometry (line, 2);
    rl2SetPoint (ln->coords, 0, sd, szm);
    rl2SetPoint (ln->coords, 1, ed, ezm);

    for (iv = start + 1; iv < end; iv++)
      {
	  /* identifying the max Distance Vertex */
	  ref = dps->valid_points + iv;
	  point = rl2CreateGeometry (GAIA_XY, GAIA_POINT);
	  point->srid = -1;
	  d = ref->progr_dist - base_dist;
	  if (update_m)
	      zm = ref->point->m;
	  else
	      zm = ref->point->z;
	  rl2AddPointXYToGeometry (point, d, zm);
	  d = douglas_peucker_dist (stmt, line, point);
	  rl2_destroy_geometry (point);
	  if (d > z_simplify_dist)
	    {
		if (d > max_dist)
		  {
		      max_dist = d;
		      max_iv = iv;
		  }
	    }
      }
    rl2_destroy_geometry (line);

    if (max_iv > -1)
      {
	  /* marking a confirmed Vertex */
	  ref = dps->valid_points + max_iv;
	  ref->point->confirmed = 1;
	  /* recursion */
	  do_compute_douglas_peucker (stmt, dps, start, max_iv, z_simplify_dist,
				      update_m);
	  do_compute_douglas_peucker (stmt, dps, max_iv, end, z_simplify_dist,
				      update_m);
      }
}

static void
destroy_douglas_peucker_seq (rl2DouglasPeuckerSeqPtr dps)
{
/* memory cleanup - destroying a Douglas-Peucker list */
    if (dps->points != NULL)
	free (dps->points);
    if (dps->valid_points != NULL)
	free (dps->valid_points);
    free (dps);
}

static int
simplify_geometry (sqlite3_stmt * stmt_dist, sqlite3_stmt * stmt,
		   sqlite3_int64 rowid, rl2GeometryPtr geom_in,
		   rl2GeometryPtr geom_out, double no_data_value,
		   double z_simplify_dist, int update_m, char **msg)
{
/* simplifying a single Geometry */
    rl2GeometryPtr geom_upd;
    rl2PointPtr pt;
    rl2LinestringPtr ln;
    rl2LinestringPtr ln_in;
    rl2LinestringPtr ln_out;
    rl2PolygonPtr pg;
    rl2PolygonPtr pg_in;
    rl2PolygonPtr pg_out;
    rl2RingPtr rng;
    rl2RingPtr rng_in;
    rl2RingPtr rng_out;
    int ib;
    int iv;
    int ret;
    unsigned char *blob;
    int blob_size;
    double x_in;
    double y_in;
    double x_out;
    double y_out;
    double z = 0.0;
    double m = 0.0;
    int start;
    rl2DynLinePtr dyn = NULL;
    rl2DouglasPeuckerSeqPtr dps = NULL;
    rl2DouglasPeuckerPointPtr ptx;

    *msg = NULL;
/* creating the simplified output Geometry */
    geom_upd = rl2CreateGeometry (geom_out->dims, geom_out->type);
    geom_upd->srid = geom_out->srid;

    pt = geom_out->first_point;
    while (pt != NULL)
      {
	  /* copying all Points exactly as they are */
	  switch (geom_upd->dims)
	    {
	    case GAIA_XY_Z_M:
		rl2AddPointXYZMToGeometry (geom_upd, pt->x, pt->y, pt->z,
					   pt->m);
		break;
	    case GAIA_XY_Z:
		rl2AddPointXYZToGeometry (geom_upd, pt->x, pt->y, pt->z);
		break;
	    case GAIA_XY_M:
		rl2AddPointXYMToGeometry (geom_upd, pt->x, pt->y, pt->m);
		break;
	    case GAIA_XY:
	    default:
		rl2AddPointXYToGeometry (geom_upd, pt->x, pt->y);
		break;
	    };
	  pt = pt->next;
      }

    ln_in = geom_in->first_linestring;
    ln_out = geom_out->first_linestring;
    while (ln_in != NULL)
      {
	  if (ln_out == NULL)
	      goto mismatching_line;
	  /* creating and populating the Douglas-Peucker object */
	  dps = alloc_douglas_peucker_seq (ln_out->points);
	  for (iv = 0; iv < ln_out->points; iv++)
	    {
		switch (ln_out->dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2GetPointZM (ln_out->coords, iv, &x_out, &y_out, &z,
				     &m);
		      break;
		  case GAIA_XY_Z:
		      rl2GetPointZ (ln_out->coords, iv, &x_out, &y_out, &z);
		      break;
		  case GAIA_XY_M:
		      rl2GetPointM (ln_out->coords, iv, &x_out, &y_out, &m);
		      break;
		  case GAIA_XY:
		  default:
		      rl2GetPoint (ln_out->coords, iv, &x_out, &y_out);
		      break;
		  };
		insert_into_douglas_peuker (dps, iv, x_out, y_out, z, m,
					    ln_out->dims, no_data_value,
					    update_m);
	    }
	  do_complete_douglas_peucker (dps);

	  /* confirming all matching Vertices from the Input Line */
	  start = 0;
	  for (iv = 0; iv < ln_in->points; iv++)
	    {
		switch (ln_in->dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2GetPointZM (ln_in->coords, iv, &x_in, &y_in, &z, &m);
		      break;
		  case GAIA_XY_Z:
		      rl2GetPointZ (ln_in->coords, iv, &x_in, &y_in, &z);
		      break;
		  case GAIA_XY_M:
		      rl2GetPointM (ln_in->coords, iv, &x_in, &y_in, &m);
		      break;
		  case GAIA_XY:
		  default:
		      rl2GetPoint (ln_in->coords, iv, &x_in, &y_in);
		      break;
		  };
		if (!confirm_douglas_peuker_original_vertex
		    (dps, x_in, y_in, &start))
		    goto mismatching_line;
	    }
	  /* verifying the first and last Output Vertex */
	  ptx = dps->points + 0;
	  if (ptx->confirmed != 1)
	      goto mismatching_line;
	  ptx = dps->points + dps->count - 1;
	  if (ptx->confirmed != 1)
	      goto mismatching_line;

	  /* applying the Dougles-Peucker simplification */
	  do_prepare_douglas_peucker (dps);
	  do_compute_douglas_peucker (stmt_dist, dps, 0, dps->valid_count - 1,
				      z_simplify_dist, update_m);

	  /* creating and populating the DynLine */
	  dyn = rl2CreateDynLine ();
	  for (iv = 0; iv < dps->count; iv++)
	    {
		ptx = dps->points + iv;
		if (ptx->confirmed == 0)
		    continue;
		switch (ln_out->dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2AddDynPointZM (dyn, ptx->x, ptx->y, ptx->z, ptx->m);
		      break;
		  case GAIA_XY_Z:
		      rl2AddDynPointZ (dyn, ptx->x, ptx->y, ptx->z);
		      break;
		  case GAIA_XY_M:
		      rl2AddDynPointM (dyn, ptx->x, ptx->y, ptx->m);
		      break;
		  };
	    }
	  /* inserting the simplified Linestring into the Simplified Output Geometry */
	  ln = rl2AddLinestringToGeometry (geom_upd,
					   rl2CountDynLinePoints (dyn));
	  copyDynLinestring (dyn, ln);
	  rl2DestroyDynLine (dyn);
	  destroy_douglas_peucker_seq (dps);
	  dps = NULL;

	  ln_in = ln_in->next;
	  ln_out = ln_out->next;
	  if (ln_in == NULL && ln_out != NULL)
	      goto mismatching_line;
      }

    pg_in = geom_in->first_polygon;
    pg_out = geom_out->first_polygon;
    while (pg_in != NULL)
      {
	  if (pg_out == NULL)
	      goto mismatching_polygon;
	  if (pg_in->num_interiors != pg_out->num_interiors)
	      goto mismatching_polygon;
	  /* simplifying the Exterior Ring */
	  rng_in = pg_in->exterior;
	  rng_out = pg_out->exterior;
	  /* creating and populating the Douglas-Peucker object */
	  dps = alloc_douglas_peucker_seq (rng_out->points);
	  for (iv = 0; iv < rng_out->points; iv++)
	    {
		switch (rng_out->dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2GetPointZM (rng_out->coords, iv, &x_out, &y_out, &z,
				     &m);
		      break;
		  case GAIA_XY_Z:
		      rl2GetPointZ (rng_out->coords, iv, &x_out, &y_out, &z);
		      break;
		  case GAIA_XY_M:
		      rl2GetPointM (rng_out->coords, iv, &x_out, &y_out, &m);
		      break;
		  case GAIA_XY:
		  default:
		      rl2GetPoint (rng_out->coords, iv, &x_out, &y_out);
		      break;
		  };
		insert_into_douglas_peuker (dps, iv, x_out, y_out, z, m,
					    rng_out->dims, no_data_value,
					    update_m);
	    }
	  do_complete_douglas_peucker (dps);

	  /* confirming all matching Vertices from the Input Line */
	  start = 0;
	  for (iv = 0; iv < rng_in->points; iv++)
	    {
		switch (rng_in->dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2GetPointZM (rng_in->coords, iv, &x_in, &y_in, &z, &m);
		      break;
		  case GAIA_XY_Z:
		      rl2GetPointZ (rng_in->coords, iv, &x_in, &y_in, &z);
		      break;
		  case GAIA_XY_M:
		      rl2GetPointM (rng_in->coords, iv, &x_in, &y_in, &m);
		      break;
		  case GAIA_XY:
		  default:
		      rl2GetPoint (rng_in->coords, iv, &x_in, &y_in);
		      break;
		  };
		if (!confirm_douglas_peuker_original_vertex
		    (dps, x_in, y_in, &start))
		    goto mismatching_ring;
	    }
	  /* verifying the first and last Output Vertex */
	  ptx = dps->points + 0;
	  if (ptx->confirmed != 1)
	      goto mismatching_ring;
	  ptx = dps->points + dps->count - 1;
	  if (ptx->confirmed != 1)
	      goto mismatching_ring;

	  /* applying the Dougles-Peucker simplification */
	  do_prepare_douglas_peucker (dps);
	  do_compute_douglas_peucker (stmt_dist, dps, 0, dps->valid_count - 1,
				      z_simplify_dist, update_m);

	  /* creating and populating the DynLine */
	  dyn = rl2CreateDynLine ();
	  for (iv = 0; iv < dps->count; iv++)
	    {
		ptx = dps->points + iv;
		if (ptx->confirmed == 0)
		    continue;
		switch (rng_out->dims)
		  {
		  case GAIA_XY_Z_M:
		      rl2AddDynPointZM (dyn, ptx->x, ptx->y, ptx->z, ptx->m);
		      break;
		  case GAIA_XY_Z:
		      rl2AddDynPointZ (dyn, ptx->x, ptx->y, ptx->z);
		      break;
		  case GAIA_XY_M:
		      rl2AddDynPointM (dyn, ptx->x, ptx->y, ptx->m);
		      break;
		  };
	    }
	  /* inserting the simplified Polygon into the Simplified Output Geometry */
	  pg = rl2AddPolygonToGeometry (geom_upd,
					rl2CountDynLinePoints (dyn),
					pg_out->num_interiors);
	  rng = pg->exterior;
	  copyDynRing (dyn, rng);
	  rl2DestroyDynLine (dyn);
	  destroy_douglas_peucker_seq (dps);
	  dps = NULL;

	  for (ib = 0; ib < pg_out->num_interiors; ib++)
	    {
		/* simplifying all Interior Rings */
		rng_in = pg_in->interiors + ib;
		rng_out = pg_out->interiors + ib;
		/* creating and populating the Douglas-Peucker object */
		dps = alloc_douglas_peucker_seq (rng_out->points);
		for (iv = 0; iv < rng_out->points; iv++)
		  {
		      switch (rng_out->dims)
			{
			case GAIA_XY_Z_M:
			    rl2GetPointZM (rng_out->coords, iv, &x_out, &y_out,
					   &z, &m);
			    break;
			case GAIA_XY_Z:
			    rl2GetPointZ (rng_out->coords, iv, &x_out, &y_out,
					  &z);
			    break;
			case GAIA_XY_M:
			    rl2GetPointM (rng_out->coords, iv, &x_out, &y_out,
					  &m);
			    break;
			case GAIA_XY:
			default:
			    rl2GetPoint (rng_out->coords, iv, &x_out, &y_out);
			    break;
			};
		      insert_into_douglas_peuker (dps, iv, x_out, y_out, z, m,
						  rng_out->dims, no_data_value,
						  update_m);
		  }
		do_complete_douglas_peucker (dps);

		/* confirming all matching Vertices from the Input Line */
		start = 0;
		for (iv = 0; iv < rng_in->points; iv++)
		  {
		      switch (rng_in->dims)
			{
			case GAIA_XY_Z_M:
			    rl2GetPointZM (rng_in->coords, iv, &x_in, &y_in, &z,
					   &m);
			    break;
			case GAIA_XY_Z:
			    rl2GetPointZ (rng_in->coords, iv, &x_in, &y_in, &z);
			    break;
			case GAIA_XY_M:
			    rl2GetPointM (rng_in->coords, iv, &x_in, &y_in, &m);
			    break;
			case GAIA_XY:
			default:
			    rl2GetPoint (rng_in->coords, iv, &x_in, &y_in);
			    break;
			};
		      if (!confirm_douglas_peuker_original_vertex
			  (dps, x_in, y_in, &start))
			  goto mismatching_ring;
		  }
		/* verifying the first and last Output Vertex */
		ptx = dps->points + 0;
		if (ptx->confirmed != 1)
		    goto mismatching_ring;
		ptx = dps->points + dps->count - 1;
		if (ptx->confirmed != 1)
		    goto mismatching_ring;

		/* applying the Dougles-Peucker simplification */
		do_prepare_douglas_peucker (dps);
		do_compute_douglas_peucker (stmt_dist, dps, 0,
					    dps->valid_count - 1,
					    z_simplify_dist, update_m);

		/* creating and populating the DynLine */
		dyn = rl2CreateDynLine ();
		for (iv = 0; iv < dps->count; iv++)
		  {
		      ptx = dps->points + iv;
		      if (ptx->confirmed == 0)
			  continue;
		      switch (rng_out->dims)
			{
			case GAIA_XY_Z_M:
			    rl2AddDynPointZM (dyn, ptx->x, ptx->y, ptx->z,
					      ptx->m);
			    break;
			case GAIA_XY_Z:
			    rl2AddDynPointZ (dyn, ptx->x, ptx->y, ptx->z);
			    break;
			case GAIA_XY_M:
			    rl2AddDynPointM (dyn, ptx->x, ptx->y, ptx->m);
			    break;
			};
		  }
		/* inserting the simplified Ring into the final Output Polygon */
		rng = rl2AddInteriorRing (pg, ib, rl2CountDynLinePoints (dyn));
		copyDynRing (dyn, rng);
		rl2DestroyDynLine (dyn);
		destroy_douglas_peucker_seq (dps);
		dps = NULL;
	    }

	  pg_in = pg_in->next;
	  pg_out = pg_out->next;
	  if (pg_in == NULL && pg_out != NULL)
	      goto mismatching_polygon;
      }

/* updating the simplified output Geometry */
    if (!rl2_geometry_to_blob (geom_upd, &blob, &blob_size))
      {
	  rl2_destroy_geometry (geom_upd);
	  return 0;
      }
    rl2_destroy_geometry (geom_upd);
    sqlite3_reset (stmt);
    sqlite3_clear_bindings (stmt);
    sqlite3_bind_blob (stmt, 1, blob, blob_size, free);
    sqlite3_bind_int64 (stmt, 2, rowid);
    ret = sqlite3_step (stmt);
    if (ret == SQLITE_DONE || ret == SQLITE_ROW)
	;
    else
	return 0;
    return 1;

  mismatching_line:
    if (dps != NULL)
	destroy_douglas_peucker_seq (dps);
    *msg =
	sqlite3_mprintf
	("DrapeGeometries - Simplify: mismatching Linestrings (feature rowid=%lld)",
	 rowid);
    rl2_destroy_geometry (geom_upd);
    return 0;

  mismatching_polygon:
    if (dps != NULL)
	destroy_douglas_peucker_seq (dps);
    *msg =
	sqlite3_mprintf
	("DrapeGeometries - Simplify: mismatching Polygons (feature rowid=%lld)",
	 rowid);
    rl2_destroy_geometry (geom_upd);
    return 0;

  mismatching_ring:
    if (dps != NULL)
	destroy_douglas_peucker_seq (dps);
    *msg =
	sqlite3_mprintf
	("DrapeGeometries - Simplify: mismatching Rings (feature rowid=%lld)",
	 rowid);
    rl2_destroy_geometry (geom_upd);
    return 0;
}

static int
do_simplify (sqlite3 * sqlite, const char *spatial_table, const char *old_geom,
	     const char *new_geom, int gtype, double no_data_value,
	     double z_simplify_dist, int update_m, char **msg)
{
/* final simplification step */
    char *xgeom_in;
    char *xgeom_out;
    char *xtable;
    char *sql;
    const char *sql2;
    int ret;
    sqlite3_stmt *stmt_dist = NULL;
    sqlite3_stmt *stmt_in = NULL;
    sqlite3_stmt *stmt_upd = NULL;

    *msg = NULL;
    if (z_simplify_dist <= 0.0)
	return 1;		/* skipping if no Simplify distance has been specified */
    switch (gtype)
      {
	  /* skipping (multi)POINT tables */
      case GAIA_POINT:
      case GAIA_POINTZ:
      case GAIA_POINTM:
      case GAIA_POINTZM:
      case GAIA_MULTIPOINT:
      case GAIA_MULTIPOINTZ:
      case GAIA_MULTIPOINTM:
      case GAIA_MULTIPOINTZM:
	  return 1;
      };

/* preparing the ST_Distance SQL statement */
    sql2 = "SELECT ST_Distance(?, ?)";
    ret = sqlite3_prepare_v2 (sqlite, sql2, strlen (sql2), &stmt_dist, NULL);
    if (ret != SQLITE_OK)
	goto error;

/* preparing the SELECT statement - geometries */
    xgeom_in = rl2_double_quoted_sql (old_geom);
    xgeom_out = rl2_double_quoted_sql (new_geom);
    xtable = rl2_double_quoted_sql (spatial_table);
    sql = sqlite3_mprintf ("SELECT rowid, \"%s\", \"%s\" FROM MAIN.\"%s\"",
			   xgeom_in, xgeom_out, xtable);
    free (xtable);
    free (xgeom_in);
    free (xgeom_out);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_in, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

/* preparing the UPDATE statement - output geometry */
    xgeom_out = rl2_double_quoted_sql (new_geom);
    xtable = rl2_double_quoted_sql (spatial_table);
    sql =
	sqlite3_mprintf ("UPDATE MAIN.\"%s\" SET \"%s\" = ? WHERE rowid = ?",
			 xtable, xgeom_out);
    free (xtable);
    free (xgeom_out);
    ret = sqlite3_prepare_v2 (sqlite, sql, strlen (sql), &stmt_upd, NULL);
    sqlite3_free (sql);
    if (ret != SQLITE_OK)
	goto error;

    while (1)
      {
	  /* scrolling the result set rows */
	  ret = sqlite3_step (stmt_in);
	  if (ret == SQLITE_DONE)
	      break;		/* end of result set */
	  if (ret == SQLITE_ROW)
	    {
		rl2GeometryPtr geom_in = NULL;
		rl2GeometryPtr geom_out = NULL;
		const unsigned char *blob = NULL;
		int size;
		sqlite3_int64 rowid = sqlite3_column_int64 (stmt_in, 0);
		if (sqlite3_column_type (stmt_in, 1) == SQLITE_BLOB)
		  {
		      blob =
			  (const unsigned char *) sqlite3_column_blob (stmt_in,
								       1);
		      size = sqlite3_column_bytes (stmt_in, 1);
		      geom_in = rl2_geometry_from_blob (blob, size);
		  }
		if (sqlite3_column_type (stmt_in, 2) == SQLITE_BLOB)
		  {
		      blob =
			  (const unsigned char *) sqlite3_column_blob (stmt_in,
								       2);
		      size = sqlite3_column_bytes (stmt_in, 2);
		      geom_out = rl2_geometry_from_blob (blob, size);
		  }
		if (geom_in != NULL && geom_out != NULL)
		  {
		      if (!simplify_geometry
			  (stmt_dist, stmt_upd, rowid, geom_in, geom_out,
			   no_data_value, z_simplify_dist, update_m, msg))
			{
			    rl2_destroy_geometry (geom_in);
			    rl2_destroy_geometry (geom_out);
			    goto error;
			}
		      rl2_destroy_geometry (geom_in);
		      rl2_destroy_geometry (geom_out);
		  }
		else
		  {
		      *msg =
			  sqlite3_mprintf
			  ("Simplify: unexpected NULL or invalid Geometry (feature rowid=%lld)",
			   rowid);
		      if (geom_in != NULL)
			  rl2_destroy_geometry (geom_in);
		      if (geom_out != NULL)
			  rl2_destroy_geometry (geom_out);
		      goto error;
		  }
	    }
	  else
	    {
		*msg =
		    sqlite3_mprintf
		    ("DrapeGeometries - Simplify: Geometry read error\n");
		goto error;
	    }
      }

    sqlite3_finalize (stmt_in);
    sqlite3_finalize (stmt_upd);
    sqlite3_finalize (stmt_dist);
    return 1;

  error:
    if (stmt_in != NULL)
	sqlite3_finalize (stmt_in);
    if (stmt_upd != NULL)
	sqlite3_finalize (stmt_upd);
    if (stmt_dist != NULL)
	sqlite3_finalize (stmt_dist);
    return 0;
}

static int
do_get_current_pragmas (sqlite3 * sqlite, char *mode, char *sync)
{
/* retrieving the current PRAGMA settings */
    int error = 0;
    int i;
    char **results;
    int rows;
    int columns;
    *mode = '\0';
    *sync = '\0';
    int ret = sqlite3_get_table (sqlite, "PRAGMA journal_mode", &results, &rows,
				 &columns, NULL);
    if (ret != SQLITE_OK)
	error = 1;
    if (rows < 1)
	error = 1;
    else
      {
	  for (i = 1; i <= rows; i++)
	      strcpy (mode, results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);
    ret =
	sqlite3_get_table (sqlite, "PRAGMA synchronous", &results, &rows,
			   &columns, NULL);
    if (ret != SQLITE_OK)
	error = 1;
    if (rows < 1)
	error = 1;
    else
      {
	  for (i = 1; i <= rows; i++)
	      strcpy (sync, results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);
    if (error)
	return 0;
    return 1;
}

RL2_DECLARE int
rl2_drape_geometries (sqlite3 * sqlite,
		      const void *data,
		      const char *db_prefix,
		      const char *raster_coverage,
		      const char *coverage_list_table,
		      const char *spatial_table,
		      const char *old_geom,
		      const char *new_geom,
		      double no_data_value,
		      double densify_dist, double z_simplify_dist, int update_m)
{
/* Adding Z-values into some Spatial Table by draping its Geometries over a Raster DEM */
    int rst_srid;
    int geom_srid;
    int gtype;
    int target_dims;
    struct rl2_private_data *priv_data = (struct rl2_private_data *) data;
    char *msg;
    const char *sql;
    char *pragma;
    char mode[64];
    char xmode[64];
    char sync[64];
    char xsync[64];

    if (raster_coverage == NULL && coverage_list_table == NULL)
      {
	  rl2_set_draping_message (priv_data,
				   "Neither a RasterCoverage name nor the name of a Table containing a List of Raster Coverages was specified.");
	  goto error;
      }
    if (raster_coverage != NULL && coverage_list_table != NULL)
      {
	  rl2_set_draping_message (priv_data,
				   "The name of a RasterCoverage and the name of a Table containing a List of Raster Coverages cannot be specified at the same time.");
	  goto error;
      }

    if (raster_coverage != NULL)
      {
	  /* checking the Raster Coverage for validity */
	  if (!do_check_raster_coverage
	      (sqlite, db_prefix, raster_coverage, &rst_srid, &msg))
	    {
		rl2_set_draping_message (priv_data, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (coverage_list_table != NULL)
      {
	  /* checking the Coverage List Table for validity */
	  if (!check_coverage_list
	      (sqlite, coverage_list_table, &rst_srid, &msg))
	    {
		rl2_set_draping_message (priv_data, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

/* checking the Spatial Table for validity */
    if (strcasecmp (old_geom, new_geom) == 0)
      {
	  msg =
	      sqlite3_mprintf
	      ("OldGeom and NewGeom shouldn't be the same Column.");
	  rl2_set_draping_message (priv_data, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    if (!check_spatial_table
	(sqlite, spatial_table, old_geom, new_geom, &geom_srid, &gtype, &msg))
      {
	  rl2_set_draping_message (priv_data, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    if (rst_srid != geom_srid)
      {
	  msg =
	      sqlite3_mprintf
	      ("Mismatching SRIDs between Raster DEM (%d) and Spatial Table (%d).",
	       rst_srid, geom_srid);
	  rl2_set_draping_message (priv_data, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* setting up all PRAGMAs */
    if (!do_get_current_pragmas (sqlite, mode, sync))
      {
	  msg =
	      sqlite3_mprintf
	      ("Unable to retrieve the current PRAGMA settings.");
	  rl2_set_draping_message (priv_data, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    sql = "PRAGMA journal_mode=OFF";
    sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    sql = "PRAGMA synchronous=OFF";
    sqlite3_exec (sqlite, sql, NULL, NULL, NULL);
    if (!do_get_current_pragmas (sqlite, xmode, xsync))
      {
	  msg =
	      sqlite3_mprintf ("Unable to check the current PRAGMA settings.");
	  rl2_set_draping_message (priv_data, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    if (strcasecmp (xmode, "off") != 0)
      {
	  msg = sqlite3_mprintf ("Unable to set PRAGMA journal_mode=OFF");
	  rl2_set_draping_message (priv_data, msg);
	  sqlite3_free (msg);
	  goto error;
      }
    if (strcmp (xsync, "0") != 0)
      {
	  msg = sqlite3_mprintf ("Unable to set PRAGMA synchronous=OFF");
	  rl2_set_draping_message (priv_data, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* creating the output Geometry */
    if (!create_output_geom
	(sqlite, spatial_table, new_geom, geom_srid, gtype, update_m,
	 &target_dims))
      {
	  msg =
	      sqlite3_mprintf ("Unable to create the New Geomety \"%s\"",
			       new_geom);
	  rl2_set_draping_message (priv_data, msg);
	  sqlite3_free (msg);
	  goto error;
      }
/* copying and densifying the Geometries */
    if (!copy_densify
	(sqlite, spatial_table, old_geom, new_geom, densify_dist,
	 no_data_value, update_m, target_dims, gtype))
      {
	  msg = sqlite3_mprintf ("Unable to populate the New Geomety");
	  rl2_set_draping_message (priv_data, msg);
	  sqlite3_free (msg);
	  goto error;
      }
/* creating a Spatial Index supporting the output Geometry */
    if (!create_output_geom_rtree (sqlite, spatial_table, new_geom))
      {
	  msg =
	      sqlite3_mprintf
	      ("Unable to create a Spatial Index supporting New Geomety \"%s\"",
	       new_geom);
	  rl2_set_draping_message (priv_data, msg);
	  sqlite3_free (msg);
	  goto error;
      }

    if (coverage_list_table != NULL)
      {
	  /* processing a List of Raster Coverages */
	  int i;
	  char **results;
	  int rows;
	  int columns;
	  char *sql2;
	  int ret;
	  char *xlist = rl2_double_quoted_sql (coverage_list_table);
	  sql2 =
	      sqlite3_mprintf
	      ("SELECT db_prefix, coverage_name FROM main.\"%s\" ORDER BY progr",
	       xlist);
	  free (xlist);
	  ret =
	      sqlite3_get_table (sqlite, sql2, &results, &rows, &columns, NULL);
	  sqlite3_free (sql2);
	  if (ret != SQLITE_OK)
	    {
		msg =
		    sqlite3_mprintf ("Invalid List of Raster Coverages \"%s\"",
				     coverage_list_table);
		rl2_set_draping_message (priv_data, msg);
		sqlite3_free (msg);
		goto error;
	    }
	  if (rows < 1)
	      ;
	  else
	    {
		for (i = 1; i <= rows; i++)
		  {
		      db_prefix = results[(i * columns) + 0];
		      raster_coverage = results[(i * columns) + 1];
		      /* processing a single Raster Coverage */
		      if (!do_drape_geometries
			  (sqlite, db_prefix, raster_coverage, spatial_table,
			   new_geom, no_data_value, update_m, &msg))
			{
			    sqlite3_free_table (results);
			    rl2_set_draping_message (priv_data, msg);
			    sqlite3_free (msg);
			    goto error;
			}
		  }
	    }
	  sqlite3_free_table (results);
      }
    else
      {
	  /* processing a single Raster Coverage */
	  if (!do_drape_geometries
	      (sqlite, db_prefix, raster_coverage, spatial_table, new_geom,
	       no_data_value, update_m, &msg))
	    {
		rl2_set_draping_message (priv_data, msg);
		sqlite3_free (msg);
		goto error;
	    }
      }

    if (!do_simplify
	(sqlite, spatial_table, old_geom, new_geom, gtype, no_data_value,
	 z_simplify_dist, update_m, &msg))
      {
	  rl2_set_draping_message (priv_data, msg);
	  sqlite3_free (msg);
	  goto error;
      }

/* restoring previous PRAGMA settings */
    pragma = sqlite3_mprintf ("PRAGMA journal_mode=%s", mode);
    sqlite3_exec (sqlite, pragma, NULL, NULL, NULL);
    sqlite3_free (pragma);
    pragma = sqlite3_mprintf ("PRAGMA synchronous=%s", sync);
    sqlite3_exec (sqlite, pragma, NULL, NULL, NULL);
    sqlite3_free (pragma);
    return 1;

  error:
/* restoring previous PRAGMA settings */
    if (*mode != '\0')
      {
	  pragma = sqlite3_mprintf ("PRAGMA journal_mode=%s", mode);
	  sqlite3_exec (sqlite, pragma, NULL, NULL, NULL);
	  sqlite3_free (pragma);
      }
    if (*sync != '\0')
      {
	  pragma = sqlite3_mprintf ("PRAGMA synchronous=%s", sync);
	  sqlite3_exec (sqlite, pragma, NULL, NULL, NULL);
	  sqlite3_free (pragma);
      }
    return 0;
}
