#include <fcgi_stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "wmslite.h"

static void
initialize (const char *config_path, WmsLiteConfigPtr * config,
	    WmsLiteConnectionsPoolPtr * pool)
{
/* initializing the FastCGI component */


/* attempting to parse the XML configuration */
    *config = wmslite_parse_config (config_path);
    if (*config == NULL)
      {
	  fprintf (stderr,
		   "WmsLite: unable to parse a valid XML configuration ... quitting\n");
	  fflush (stderr);
	  return;
      }

/* attempting to validate the XML configuration */
    if (!wmslite_validate_config (*config))
      {
	  fprintf (stderr, "WmsLite: invalid XML configuration ... quitting\n");
	  fflush (stderr);
	  return;
      }

/* creating the read connections pool */
    *pool = alloc_connections_pool (*config, 1);
    if (*pool == NULL)
      {
	  fprintf (stderr,
		   "WmsLite: unable to initialize a connections pool\n");
	  fflush (stderr);
	  return;
      }
}

static char *
save_envvar (const char *str)
{
/* allocating a string corresponding to some CGI env-var */
    int len;
    char *save;

    if (str == NULL)
	return NULL;

    len = strlen (str);
    save = malloc (len + 1);
    strcpy (save, str);
    return save;
}

static WmsLiteHttpRequestPtr
start_cgi_request (WmsLiteConfigPtr config, WmsLiteConnectionsPoolPtr pool,
		   int count)
{
/* processing a CGI / FastCGI request */
    int ic;
    char *server_addr;
    int port_no = -1;
    const char *var;
    WmsLiteConnectionPtr conn;
    WmsLiteHttpRequestPtr req;

    server_addr = getenv ("SERVER_NAME");
    var = getenv ("SERVER_PORT");
    if (var != NULL)
	port_no = atoi (var);
    req = create_http_request (count, config, server_addr, port_no, NULL);
    while (1)
      {
	  /* looping until an available read connection is found */
	  for (ic = 0; ic < pool->count; ic++)
	    {
		/* selecting an available connection (if any) */
		WmsLiteConnectionPtr ptr = &(pool->connections[ic]);
		if (ptr->status == CONNECTION_AVAILABLE)
		  {
		      conn = ptr;
		      conn->status = CONNECTION_BUSY;
		      goto conn_found;
		  }
	    }
	  usleep (50);
      }
  conn_found:
    req->conn = conn;
    req->client_ip_addr = save_envvar (getenv ("REMOTE_ADDR"));
    var = getenv ("REMOTE_PORT");;
    if (var != NULL)
	req->client_ip_port = atoi (var);
    req->protocol = save_envvar (getenv ("SERVER_PROTOCOL"));
    req->request_method = save_envvar (getenv ("REQUEST_METHOD"));
    req->user_agent = save_envvar (getenv ("HTTP_USER_AGENT"));
    req->request_url = save_envvar (getenv ("REQUEST_URI"));
    var = getenv ("CONTENT_LENGTH");
    if (var != NULL)
	req->content_length = atoi (var);
    if (req->request_method == NULL)
	goto skip;
    if (strcmp (req->request_method, "POST") == 0)
      {
	  /* method POST */
	  char *args = malloc (req->content_length + 1);
	  memset (args, '\0', req->content_length + 1);
	  fread (args, 1, req->content_length, stdin);
	  parse_request_args (req, args, strlen (args));
	  free (args);
      }
    else
      {
	  /* method GET */
	  const char *args = getenv ("QUERY_STRING");
	  int len = strlen (args);
	  parse_request_args (req, args, len);
      }

/* processing the HTTP request */
    process_http_request (req);

  skip:
    conn->status = CONNECTION_AVAILABLE;
    return req;
}

/*
static char *
create_error_response ()
{
/ creating a generic error response /
    const char *html;
    char *http;
    int content_length;

    html = "<html><head><title>WmsLiteCgi Error</title>\r\n"
	"<body><h1>WmsLiteCgi Error !!!</h1>\r\n"
	"Unable to get a valid configuration\r\n" "</body></html>\r\n";
    content_length = strlen (html);
    http = sqlite3_mprintf ("Content-Type: text/html;charset=UTF-8\r\n"
			    "Content-Length: %d\r\n"
			    "Connection: close\r\n\r\n%s", content_length,
			    html);
    return http;
}
*/

int
main (int argc, char *argv[])
{
/* common entry point */
    WmsLiteConfigPtr config = NULL;
    WmsLiteConnectionsPoolPtr pool;
    WmsLiteHttpRequestPtr req;
    const char *config_path = "./WmsLiteConfig.xml";
    const char *mime_type;
    int count = 0;
    char *response;
    int server_mode = 0;
    int i;

    for (i = 1; i < argc; i++)
      {
	  /* parsing the invocation arguments (if any) */
	  if (strcasecmp (argv[i], "--mini-server") == 0
	      || strcmp (argv[i], "-svr") == 0)
	      server_mode = 1;
      }
    if (server_mode)
      {
	  /* mini-server entry point */
	  return server_main (argc, argv);
      }

/* FastCGI entry point */
    while (FCGI_Accept () >= 0)
      {
	  /* accepting a CGI/FastCGI request */
	  if (count == 0)
	    {
		/* 
		 * this is the first invocation of FastCGI (or is simple CGI) 
		 * we need to iniatialize the context
		 */
		initialize (config_path, &config, &pool);
	    }

	  req = start_cgi_request (config, pool, count);
	  if (req == NULL)
	    {
		/* should never happen: preparing a generic HTTP error message */
		const char *iobuf =
		    "<html><head><title>WmsLite internal error</title></head><body><h1>WmsLite: something absolutely unexpected happened !!!</h1></body></html>\r\n";
		response =
		    sqlite3_mprintf
		    ("Content-type: text/html; charset=UTF-8\r\n"
		     "Content-Length: %d\r\nConnection: close\r\n\r\n%s",
		     strlen (iobuf), iobuf);
		printf (response);
		sqlite3_free (response);
	    }
	  else
	    {
		/* sending the HTTP fheaders */
		switch (req->http_mime_type)
		  {
		  case MIME_HTML:
		      mime_type = "text/html; charset=UTF-8";
		      break;
		  case MIME_XML:
		      mime_type = "text/xml; charset=UTF-8";
		      break;
		  case MIME_PNG:
		      mime_type = "image/png";
		      break;
		  case MIME_JPEG:
		      mime_type = "image/jpeg";
		      break;
		  case MIME_TIFF:
		      mime_type = "image/tiff";
		      break;
		  case MIME_PDF:
		      mime_type = "application/x-pdf";
		      break;
		  default:
		      mime_type = "text/plain; charset=UTF-8";
		      break;
		  };
		response =
		    sqlite3_mprintf
		    ("Content-type: %s\r\n"
		     "Content-Length: %d\r\nConnection: close\r\n\r\n%s",
		     mime_type, req->http_content_length, req->http_response);
		printf (response);
		sqlite3_free (response);
		destroy_http_request (req);
	    }
	  count++;
      }

/* releasing all resources */
    destroy_connections_pool (pool);
    destroy_wmslite_config (config);
    return 0;
}
