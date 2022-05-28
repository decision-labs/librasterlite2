/* 
/ wmslite_common
/
/ a light-weight WMS server / GCI supporting RasterLite2 DataSources
/ main core implementation
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
#include <math.h>

#include "wmslite.h"
#include "rasterlite2_private.h"

extern void
clean_shutdown ()
{
/* performing a clean shutdown */
    fprintf (stderr, "wmslite server shutdown in progress\n");
    fflush (stderr);
    /*
       flush_log (glob.handle, glob.stmt_log, glob.log);
       if (glob.stmt_log != NULL)
       sqlite3_finalize (glob.stmt_log);
       if (glob.cached_capabilities != NULL)
       free (glob.cached_capabilities);
       if (glob.list != NULL)
       destroy_wms_list (glob.list);
       if (glob.log != NULL)
       destroy_server_log (glob.log);
       if (glob.pool != NULL)
       destroy_connections_pool (glob.pool);
       if (glob.handle != NULL)
       sqlite3_close (glob.handle);
       if (glob.cache != NULL)
       spatialite_cleanup_ex (glob.cache);
       if (glob.priv_data != NULL)
       rl2_cleanup_private (glob.priv_data);
     */
    spatialite_shutdown ();
    fprintf (stderr, "wmslite shutdown completed ... bye bye\n\n");
    fflush (stderr);
}

static char *
doubleQuotedSql (const char *value)
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

static char *
make_url_spaces (const char *str)
{
    int plus = 0;
    const char *in = str;
    char *outstr = malloc (strlen (str) + 1);
    char *out = outstr;
    while (*in != '\0')
      {
	  if (*in == '+')
	    {
		if (plus)
		  {
		      *out++ = '+';
		      plus = 0;
		  }
		else
		    plus = 1;
	    }
	  else
	    {
		if (plus)
		  {
		      *out++ = ' ';
		      plus = 0;
		  }
		*out++ = *in;
	    }
	  in++;
      }
    if (plus)
	*out++ = ' ';
    *out = '\0';
    return outstr;
}

extern char *
url_decode (CURL * curl, const char *encoded_url)
{
/* decoding a possibly encoded URL */
    char *clean;
    char *clean_sp;
    clean = curl_easy_unescape (curl, encoded_url, strlen (encoded_url), NULL);
    clean_sp = make_url_spaces (clean);
    curl_free (clean);
    return (clean_sp);
}

static sqlite3_stmt *
prepare_stmt_vector (sqlite3 * db_handle)
{
/* creatint a prepared statement for vector maps */
    sqlite3_stmt *stmt = NULL;
    const char *sql =
	"SELECT RL2_GetMapImageFromVector(?, ?, BuildMbr(?, ?, ?, ?, ?), ?, ?, ?, ?, ?, ?, ?, ?)";
    int ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  if (stmt != NULL)
	      sqlite3_finalize (stmt);
	  return NULL;
      }
    return stmt;
}

static sqlite3_stmt *
prepare_stmt_raster (sqlite3 * db_handle)
{
/* creatint a prepared statement for raster maps */
    sqlite3_stmt *stmt = NULL;
    const char *sql =
	"SELECT RL2_GetMapImageFromRaster(?, ?, BuildMbr(?, ?, ?, ?, ?), ?, ?, ?, ?, ?, ?, ?, ?)";
    int ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  if (stmt != NULL)
	      sqlite3_finalize (stmt);
	  return NULL;
      }
    return stmt;
}

static sqlite3_stmt *
prepare_stmt_config (sqlite3 * db_handle)
{
/* creatint a prepared statement for MapConfigurations */
    sqlite3_stmt *stmt = NULL;
    const char *sql =
	"SELECT RL2_GetImageFromMapConfiguration(?, BuildMbr(?, ?, ?, ?, ?), ?, ?, ?, ?, ?)";
    int ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  if (stmt != NULL)
	      sqlite3_finalize (stmt);
	  return NULL;
      }
    return stmt;
}

static sqlite3_stmt *
prepare_stmt_wms (sqlite3 * db_handle)
{
/* creatint a prepared statement for Cascading WMS */
    sqlite3_stmt *stmt = NULL;
    const char *sql =
	"SELECT RL2_GetImageFromWMS(?, ?, BuildMbr(?, ?, ?, ?, ?), ?, ?, ?, ?, ?, ?, ?)";
    int ret = sqlite3_prepare_v2 (db_handle, sql, strlen (sql), &stmt, NULL);
    if (ret != SQLITE_OK)
      {
	  if (stmt != NULL)
	      sqlite3_finalize (stmt);
	  return NULL;
      }
    return stmt;
}

extern void
connection_init (WmsLiteConnectionPtr conn, WmsLiteConfigPtr config)
{
/* creating a DB connection */
    int ret;
    sqlite3 *db_handle;
    void *cache;
    void *priv_data;
    char *sql;
    const char *path = config->MainDbPath;
    int max_threads = 1;

    if (path == NULL)
	return;

    ret =
	sqlite3_open_v2 (path, &db_handle,
			 SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX |
			 SQLITE_OPEN_SHAREDCACHE, NULL);
    if (ret != SQLITE_OK)
      {
	  fprintf (stderr, "WmsLite: cannot open '%s': %s\n", path,
		   sqlite3_errmsg (db_handle));
	  fflush (stderr);
	  sqlite3_close (db_handle);
	  return;
      }
    cache = spatialite_alloc_connection ();
    spatialite_init_ex (db_handle, cache, 0);
    priv_data = rl2_alloc_private ();
    rl2_init (db_handle, priv_data, 0);

    if (config->MultithreadEnabled)
      {
	  /* setting up MaxThreads */
	  max_threads = config->MaxThreads;
	  if (max_threads < 1)
	      max_threads = 1;
	  if (max_threads > 64)
	      max_threads = 64;
	  sql = sqlite3_mprintf ("SELECT RL2_SetMaxThreads(%d)", max_threads);
	  sqlite3_exec (db_handle, sql, NULL, NULL, NULL);
	  sqlite3_free (sql);
      }

    conn->handle = db_handle;
    conn->splite_privdata = cache;
    conn->rl2_privdata = priv_data;
    conn->stmt_vector = prepare_stmt_vector (db_handle);
    conn->stmt_raster = prepare_stmt_raster (db_handle);
    conn->stmt_config = prepare_stmt_config (db_handle);
    conn->stmt_wms = prepare_stmt_wms (db_handle);
    conn->curl = curl_easy_init ();
    conn->status = CONNECTION_AVAILABLE;
    return;
}

static void
close_connection (WmsLiteConnectionPtr conn)
{
/* closing a connection */
    if (conn == NULL)
	return;
    if (conn->stmt_vector != NULL)
	sqlite3_finalize (conn->stmt_vector);
    if (conn->stmt_raster != NULL)
	sqlite3_finalize (conn->stmt_raster);
    if (conn->stmt_config != NULL)
	sqlite3_finalize (conn->stmt_config);
    if (conn->stmt_wms != NULL)
	sqlite3_finalize (conn->stmt_wms);
    if (conn->handle != NULL)
	sqlite3_close (conn->handle);
    if (conn->splite_privdata != NULL)
	spatialite_cleanup_ex (conn->splite_privdata);
    if (conn->rl2_privdata != NULL)
	rl2_cleanup_private (conn->rl2_privdata);
    if (conn->curl != NULL)
	curl_easy_cleanup (conn->curl);
    conn->status = CONNECTION_INVALID;
}

extern void
destroy_connections_pool (WmsLiteConnectionsPoolPtr pool)
{
/* memory clean-up: destroying a connections pool */
    int i;
    WmsLiteConnectionPtr conn;

    if (pool == NULL)
	return;
    for (i = 0; i < pool->count; i++)
      {
	  /* closing all connections */
	  conn = &(pool->connections[i]);
	  close_connection (conn);
      }
    free (pool->connections);
    free (pool);
}

extern WmsLiteConnectionsPoolPtr
alloc_connections_pool (WmsLiteConfigPtr config, int max_connections)
{
/* creating and initializing the connections pool */
    int i;
    int count;
    WmsLiteConnectionPtr conn;
    WmsLiteConnectionsPoolPtr pool = malloc (sizeof (WmsLiteConnectionsPool));

    if (pool == NULL)
	return NULL;
/* a valid Connection should already exist */
    if (config->Connection.handle == NULL)
	return NULL;

    if (max_connections < 1)
	max_connections = 1;
    if (max_connections > 64)
	max_connections = 64;
    pool->connections = malloc (sizeof (WmsLiteConnection) * max_connections);
    if (pool->connections == NULL)
      {
	  free (pool);
	  return NULL;
      }
    pool->count = max_connections;
    for (i = 0; i < max_connections; i++)
      {
	  /* initializing empty connections */
	  conn = &(pool->connections[i]);
	  conn->splite_privdata = NULL;
	  conn->rl2_privdata = NULL;
	  conn->handle = NULL;
	  conn->stmt_vector = NULL;
	  conn->stmt_raster = NULL;
	  conn->stmt_config = NULL;
	  conn->stmt_wms = NULL;
	  conn->curl = NULL;
	  conn->status = CONNECTION_INVALID;
      }
    for (i = 0; i < max_connections; i++)
      {
	  /* creating the connections */
	  conn = &(pool->connections[i]);
	  if (i == 0)
	    {
		/* reusing the already established DB connection */
		conn->handle = config->Connection.handle;
		conn->splite_privdata = config->Connection.splite_privdata;
		conn->rl2_privdata = config->Connection.rl2_privdata;
		conn->stmt_vector = config->Connection.stmt_vector;
		conn->stmt_raster = config->Connection.stmt_raster;
		conn->stmt_config = config->Connection.stmt_config;
		conn->stmt_wms = config->Connection.stmt_wms;
		conn->curl = config->Connection.curl;
		conn->status = CONNECTION_AVAILABLE;
		/* releasing ownership on Config Connection */
		config->Connection.handle = NULL;
		config->Connection.stmt_vector = NULL;
		config->Connection.stmt_raster = NULL;
		config->Connection.stmt_config = NULL;
		config->Connection.stmt_wms = NULL;
		config->Connection.splite_privdata = NULL;
		config->Connection.rl2_privdata = NULL;
	    }
	  else
	      connection_init (conn, config);
      }
    count = 0;
    for (i = 0; i < max_connections; i++)
      {
	  /* final validity check */
	  conn = &(pool->connections[i]);
	  if (conn->status == CONNECTION_AVAILABLE)
	      count++;
      }
    if (count == 0)
      {
	  /* invalid pool ... sorry ... */
	  destroy_connections_pool (pool);
	  return NULL;
      }
    return pool;
}

extern WmsLiteArgumentPtr
alloc_wms_argument (char *name, char *value)
{
/* allocating a WMS argument */
    WmsLiteArgumentPtr arg = malloc (sizeof (WmsLiteArgument));
    arg->arg_name = name;
    arg->arg_value = value;
    arg->next = NULL;
    return arg;
}

extern void
destroy_wms_argument (WmsLiteArgumentPtr arg)
{
/* memory cleanup - destroying a WMS arg struct */
    if (arg == NULL)
	return;
    if (arg->arg_name != NULL)
	free (arg->arg_name);
    if (arg->arg_value != NULL)
	curl_free (arg->arg_value);
    free (arg);
}


extern int
add_wms_argument (WmsLiteHttpRequestPtr req, const char *token)
{
/* attempting to add an HTTP CGI argument */
    int len;
    WmsLiteArgumentPtr arg;
    char *name;
    char *value;
    char *decoded_value;
    const char *ptr;
    ptr = strstr (token, "=");
    if (ptr == NULL)
	return 0;
    len = strlen (ptr + 1);
    value = malloc (len + 1);
    strcpy (value, ptr + 1);
    decoded_value = url_decode (req->conn->curl, value);
    free (value);
    len = ptr - token;
    name = malloc (len + 1);
    memcpy (name, token, len);
    *(name + len) = '\0';
    arg = alloc_wms_argument (name, decoded_value);
    if (req->first_arg == NULL)
	req->first_arg = arg;
    if (req->last_arg != NULL)
	req->last_arg->next = arg;
    req->last_arg = arg;
    return 1;
}

static int
check_cascaded_wms_style (WmsLiteCascadedWmsPtr lyr, const char *styleName)
{
/* checking for a valid Layer Style (Cascaded WMS) */
    WmsLiteStylePtr style;
    style = lyr->StyleFirst;
    while (style != NULL)
      {
	  if (style->Name == NULL)
	    {
		/* skipping any unnamed style */
		style = style->Next;
		continue;
	    }
	  if (strcmp (style->Name, styleName) == 0)
	      return 1;
	  style = style->Next;
      }
    return 0;
}

static int
check_wms_raster_style (WmsLiteRasterPtr lyr, const char *styleName)
{
/* checking for a valid Layer Style (Raster) */
    WmsLiteStylePtr style;
    style = lyr->StyleFirst;
    while (style != NULL)
      {
	  if (style->Name == NULL)
	    {
		/* skipping any unnamed style */
		style = style->Next;
		continue;
	    }
	  if (strcmp (style->Name, styleName) == 0)
	      return 1;
	  style = style->Next;
      }
    return 0;
}

static int
check_wms_vector_style (WmsLiteVectorPtr lyr, const char *styleName)
{
/* checking for a valid Layer Style (Vector) */
    WmsLiteStylePtr style;
    style = lyr->StyleFirst;
    while (style != NULL)
      {
	  if (style->Name == NULL)
	    {
		/* skipping any unnamed style */
		style = style->Next;
		continue;
	    }
	  if (strcmp (style->Name, styleName) == 0)
	      return 1;
	  style = style->Next;
      }
    return 0;
}

static int
check_wms_layer (WmsLiteConfigPtr config, WmsLiteStyledLayerPtr lyr)
{
/* checking for a valid Layer (Style) */
    WmsLiteLayerPtr wmsLyr;
    WmsLiteAttachedPtr db;
    WmsLiteAttachedLayerPtr attLyr;

    if (lyr->LayerName == NULL)
	return WMS_NULL_LAYER;

    wmsLyr = config->MainFirst;
    while (wmsLyr != NULL)
      {
	  /* printing all Layers from MAIN */
	  if (!is_valid_wmslite_layer (wmsLyr))
	    {
		/* skipping any invalid Layers */
		wmsLyr = wmsLyr->Next;
		continue;
	    }
	  if (strcasecmp (wmsLyr->AliasName, lyr->LayerName) == 0)
	    {
		if (lyr->StyleName == NULL)
		    return WMS_LAYER_OK;	/* no Style is defined */
		/* checking for a valid Style */
		switch (wmsLyr->Type)
		  {
		  case WMS_LAYER_CASCADED_WMS:
		      if (check_cascaded_wms_style
			  (wmsLyr->CascadedWMS, lyr->StyleName))
			  return WMS_LAYER_OK;
		      break;
		  case WMS_LAYER_RASTER:
		      if (check_wms_raster_style
			  (wmsLyr->Raster, lyr->StyleName))
			  return WMS_LAYER_OK;
		      break;
		  case WMS_LAYER_VECTOR:
		      if (check_wms_vector_style
			  (wmsLyr->Vector, lyr->StyleName))
			  return WMS_LAYER_OK;
		      break;
		  case WMS_LAYER_MAP_CONFIG:	/* Map Configuration has no Styles */
		  default:
		      break;
		  };
		return WMS_BAD_STYLE;
	    }
	  wmsLyr = wmsLyr->Next;
      }
    db = config->DbFirst;
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

		if (!is_valid_wmslite_attached_layer (attLyr))
		  {
		      /* skipping any invalid or Child Layers */
		      attLyr = attLyr->Next;
		      continue;
		  }
		if (strcasecmp (attLyr->AliasName, lyr->LayerName) == 0)
		  {
		      if (lyr->StyleName == NULL)
			  return WMS_LAYER_OK;	/* no Style is defined */
		      /* checking for a valid Style */
		      switch (attLyr->Type)
			{
			case WMS_LAYER_CASCADED_WMS:
			    if (check_cascaded_wms_style
				(attLyr->CascadedWMS, lyr->StyleName))
				return WMS_LAYER_OK;
			    break;
			case WMS_LAYER_RASTER:
			    if (check_wms_raster_style
				(attLyr->Raster, lyr->StyleName))
				return WMS_LAYER_OK;
			    break;
			case WMS_LAYER_VECTOR:
			    if (check_wms_vector_style
				(attLyr->Vector, lyr->StyleName))
				return WMS_LAYER_OK;
			    break;
			default:
			    break;
			};
		      return WMS_BAD_STYLE;
		  }
		attLyr = attLyr->Next;
	    }
	  db = db->Next;
      }
    return WMS_BAD_STYLE;
}

static int
check_wms_query_layer (WmsLiteConfigPtr config, WmsLiteStyledLayerPtr lyr)
{
/* checking for a valid QueryLayer */
    WmsLiteLayerPtr wmsLyr;
    WmsLiteAttachedPtr db;
    WmsLiteAttachedLayerPtr attLyr;
    wmsLyr = config->MainFirst;
    while (wmsLyr != NULL)
      {
	  /* printing all Layers from MAIN */
	  if (!is_valid_wmslite_layer (wmsLyr))
	    {
		/* skipping any invalid Layers */
		wmsLyr = wmsLyr->Next;
		continue;
	    }
	  if (strcasecmp (wmsLyr->AliasName, lyr->LayerName) == 0)
	    {
		if (is_queryable_wmslite_layer (wmsLyr))
		    return WMS_LAYER_OK;
		return WMS_BAD_LAYER;
	    }
	  wmsLyr = wmsLyr->Next;
      }
    db = config->DbFirst;
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

		if (!is_valid_wmslite_attached_layer (attLyr))
		  {
		      /* skipping any invalid or Child Layers */
		      attLyr = attLyr->Next;
		      continue;
		  }
		if (strcasecmp (attLyr->AliasName, lyr->LayerName) == 0)
		  {
		      if (is_queryable_wmslite_attached_layer (attLyr))
			  return WMS_LAYER_OK;
		      return WMS_BAD_LAYER;
		  }
		attLyr = attLyr->Next;
	    }
	  db = db->Next;
      }
    return WMS_BAD_LAYER;
}

static int
check_raster_layer_style (sqlite3 * sqlite, const char *db_prefix,
			  const char *layer, const char *style, int *type)
{
/* checking for a valid Raster Coverage + Style */
    int ret;
    char *sql;
    int i;
    char **results;
    int rows;
    int columns;
    int count = 0;
    char *xprefix = doubleQuotedSql (db_prefix);
    int xtype = RL2_PIXEL_UNKNOWN;
    sql =
	sqlite3_mprintf
	("SELECT Count(*), c.pixel_type FROM \"%s\".SE_raster_styled_layers_view AS s "
	"LEFT JOIN \"%s\".raster_coverages AS c ON (s.coverage_name = c.coverage_name) "
	 "WHERE s.coverage_name = %Q AND s.name = %Q", xprefix, layer, style);
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
		  const char *pixel_type = results[(i * columns) + 1];
		  count += atoi(results[(i * columns) + 0]);
		  if (strcasecmp(pixel_type, "MONOCHROME") == 0)
			xtype = RL2_PIXEL_MONOCHROME;
		if (strcasecmp(pixel_type, "PALETTE") == 0)
			xtype = RL2_PIXEL_PALETTE;
		if (strcasecmp(pixel_type, "GRAYSCALE") == 0)
			xtype = RL2_PIXEL_GRAYSCALE;
		if (strcasecmp(pixel_type, "RGB") == 0)
			xtype = RL2_PIXEL_RGB;
		if (strcasecmp(pixel_type, "MULTIBAND") == 0)
			xtype = RL2_PIXEL_MULTIBAND;
		if (strcasecmp(pixel_type, "DATAGRID") == 0)
			xtype = RL2_PIXEL_DATAGRID;
	  }
      }
    sqlite3_free_table (results);
    if (count != 1)
		count = 0;
	else 
		*type = xtype;
    return count;
}

static int
check_vector_layer_style (sqlite3 * sqlite, const char *db_prefix,
			  const char *layer, const char *style, int *type)
{
/* checking for a valid Vector Coverage + Style */
    int ret;
    char *sql;
    int i;
    char **results;
    int rows;
    int columns;
    int count = 0;
    char *xprefix = doubleQuotedSql (db_prefix);
    sql =
	sqlite3_mprintf
	("SELECT Count(*) FROM \"%s\".SE_vector_styled_layers_view "
	 "WHERE coverage_name = %Q AND name = %Q", xprefix, layer, style);
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
	      count += atoi (results[(i * columns) + 0]);
      }
    sqlite3_free_table (results);
    if (count != 1)
		count = 0;
	*type = 6969;
    return count;
}

static int
check_layer_legend (sqlite3 * sqlite, WmsLiteConfigPtr config,
		    const char *legend_layer, const char *legend_style,
		    int *type, int *sub_type, const char **prefix)
{
/* checking for valid Layer and Style (GetLegendGraphic) */
    const char *db_prefix = NULL;
    const char *layer_name = NULL;
    int xsub_type;
    int layer_type = WMS_LAYER_UNKNOWN;
    WmsLiteLayerPtr wmsLyr;
    WmsLiteAttachedPtr db;
    WmsLiteAttachedLayerPtr attLyr;
    wmsLyr = config->MainFirst;
    *type = WMS_LAYER_UNKNOWN;
    *prefix = NULL;
    while (wmsLyr != NULL)
      {
	  /* scanning all Layers from MAIN */
	  if (!is_valid_wmslite_layer (wmsLyr))
	    {
		/* skipping any invalid Layers */
		wmsLyr = wmsLyr->Next;
		continue;
	    }
	  if (strcasecmp (wmsLyr->AliasName, legend_layer) == 0)
	    {
		layer_type = wmsLyr->Type;
		db_prefix = "MAIN";
		layer_name = wmsLyr->Name;
	    }
	  wmsLyr = wmsLyr->Next;
      }
    db = config->DbFirst;
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
		/* scanning all Attached Layers */

		if (!is_valid_wmslite_attached_layer (attLyr))
		  {
		      /* skipping any invalid or Child Layers */
		      attLyr = attLyr->Next;
		      continue;
		  }
		if (strcasecmp (attLyr->AliasName, legend_layer) == 0)
		  {
		      layer_type = attLyr->Type;
		      db_prefix = db->DbPrefix;
		      layer_name = attLyr->Name;
		  }
		attLyr = attLyr->Next;
	    }
	  db = db->Next;
      }
    if (layer_name == NULL)
	return WMS_BAD_LAYER;
    switch (layer_type)
      {
      case WMS_LAYER_RASTER:
	  if (!check_raster_layer_style
	      (sqlite, db_prefix, legend_layer, legend_style, &xsub_type))
	      return WMS_BAD_STYLE;
	  break;
      case WMS_LAYER_VECTOR:
	  if (!check_vector_layer_style
	      (sqlite, db_prefix, legend_layer, legend_style, &xsub_type))
	      return WMS_BAD_STYLE;
	  break;
      default:
	  return WMS_BAD_STYLE;
      };
    *type = layer_type;
    *sub_type = xsub_type;
    *prefix = db_prefix;
    return WMS_LAYER_OK;
}

static WmsLiteLayersListPtr
create_layers_list ()
{
/* creating an empty list of WMS Styled Layers */
    WmsLiteLayersListPtr list = malloc (sizeof (WmsLiteLayersList));
    list->first = NULL;
    list->last = NULL;
    return list;
}

static void
destroy_layers_list (WmsLiteLayersListPtr list)
{
/* memory cleanup - destroying a list of WMS Styled Layers */
    WmsLiteStyledLayerPtr lyr;
    WmsLiteStyledLayerPtr lyr_n;
    lyr = list->first;
    while (lyr != NULL)
      {
	  lyr_n = lyr->Next;
	  if (lyr->LayerName != NULL)
	      free (lyr->LayerName);
	  if (lyr->StyleName != NULL)
	      free (lyr->StyleName);
	  free (lyr);
	  lyr = lyr_n;
      }
    free (list);
}

static void
add_layer_to_list (WmsLiteLayersListPtr list, char *lyr_name)
{
/* adding a Styled Layer to the list */
    WmsLiteStyledLayerPtr lyr = malloc (sizeof (WmsLiteStyledLayer));
    lyr->LayerName = lyr_name;
    lyr->StyleName = NULL;
    lyr->Next = NULL;

    if (list->first == NULL)
	list->first = lyr;
    if (list->last != NULL)
	list->last->Next = lyr;
    list->last = lyr;
}

static void
add_style_to_list (WmsLiteLayersListPtr list, int pos, char *stl_name)
{
/* setting the style to a given layer */
    WmsLiteStyledLayerPtr lyr;
    int ind = 0;
    lyr = list->first;
    while (lyr != NULL)
      {
	  if (ind == pos)
	    {
		if (lyr->StyleName != NULL)
		    free (lyr->StyleName);
		lyr->StyleName = stl_name;
		return;
	    }
	  ind++;
	  lyr = lyr->Next;
      }
    free (stl_name);
}

static int
check_cascaded_wms_crs (WmsLiteCascadedWmsPtr lyr, const char *crsName)
{
/* checking for a valid Layer CRS (Cascaded WMS) */
    WmsLiteWmsCrsPtr crs;
    crs = lyr->CrsFirst;
    while (crs != NULL)
      {
	  if (crs->BBox == NULL)
	    {
		/* skipping any undefined BBox */
		crs = crs->Next;
		continue;
	    }
	  if (strcasecmp (crs->Srs, crsName) == 0)
	      return 1;
	  crs = crs->Next;
      }
    return 0;
}

static int
check_wms_raster_crs (WmsLiteRasterPtr lyr, const char *crsName)
{
/* checking for a valid Layer CRS (Raster) */
    char dummy[32];
    WmsLiteCrsPtr crs;
    crs = lyr->CrsFirst;
    while (crs != NULL)
      {
	  if (crs->BBox == NULL)
	    {
		/* skipping any undefined BBox */
		crs = crs->Next;
		continue;
	    }
	  sprintf (dummy, "EPSG:%d", crs->Srid);
	  if (strcasecmp (dummy, crsName) == 0)
	      return 1;
	  crs = crs->Next;
      }
    return 0;
}

static int
check_wms_vector_crs (WmsLiteVectorPtr lyr, const char *crsName)
{
/* checking for a valid Layer CRS (Vector) */
    char dummy[32];
    WmsLiteCrsPtr crs;
    crs = lyr->CrsFirst;
    while (crs != NULL)
      {
	  if (crs->BBox == NULL)
	    {
		/* skipping any undefined BBox */
		crs = crs->Next;
		continue;
	    }
	  sprintf (dummy, "EPSG:%d", crs->Srid);
	  if (strcasecmp (dummy, crsName) == 0)
	      return 1;
	  crs = crs->Next;
      }
    return 0;
}

static int
check_wms_map_config_crs (WmsLiteMapConfigMultiSRIDPtr srids,
			  const char *crsName)
{
/* checking for a valid Layer CRS (MapConfiguration) */
    char dummy[32];
    WmsLiteMapConfigSRIDPtr crs;
    crs = srids->First;
    while (crs != NULL)
      {
	  sprintf (dummy, "EPSG:%d", crs->Srid);
	  if (strcasecmp (dummy, crsName) == 0)
	      return 1;
	  crs = crs->Next;
      }
    return 0;
}

static int
check_wms_layer_crs (WmsLiteConfigPtr config, const char *layerName,
		     const char *crs)
{
/* checking for supported CRS */
    WmsLiteLayerPtr wmsLyr;
    WmsLiteAttachedPtr db;
    WmsLiteAttachedLayerPtr attLyr;
    wmsLyr = config->MainFirst;
    while (wmsLyr != NULL)
      {
	  /* printing all Layers from MAIN */
	  if (!is_valid_wmslite_layer (wmsLyr))
	    {
		/* skipping any invalid Layers */
		wmsLyr = wmsLyr->Next;
		continue;
	    }
	  if (strcasecmp (wmsLyr->AliasName, layerName) == 0)
	    {
		switch (wmsLyr->Type)
		  {
		  case WMS_LAYER_CASCADED_WMS:
		      if (check_cascaded_wms_crs (wmsLyr->CascadedWMS, crs))
			  return 1;
		      break;
		  case WMS_LAYER_RASTER:
		      if (check_wms_raster_crs (wmsLyr->Raster, crs))
			  return 1;
		      break;
		  case WMS_LAYER_VECTOR:
		      if (check_wms_vector_crs (wmsLyr->Vector, crs))
			  return 1;
		      break;
		  case WMS_LAYER_MAP_CONFIG:
		      if (check_wms_map_config_crs
			  (wmsLyr->MapConfigSrids, crs))
			  return 1;
		  default:
		      break;
		  };
		return 0;
	    }
	  wmsLyr = wmsLyr->Next;
      }
    db = config->DbFirst;
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

		if (!is_valid_wmslite_attached_layer (attLyr))
		  {
		      /* skipping any invalid or Child Layers */
		      attLyr = attLyr->Next;
		      continue;
		  }
		if (strcasecmp (attLyr->AliasName, layerName) == 0)
		  {
		      switch (attLyr->Type)
			{
			case WMS_LAYER_CASCADED_WMS:
			    if (check_cascaded_wms_crs
				(attLyr->CascadedWMS, crs))
				return 1;
			    break;
			case WMS_LAYER_RASTER:
			    if (check_wms_raster_crs (attLyr->Raster, crs))
				return 1;
			    break;
			case WMS_LAYER_VECTOR:
			    if (check_wms_vector_crs (attLyr->Vector, crs))
				return 1;
			    break;
			default:
			    break;
			};
		      return 0;
		  }
		attLyr = attLyr->Next;
	    }
	  db = db->Next;
      }
    return 0;
}

static int
check_wms_bbox (WmsLiteBBoxPtr bbox)
{
/* checking a BBOX for validity */
    if (bbox == NULL)
	return 0;
    if (bbox->MaxX <= bbox->MinX)
	return 0;
    if (bbox->MaxY <= bbox->MinY)
	return 0;
    return 1;
}

static int
check_wms_bbox_consistency (WmsLiteHttpRequestPtr req)
{
/* checking if a BBOX is consistent with WIDTH and HEIGHT */
    double diff;
    double percent;
    double horz_res =
	(req->bbox->MaxX - req->bbox->MinX) / (double) (req->width);
    double vert_res =
	(req->bbox->MaxY - req->bbox->MinY) / (double) (req->height);
    if (horz_res == vert_res)
	return 1;
    diff = fabs (horz_res - vert_res);
    percent = horz_res / 1000.0;
    if (diff <= percent)
	return 1;
    return 0;
}

static int
check_wms_transparent (WmsLiteHttpRequestPtr req)
{
/* checking TRANSPARENT for validity */
    if (strcasecmp (req->param_transparent, "TRUE") == 0)
      {
	  req->transparent = 1;
	  return 1;
      }
    if (strcasecmp (req->param_transparent, "FALSE") == 0)
      {
	  req->transparent = 0;
	  return 1;
      }
    return 0;
}

static int
check_wms_reaspect (WmsLiteHttpRequestPtr req)
{
/* checking REASPECT for validity */
    if (strcasecmp (req->param_reaspect, "TRUE") == 0)
      {
	  req->reaspect = 1;
	  return 1;
      }
    if (strcasecmp (req->param_reaspect, "FALSE") == 0)
      {
	  req->reaspect = 0;
	  return 1;
      }
    return 0;
}

static int
parse_hex_rgb (char hi, char lo)
{
/* parsing an hexadecimal RGB color */
    int err = 0;
    int value;
    switch (hi)
      {
      case '0':
	  value = 0;
	  break;
      case '1':
	  value = 16;
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
	  err = 1;
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
	  err = 1;
	  break;
      };
    if (err)
	return -1;
    return value;
}

static int
check_wms_bgcolor (WmsLiteHttpRequestPtr req)
{
/* checking BGCOLOR for validity */
    char hi;
    char lo;
    int red;
    int green;
    int blue;
    if (strlen (req->param_bgcolor) != 8)
	return 0;
    if (req->param_bgcolor[0] != '0')
	return 0;
    if (req->param_bgcolor[1] == 'x' || req->param_bgcolor[1] == 'X')
	;
    else
	return 0;
    hi = req->param_bgcolor[2];
    lo = req->param_bgcolor[3];
    red = parse_hex_rgb (hi, lo);
    if (red < 0)
	return 0;
    hi = req->param_bgcolor[4];
    lo = req->param_bgcolor[5];
    green = parse_hex_rgb (hi, lo);
    if (green < 0)
	return 0;
    hi = req->param_bgcolor[6];
    lo = req->param_bgcolor[7];
    blue = parse_hex_rgb (hi, lo);
    if (blue < 0)
	return 0;

/* setting the background color components */
    req->bg_red = red;
    req->bg_green = green;
    req->bg_blue = blue;
    return 1;
}

extern void
parse_request_args (WmsLiteHttpRequestPtr req, const char *query_string,
		    int len)
{
/* parsing the request arguments */
    char token[2000];
    char *out;
    const char *p = query_string;
    const char *end = p + len;
    out = token;
    while (p < end)
      {
	  if (*p == '&')
	    {
		/* a key-value pair (argument) ends here */
		*out = '\0';
		if (!add_wms_argument (req, token))
		    goto error;
		out = token;
		p++;
		continue;
	    }
	  *out++ = *p++;
      }
    if (out > token)
      {
	  /* processing the last arg */
	  *out = '\0';
	  if (!add_wms_argument (req, token))
	      goto error;
      }
  error:
    return;
}

static void
parse_version (WmsLiteHttpRequestPtr req, const char *version_string)
{
/* attempting to parse the version string */
    int pts = 0;
    const char *pi;
    char *buf;
    char *po;
    int which = 0;

    if (version_string == NULL)
	return;
    pi = version_string;
    while (*pi != '\0')
      {
	  if (*pi == '.')
	      pts++;
	  pi++;
      }
    if (pts != 2)
	return;
    buf = malloc (strlen (version_string) + 1);
    strcpy (buf, version_string);
    pi = buf;
    po = buf;
    while (*po != '\0')
      {
	  if (*po == '.')
	    {
		*po = '\0';
		if (which == 0)
		  {
		      req->version_1 = atoi (pi);
		      which = 1;
		      pi = po + 1;
		  }
		else
		  {
		      req->version_2 = atoi (pi);
		      pi = po + 1;
		  }
	    }
	  if (*po == '\0')
	      req->version_3 = atoi (pi);
	  po++;
      }
    free (buf);
}

static void
set_ok_version (WmsLiteHttpRequestPtr req)
{
/* setting the protocol versione negotiated by server and client */
    if (req->version_1 > 1)
      {
	  req->ok_version = WMS_VERSION_130;
	  return;
      }
    if (req->version_2 >= 3)
      {
	  req->ok_version = WMS_VERSION_130;
	  return;
      }
    if (req->version_2 > 1)
      {
	  req->ok_version = WMS_VERSION_111;
	  return;
      }
    if (req->version_2 == 1)
      {
	  if (req->version_3 >= 1)
	      req->ok_version = WMS_VERSION_111;
	  else
	      req->ok_version = WMS_VERSION_110;
	  return;
      }
    req->ok_version = WMS_VERSION_100;
}

static void
throw_wmsxml_exception (WmsLiteHttpRequestPtr req, const char *msg)
{
/* throwing an exception (of the WMS_XML type) */
    req->http_response =
	sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
			 "<WMTException version=\"1.0.0\">"
			 "\tWMS server error. Incomplete WMS request: %s</WMTException>\r\n",
			 msg);
    req->freeor = sqlite3_free;
    req->http_content_length = strlen (req->http_response);
    req->http_mime_type = MIME_XML;
}

static void
throw_xml_exception (WmsLiteHttpRequestPtr req, const char *msg)
{
/* throwing an exception (of the XML type) */
    const char *version;
    const char *xversion;
    switch (req->ok_version)
      {
      case WMS_VERSION_100:
	  version = "1.0.0";
	  xversion = "1_0_0";
	  break;
      case WMS_VERSION_110:
	  version = "1.1.0";
	  xversion = "1_1_0";
	  break;
      case WMS_VERSION_111:
	  version = "1.1.1";
	  xversion = "1_1_1";
	  break;
      case WMS_VERSION_130:;
      default:
	  version = "1.3.0";
	  xversion = "1_3_0";
	  break;
      };
    if (req->ok_version != WMS_VERSION_130)
	req->http_response =
	    sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
			     "<!DOCTYPE ServiceExceptionReport SYSTEM \"http://schemas.opengeospatial.net/wms/%s.dtd\">\r\n"
			     "<ServiceExceptionReport version=\"%s\">\r\n"
			     "\t<ServiceException>WmsLite: WMS server error. %s</ServiceException>\r\n"
			     "</ServiceExceptionReport>\r\n", xversion, version,
			     msg);
    else
	req->http_response =
	    sqlite3_mprintf ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
			     "<ServiceExceptionReport version=\"%s\" mlns=\"http://www.opengis.net/ogc\" "
			     "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
			     "xsi:schemaLocation=\"http://www.opengis.net/ogc "
			     "http://schemas.opengis.net/mws/%s/exceptions_%s.xsd\">\r\n"
			     "\t<ServiceException>WmsLite: WMS server error. %s</ServiceException>\r\n"
			     "</ServiceExceptionReport>\r\n", version, version,
			     xversion, msg);
    req->freeor = sqlite3_free;
    req->http_content_length = strlen (req->http_response);
    req->http_mime_type = MIME_XML;
}

static void
throw_image_exception (WmsLiteHttpRequestPtr req, const char *msg, int blank)
{
/* throwing an exception (of the IMAGE type) */
    rl2RasterPtr rst = NULL;
    unsigned int width = 640;
    unsigned int height = 480;
    unsigned char *rgb = NULL;
    unsigned char *alpha = NULL;
    rl2GraphicsContextPtr ctx = NULL;
    rl2GraphicsFontPtr font = NULL;
    double pre_x;
    double pre_y;
    double text_w;
    double text_h;
    double post_x;
    double post_y;
    unsigned int row;
    unsigned int col;
    unsigned char *p_alpha;
    int half_transparent;
    unsigned char *png;
    int png_size;
    unsigned char bg_alpha = 255;
    FILE *out;

    ctx = rl2_graph_create_context (req->conn->rl2_privdata, width, height);
    if (ctx == NULL)
      {
	  fprintf (stderr, "Unable to create a graphics backend\n");
	  fflush (stderr);
	  goto stop;
      }
    if (req->transparent)
	bg_alpha = 0;
    rl2_prime_background (ctx, req->bg_red, req->bg_green, req->bg_blue,
			  bg_alpha);
    if (!blank)
      {
	  font =
	      rl2_graph_create_toy_font (NULL, 12, RL2_FONTSTYLE_NORMAL,
					 RL2_FONTWEIGHT_NORMAL);
	  if (font == NULL)
	    {
		fprintf (stderr, "Unable to create a Font\n");
		fflush (stderr);
		goto stop;
	    }
	  if (!rl2_graph_font_set_color (font, 0, 0, 0, 255))
	    {
		fprintf (stderr, "Unable to set the font color\n");
		fflush (stderr);
		goto stop;
	    }
	  if (!rl2_graph_set_font (ctx, font))
	    {
		fprintf (stderr, "Unable to set up a font\n");
		fflush (stderr);
		goto stop;
	    }
	  rl2_graph_get_text_extent (ctx, msg, &pre_x, &pre_y, &text_w, &text_h,
				     &post_x, &post_y);
	  rl2_graph_draw_text (ctx, msg, (width / 2.0) - (text_w / 2.0),
			       (height / 2.0) + (text_h / 2.0), 0.0, 0.0, 0.0);
      }

    rgb = rl2_graph_get_context_rgb_array (ctx);
    if (rgb == NULL)
      {
	  fprintf (stderr, "invalid RGB buffer from Graphics Context\n");
	  fflush (stderr);
	  goto stop;
      }
    alpha = rl2_graph_get_context_alpha_array (ctx, &half_transparent);
    if (alpha == NULL)
      {
	  fprintf (stderr, "invalid Alpha buffer from Graphics Context\n");
	  fflush (stderr);
	  goto stop;
      }
    rl2_graph_destroy_context (ctx);
    ctx = NULL;

    p_alpha = alpha;
    for (row = 0; row < height; row++)
      {
	  for (col = 0; col < width; col++)
	    {
		if (*p_alpha > 128)
		    *p_alpha++ = 1;
		else
		    *p_alpha++ = 0;
	    }
      }
    rst =
	rl2_create_raster (width, height, RL2_SAMPLE_UINT8, RL2_PIXEL_RGB, 3,
			   rgb, width * height * 3, NULL, alpha, width * height,
			   NULL);
    if (rst == NULL)
      {
	  fprintf (stderr, "Unable to create the output raster+mask\n");
	  fflush (stderr);
	  goto stop;
      }
    rgb = NULL;
    alpha = NULL;
    rl2_raster_to_png (rst, &png, &png_size);

    out = fopen ("./test_exception.png", "wb");
    if (out == NULL)
      {
	  fprintf (stderr, "Unable to create \"test_exception.png\"\n");
	  fflush (stderr);
      }
    else
      {
	  fwrite (png, 1, png_size, out);
	  fclose (out);
      }

    req->http_response = png;
    req->freeor = free;
    req->http_content_length = png_size;
    req->http_mime_type = MIME_PNG;

  stop:
    if (rgb != NULL)
	free (rgb);
    if (alpha != NULL)
	free (alpha);
    if (rst != NULL)
	rl2_destroy_raster (rst);
    if (ctx != NULL)
	rl2_graph_destroy_context (ctx);
    if (font != NULL)
	rl2_graph_destroy_font (font);
}

static void
throw_exception (WmsLiteHttpRequestPtr req, const char *msg)
{
/* throwing an exception */
    int exceptions;
    if (req->exceptions == WMS_EXCEPTIONS_DEFAULT)
      {
	  if (req->ok_version == WMS_VERSION_100)
	      exceptions = WMS_EXCEPTIONS_IMAGE;
	  else
	      exceptions = WMS_EXCEPTIONS_XML;
      }
    else
	exceptions = req->exceptions;
    if (req->http_response != NULL)
      {
	  /* cleaning the http_response */
	  if (req->freeor != NULL)
	      req->freeor (req->http_response);
	  req->http_response = NULL;
      }
    if (exceptions == WMS_EXCEPTIONS_BLANK)
	throw_image_exception (req, msg, 1);
    else if (exceptions == WMS_EXCEPTIONS_IMAGE)
	throw_image_exception (req, msg, 0);
    else if (exceptions == WMS_EXCEPTIONS_WMSXML)
	throw_wmsxml_exception (req, msg);
    else
	throw_xml_exception (req, msg);
}

extern void
do_get_capabilities (WmsLiteHttpRequestPtr req)
{
/* preparing the GetCapabilities response */
    if (req->http_response != NULL)
      {
	  /* cleaning the http_response */
	  if (req->freeor != NULL)
	      req->freeor (req->http_response);
	  req->http_response = NULL;
      }
    switch (req->ok_version)
      {
      case WMS_VERSION_100:
	  if (req->config->Capabilities_100 == NULL)
	    {
		/* first invocation: creating the XML response for this version */
		req->config->Capabilities_100 =
		    wmslite_create_getcapabilities (req->config,
						    WMS_VERSION_100);
		if (req->config->Capabilities_100 != NULL)
		    req->config->Capabilities_100_Length =
			strlen (req->config->Capabilities_100);
		else
		    req->config->Capabilities_100_Length = 0;
	    }
	  if (req->config->Capabilities_100 != NULL)
	    {
		/* reusing the cached XML for this version */
		req->http_response = req->config->Capabilities_100;
		req->http_content_length = req->config->Capabilities_100_Length;
	    }
	  break;
      case WMS_VERSION_110:
	  if (req->config->Capabilities_110 == NULL)
	    {
		/* first invocation: creating the XML response for this version */
		req->config->Capabilities_110 =
		    wmslite_create_getcapabilities (req->config,
						    WMS_VERSION_110);
		if (req->config->Capabilities_110 != NULL)
		    req->config->Capabilities_110_Length =
			strlen (req->config->Capabilities_110);
		else
		    req->config->Capabilities_110_Length = 0;
	    }
	  if (req->config->Capabilities_110 != NULL)
	    {
		/* reusing the cached XML for this version */
		req->http_response = req->config->Capabilities_110;
		req->http_content_length = req->config->Capabilities_110_Length;
	    }
	  break;
      case WMS_VERSION_111:
	  if (req->config->Capabilities_111 == NULL)
	    {
		/* first invocation: creating the XML response for this version */
		req->config->Capabilities_111 =
		    wmslite_create_getcapabilities (req->config,
						    WMS_VERSION_111);
		if (req->config->Capabilities_111 != NULL)
		    req->config->Capabilities_111_Length =
			strlen (req->config->Capabilities_111);
		else
		    req->config->Capabilities_111_Length = 0;
	    }
	  if (req->config->Capabilities_111 != NULL)
	    {
		/* reusing the cached XML for this version */
		req->http_response = req->config->Capabilities_111;
		req->http_content_length = req->config->Capabilities_111_Length;
	    }
	  break;
      case WMS_VERSION_130:
      default:
	  if (req->config->Capabilities_130 == NULL)
	    {
		/* first invocation: creating the XML response for this version */
		req->config->Capabilities_130 =
		    wmslite_create_getcapabilities (req->config,
						    WMS_VERSION_130);
		if (req->config->Capabilities_130 != NULL)
		    req->config->Capabilities_130_Length =
			strlen (req->config->Capabilities_130);
		else
		    req->config->Capabilities_130_Length = 0;
	    }
	  if (req->config->Capabilities_130 != NULL)
	    {
		/* reusing the cached XML for this version */
		req->http_response = req->config->Capabilities_130;
		req->http_content_length = req->config->Capabilities_130_Length;
	    }
	  break;
      };
    req->freeor = NULL;
    req->http_mime_type = MIME_XML;
}

static void
do_find_layer (WmsLiteConfigPtr config, const char *layer_name, int *layer_type,
	       const char **db_prefix, const char **coverage_name)
{
/* attempting to find some Layer */
    WmsLiteLayerPtr lyr;
    WmsLiteAttachedPtr db;
    WmsLiteAttachedLayerPtr attLyr;
    int found = 0;

    *layer_type = WMS_NOT_FOUND;
    *db_prefix = NULL;
    *coverage_name = NULL;

    lyr = config->MainFirst;
    while (lyr != NULL)
      {
	  /* checking all Layers from MAIN */
	  if (!is_valid_wmslite_layer (lyr))
	    {
		/* skipping any invalid Layer */
		lyr = lyr->Next;
		continue;
	    }
	  if (strcasecmp (layer_name, lyr->AliasName) == 0)
	    {
		switch (lyr->Type)
		  {
		  case WMS_LAYER_CASCADED_WMS:
		      found = 1;
		      *coverage_name = lyr->Name;
		      *layer_type = WMS_MAIN_CASCADED;
		      break;
		  case WMS_LAYER_RASTER:
		      found = 1;
		      *coverage_name = lyr->Name;
		      *layer_type = WMS_MAIN_RASTER;
		      break;
		  case WMS_LAYER_VECTOR:
		      found = 1;
		      *coverage_name = lyr->Name;
		      *layer_type = WMS_MAIN_VECTOR;
		      break;
		  case WMS_LAYER_MAP_CONFIG:
		      found = 1;
		      *coverage_name = lyr->Name;
		      *layer_type = WMS_MAIN_CONFIG;
		      break;
		  };
	    }
	  if (found)
	      break;
	  lyr = lyr->Next;
      }

    if (!found)
      {
	  /* checking all Layers from ATTACHED DBs */
	  db = config->DbFirst;
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
		      if (!is_valid_wmslite_attached_layer (attLyr))
			{
			    /* skipping any invalid Layer */
			    attLyr = attLyr->Next;
			    continue;
			}
		      if (strcasecmp (layer_name, attLyr->AliasName) == 0)
			{
			    switch (attLyr->Type)
			      {
			      case WMS_LAYER_CASCADED_WMS:
				  found = 1;
				  *db_prefix = db->DbPrefix;
				  *coverage_name = attLyr->Name;
				  *layer_type = WMS_ATTACH_CASCADED;
				  break;
			      case WMS_LAYER_RASTER:
				  found = 1;
				  *db_prefix = db->DbPrefix;
				  *coverage_name = attLyr->Name;
				  *layer_type = WMS_ATTACH_RASTER;
				  break;
			      case WMS_LAYER_VECTOR:
				  found = 1;
				  *db_prefix = db->DbPrefix;
				  *coverage_name = attLyr->Name;
				  *layer_type = WMS_ATTACH_VECTOR;
				  break;
			      };
			}
		      if (found)
			  break;
		      attLyr = attLyr->Next;
		  }
		db = db->Next;
	    }
      }
}

static void
do_bind_raster_vector_values (WmsLiteHttpRequestPtr req, sqlite3_stmt * stmt,
			      const char *db_prefix, const char *coverage_name,
			      WmsLiteStyledLayerPtr lyr, int layer_count)
{
/* binding values for RL2_GetMapImageFromRaster or RL2_GetMapImageFromVector */
    const char *format;
    char bg_color[16];
    if (stmt != NULL)
      {
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (db_prefix == NULL)
	      sqlite3_bind_null (stmt, 1);
	  else
	      sqlite3_bind_text (stmt, 1, db_prefix, strlen (db_prefix),
				 SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_double (stmt, 3, req->bbox->MinX);
	  sqlite3_bind_double (stmt, 4, req->bbox->MinY);
	  sqlite3_bind_double (stmt, 5, req->bbox->MaxX);
	  sqlite3_bind_double (stmt, 6, req->bbox->MaxY);
	  sqlite3_bind_int (stmt, 7, req->srid);
	  sqlite3_bind_int (stmt, 8, req->width);
	  sqlite3_bind_int (stmt, 9, req->height);
	  if (lyr->StyleName == NULL)
	      sqlite3_bind_text (stmt, 10, "default", -1, SQLITE_TRANSIENT);
	  else
	      sqlite3_bind_text (stmt, 10, lyr->StyleName,
				 strlen (lyr->StyleName), SQLITE_STATIC);
	  if (layer_count > 1)
	      format = "image/vnd.rl2rgba";	/* multiple layers: RGBA raw buffer */
	  else
	    {
		switch (req->format)
		  {
		  case MIME_JPEG:
		      format = "image/jpeg";
		      break;
		  case MIME_TIFF:
		      format = "image/tiff";
		      break;
		  case MIME_PDF:
		      format = "application/x-pdf";
		      break;
		  case MIME_PNG:
		  default:
		      format = "image/png";
		      break;
		  };
	    }
	  sqlite3_bind_text (stmt, 11, format, strlen (format),
			     SQLITE_TRANSIENT);
	  sprintf (bg_color, "#%02x%02x%02x", req->bg_red, req->bg_green,
		   req->bg_blue);
	  sqlite3_bind_text (stmt, 12, bg_color, strlen (bg_color),
			     SQLITE_TRANSIENT);
	  if (layer_count > 1)
	      sqlite3_bind_int (stmt, 13, 1);	/* always transparent */
	  sqlite3_bind_int (stmt, 13, req->transparent);
	  if (req->format == MIME_JPEG)
	      sqlite3_bind_int (stmt, 14, 80);
	  else
	      sqlite3_bind_int (stmt, 14, 100);
	  sqlite3_bind_int (stmt, 15, req->reaspect);
      }
}

static void
do_bind_cascaded_wms_values (WmsLiteHttpRequestPtr req, sqlite3_stmt * stmt,
			     const char *db_prefix, const char *coverage_name,
			     WmsLiteStyledLayerPtr lyr)
{
/* binding values for RL2_GetMapImageFromWMS */
    const char *format;
    char bg_color[16];
    char wms_version[64];

    sprintf (wms_version, "%d.%d.%d", req->version_1, req->version_2,
	     req->version_3);
    if (stmt != NULL)
      {
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  if (db_prefix == NULL)
	      sqlite3_bind_null (stmt, 1);
	  else
	      sqlite3_bind_text (stmt, 1, db_prefix, strlen (db_prefix),
				 SQLITE_STATIC);
	  sqlite3_bind_text (stmt, 2, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_double (stmt, 3, req->bbox->MinX);
	  sqlite3_bind_double (stmt, 4, req->bbox->MinY);
	  sqlite3_bind_double (stmt, 5, req->bbox->MaxX);
	  sqlite3_bind_double (stmt, 6, req->bbox->MaxY);
	  sqlite3_bind_int (stmt, 7, req->srid);
	  sqlite3_bind_int (stmt, 8, req->width);
	  sqlite3_bind_int (stmt, 9, req->height);
	  sqlite3_bind_text (stmt, 10, wms_version, strlen (wms_version),
			     SQLITE_STATIC);
	  if (lyr->StyleName == NULL)
	      sqlite3_bind_text (stmt, 11, "default", -1, SQLITE_TRANSIENT);
	  else
	      sqlite3_bind_text (stmt, 11, lyr->StyleName,
				 strlen (lyr->StyleName), SQLITE_STATIC);
	  switch (req->format)
	    {
	    case MIME_JPEG:
		format = "image/jpeg";
		break;
	    case MIME_TIFF:
		format = "image/tiff";
		break;
	    case MIME_PDF:
		format = "application/x-pdf";
		break;
	    case MIME_PNG:
	    default:
		format = "image/png";
		break;
	    };
	  sqlite3_bind_text (stmt, 12, format, strlen (format),
			     SQLITE_TRANSIENT);
	  sprintf (bg_color, "#%02x%02x%02x", req->bg_red, req->bg_green,
		   req->bg_blue);
	  sqlite3_bind_text (stmt, 13, bg_color, strlen (bg_color),
			     SQLITE_TRANSIENT);
	  sqlite3_bind_int (stmt, 14, req->transparent);
      }
}

static void
do_bind_map_config_values (WmsLiteHttpRequestPtr req, sqlite3_stmt * stmt,
			   const char *coverage_name)
{
/* binding values for RL2_GetImageFromMapConfiguration */
    const char *format;

    if (stmt != NULL)
      {
	  sqlite3_reset (stmt);
	  sqlite3_clear_bindings (stmt);
	  sqlite3_bind_text (stmt, 1, coverage_name, strlen (coverage_name),
			     SQLITE_STATIC);
	  sqlite3_bind_double (stmt, 2, req->bbox->MinX);
	  sqlite3_bind_double (stmt, 3, req->bbox->MinY);
	  sqlite3_bind_double (stmt, 4, req->bbox->MaxX);
	  sqlite3_bind_double (stmt, 5, req->bbox->MaxY);
	  sqlite3_bind_int (stmt, 6, req->srid);
	  sqlite3_bind_int (stmt, 7, req->width);
	  sqlite3_bind_int (stmt, 8, req->height);
	  switch (req->format)
	    {
	    case MIME_JPEG:
		format = "image/jpeg";
		break;
	    case MIME_TIFF:
		format = "image/tiff";
		break;
	    case MIME_PDF:
		format = "application/x-pdf";
		break;
	    case MIME_PNG:
	    default:
		format = "image/png";
		break;
	    };
	  sqlite3_bind_text (stmt, 9, format, strlen (format),
			     SQLITE_TRANSIENT);
	  if (req->format == MIME_JPEG)
	      sqlite3_bind_int (stmt, 10, 80);
	  else
	      sqlite3_bind_int (stmt, 10, 100);
	  sqlite3_bind_int (stmt, 11, req->reaspect);
      }
}

static void
multiple_layer_request (WmsLiteHttpRequestPtr req, const unsigned char *payload,
			int payload_size, rl2GraphicsContextPtr * ctx_out,
			unsigned char **rgba_base)
{
/* multiple layer request */
    unsigned char *rgba = malloc (payload_size);
    memcpy (rgba, payload, payload_size);
    if (*ctx_out == NULL)
      {
	  /* first layer */
	  *ctx_out =
	      rl2_graph_create_context_rgba
	      (req->conn->rl2_privdata, req->width, req->height, rgba);
	  *rgba_base = rgba;
      }
    else
      {
	  /* merging layers */
	  rl2GraphicsContextPtr ctx_in =
	      rl2_graph_create_context_rgba (req->conn->rl2_privdata,
					     req->width, req->height,
					     rgba);
	  rl2_graph_merge (*ctx_out, ctx_in);
	  rl2_graph_destroy_context (ctx_in);
	  if (rgba != NULL)
	      free (rgba);
      }
}

static void
do_get_map (WmsLiteHttpRequestPtr req)
{
/* preparing the GetMap response */
    WmsLiteStyledLayerPtr lyr;
    int layer_type;
    const char *db_prefix;
    const char *coverage_name;
    sqlite3_stmt *stmt = NULL;
    int layer_count = 0;
    int valid = 0;
    rl2GraphicsContextPtr ctx_out = NULL;
    unsigned char *rgba_base = NULL;

    if (req->http_response != NULL)
      {
	  /* cleaning the http_response */
	  if (req->freeor != NULL)
	      req->freeor (req->http_response);
	  req->http_response = NULL;
      }
    lyr = req->layers_list->first;
    while (lyr != NULL)
      {
	  /* counting how many Layers are in this GetMap request */
	  layer_count++;
	  lyr = lyr->Next;
      }

    lyr = req->layers_list->first;
    while (lyr != NULL)
      {
	  /* attempting to service all required Layers */
	  do_find_layer (req->config, lyr->LayerName, &layer_type, &db_prefix,
			 &coverage_name);
	  switch (layer_type)
	    {
	    case WMS_MAIN_RASTER:
		stmt = req->conn->stmt_raster;
		do_bind_raster_vector_values (req, stmt, NULL, coverage_name,
					      lyr, layer_count);
		break;
	    case WMS_MAIN_VECTOR:
		stmt = req->conn->stmt_vector;
		do_bind_raster_vector_values (req, stmt, NULL, coverage_name,
					      lyr, layer_count);
		break;
	    case WMS_MAIN_CASCADED:
		stmt = req->conn->stmt_wms;
		do_bind_cascaded_wms_values (req, stmt, NULL, coverage_name,
					     lyr);
		break;
	    case WMS_MAIN_CONFIG:
		stmt = req->conn->stmt_config;
		do_bind_map_config_values (req, stmt, coverage_name);
		break;
	    case WMS_ATTACH_RASTER:
		stmt = req->conn->stmt_raster;
		do_bind_raster_vector_values (req, stmt, db_prefix,
					      coverage_name, lyr, layer_count);
		break;
	    case WMS_ATTACH_VECTOR:
		stmt = req->conn->stmt_vector;
		do_bind_raster_vector_values (req, stmt, db_prefix,
					      coverage_name, lyr, layer_count);
		break;
	    case WMS_ATTACH_CASCADED:
		stmt = req->conn->stmt_wms;
		do_bind_cascaded_wms_values (req, stmt, db_prefix,
					     coverage_name, lyr);
		break;
	    };

	  if (stmt != NULL)
	    {
		/* performing the SQL query returning the MapImage */
		int ret;
		const unsigned char *payload = NULL;
		int payload_size = 0;
		while (1)
		  {
		      ret = sqlite3_step (stmt);
		      if (ret == SQLITE_DONE)
			  break;
		      if (ret == SQLITE_ROW)
			{
			    if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
			      {
				  payload =
				      (unsigned char *)
				      sqlite3_column_blob (stmt, 0);
				  payload_size = sqlite3_column_bytes (stmt, 0);
				  if (layer_count > 1)
				    {
					/* multiple layer request */
					multiple_layer_request (req, payload,
								payload_size,
								&ctx_out,
								&rgba_base);
				    }
				  else
				    {
					/* single layer request: directly saving the MapImage */
					if (req->http_response != NULL)
					    free (req->http_response);
					req->http_response =
					    malloc (payload_size);
					memcpy (req->http_response, payload,
						payload_size);
					req->http_content_length = payload_size;
				    }
				  valid++;
			      }
			}
		  }
	    }
	  else
	    {
		/* throwing an exception */
		throw_exception (req,
				 "WmsLite internal error: GetMap unexpected NULL <stmt>");
		req->http_status = 200;
		req->freeor = NULL;
	    }
	  lyr = lyr->Next;
      }

    if (layer_count > 1)
      {
	  /* multiple layer request - creating the final MapImage */
	  if (valid != layer_count)
	    {
		/* throwing an exception */
		throw_exception (req,
				 "WmsLite internal error: GetMap unexpected NULL image");
		req->http_status = 200;
		req->freeor = NULL;
	    }
	  else
	    {
		/* valid MapImage */
		unsigned char *rgba;
		unsigned char *rgb;
		unsigned char *alpha;
		unsigned char *image;
		int image_size;
		int half_transparent;
		int ret;
		int error = 0;
		if (req->format == MIME_PNG)
		  {
		      rgb = rl2_graph_get_context_rgb_array (ctx_out);
		      alpha =
			  rl2_graph_get_context_alpha_array (ctx_out,
							     &half_transparent);
		      ret =
			  rl2_rgb_real_alpha_to_png (req->width, req->height,
						     rgb, alpha, &image,
						     &image_size);
		      free (rgb);
		      free (alpha);
		      if (ret != RL2_OK)
			  error = 1;
		  }
		else if (req->format == MIME_JPEG)
		  {
		      rgb = rl2_graph_get_context_rgb_array (ctx_out);
		      ret =
			  rl2_rgb_to_jpeg (req->width, req->height, rgb,
					   80, &image, &image_size);
		      free (rgb);
		      if (ret != RL2_OK)
			  error = 1;
		  }
		else if (req->format == MIME_TIFF)
		  {
		      rgb = rl2_graph_get_context_rgb_array (ctx_out);
		      ret =
			  rl2_rgb_to_tiff (req->width, req->height, rgb, &image,
					   &image_size);
		      free (rgb);
		      if (ret != RL2_OK)
			  error = 1;
		  }
		else if (req->format == MIME_PDF)
		  {
		      rgba = rl2_graph_get_context_rgba_array (ctx_out);
		      ret =
			  rl2_rgba_to_pdf (req->conn->rl2_privdata, req->width,
					   req->height, rgba, &image,
					   &image_size);
		      if (ret != RL2_OK)
			  error = 1;
		  }
		else
		    error = 1;

		if (error)
		  {
		      /* throwing an exception */
		      throw_exception (req,
				       "WmsLite internal error: GetMap unexpected NULL image");
		      req->http_status = 200;
		      req->freeor = NULL;
		  }
		else
		  {
		      if (req->http_response != NULL)
			  free (req->http_response);
		      req->http_response = image;
		      req->http_content_length = image_size;
		      req->freeor = free;
		      req->http_mime_type = req->format;
		  }
	    }
	  if (ctx_out != NULL)
	    {
		rl2_graph_destroy_context (ctx_out);
		if (rgba_base != NULL)
		    free (rgba_base);
	    }
	  return;
      }

/* single layer request */
    if (valid != layer_count)
      {
	  /* throwing an exception */
	  throw_exception (req,
			   "WmsLite internal error: GetMap unexpected NULL image");
	  req->http_status = 200;
	  req->freeor = NULL;
      }
    else
      {
	  /* valid MapImage */
	  req->freeor = free;
	  req->http_mime_type = req->format;
      }
}

static void
do_get_legend_graphic (WmsLiteHttpRequestPtr req)
{
/* preparing the GetLegendGraphic response */

    fprintf (stderr, "GetLegendGraphic layer=<%s> style=<%s> format=<%s>\n",
	     req->param_legend_layer, req->param_legend_style,
	     req->param_format);
    fflush (stderr);

    throw_exception (req, "WmsLite cazzo LegendURL");
    req->http_status = 200;
    req->freeor = NULL;
}

static char *
trim_value (const char *dirty)
{
/* removing leading and trailing whitespaces */
    const char *pi = dirty;
    const char *pS = dirty;
    char *clean;
    char *po;
    if (*pi == '\0')
	return NULL;
    while (1)
      {
	  /* removing leading whitespaces */
	  if (*pi == ' ' || *pi == '\t')
	    {
		pi++;
		pS = pi;
		continue;
	    }

	  break;
      }
    clean = malloc (strlen (pS) + 1);
    strcpy (clean, pS);
    po = clean + strlen (clean) - 1;
    while (po >= clean)
      {
	  /* removing trailing whitespaces */
	  if (*po == ' ' || *po == '\t')
	    {
		*po-- = '\0';
		continue;
	    }
	  break;
      }
    if (strlen (clean) == 0)
      {
	  free (clean);
	  return NULL;
      }
    return clean;
}

static void
parse_layers_list (WmsLiteLayersListPtr list, const char *layers)
{
/* parsing a comma separated list of layers */
    char *token;
    char *clean;
    char *buf = malloc (strlen (layers) + 1);
    char *in = buf;
    const char *pS = buf;
    strcpy (buf, layers);
    while (1)
      {
	  if (*in == '\0')
	    {
		token = malloc (strlen (pS) + 1);
		strcpy (token, pS);
		clean = trim_value (token);
		free (token);
		if (clean == NULL)
		    add_layer_to_list (list, NULL);
		else if (strlen (clean) == 0)
		  {
		      add_layer_to_list (list, NULL);
		      free (clean);
		  }
		else
		    add_layer_to_list (list, clean);
		break;
	    }
	  if (*in == ',')
	    {
		*in = '\0';
		token = malloc (strlen (pS) + 1);
		strcpy (token, pS);
		clean = trim_value (token);
		free (token);
		if (clean == NULL)
		    add_layer_to_list (list, NULL);
		else if (strlen (clean) == 0)
		  {
		      add_layer_to_list (list, NULL);
		      free (clean);
		  }
		else
		    add_layer_to_list (list, clean);
		in++;
		pS = in;
		continue;
	    }
	  in++;
      }
    free (buf);
}

static void
parse_styles_list (WmsLiteLayersListPtr list, const char *styles)
{
/* parsing a comma separated list of styles */
    char *token;
    char *clean;
    char *buf = malloc (strlen (styles) + 1);
    char *in = buf;
    const char *pS = buf;
    int ind = 0;
    strcpy (buf, styles);
    while (1)
      {
	  if (*in == '\0')
	    {
		token = malloc (strlen (pS) + 1);
		strcpy (token, pS);
		clean = trim_value (token);
		free (token);
		add_style_to_list (list, ind++, clean);
		break;
	    }
	  if (*in == ',')
	    {
		*in = '\0';
		token = malloc (strlen (pS) + 1);
		strcpy (token, pS);
		clean = trim_value (token);
		free (token);
		add_style_to_list (list, ind++, clean);
		in++;
		pS = in;
		continue;
	    }
	  in++;
      }
    free (buf);
}

static void
parse_wms_bbox (WmsLiteHttpRequestPtr req)
{
/* attempting to parse a BBOX */
    char *token;
    char *buf;
    char *in;
    const char *pS;
    double minx;
    double miny;
    double maxx;
    double maxy;
    int count = 0;

    if (req->bbox != NULL)
      {
	  free (req->bbox);
	  req->bbox = NULL;
      }
    if (req->param_bbox == NULL)
	return;

    buf = malloc (strlen (req->param_bbox) + 1);
    in = buf;
    pS = buf;
    strcpy (buf, req->param_bbox);
    while (1)
      {
	  if (*in == '\0')
	    {
		token = malloc (strlen (pS) + 1);
		strcpy (token, pS);
		switch (count)
		  {
		  case 0:
		      minx = atof (token);
		      break;
		  case 1:
		      miny = atof (token);
		      break;
		  case 2:
		      maxx = atof (token);
		      break;
		  case 3:
		      maxy = atof (token);
		      break;
		  };
		count++;
		free (token);
		break;
	    }
	  if (*in == ',')
	    {
		*in = '\0';
		token = malloc (strlen (pS) + 1);
		strcpy (token, pS);
		switch (count)
		  {
		  case 0:
		      minx = atof (token);
		      break;
		  case 1:
		      miny = atof (token);
		      break;
		  case 2:
		      maxx = atof (token);
		      break;
		  case 3:
		      maxy = atof (token);
		      break;
		  };
		count++;
		free (token);
		in++;
		pS = in;
		continue;
	    }
	  in++;
      }
    free (buf);

    if (count != 4)
	return;

/* allocating and initializing the BBOX */
    req->bbox = malloc (sizeof (WmsLiteBBox));
    req->bbox->MinX = minx;
    req->bbox->MinY = miny;
    req->bbox->MaxX = maxx;
    req->bbox->MaxY = maxy;
}

static int
parse_srs (const char *srs)
{
/* parsing the EPSG:x item */
    int srid;
    if (strlen (srs) < 6)
	return -1;
    if (strncmp (srs, "EPSG:", 4) != 0)
	return -1;
    srid = atoi (srs + 5);
    return srid;
}

static void
do_get_layer_legend (WmsLiteHttpRequestPtr req, int type, int sub_type, const char *db_prefix,
		     const char *layer, const char *style, int width,
		     int height, const char *format, const char *font_name,
		     double font_size, int font_italic, int font_bold,
		     const char *font_color)
{
/* attempting to get a LegendGraphic item */
    sqlite3 *sqlite = req->conn->handle;
    sqlite3_stmt *stmt = NULL;
    int ret;
    char *sql = NULL;
    int valid = 0;
    const char *layer_type;

    if (type == WMS_LAYER_RASTER)
    {
		switch(sub_type)
		{
			case RL2_PIXEL_MONOCHROME:
				layer_type = "MONOCHROME";
				break;
			case RL2_PIXEL_PALETTE:
				layer_type = "PALETTE";
				break;
			case RL2_PIXEL_GRAYSCALE:
				layer_type = "GRAYSCALE";
				break;
			case RL2_PIXEL_RGB:
				layer_type = "RGB";
				break;
			case RL2_PIXEL_MULTIBAND:
				layer_type = "MULTIBAND";
				break;
			case RL2_PIXEL_DATAGRID:
				layer_type = "DATAGRID";
				break;
			default:
				layer_type = "";
				break;
		};
	sql =
	    sqlite3_mprintf
	    ("SELECT RL2_GetRasterLegendGraphic(%Q, %Q, %Q, %Q, %d, %d, %Q, %Q, %f, %d, %d, %s)",
	     db_prefix, layer, layer_type, style, width, height, format, font_name,
	     font_size, font_italic, font_bold, font_color);
	 }
    if (type == WMS_LAYER_VECTOR)
	sql =
	    sqlite3_mprintf
	    ("SELECT RL2_GetVectorLegendGraphic(%Q, %Q, %Q, %d, %d, %Q, %Q, %f, %d, %d, %s)",
	     db_prefix, layer, style, width, height, format, font_name,
	     font_size, font_italic, font_bold, font_color);
    if (sql == NULL)
	goto error;
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
		if (sqlite3_column_type (stmt, 0) == SQLITE_BLOB)
		  {
		      unsigned char *payload =
			  (unsigned char *) sqlite3_column_blob (stmt, 0);
		      int payload_size = sqlite3_column_bytes (stmt, 0);
		      if (req->http_response != NULL)
			  free (req->http_response);
		      req->http_response = malloc (payload_size);
		      memcpy (req->http_response, payload, payload_size);
		      req->http_content_length = payload_size;
		      valid = 1;
		  }
	    }
      }
    sqlite3_finalize (stmt);
    stmt = NULL;
    if (!valid)
	goto error;
    return;

  error:
    if (stmt != NULL)
	sqlite3_finalize (stmt);
/* throwing an exception */
    throw_exception (req,
		     "WmsLite internal error: GetLegendGraphics unexpected NULL image");
}

extern void
process_http_request (WmsLiteHttpRequestPtr req)
{
/* checking for a valid WMS request */
    int intval;
    WmsLiteArgumentPtr arg = req->first_arg;
    while (arg != NULL)
      {
	  /* evaluating any critical argument */
	  if (strcasecmp (arg->arg_name, "SERVICE") == 0)
	    {
		if (strcasecmp (arg->arg_value, "WMS") == 0)
		    req->service_wms = 1;
	    }
	  if (strcasecmp (arg->arg_name, "REQUEST") == 0)
	    {
		if (req->config->IsMiniServer)
		  {
		      if (strcasecmp (arg->arg_value, "SHUTDOWN") == 0
			  || strcasecmp (arg->arg_value, "EXIT") == 0
			  || strcasecmp (arg->arg_value, "QUIT") == 0
			  || strcasecmp (arg->arg_value, "STOP") == 0)
			{
			    /* special case for MiniServer only: Shutdown requested */
			    if (req->http_response != NULL)
			      {
				  if (req->freeor != NULL)
				      req->freeor (req->http_response);
			      }
			    req->http_response =
				sqlite3_mprintf
				("<html>\r\n<head><title>WmsLite Message</title></head>\r\n"
				 "<body>\r\nWmsLite: (%s) the service will now stop\r\n"
				 "</body></html>\r\n", arg->arg_value);
			    req->freeor = sqlite3_free;
			    req->http_content_length =
				strlen (req->http_response);
			    req->http_mime_type = MIME_HTML;
			    req->http_status = 200;
			    req->config->PendingShutdown = 1;
			    return;
			}
		  }
		if (strcasecmp (arg->arg_value, "GetCapabilities") == 0)
		    req->request_type = WMS_GET_CAPABILITIES;
		if (strcasecmp (arg->arg_value, "GetMap") == 0)
		    req->request_type = WMS_GET_MAP;
		if (strcasecmp (arg->arg_value, "GetFeatureInfo") == 0)
		    req->request_type = WMS_GET_FEATURE_INFO;
		if (strcasecmp (arg->arg_value, "GetLegendGraphic") == 0)
		    req->request_type = WMS_GET_LEGEND_GRAPHIC;
	    }
	  if (strcasecmp (arg->arg_name, "LAYERS") == 0)
	      req->param_layers = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "STYLES") == 0)
	      req->param_styles = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "LAYER") == 0)
	      req->param_legend_layer = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "STYLE") == 0)
	      req->param_legend_style = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "CRS") == 0
	      || strcasecmp (arg->arg_name, "SRS") == 0)
	      req->param_crs = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "BBOX") == 0)
	      req->param_bbox = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "WIDTH") == 0)
	      req->param_width = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "HEIGHT") == 0)
	      req->param_height = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "FORMAT") == 0)
	      req->param_format = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "TRANSPARENT") == 0)
	      req->param_transparent = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "BGCOLOR") == 0)
	      req->param_bgcolor = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "REASPECT") == 0)
	      req->param_reaspect = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "QUERY_LAYERS") == 0)
	      req->param_query_layers = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "INFO_FORMAT") == 0)
	      req->param_info_format = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "FEATURE_COUNT") == 0)
	      req->param_feature_count = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "X") == 0
	      || strcasecmp (arg->arg_name, "I") == 0)
	      req->param_point_x = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "Y") == 0
	      || strcasecmp (arg->arg_name, "J") == 0)
	      req->param_point_y = arg->arg_value;
	  if (strcasecmp (arg->arg_name, "EXCEPTIONS") == 0)
	    {
		req->exceptions = WMS_EXCEPTIONS_ERROR;
		if (strcasecmp (arg->arg_value, "INIMAGE") == 0)
		    req->exceptions = WMS_EXCEPTIONS_IMAGE;
		if (strcasecmp (arg->arg_value, "BLANK") == 0)
		    req->exceptions = WMS_EXCEPTIONS_BLANK;
		if (strcasecmp (arg->arg_value, "XML") == 0)
		    req->exceptions = WMS_EXCEPTIONS_XML;
		if (strcasecmp (arg->arg_value, "WMS_XML") == 0)
		    req->exceptions = WMS_EXCEPTIONS_WMSXML;
	    }
	  if (strcasecmp (arg->arg_name, "VERSION") == 0)
	    {
		parse_version (req, arg->arg_value);
		req->param_version = arg->arg_value;
	    }
	  arg = arg->next;
      }
    set_ok_version (req);

/* attempting to validate the HTTP request */
    if (req->service_wms == 0)
      {
	  /* absolutely invalid request - missing Service=WMS */
	  if (req->http_response != NULL)
	    {
		if (req->freeor != NULL)
		    req->freeor (req->http_response);
	    }
	  req->http_response =
	      sqlite3_mprintf
	      ("<html>\r\n<head><title>WmsLite Message</title></head>\r\n"
	       "<body>\r\nWmsLite: bad request<br>mandatory parameter <b>SERVICE=WMS</b> not found\r\n"
	       "</body></html>\r\n");
	  req->freeor = sqlite3_free;
	  req->http_content_length = strlen (req->http_response);
	  req->http_mime_type = MIME_HTML;
	  req->http_status = 200;
	  return;
      }

    if (req->request_type == WMS_GET_MAP
	|| req->request_type == WMS_GET_FEATURE_INFO
	|| req->request_type == WMS_GET_LEGEND_GRAPHIC)
      {
	  /* validating a GetMap, GetFeatureInfo or GetLegendGraphic request */
	  WmsLiteLayersListPtr layers_list;
	  WmsLiteLayersListPtr query_layers;
	  WmsLiteStyledLayerPtr lyr;
	  char *err_msg = NULL;
	  const char *request;
	  int count;

	  if (req->request_type == WMS_GET_FEATURE_INFO)
	      request = "GetFeatureInfo";
	  else if (req->request_type == WMS_GET_LEGEND_GRAPHIC)
	      request = "GetLegendGraphic";
	  else
	      request = "GetMap";

	  /* checking for empty parameters */
	  if (req->param_version != NULL && *(req->param_version) == '\0')
	      err_msg =
		  sqlite3_mprintf
		  ("Incomplete WMS %s request: empty VERSION parameter",
		   request);
	  else if (req->param_layers != NULL && *(req->param_layers) == '\0')
	      err_msg =
		  sqlite3_mprintf
		  ("Incomplete WMS %s request: empty LAYERS parameter",
		   request);
	  else if (req->param_bbox != NULL && *(req->param_bbox) == '\0')
	      err_msg =
		  sqlite3_mprintf
		  ("Incomplete WMS %s request: empty BBOX parameter", request);
	  else if (req->param_crs != NULL && *(req->param_crs) == '\0')
	      err_msg =
		  sqlite3_mprintf
		  ("Incomplete WMS %s request: empty CRS/SRS parameter",
		   request);
	  else if (req->param_width != NULL && *(req->param_width) == '\0')
	      err_msg =
		  sqlite3_mprintf
		  ("Incomplete WMS %s request: empty WIDTH parameter", request);
	  else if (req->param_height != NULL && *(req->param_height) == '\0')
	      err_msg =
		  sqlite3_mprintf
		  ("Incomplete WMS %s request: empty HEIGHT parameter",
		   request);
	  else if (req->param_format != NULL && *(req->param_format) == '\0'
		   && (req->request_type == WMS_GET_MAP
		       || req->request_type == WMS_GET_LEGEND_GRAPHIC))
	      err_msg =
		  sqlite3_mprintf
		  ("Incomplete WMS %s request: empty FORMAT parameter",
		   request);
	  else if (req->param_query_layers != NULL
		   && *(req->param_query_layers) == '\0'
		   && req->request_type == WMS_GET_FEATURE_INFO)
	      err_msg =
		  sqlite3_mprintf
		  ("Incomplete WMS %s request: empty QUERY_LAYERS parameter",
		   request);
	  else if (req->param_point_x != NULL && *(req->param_point_x) == '\0'
		   && req->request_type == WMS_GET_FEATURE_INFO)
	      err_msg =
		  sqlite3_mprintf
		  ("Incomplete WMS %s request: empty point X/I parameter",
		   request);
	  else if (req->param_point_y != NULL && *(req->param_point_y) == '\0'
		   && req->request_type == WMS_GET_FEATURE_INFO)
	      err_msg =
		  sqlite3_mprintf
		  ("Incomplete WMS %s request: empty point Y/J parameter",
		   request);
	  if (err_msg != NULL)
	    {
		throw_exception (req, err_msg);
		sqlite3_free (err_msg);
		req->http_status = 200;
		return;
	    }

/* checking for undeclared parameters */
	  if (req->request_type == WMS_GET_LEGEND_GRAPHIC)
	    {
		/* special case: GetLegendGraphic */
		if (req->param_version == NULL)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: VERSION parameter missing",
			 request);
		else if (req->param_legend_layer == NULL)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: LAYER parameter missing",
			 request);
		else if (req->param_legend_style == NULL)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: STYLE parameter missing",
			 request);
		else if (req->param_format == NULL)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: FORMAT parameter missing",
			 request);
		if (err_msg == NULL)
		  {
		      /* checking for an existing Layer and Style */
		      int type;
		      int sub_type;
		      const char *db_prefix = NULL;
		      int ret =
			  check_layer_legend (req->conn->handle, req->config,
					      req->param_legend_layer,
					      req->param_legend_style, &type, &sub_type,
					      &db_prefix);
		      if (ret == WMS_LAYER_OK)
			{
			    do_get_layer_legend (req, type, sub_type, db_prefix,
						 req->param_legend_layer,
						 req->param_legend_style,
						 req->config->LegendWidth,
						 req->config->LegendHeight,
						 req->config->LegendFormat,
						 req->config->LegendFontName,
						 req->config->LegendFontSize,
						 req->config->LegendFontItalic,
						 req->config->LegendFontBold,
						 req->config->LegendFontColor);
			    req->http_status = 200;
			    return;
			}
		      else if (ret == WMS_BAD_LAYER)
			{
			    char *msg =
				sqlite3_mprintf
				("WMS %s: Not existing layer (%s) given in the LAYER parameter.",
				 request, req->param_legend_layer);
			    throw_exception (req, msg);
			    sqlite3_free (msg);
			    req->http_status = 200;
			    return;
			}
		      else
			{
			    char *msg =
				sqlite3_mprintf
				("WMS %s: Layer (%s): Not existing style (%s) given in the STYLE parameter.",
				 request,
				 req->param_legend_layer,
				 req->param_legend_style);
			    throw_exception (req, msg);
			    sqlite3_free (msg);
			    req->http_status = 200;
			    return;
			}
		  }
	    }
	  else
	    {
		/* GetMap and GetFeatureInfo */
		if (req->param_version == NULL)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: VERSION parameter missing",
			 request);
		else if (req->param_layers == NULL)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: LAYERS parameter missing",
			 request);
		else if (req->param_query_layers == NULL
			 && req->request_type == WMS_GET_FEATURE_INFO)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: QUERY_LAYERS parameter missing",
			 request);
		else if (req->param_crs == NULL)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: CRS/SRS parameter missing",
			 request);
		else if (req->param_bbox == NULL)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: BBOX parameter missing",
			 request);
		else if (req->param_format == NULL
			 && req->request_type == WMS_GET_MAP)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: FORMAT parameter missing",
			 request);
		else if (req->param_width == NULL)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: WIDTH parameter missing",
			 request);
		else if (req->param_height == NULL)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: HEIGHT parameter missing",
			 request);
		else if (req->param_point_x == NULL
			 && req->request_type == WMS_GET_FEATURE_INFO)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: point X/I parameter missing",
			 request);
		else if (req->param_point_y == NULL
			 && req->request_type == WMS_GET_FEATURE_INFO)
		    err_msg =
			sqlite3_mprintf
			("Incomplete WMS %s request: point Y/J parameter missing",
			 request);
	    }
	  if (err_msg != NULL)
	    {
		throw_exception (req, err_msg);
		sqlite3_free (err_msg);
		req->http_status = 200;
		return;
	    }

	  /* checking VERSION */
	  if (req->version_1 == 1 && req->version_2 == 0 && req->version_3 == 0)
	      ;
	  else if (req->version_1 == 1 && req->version_2 == 1
		   && req->version_3 == 0)
	      ;
	  else if (req->version_1 == 1 && req->version_2 == 1
		   && req->version_3 == 1)
	      ;
	  else if (req->version_1 == 1 && req->version_2 == 3
		   && req->version_3 == 0)
	      ;
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("WMS %s: VERSION %d.%d.%d is not supported.", request,
		     req->version_1, req->version_2, req->version_3);
		throw_exception (req, msg);
		sqlite3_free (msg);
		req->http_status = 200;
		return;
	    }

	  if (req->request_type == WMS_GET_LEGEND_GRAPHIC)
	    {
		/* special case: GetLegendGraphic */

		/* checking FORMAT */
		req->format = MIME_UNKNOWN;
		if (strcasecmp (req->param_format, "PNG") == 0
		    || strcasecmp (req->param_format, "image/png") == 0)
		    req->format = MIME_PNG;
		if (strcasecmp (req->param_format, "JPEG") == 0
		    || strcasecmp (req->param_format, "image/jpeg") == 0)
		    req->format = MIME_JPEG;
		if (req->format == MIME_UNKNOWN)
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("WMS GetLegendGraphic: Unsupported FORMAT (%s)",
			   req->param_format);
		      throw_exception (req, msg);
		      sqlite3_free (msg);
		      req->http_status = 200;
		      return;
		  }
		goto post_check;
	    }
	  /* checking LAYERS and STYLES */
	  layers_list = create_layers_list ();
	  parse_layers_list (layers_list, req->param_layers);
	  if (req->param_styles != NULL)
	      parse_styles_list (layers_list, req->param_styles);
	  count = 0;
	  lyr = layers_list->first;
	  while (lyr != NULL)
	    {
		int ret = check_wms_layer (req->config, lyr);
		if (ret == WMS_NULL_LAYER)
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("WMS %s: Invalid NULL layer (#%d) in the LAYERS parameter.",
			   request, count);
		      throw_exception (req, msg);
		      sqlite3_free (msg);
		      req->http_status = 200;
		      destroy_layers_list (layers_list);
		      return;
		  }
		if (ret == WMS_BAD_LAYER)
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("WMS %s: Invalid layer (%s) given in the LAYERS parameter.",
			   request,
			   lyr->LayerName);
		      throw_exception (req, msg);
		      sqlite3_free (msg);
		      req->http_status = 200;
		      destroy_layers_list (layers_list);
		      return;
		  }
		if (ret == WMS_BAD_STYLE)
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("WMS %s: Layer (%s): Invalid style (%s) given in the STYLES parameter.",
			   request,
			   lyr->LayerName, lyr->StyleName);
		      throw_exception (req, msg);
		      sqlite3_free (msg);
		      req->http_status = 200;
		      destroy_layers_list (layers_list);
		      return;
		  }
		count++;
		lyr = lyr->Next;
	    }
	  if (count < 1)
	    {
		/* checking that there is at least a valid layer */
		char *msg =
		    sqlite3_mprintf
		    ("WMS %s: Invalid LAYERS parameter (empty list).",
		     request);
		throw_exception (req, msg);
		sqlite3_free (msg);
		req->http_status = 200;
		destroy_layers_list (layers_list);
		return;
	    }
	  if (count > req->config->LayerLimit)
	    {
		/* checking to not exceed the LayerLimit setting */
		char *msg =
		    sqlite3_mprintf
		    ("WMS %s: Invalid LAYERS parameter (too many layers, max %d).",
		     request, req->config->LayerLimit);
		throw_exception (req, msg);
		sqlite3_free (msg);
		req->http_status = 200;
		destroy_layers_list (layers_list);
		return;
	    }

	  /* checking CRS */
	  lyr = layers_list->first;
	  while (lyr != NULL)
	    {
		if (!check_wms_layer_crs
		    (req->config, lyr->LayerName, req->param_crs))
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("WMS %s: Layer (%s): Unsupported (%s) given in the CRS/SRS parameter.",
			   request,
			   lyr->LayerName, req->param_crs);
		      throw_exception (req, msg);
		      sqlite3_free (msg);
		      req->http_status = 200;
		      destroy_layers_list (layers_list);
		      return;
		  }
		lyr = lyr->Next;
	    }

	  /* saving the validated list of LAYERS */
	  req->layers_list = layers_list;

	  /* checking BBOX */
	  parse_wms_bbox (req);
	  if (!check_wms_bbox (req->bbox))
	    {
		char *msg =
		    sqlite3_mprintf
		    ("WMS %s: invalid values in the BBOX parameter.", request);
		throw_exception (req, msg);
		sqlite3_free (msg);
		req->http_status = 200;
		return;
	    }

	  /* checking WIDTH */
	  intval = atoi (req->param_width);
	  if (intval > 0 && intval <= req->config->MaxWidth)
	      req->width = intval;
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("WMS %s: out of range WIDTH (must be between 1 and %d)",
		     request,
		     req->config->MaxWidth);
		throw_exception (req, msg);
		sqlite3_free (msg);
		req->http_status = 200;
		return;
	    }

	  /* checking HEIGHT */
	  intval = atoi (req->param_height);
	  if (intval > 0 && intval <= req->config->MaxHeight)
	      req->height = intval;
	  else
	    {
		char *msg =
		    sqlite3_mprintf
		    ("WMS %s: out of range HEIGHT (must be between 1 and %d)",
		     request,
		     req->config->MaxHeight);
		throw_exception (req, msg);
		sqlite3_free (msg);
		req->http_status = 200;
		return;
	    }

	  if (req->request_type == WMS_GET_MAP)
	    {
		/* checking FORMAT */
		req->format = MIME_UNKNOWN;
		if (strcasecmp (req->param_format, "PNG") == 0
		    || strcasecmp (req->param_format, "image/png") == 0
		    || strcasecmp (req->param_format, "image/png8") == 0)
		    req->format = MIME_PNG;
		if (strcasecmp (req->param_format, "GIF") == 0
		    || strcasecmp (req->param_format, "image/gif") == 0)
		    req->format = MIME_GIF;
		if (strcasecmp (req->param_format, "JPEG") == 0
		    || strcasecmp (req->param_format, "image/jpeg") == 0)
		    req->format = MIME_JPEG;
		if (strcasecmp (req->param_format, "TIFF") == 0
		    || strcasecmp (req->param_format, "image/tiff") == 0
		    || strcasecmp (req->param_format, "image/tiff8") == 0
		    || strcasecmp (req->param_format, "image/geotiff") == 0
		    || strcasecmp (req->param_format, "image/geotiff8") == 0)
		    req->format = MIME_TIFF;
		if (strcasecmp (req->param_format, "PDF") == 0
		    || strcasecmp (req->param_format, "application/x-pdf") == 0
		    || strcasecmp (req->param_format, "application/pdf") == 0)
		    req->format = MIME_PDF;
		if (req->format == MIME_UNKNOWN)
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("WMS GetMap: Unsupported FORMAT (%s)",
			   req->param_format);
		      throw_exception (req, msg);
		      sqlite3_free (msg);
		      req->http_status = 200;
		      return;
		  }

		if (req->param_transparent != NULL)
		  {
		      /* checking TRANSPARENT */
		      if (!check_wms_transparent (req))
			{
			    const char *msg =
				"WMS GetMap: invalid value in the TRANSPARENT parameter.";
			    throw_exception (req, msg);
			    req->http_status = 200;
			    return;
			}
		  }

		if (req->param_reaspect != NULL)
		  {
		      /* checking REASPECT */
		      if (!check_wms_reaspect (req))
			{
			    const char *msg =
				"WMS GetMap: invalid value in the REASPECT (vendor-specific) parameter.";
			    throw_exception (req, msg);
			    req->http_status = 200;
			    return;
			}
		  }

		if (req->reaspect == 0)
		  {
		      /* checking consistency between BBOX, WIDTH and HEIGHT */
		      if (!check_wms_bbox_consistency (req))
			{
			    const char *msg =
				"WMS GetMap: inconsistent BBOX (not matching WIDTH and HEIGHT).";
			    throw_exception (req, msg);
			    req->http_status = 200;
			    return;
			}
		  }

		if (req->param_bgcolor != NULL)
		  {
		      /* checking BGCOLOR */
		      if (!check_wms_bgcolor (req))
			{
			    const char *msg =
				"WMS GetMap: invalid value in the BGCOLOR parameter.";
			    throw_exception (req, msg);
			    req->http_status = 200;
			    return;
			}
		  }

		/* setting the Request SRID */
		req->srid = parse_srs (req->param_crs);
	    }

	  if (req->request_type == WMS_GET_FEATURE_INFO)
	    {
		/* checking QUERY_LAYERS */
		query_layers = create_layers_list ();
		parse_layers_list (query_layers, req->param_query_layers);
		count = 0;
		lyr = query_layers->first;
		while (lyr != NULL)
		  {
		      int ret = check_wms_query_layer (req->config, lyr);
		      if (ret == WMS_BAD_LAYER)
			{
			    char *msg =
				sqlite3_mprintf
				("WMS %s: Invalid layer (%s) given in the QUERY_LAYERS parameter.",
				 request,
				 lyr->LayerName);
			    throw_exception (req, msg);
			    sqlite3_free (msg);
			    req->http_status = 200;
			    destroy_layers_list (layers_list);
			    return;
			}
		      count++;
		      lyr = lyr->Next;
		  }
		/* saving the validated list of QUERY_LAYERS */
		req->query_layers = query_layers;

		if (req->param_info_format == NULL)
		    req->info_format = MIME_XML;
		else
		  {
		      /* checking INFO_FORMAT */
		      req->format = MIME_UNKNOWN;
		      if (strcasecmp (req->param_info_format, "XML") == 0
			  || strcasecmp (req->param_info_format,
					 "text/xml") == 0)
			  req->info_format = MIME_XML;
		      if (req->info_format == MIME_UNKNOWN)
			{
			    char *msg =
				sqlite3_mprintf
				("WMS GetFeatureInfo: Unsupported INFO_FORMAT (%s)",
				 req->param_info_format);
			    throw_exception (req, msg);
			    sqlite3_free (msg);
			    req->http_status = 200;
			    return;
			}
		  }

		if (req->param_feature_count == NULL)
		    req->feature_count = 1;
		else
		  {
		      /* checking FEATURE_COUNT */
		      intval = atoi (req->param_feature_count);
		      if (intval > 0)
			  req->feature_count = intval;
		      else
			{
			    const char *msg =
				"WMS GetCapabilities: Invalid value in the FEATURE_COUNT parameter.";
			    throw_exception (req, msg);
			    req->http_status = 200;
			    return;
			}
		  }

		/* checking X */
		intval = atoi (req->param_point_x);
		if (intval >= 0 && intval < req->width)
		    req->point_x = intval;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("WMS GetFeatureInfo: invalid Point X/I coordinate (must be between 0 and %d)",
			   req->width - 1);
		      throw_exception (req, msg);
		      sqlite3_free (msg);
		      req->http_status = 200;
		      return;
		  }

		/* checking Y */
		intval = atoi (req->param_point_y);
		if (intval >= 0 && intval < req->height)
		    req->point_y = intval;
		else
		  {
		      char *msg =
			  sqlite3_mprintf
			  ("WMS GetFeatureInfo: invalid Point Y/J coordinate (must be between 0 and %d)",
			   req->height - 1);
		      throw_exception (req, msg);
		      sqlite3_free (msg);
		      req->http_status = 200;
		      return;
		  }
	    }
      }

/* processing the HTTP request */
  post_check:
    switch (req->request_type)
      {
      case WMS_GET_CAPABILITIES:
	  do_get_capabilities (req);
	  break;
      case WMS_GET_MAP:
	  do_get_map (req);
	  break;
      case WMS_GET_FEATURE_INFO:
	  throw_exception (req, "IT SHOULD BE AN XML");
	  break;
      case WMS_GET_LEGEND_GRAPHIC:
	  do_get_legend_graphic (req);
	  break;
      default:
	  throw_exception (req,
			   "Incomplete WMS request: REQUEST parameter missing or invalid");
	  req->http_status = 200;
	  return;
      };
    req->http_status = 200;
}

extern char *
get_timestamp ()
{
/* returning the current timestamp */
    const char *month = "Xxx";
    int gt_h;
    int gt_m;
    int lt_h;
    int lt_m;
    char *dummy;
    struct tm *ltm;
    time_t now;
    time (&now);
    ltm = gmtime (&now);
    gt_h = ltm->tm_hour;
    gt_m = ltm->tm_min;
    ltm = localtime (&now);
    lt_h = ltm->tm_hour;
    lt_m = ltm->tm_min;
    switch (ltm->tm_mon)
      {
      case 0:
	  month = "Jan";
	  break;
      case 1:
	  month = "Feb";
	  break;
      case 2:
	  month = "Mar";
	  break;
      case 3:
	  month = "Apr";
	  break;
      case 4:
	  month = "May";
	  break;
      case 5:
	  month = "Jun";
	  break;
      case 6:
	  month = "Jul";
	  break;
      case 7:
	  month = "Aug";
	  break;
      case 8:
	  month = "Sep";
	  break;
      case 9:
	  month = "Oct";
	  break;
      case 10:
	  month = "Nov";
	  break;
      case 11:
	  month = "Dec";
	  break;
      };
    dummy =
	sqlite3_mprintf ("%02d/%s/%04d:%02d:%02d:%02d +%02d%02d", ltm->tm_mday,
			 month, ltm->tm_year + 1900, ltm->tm_hour, ltm->tm_min,
			 ltm->tm_sec, lt_h - gt_h, lt_m - gt_m);
    return dummy;
}

extern void
do_update_logfile (WmsLiteHttpRequestPtr req)
{
/* updating the Logfile */
    req->end_time = get_timestamp ();
    printf ("%s - - [%s] \"%s\" %d %d \"%s\"\r\n", req->client_ip_addr,
	    req->end_time, req->request_url, req->http_status,
	    req->http_content_length, req->user_agent);
    fflush (stdout);
}

extern WmsLiteHttpRequestPtr
create_http_request (int id, WmsLiteConfigPtr config, const char *server_addr,
		     int port_no, struct neutral_socket *socket)
{
/* allocating an HTTP Request object */
    WmsLiteHttpRequestPtr req = malloc (sizeof (WmsLiteHttpRequest));
    req->id = id;
    req->config = config;
    req->ip_addr = server_addr;
    req->port_no = port_no;
    if (socket != NULL)
	req->socket.socket = socket->socket;
    req->protocol = NULL;
    req->client_ip_addr = NULL;
    req->client_ip_port = -1;
    req->request_url = NULL;
    req->user_agent = NULL;
    req->request_method = NULL;
    req->content_length = -1;
    req->first_arg = NULL;
    req->last_arg = NULL;
    req->begin_time = NULL;
    req->end_time = NULL;
    req->http_status = -1;
    req->begin_time = get_timestamp ();
    req->service_wms = 0;
    req->request_type = 0;
    req->exceptions = WMS_EXCEPTIONS_DEFAULT;
    req->version_1 = 99;
    req->version_2 = 99;
    req->version_3 = 99;
    req->ok_version = WMS_VERSION_130;
    req->layers_list = NULL;
    req->bbox = NULL;
    req->srid = -1;
    req->width = 0;
    req->height = 0;
    req->format = 0;
    req->transparent = 0;
    req->reaspect = 0;
    req->bg_red = 255;
    req->bg_green = 255;
    req->bg_blue = 255;
    req->query_layers = NULL;
    req->info_format = 0;
    req->feature_count = 1;
    req->point_x = 0;
    req->point_y = 0;
    req->http_response = NULL;
    req->freeor = NULL;
    req->http_content_length = 0;
    req->http_mime_type = 0;
    req->param_version = NULL;
    req->param_layers = NULL;
    req->param_styles = NULL;
    req->param_legend_layer = NULL;
    req->param_legend_style = NULL;
    req->param_crs = NULL;
    req->param_bbox = NULL;
    req->param_width = NULL;
    req->param_height = NULL;
    req->param_format = NULL;
    req->param_transparent = NULL;
    req->param_bgcolor = NULL;
    req->param_reaspect = NULL;
    req->param_query_layers = NULL;
    req->param_info_format = NULL;
    req->param_feature_count = NULL;
    req->param_point_x = NULL;
    req->param_point_y = NULL;
    return req;
}

extern void
destroy_http_request (WmsLiteHttpRequestPtr req)
{
/* memory cleanup: freeing an HTTP Request struct */
    WmsLiteArgumentPtr pa;
    WmsLiteArgumentPtr pan;
    if (req == NULL)
	return;
    if (req->protocol != NULL)
	free (req->protocol);
    if (req->user_agent != NULL)
	free (req->user_agent);
    if (req->client_ip_addr != NULL)
	free (req->client_ip_addr);
    if (req->request_url != NULL)
	free (req->request_url);
    if (req->request_method != NULL)
	free (req->request_method);
    if (req->layers_list != NULL)
	destroy_layers_list (req->layers_list);
    if (req->query_layers != NULL)
	destroy_layers_list (req->query_layers);
    if (req->bbox != NULL)
	free (req->bbox);
    pa = req->first_arg;
    while (pa != NULL)
      {
	  pan = pa->next;
	  destroy_wms_argument (pa);
	  pa = pan;
      }
    if (req->begin_time != NULL)
	sqlite3_free (req->begin_time);
    if (req->end_time != NULL)
	sqlite3_free (req->end_time);
    if (req->http_response != NULL)
      {
	  if (req->freeor != NULL)
	      req->freeor (req->http_response);
      }
    free (req);
}
