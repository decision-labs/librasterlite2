/* 
/ wmslite
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

#include "wmslite.h"

#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <float.h>

#ifdef _WIN32
/* This code is for win32 only */
#include <windows.h>
#include <process.h>
#include <io.h>
#else
/* this code is for any sane minded system (*nix) */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#endif

#ifdef _WIN32
BOOL WINAPI
signal_handler (DWORD dwCtrlType)
{
/* intercepting some Windows signal */
    clean_shutdown ();
    return FALSE;
}
#else
static void
signal_handler (int signo)
{
/* intercepting some signal */
    if (signo == SIGINT)
	signo = SIGINT;		/* suppressing compiler warnings */
    clean_shutdown ();
    exit (0);
}
#endif

static void
parse_http_request (WmsLiteHttpRequestPtr req, const char *http_hdr)
{
/* attempting to parse an HTTP Request */
    int ok = 1;
    int len;
    char token[2000];
    char *url = NULL;
    char *out;
    const char *p;
    const char *start = strstr (http_hdr, "GET ");
    const char *end = NULL;
    if (start == NULL)
	ok = 0;
    if (ok)
      {
	  end = strstr (start, " HTTP/1.1");
	  if (end == NULL)
	    {
		end = strstr (start, " HTTP/1.0");
		if (end == NULL)
		    ;
		else
		  {
		      req->protocol = malloc (9);
		      strcpy (req->protocol, "HTTP/1.0");
		  }
	    }
	  else
	    {
		req->protocol = malloc (9);
		strcpy (req->protocol, "HTTP/1.1");
	    }
      }

    if (ok)
      {
	  req->request_method = malloc (4);
	  strcpy (req->request_method, "GET");
	  len = end - start;
	  len -= 4;
	  url = malloc (end - start);
	  memcpy (url, start + 4, len);
	  *(url + len) = '\0';
      }
    else
      {
	  end = NULL;
	  start = strstr (http_hdr, "POST ");
	  if (start != NULL)
	    {
		end = strstr (start, " HTTP/1.1");
		if (end == NULL)
		  {
		      end = strstr (start, " HTTP/1.0");
		      if (end == NULL)
			  ;
		      else
			{
			    req->protocol = malloc (9);
			    strcpy (req->protocol, "HTTP/1.0");
			}
		  }
		else
		  {
		      req->protocol = malloc (9);
		      strcpy (req->protocol, "HTTP/1.1");
		  }
	    }
	  req->request_method = malloc (5);
	  strcpy (req->request_method, "POST");
	  len = end - start;
	  len -= 5;
	  len++;
	  url = malloc (end - start);
	  memcpy (url, start + 5, len);
	  *(url + len) = '\0';
      }

    if (strcasecmp (req->request_method, "GET") == 0)
      {
	  /* method GET */
	  start = url + 1;
	  end = start + strlen (url) - 10;
	  p = start;
	  out = token;
	  while (p < end)
	    {
		if (*p == '?')
		  {
		      out = token;
		      p++;
		      break;
		  }
		else
		    *out++ = *p++;
	    }
	  len = strlen (p);
	  parse_request_args (req, p, len);
      }

    if (strcasecmp (req->request_method, "POST") == 0)
      {
	  /* retrieving the Content-Length */
	  int i;
	  char *req_string;
	  char dummy[1024];
	  start = strstr (http_hdr, "Content-Length: ");
	  if (start == NULL)
	      goto request_url;
	  start += 16;
	  end = strstr (start, "\r\n");
	  if (end == NULL)
	      goto request_url;
	  len = end - start;
	  memcpy (dummy, start, len);
	  *(dummy + len) = '\0';
	  req->content_length = atoi (dummy);

	  /* extracting the request string */
	  start = strstr (http_hdr, "\r\n\r\n");
	  if (start == NULL)
	      goto request_url;
	  p = start + 4;
	  req_string = malloc (req->content_length + 1);
	  for (i = 0; i < req->content_length; i++)
	    {
		req_string[i] = *p++;
	    }
	  *(req_string + req->content_length) = '\0';
	  parse_request_args (req, req_string, req->content_length);
	  free (req_string);
      }

  request_url:
/* retrieving the Request URL */
    if (strcasecmp (req->request_method, "POST") == 0)
      {
	  start = strstr (http_hdr, "POST /");
	  if (start == NULL)
	      goto user_agent;
	  p = start;
      }
    else
      {
	  start = strstr (http_hdr, "GET /");
	  if (start == NULL)
	      goto user_agent;
	  p = start;
      }
    if (start == NULL)
	goto user_agent;
    end = strstr (start, "\r\n");
    if (end == NULL)
	goto user_agent;
    len = end - start;
    req->request_url = malloc (len + 1);
    memcpy (req->request_url, start, len);
    *(req->request_url + len) = '\0';

  user_agent:
/* retrieving the User Agent */
    start = strstr (http_hdr, "User-Agent: ");
    if (start == NULL)
	goto stop;
    start += 12;
    end = strstr (start, "\r\n");
    if (end == NULL)
	goto stop;
    len = end - start;
    req->user_agent = malloc (len + 1);
    memcpy (req->user_agent, start, len);
    *(req->user_agent + len) = '\0';
    if (url != NULL)
	free (url);
    return;

  stop:
    if (url != NULL)
	free (url);
    return;
}

#ifdef _WIN32
/* Winsockets - some kind of Windows */
static void
win32_http_request (void *data)
{
/* Processing an incoming HTTP request */
    WmsLiteHttpRequestPtr req = (WmsLiteHttpRequestPtr) data;
    int curr;
    int rd;
    char *ptr;
    char http_hdr[2000];	/* The HTTP request header */
    const char *iobuf;
    char *response;
    const char *mime_type;
    char *headers;

    curr = 0;
    while ((unsigned int) curr < sizeof (http_hdr))
      {
	  rd = recv (req->socket.socket, &http_hdr[curr],
		     sizeof (http_hdr) - 1 - curr, 0);
	  if (rd == SOCKET_ERROR)
	      goto end_request;
	  if (rd == 0)
	    {
		req->http_status = 400;
		goto http_error;
	    }
	  curr += rd;
	  http_hdr[curr] = '\0';
	  ptr = strstr (http_hdr, "\r\n\r\n");
	  if (ptr)
	      break;
      }
    if ((unsigned int) curr >= sizeof (http_hdr))
      {
	  req->http_status = 400;
	  goto http_error;
      }

/* parsing the HTTP request */
    parse_http_request (req, http_hdr);
/* processing the HTTP request */
    process_http_request (req);
    if (req->http_status != 200 || req->http_response == NULL)
	goto http_error;
/* sending the HTTP full headers */
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
    headers =
	sqlite3_mprintf
	("HTTP/1.1 200 OK\r\nContent-type: %s\r\n"
	 "Content-Length: %d\r\nConnection: close\r\n\r\n",
	 mime_type, req->http_content_length);
    send (req->socket.socket, headers, strlen (headers), 0);
    sqlite3_free (headers);
/* sending the HTTP response to the client */
    send (req->socket.socket, req->http_response, req->http_content_length, 0);
    goto end_request;
  http_error:
/* should never happen: preparing a generic HTTP error message */
    iobuf =
	"<html><head><title>WmsLite internal error</title></head><body><h1>WmsLite: something absolutely unexpected happened !!!</h1></body></html>\r\n";
    response =
	sqlite3_mprintf
	("HTTP/1.1 200 OK\r\nContent-type: text/html; charset=UTF-8\r\n\r\n"
	 "Content-Length: %d\r\nConnection: close\r\n\r\n%s", strlen (iobuf),
	 iobuf);
    send (req->socket.socket, response, strlen (response), 0);
    sqlite3_free (response);
    req->end_time = get_timestamp ();
  end_request:
    closesocket (req->socket.socket);
    req->conn->status = CONNECTION_AVAILABLE;
    do_update_logfile (req);
    destroy_http_request (req);
}
#else
/* standard Berkeley Sockets - may be Linux or *nix */
static void *
berkeley_http_request (void *data)
{
/* Processing an incoming HTTP request */
    WmsLiteHttpRequestPtr req = (WmsLiteHttpRequestPtr) data;
    int curr;
    int rd;
    char *ptr;
    char http_hdr[2000];	/* The HTTP request header */
    const char *iobuf;
    char *response;
    const char *mime_type;
    char *headers;

    curr = 0;
    while ((unsigned int) curr < sizeof (http_hdr))
      {
	  rd = recv (req->socket.socket, &http_hdr[curr],
		     sizeof (http_hdr) - 1 - curr, 0);
	  if (rd == -1)
	      goto end_request;
	  if (rd == 0)
	    {
		req->http_status = 400;
		goto http_error;
	    }
	  curr += rd;
	  http_hdr[curr] = '\0';
	  ptr = strstr (http_hdr, "\r\n\r\n");
	  if (ptr)
	      break;
      }
    if ((unsigned int) curr >= sizeof (http_hdr))
      {
	  req->http_status = 400;
	  goto http_error;
      }

/* parsing the HTTP request */
    parse_http_request (req, http_hdr);
/* processing the HTTP request */
    process_http_request (req);

    if (req->http_status != 200 || req->http_response == NULL)
	goto http_error;

/* sending the HTTP full headers */
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
    headers =
	sqlite3_mprintf
	("HTTP/1.1 200 OK\r\nContent-type: %s\r\n"
	 "Content-Length: %d\r\nConnection: close\r\n\r\n", mime_type,
	 req->http_content_length);
    send (req->socket.socket, headers, strlen (headers), 0);
    sqlite3_free (headers);

/* sending the HTTP response to the client */
    send (req->socket.socket, req->http_response, req->http_content_length, 0);
    goto end_request;

  http_error:
/* should never happen: preparing a generic HTTP error message */
    iobuf =
	"<html><head><title>WmsLite internal error</title></head><body><h1>WmsLite: something absolutely unexpected happened !!!</h1></body></html>\r\n";
    response =
	sqlite3_mprintf
	("HTTP/1.1 200 OK\r\nContent-type: text/html; charset=UTF-8\r\n\r\n"
	 "Content-Length: %d\r\nConnection: close\r\n\r\n%s", strlen (iobuf),
	 iobuf);
    send (req->socket.socket, response, strlen (response), 0);
    sqlite3_free (response);
    req->end_time = get_timestamp ();

  end_request:
    close (req->socket.socket);
    req->conn->status = CONNECTION_AVAILABLE;
    do_update_logfile (req);
    destroy_http_request (req);

    pthread_detach (pthread_self ());
    pthread_exit (NULL);
}
#endif

static int
do_start_http (WmsLiteConfigPtr config, const char *ip_addr, int port_no,
	       struct neutral_socket *srv_skt, int max_conn)
{
/* starting the HTTP server */

#ifdef _WIN32
/* Winsockets */
    WSADATA wd;
    SOCKET skt = INVALID_SOCKET;
    SOCKADDR_IN addr;

    if (WSAStartup (MAKEWORD (1, 1), &wd))
      {
	  fprintf (stderr, "WmsLite: unable to initialize winsock\n");
	  fflush (stderr);
	  return 0;
      }
    skt = socket (AF_INET, SOCK_STREAM, 0);
    if (skt == INVALID_SOCKET)
      {
	  fprintf (stderr, "WmsLite: unable to create a socket\n");
	  fflush (stderr);
	  return 0;
      }
    addr.sin_family = AF_INET;
    addr.sin_port = htons (port_no);
    addr.sin_addr.s_addr = inet_addr (ip_addr);
    if (bind (skt, (struct sockaddr *) &addr, sizeof (addr)) == SOCKET_ERROR)
      {
	  fprintf (stderr, "WmsLite: unable to bind the socket\n");
	  fflush (stderr);
	  closesocket (skt);
	  return 0;
      }
    if (listen (skt, SOMAXCONN) == SOCKET_ERROR)
      {
	  fprintf (stderr, "WmsLite: unable to listen on the socket\n");
	  fflush (stderr);
	  closesocket (skt);
	  return 0;
      }
    srv_skt->socket = skt;
#else
/* standard Berkeley sockets */
    int skt = -1;
    struct sockaddr_in addr;

    skt = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (skt < 0)
      {
	  fprintf (stderr, "WmsLite: unable to create a socket\n");
	  fflush (stderr);
	  return 0;
      }
    memset (&addr, 0, sizeof (addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons (port_no);
    addr.sin_addr.s_addr = inet_addr (ip_addr);
    if (bind (skt, (struct sockaddr *) &addr, sizeof (addr)) < 0)
      {
	  fprintf (stderr, "WmsLite: unable to bind the socket\n");
	  fflush (stderr);
	  close (skt);
	  return 0;
      }
    if (listen (skt, SOMAXCONN) < 0)
      {
	  fprintf (stderr, "WmsLite: unable to listen on the socket\n");
	  fflush (stderr);
	  close (skt);
	  return 0;
      }
    srv_skt->socket = skt;
#endif

    printf
	("================================================================================\n");
    printf ("          WmsLite HTTP micro-server listening on port: %d\n",
	    port_no);
    printf ("              ConfigFile: %s\n", config->Path);
    printf ("              Max Connections: %d\n", max_conn);
    if (config->MultithreadEnabled)
	printf ("              RasterLite2 MaxThreads: %d\n",
		config->MaxThreads);
    else
	printf ("              RasterLite2 MaxThreads: 1\n");
    printf
	("================================================================================\n");
    printf ("HINT: test the following URL on your preferred web browser:\n\n");
    printf
	("http://localhost:%d/wmslite?SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities\n",
	 port_no);
    printf ("  or\n");
    printf
	("http://127.0.0.1:%d/wmslite?SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities\n",
	 port_no);
    printf
	("================================================================================\n\n");
    fflush (stdout);
    return 1;
}

static void
do_accept_loop (struct neutral_socket *skt, const char *xip_addr, int port_no,
		WmsLiteConfigPtr config, WmsLiteConnectionsPoolPtr pool)
{
/* implementing the ACCEPT loop */
    unsigned int id = 0;
    WmsLiteConnectionPtr conn;
    int ic;
    char *ip_addr;
    char *ip_addr2;
    struct neutral_socket neutral;

#ifdef _WIN32			/* using Windows Sokets */
    SOCKET socket = skt->socket;
    SOCKET client;
    SOCKADDR_IN client_addr;
    int len = sizeof (client_addr);
    int wsaError;
    struct http_request *req;

    while (1)
      {
	  /* never ending loop */
	  if (config->PendingShutdown)
	      break;		/* breaking on Pending Shutdown request */

	  client = accept (socket, (struct sockaddr *) &client_addr, &len);
	  if (client == INVALID_SOCKET)
	    {
		wsaError = WSAGetLastError ();
		if (wsaError == WSAEINTR || wsaError == WSAENOTSOCK)
		  {
		      WSACleanup ();
		      fprintf (stderr, "WmsLite: accept error: %d\n", wsaError);
		      fflush (stderr);
		      return;
		  }
		else
		  {
		      closesocket (socket);
		      WSACleanup ();
		      fprintf (stderr, "WmsLite: error from accept()\n");
		      fflush (stderr);
		      return;
		  }
	    }

	  /* allocating an HTTP Request object */
	  neutral.socket = client;
	  req = create_http_request (id++, config, xip_addr, port_no, &neutral);
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
		Sleep (50);
	    }
	conn_found:
	  req->conn = conn;
	  ip_addr = inet_ntoa (client_addr.sin_addr);
	  ip_addr2 = NULL;
	  if (ip_addr != NULL)
	    {
		int len = strlen (ip_addr);
		ip_addr2 = malloc (len + 1);
		strcpy (ip_addr2, ip_addr);
	    }
	  req->client_ip_addr = ip_addr2;
	  req->client_ip_port = client_addr.sin_port;

	  _beginthread (win32_http_request, 0, (void *) req);
      }
    return;
#else /* standard Berkeley sockets */
    pthread_t thread_id;
    int socket = skt->socket;
    int client;
    struct sockaddr_in client_addr;
    socklen_t len = sizeof (struct sockaddr_in);
    WmsLiteHttpRequestPtr req;

    while (1)
      {
	  /* never ending loop */
	  if (config->PendingShutdown)
	      break;		/* breaking on Pending Shutdown request */

	  client = accept (socket, (struct sockaddr *) &client_addr, &len);
	  if (client < 0)
	    {
		close (socket);
		fprintf (stderr, "WmsLite: error from accept()\n");
		fflush (stderr);
		return;
	    }

	  /* allocating an HTTP Request object */
	  neutral.socket = client;
	  req = create_http_request (id++, config, xip_addr, port_no, &neutral);
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
	  ip_addr = inet_ntoa (client_addr.sin_addr);
	  ip_addr2 = NULL;
	  if (ip_addr != NULL)
	    {
		int len = strlen (ip_addr);
		ip_addr2 = malloc (len + 1);
		strcpy (ip_addr2, ip_addr);
	    }
	  req->client_ip_addr = ip_addr2;
	  req->client_ip_port = client_addr.sin_port;

	  pthread_create (&thread_id, NULL, berkeley_http_request,
			  (void *) req);
      }
#endif /* end conditional implementation of Windows/Berkeley sockets */
}

extern void
start_miniserver (WmsLiteConfigPtr config, const char *ip_addr,
		  int port_no, WmsLiteConnectionsPoolPtr pool)
{
/* starting the WMS miniserver */
    struct neutral_socket skt_ptr;
    ;

#ifdef _WIN32
    SetConsoleCtrlHandler (signal_handler, TRUE);
#else
    signal (SIGINT, signal_handler);
    signal (SIGTERM, signal_handler);
    signal (SIGPIPE, SIG_IGN);
#endif

/* starting the HTTP server */
    if (!do_start_http (config, ip_addr, port_no, &skt_ptr, pool->count))
	return;

/* looping on requests */
    do_accept_loop (&skt_ptr, ip_addr, port_no, config, pool);
}

static void
do_version ()
{
/* printing version infos */
    printf ("\nVersion infos\n");
    printf ("===========================================\n");
    printf ("wmslite ........: %s\n", RL2_VERSION);
    printf ("target CPU .....: %s\n", rl2_target_cpu ());
    printf ("librasterlite2 .: %s\n", rl2_version ());
    printf ("libspatialite ..: %s\n", spatialite_version ());
    printf ("libsqlite3 .....: %s\n", sqlite3_libversion ());
    printf ("libcairo .......: %s\n", rl2_cairo_version ());
    printf ("libcurl ........: %s\n", rl2_curl_version ());
    printf ("DEFLATE ........: %s\n", rl2_zlib_version ());
    printf ("LZMA ...........: %s\n", rl2_lzma_version ());
    printf ("LZ4 ............: %s\n", rl2_lz4_version ());
    printf ("ZSTD ...........: %s\n", rl2_zstd_version ());
    printf ("PNG ............: %s\n", rl2_png_version ());
    printf ("JPEG ...........: %s\n", rl2_jpeg_version ());
    printf ("TIFF ...........: %s\n", rl2_tiff_version ());
    printf ("GeoTIFF ........: %s\n", rl2_geotiff_version ());
    printf ("WEBP ...........: %s\n", rl2_webp_version ());
    printf ("JPEG2000 .......: %s\n", rl2_openJPEG_version ());
    printf ("\n");
}

static void
do_help ()
{
/* printing the argument list */
    printf ("\n\nusage: wmslite [ ARGLIST ]\n");
    printf
	("=============================================================================\n");
    printf ("-svr or --mini-server               MANDATORY ARGUMENT !!!\n");
    printf ("-h or --help                        print this help message\n");
    printf ("-v or --version                     print version infos\n");
    printf
	("-cf or --config-file       path     XML config path [./WmsLiteConfig.xml]\n");
    printf
	("-al or --access-log        path     Access-Log path [default: stdout]\n");
    printf
	("-el or --error-log         path     Error-Log path  [default: stderr]\n");
    printf
	("-p or --ip-port             int     IP port number  [default: 8080]\n");
    printf
	("-c or --max-connections     int     max number of DB connections [default: 1]\n");
}

extern int
server_main (int argc, char *argv[])
{
/* the MAIN function simply perform arguments checking */
    int i;
    int error = 0;
    int next_arg = ARG_NONE;
    const char *config_file = "./WmsLiteConfig.xml";
    const char *access_log = NULL;
    const char *error_log = NULL;
    const char *ip_addr = "127.0.0.1";
    int port_no = 8080;
    int max_connections = 1;
    WmsLiteConfigPtr config = NULL;
    WmsLiteConnectionsPoolPtr pool;
#ifdef _WIN32
    int save_stdout = _dup (1);
    int save_stderr = _dup (2);
#else
    int save_stdout = dup (1);
    int save_stderr = dup (2);
#endif
    FILE *log = NULL;
    FILE *errlog = NULL;

    for (i = 1; i < argc; i++)
      {
	  /* parsing the invocation arguments */
	  if (next_arg != ARG_NONE)
	    {
		switch (next_arg)
		  {
		  case ARG_SERVER:
		      goto skip_arg;
		  case ARG_CONFIG_FILE:
		      config_file = argv[i];
		      break;
		  case ARG_ACCESS_LOG:
		      access_log = argv[i];
		      break;
		  case ARG_ERROR_LOG:
		      error_log = argv[i];
		      break;
		  case ARG_IP_PORT:
		      port_no = atoi (argv[i]);
		      break;
		  case ARG_MAX_CONN:
		      max_connections = atoi (argv[i]);
		      break;
		  };
		next_arg = ARG_NONE;
		continue;
	    }

	skip_arg:
	  if (strcasecmp (argv[i], "--mini-server") == 0
	      || strcmp (argv[i], "-svr") == 0)
	    {
		next_arg = ARG_SERVER;
		continue;
	    }

	  if (strcasecmp (argv[i], "--help") == 0
	      || strcmp (argv[i], "-h") == 0)
	    {
		do_help ();
		return -1;
	    }
	  if (strcasecmp (argv[i], "--version") == 0
	      || strcmp (argv[i], "-v") == 0)
	    {
		do_version ();
		return -1;
	    }
	  if (strcmp (argv[i], "-cf") == 0
	      || strcasecmp (argv[i], "--config-file") == 0)
	    {
		next_arg = ARG_CONFIG_FILE;
		continue;
	    }
	  if (strcmp (argv[i], "-al") == 0
	      || strcasecmp (argv[i], "--access-log") == 0)
	    {
		next_arg = ARG_ACCESS_LOG;
		continue;
	    }
	  if (strcmp (argv[i], "-el") == 0
	      || strcasecmp (argv[i], "--error-log") == 0)
	    {
		next_arg = ARG_ERROR_LOG;
		continue;
	    }
	  if (strcmp (argv[i], "-p") == 0
	      || strcasecmp (argv[i], "--ip-port") == 0)
	    {
		next_arg = ARG_IP_PORT;
		continue;
	    }
	  if (strcasecmp (argv[i], "--max-connections") == 0
	      || strcmp (argv[i], "-mc") == 0)
	    {
		next_arg = ARG_MAX_CONN;
		continue;
	    }
	  fprintf (stderr, "WmsLite: unknown argument: %s\n", argv[i]);
	  fflush (stderr);
	  error = 1;
      }
    if (error)
      {
	  do_help ();
	  return -1;
      }

/* attempting to parse the XML configuration */
    config = wmslite_parse_config (config_file);
    if (config == NULL)
      {
	  fprintf (stderr,
		   "WmsLite: unable to parse a valid XML configuration ... quitting\n");
	  fflush (stderr);
	  return -1;
      }

/* attempting to validate the XML configuration */
    if (!wmslite_validate_config (config))
      {
	  fprintf (stderr, "WmsLite: invalid XML configuration ... quitting\n");
	  fflush (stderr);
	  return -1;
      }

/* setting the MiniServer mode flag */
    config->IsMiniServer = 1;

/* creating the read connections pool */
    pool = alloc_connections_pool (config, max_connections);
    if (pool == NULL)
      {
	  fprintf (stderr,
		   "WmsLite: unable to initialize a connections pool\n");
	  fflush (stderr);
	  return -1;
      }

/* initializing libcurl */
    curl_global_init (CURL_GLOBAL_NOTHING);

/* starting the miniserver */
    if (access_log != NULL)
      {
	  /* redirecting stdout */
	  log = freopen (access_log, "wb", stdout);
	  if (log == NULL)
	    {
#ifdef _WIN32
		_dup2 (save_stdout, 1);
#else
		dup2 (save_stdout, 1);
#endif
		fprintf (stderr,
			 "WmsLite: unable to create the AccessLog \"%s\"\n",
			 access_log);
		fflush (stderr);
	    }
      }
    if (error_log != NULL)
      {
	  /* redirecting stderr */
	  errlog = freopen (error_log, "wb", stderr);
	  if (errlog == NULL)
	    {
#ifdef _WIN32
		_dup2 (save_stderr, 2);
#else
		dup2 (save_stderr, 2);
#endif
		fprintf (stderr,
			 "WmsLite: unable to create the ErrorLog \"%s\"\n",
			 error_log);
		fflush (stderr);
	    }
      }
    start_miniserver (config, ip_addr, port_no, pool);

    destroy_connections_pool (pool);
    destroy_wmslite_config (config);
    curl_global_cleanup ();
    if (log != NULL)
      {
	  fclose (log);
	  dup2 (save_stdout, 1);
      }
    if (errlog != NULL)
      {
	  fclose (errlog);
	  dup2 (save_stderr, 2);
      }
    return 0;
}
