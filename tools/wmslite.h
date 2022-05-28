/* 
/ wmslite - common definitions
/
/ a light-weight WMS server / GCI supporting RasterLite2 DataSources
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

#include <curl/curl.h>

#include <rasterlite2/rasterlite2.h>
#include <rasterlite2/rl2mapconfig.h>
#include <rasterlite2/rl2graphics.h>
#include <spatialite.h>
#include <spatialite/gaiaaux.h>

#define ARG_NONE		1000
#define ARG_SERVER		1001
#define ARG_CONFIG_FILE	1002
#define ARG_ACCESS_LOG	1003
#define ARG_ERROR_LOG	1004
#define ARG_IP_PORT		1005
#define ARG_MAX_CONN	1006

#define WMS_LAYER_UNKNOWN		0x00
#define WMS_LAYER_RASTER		0x41
#define WMS_LAYER_VECTOR		0x52
#define WMS_LAYER_CASCADED_WMS	0x63
#define WMS_LAYER_MAP_CONFIG	0x74

#define WMS_VERSION_UNKNOWN	0
#define WMS_VERSION_100		100
#define WMS_VERSION_110		110
#define WMS_VERSION_111		111
#define WMS_VERSION_130		130

#define WMS_GET_CAPABILITIES	1
#define WMS_GET_MAP				2
#define WMS_GET_FEATURE_INFO	3
#define WMS_GET_LEGEND_GRAPHIC	4

#define WMS_EXCEPTIONS_DEFAULT	0
#define WMS_EXCEPTIONS_XML		1
#define WMS_EXCEPTIONS_WMSXML	2
#define WMS_EXCEPTIONS_IMAGE	3
#define WMS_EXCEPTIONS_BLANK	4
#define WMS_EXCEPTIONS_ERROR	5

#define MIME_UNKNOWN	0
#define MIME_HTML	1
#define MIME_XML	2
#define MIME_PNG	3
#define MIME_GIF	4
#define MIME_JPEG	5
#define MIME_TIFF	6
#define MIME_PDF	7

#define CONNECTION_INVALID	0
#define CONNECTION_AVAILABLE	1
#define CONNECTION_BUSY		2

#define WMS_LAYER_OK	0
#define WMS_BAD_LAYER	1
#define WMS_BAD_STYLE	2
#define WMS_NULL_LAYER	3

#define WMS_NOT_FOUND		0
#define WMS_MAIN_RASTER		1
#define WMS_MAIN_VECTOR		2
#define WMS_MAIN_CASCADED	3
#define WMS_MAIN_CONFIG		4
#define WMS_ATTACH_RASTER	5
#define WMS_ATTACH_VECTOR	6
#define WMS_ATTACH_CASCADED	7

struct neutral_socket
{
#ifdef _WIN32
    SOCKET socket;
#else
    int socket;
#endif
};

typedef struct wms_lite_child_layer
{
/* a struct wrapping a WmsLite Child Layer */
    char *ChildAliasName;
    struct wms_lite_child_layer *Next;
} WmsLiteChildLayer;
typedef WmsLiteChildLayer *WmsLiteChildLayerPtr;

typedef struct wms_lite_keyword
{
/* a struct wrapping a WmsLite Keyword */
    char *Keyword;
    struct wms_lite_keyword *Next;
} WmsLiteKeyword;
typedef WmsLiteKeyword *WmsLiteKeywordPtr;

typedef struct wms_lite_bbox
{
/* a struct wrapping a WmsLite BoundingBox */
    double MinX;
    double MinY;
    double MaxX;
    double MaxY;
} WmsLiteBBox;
typedef WmsLiteBBox *WmsLiteBBoxPtr;

typedef struct wms_lite_crs
{
/* a struct wrapping a WmsLite CRS */
    int Srid;
    WmsLiteBBoxPtr BBox;
    int IsDefault;
    struct wms_lite_crs *Next;
} WmsLiteCrs;
typedef WmsLiteCrs *WmsLiteCrsPtr;

typedef struct wms_lite_wms_crs
{
/* a struct wrapping a WmsLite WMS CRS */
    char *Srs;
    WmsLiteBBoxPtr BBox;
    int IsDefault;
    struct wms_lite_wms_crs *Next;
} WmsLiteWmsCrs;
typedef WmsLiteWmsCrs *WmsLiteWmsCrsPtr;

typedef struct wms_lite_style
{
/* a struct wrapping an SLD/SE Style */
    char *Name;
    char *Title;
    char *Abstract;
    struct wms_lite_style *Next;
} WmsLiteStyle;
typedef WmsLiteStyle *WmsLiteStylePtr;

typedef struct wms_lite_cascaded_wms
{
/* a struct wrapping a WmsLite Cascaded WMS Layer */
    int Id;
    char *Title;
    char *Abstract;
    const char *Srs;
    WmsLiteBBoxPtr GeographicBBox;
    int IsQueryable;
    int IsTransparent;
    int Cascaded;
    WmsLiteKeywordPtr KeyFirst;
    WmsLiteKeywordPtr KeyLast;
    WmsLiteWmsCrsPtr CrsFirst;
    WmsLiteWmsCrsPtr CrsLast;
    WmsLiteStylePtr StyleFirst;
    WmsLiteStylePtr StyleLast;
} WmsLiteCascadedWms;
typedef WmsLiteCascadedWms *WmsLiteCascadedWmsPtr;

typedef struct wms_lite_raster
{
/* a struct wrapping a WmsLite Raster Layer */
    char *Title;
    char *Abstract;
    int Srid;
    WmsLiteBBoxPtr GeographicBBox;
    int IsQueryable;
    WmsLiteKeywordPtr KeyFirst;
    WmsLiteKeywordPtr KeyLast;
    WmsLiteCrsPtr CrsFirst;
    WmsLiteCrsPtr CrsLast;
    WmsLiteStylePtr StyleFirst;
    WmsLiteStylePtr StyleLast;
} WmsLiteRaster;
typedef WmsLiteRaster *WmsLiteRasterPtr;

typedef struct wms_lite_vector
{
/* a struct wrapping a WmsLite Vector Layer */
    char *FTableName;
    char *FGeometryColumn;
    char *ViewName;
    char *ViewGeometry;
    char *VirtName;
    char *VirtGeometry;
    char *TopologyName;
    char *NetworkName;
    char *Title;
    char *Abstract;
    int Srid;
    WmsLiteBBoxPtr GeographicBBox;
    int IsQueryable;
    WmsLiteKeywordPtr KeyFirst;
    WmsLiteKeywordPtr KeyLast;
    WmsLiteCrsPtr CrsFirst;
    WmsLiteCrsPtr CrsLast;
    WmsLiteStylePtr StyleFirst;
    WmsLiteStylePtr StyleLast;
} WmsLiteVector;
typedef WmsLiteVector *WmsLiteVectorPtr;

typedef struct wms_lite_map_config_srid
{
/* a struct wrapping a single MapConfig SRID */
    int Srid;
    WmsLiteBBoxPtr BBox;
    struct wms_lite_map_config_srid *Next;
} WmsLiteMapConfigSRID;
typedef WmsLiteMapConfigSRID *WmsLiteMapConfigSRIDPtr;

typedef struct wms_lite_map_config_multisrid
{
/* a struct wrapping multiples MapConfig SRIDs */
    int Srid;
    WmsLiteBBoxPtr GeographicBBox;
    WmsLiteMapConfigSRIDPtr First;
    WmsLiteMapConfigSRIDPtr Last;
} WmsLiteMapConfigMultiSRID;
typedef WmsLiteMapConfigMultiSRID *WmsLiteMapConfigMultiSRIDPtr;

typedef struct wms_lite_srid
{
/* a struct wrapping a SRID */
    int Srid;
    struct wms_lite_srid *Next;
} WmsLiteSrid;
typedef WmsLiteSrid *WmsLiteSridPtr;

typedef struct wms_lite_srid_list
{
/* a struct wrapping a list of Srids */
    WmsLiteSridPtr First;
    WmsLiteSridPtr Last;
} WmsLiteSridList;
typedef WmsLiteSridList *WmsLiteSridListPtr;

typedef struct wms_lite_layer
{
/* a struct wrapping a WmsLite Layer */
    char *AliasName;
    char Type;
    char *Name;
    rl2MapConfigPtr MapConfig;
    WmsLiteMapConfigMultiSRIDPtr MapConfigSrids;
    WmsLiteCascadedWmsPtr CascadedWMS;
    WmsLiteRasterPtr Raster;
    WmsLiteVectorPtr Vector;
    WmsLiteChildLayerPtr First;
    WmsLiteChildLayerPtr Last;
    double MinScaleDenominator;
    double MaxScaleDenominator;
    int ChildLayer;
    struct wms_lite_layer *Next;
} WmsLiteLayer;
typedef WmsLiteLayer *WmsLiteLayerPtr;

typedef struct wms_lite_attached_layer
{
/* a struct wrapping a WmsLite AttachedLayer */
    char *AliasName;
    char Type;
    char *Name;
    char *WmsUrl;
    WmsLiteCascadedWmsPtr CascadedWMS;
    WmsLiteRasterPtr Raster;
    WmsLiteVectorPtr Vector;
    double MinScaleDenominator;
    double MaxScaleDenominator;
    int ChildLayer;
    struct wms_lite_attached_layer *Next;
} WmsLiteAttachedLayer;
typedef WmsLiteAttachedLayer *WmsLiteAttachedLayerPtr;

typedef struct wms_lite_attached
{
/* a struct wrapping a WmsLite Attached DB */
    char *DbPrefix;
    char *Path;
    int Valid;
    WmsLiteAttachedLayerPtr First;
    WmsLiteAttachedLayerPtr Last;
    struct wms_lite_attached *Next;
} WmsLiteAttached;
typedef WmsLiteAttached *WmsLiteAttachedPtr;

typedef struct wms_lite_connection
{
/* a struct wrapping a DB connection */
    void *splite_privdata;
    void *rl2_privdata;
    sqlite3 *handle;
    sqlite3_stmt *stmt_vector;
    sqlite3_stmt *stmt_raster;
    sqlite3_stmt *stmt_config;
    sqlite3_stmt *stmt_wms;
    CURL *curl;
    int status;
} WmsLiteConnection;
typedef WmsLiteConnection *WmsLiteConnectionPtr;

typedef struct wms_lite_connections_pool
{
/* a struct wrapping a Pool of DB connections */
    int count;
    WmsLiteConnectionPtr connections;
} WmsLiteConnectionsPool;
typedef WmsLiteConnectionsPool *WmsLiteConnectionsPoolPtr;

typedef struct wms_lite_configuration
{
/* a struct wrapping a WmsLite configuration */
    int IsMiniServer;
    int PendingShutdown;
    char *Path;
    int MultithreadEnabled;
    int MaxThreads;
    int WmsMaxRetries;
    int WmsPause;
    unsigned char BackgroundRed;
    unsigned char BackgroundGreen;
    unsigned char BackgroundBlue;
    unsigned char Transparent;
    unsigned char LabelAntiCollision;
    unsigned char LabelWrapText;
    unsigned char LabelAutoRotate;
    unsigned char LabelShiftPosition;
    char *Name;
    char *Title;
    char *Abstract;
    char *OnlineResource;
    char *ContactPerson;
    char *ContactOrganization;
    char *ContactPosition;
    char *AddressType;
    char *Address;
    char *City;
    char *State;
    char *PostCode;
    char *Country;
    char *ContactEMail;
    char *Fees;
    char *AccessConstraints;
    int LayerLimit;
    int MaxWidth;
    int MaxHeight;
    int TotalLayersCount;
    char *TopLayerName;
    char *TopLayerTitle;
    char *TopLayerAbstract;
    double TopLayerMinScale;
    double TopLayerMaxScale;
    WmsLiteLayerPtr TopLayerRef;
    WmsLiteAttachedLayerPtr TopAttachedLayerRef;
    int EnableLegendURL;
    int LegendWidth;
    int LegendHeight;
    char *LegendFormat;
    char *LegendFontName;
    double LegendFontSize;
    int LegendFontItalic;
    int LegendFontBold;
    char *LegendFontColor;
    char *MainDbPath;
    WmsLiteConnection Connection;
    WmsLiteKeywordPtr KeyFirst;
    WmsLiteKeywordPtr KeyLast;
    WmsLiteLayerPtr MainFirst;
    WmsLiteLayerPtr MainLast;
    WmsLiteAttachedPtr DbFirst;
    WmsLiteAttachedPtr DbLast;
    void *Capabilities_100;
    void *Capabilities_110;
    void *Capabilities_111;
    void *Capabilities_130;
    int Capabilities_100_Length;
    int Capabilities_110_Length;
    int Capabilities_111_Length;
    int Capabilities_130_Length;
} WmsLiteConfig;
typedef WmsLiteConfig *WmsLiteConfigPtr;

typedef struct wms_lite_styled_layer
{
/* a struct wrapping a WMS Styled Layer */
    char *LayerName;
    char *StyleName;
    struct wms_lite_styled_layer *Next;
} WmsLiteStyledLayer;
typedef WmsLiteStyledLayer *WmsLiteStyledLayerPtr;

typedef struct wms_lite_layers_list
{
/* a struct wrapping a list of WMS Styled Layers */
    WmsLiteStyledLayerPtr first;
    WmsLiteStyledLayerPtr last;
} WmsLiteLayersList;
typedef WmsLiteLayersList *WmsLiteLayersListPtr;

typedef struct wms_lite_argument
{
/* a struct wrapping a single WMS arg */
    char *arg_name;
    char *arg_value;
    struct wms_lite_argument *next;
} WmsLiteArgument;
typedef WmsLiteArgument *WmsLiteArgumentPtr;

typedef struct http_request
{
/* a struct wrapping an HTTP request */
    unsigned int id;		/* request ID */
    const char *ip_addr;
    int port_no;
    struct neutral_socket socket;	/* Socket on which to receive/send data */
    WmsLiteConfigPtr config;
    WmsLiteConnectionPtr conn;
    char *protocol;
    char *client_ip_addr;
    int client_ip_port;
    char *user_agent;
    char *request_url;
    char *request_method;
    int content_length;
    WmsLiteArgumentPtr first_arg;
    WmsLiteArgumentPtr last_arg;
    char *begin_time;
    char *end_time;
    int http_status;
    int service_wms;
    int request_type;
    int exceptions;
    int version_1;
    int version_2;
    int version_3;
    int ok_version;
    WmsLiteLayersListPtr layers_list;
    WmsLiteBBoxPtr bbox;
    int srid;
    int width;
    int height;
    int format;
    int transparent;
    int reaspect;
    unsigned char bg_red;
    unsigned char bg_green;
    unsigned char bg_blue;
    WmsLiteLayersListPtr query_layers;
    int info_format;
    int feature_count;
    int point_x;
    int point_y;
    void *http_response;
    void (*freeor) (void *);
    int http_content_length;
    int http_mime_type;
    const char *param_version;
    const char *param_layers;
    const char *param_styles;
    const char *param_legend_layer;
    const char *param_legend_style;
    const char *param_crs;
    const char *param_bbox;
    const char *param_width;
    const char *param_height;
    const char *param_format;
    const char *param_transparent;
    const char *param_bgcolor;
    const char *param_reaspect;
    const char *param_query_layers;
    const char *param_info_format;
    const char *param_feature_count;
    const char *param_point_x;
    const char *param_point_y;
} WmsLiteHttpRequest;
typedef WmsLiteHttpRequest *WmsLiteHttpRequestPtr;

extern WmsLiteConfigPtr wmslite_parse_config (const char *path);
extern char *wmslite_create_getcapabilities (WmsLiteConfigPtr config,
					     int version);
extern void destroy_wmslite_config (WmsLiteConfigPtr config);
extern int wmslite_validate_config (WmsLiteConfigPtr config);
extern WmsLiteRasterPtr create_wmslite_raster (const char *title,
					       const char *abstract, int srid,
					       int is_queryable);
extern WmsLiteVectorPtr create_wmslite_vector (const char *f_table_name,
					       const char *f_geometry_column,
					       const char *view_name,
					       const char *view_geometry,
					       const char *virt_name,
					       const char *virt_geometry,
					       const char *topology_name,
					       const char *network_name,
					       const char *title,
					       const char *abstract,
					       int is_queryable);
extern WmsLiteCascadedWmsPtr create_wmslite_cascaded_wms (int id,
							  const char *title,
							  const char *abstract,
							  int is_transparent,
							  int is_queryable,
							  int cascaded);
extern WmsLiteBBoxPtr create_wmslite_bbox (double minx, double miny,
					   double maxx, double maxy);
extern WmsLiteKeywordPtr add_raster_keyword (WmsLiteRasterPtr raster,
					     const char *keyword);
extern WmsLiteKeywordPtr add_vector_keyword (WmsLiteVectorPtr vector,
					     const char *keyword);
extern WmsLiteCrsPtr add_raster_crs (WmsLiteRasterPtr raster, int srid,
				     double minx, double miny, double maxx,
				     double maxy);
extern WmsLiteCrsPtr add_vector_crs (WmsLiteVectorPtr vector, int srid,
				     double minx, double miny, double maxx,
				     double maxy);
extern WmsLiteWmsCrsPtr add_cascaded_wms_crs (WmsLiteCascadedWmsPtr wms,
					      const char *srs, double minx,
					      double miny, double maxx,
					      double maxy, int is_default);
extern WmsLiteStylePtr add_raster_style (WmsLiteRasterPtr raster,
					 const char *name, const char *title,
					 const char *abstract);
extern WmsLiteStylePtr add_vector_style (WmsLiteVectorPtr vector,
					 const char *name, const char *title,
					 const char *abstract);
extern WmsLiteStylePtr add_cascaded_wms_style (WmsLiteCascadedWmsPtr wms,
					       const char *name,
					       const char *title,
					       const char *abstract);

extern void destroy_wmslite_cascaded_wms (WmsLiteCascadedWmsPtr lyr);
extern void destroy_wmslite_raster (WmsLiteRasterPtr lyr);
extern void destroy_wmslite_vector (WmsLiteVectorPtr lyr);
extern int is_valid_wmslite_layer (WmsLiteLayerPtr lyr);
extern int is_valid_wmslite_attached_layer (WmsLiteAttachedLayerPtr lyr);
extern int is_queryable_wmslite_layer (WmsLiteLayerPtr lyr);
extern int is_queryable_wmslite_attached_layer (WmsLiteAttachedLayerPtr lyr);
extern void do_set_cascaded_wms_srs (sqlite3 * sqlite,
				     WmsLiteCascadedWmsPtr lyr);
extern WmsLiteMapConfigMultiSRIDPtr create_map_config_multi_srid (int srid);
extern void destroy_map_config_multi_srid (WmsLiteMapConfigMultiSRIDPtr multi);

extern void start_cgi (WmsLiteConfigPtr config, WmsLiteConnectionsPoolPtr pool);
extern void start_miniserver (WmsLiteConfigPtr config,
			      const char *ip_addr, int port_no,
			      WmsLiteConnectionsPoolPtr pool);
extern void clean_shutdown ();
extern char *url_decode (CURL * curl, const char *encoded_url);
extern void connection_init (WmsLiteConnectionPtr conn,
			     WmsLiteConfigPtr config);
extern void parse_request_args (WmsLiteHttpRequestPtr req,
				const char *query_string, int len);
extern int add_wms_argument (WmsLiteHttpRequestPtr req, const char *token);
extern void destroy_wms_argument (WmsLiteArgumentPtr arg);
extern WmsLiteArgumentPtr alloc_wms_argument (char *name, char *value);
extern WmsLiteHttpRequestPtr create_http_request (int id,
						  WmsLiteConfigPtr config,
						  const char *server_addr,
						  int port_no,
						  struct neutral_socket
						  *socket);
extern void destroy_http_request (WmsLiteHttpRequestPtr req);
extern char *get_timestamp ();
extern WmsLiteConnectionsPoolPtr alloc_connections_pool (WmsLiteConfigPtr
							 config,
							 int max_connections);
extern void destroy_connections_pool (WmsLiteConnectionsPoolPtr pool);
extern int server_main (int argc, char *argv[]);
extern void process_http_request (WmsLiteHttpRequestPtr req);
extern void do_get_capabilities (WmsLiteHttpRequestPtr req);
extern void do_update_logfile (WmsLiteHttpRequestPtr req);
