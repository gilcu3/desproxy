/*
 * util.c: misc functions common to all desproxy binaries
 *
 * Copyright (C) 2003 Miguelanxo Otero Salgueiro
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the tems of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <stdarg.h>
#include <event.h>  
#include "desproxy.h"

/*
 * Function : encode_base64 (const char *string, char *p)
 * Purpose  : creates a base 64 encoded string
 * Params   : p - string to encode
 *          : s - string buffer to write into
 *
 * Note     : Taken from transconnect (http://transconnect.sourceforge.net/)
 */
static void
base64_encode (const char *s, char *p)
{
  char base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int i, length;

  length = strlen (s);
  for (i = 0; i < length; i += 3)
    {
      *p++ = base64[s[0] >> 2];
      *p++ = base64[((s[0] & 3) << 4) + (s[1] >> 4)];
      *p++ = base64[((s[1] & 0xf) << 2) + (s[2] >> 6)];
      *p++ = base64[s[2] & 0x3f];
      s += 3;
    }
  if (i == length + 1)
    *(p - 1) = '=';
  else if (i == length + 2)
    *(p - 1) = *(p - 2) = '=';
  *p = '\0';
}

/*
 * Function : debug_printf(char *fmt, ...)
 * Purpose  : prints arguments like printf if DEBUG is set, and
 *            adds fflush(stdout)
 * Params   : same as printf, as defined in <stdio.h>
 *        
 * Note     : this function obtained via IRC chat in 
 *            irc.openprojects.net, #c channel.
 */
int
debug_printf (const char *fmt, ...)
{
  int n;
  va_list ap;

  if (DEBUG)
    {
      va_start (ap, fmt);
      n = vprintf (fmt, ap);
      va_end (ap);
      fflush (stdout);
      return n;
    }
  return 0;
}

/*
 * Function : char *get_console_line (void)
 * Purpose  : gets a line from console to char console_line[256]
 * Params   : none
 */
char *
get_console_line (void)
{
  int count;

  fgets (console_line, 256, stdin);
  for (count = 0; count < strlen (console_line); count++)
    if (console_line[count] < 32)
      console_line[count] = 0;
  return console_line;
}

/*
 * Function : void strtolower(char *string)
 * Purpose  : changes char *string to lowercase
 * Params   : char *string - string to lowercase
 */
void
strtolower (char *string)
{
  int count;

  for (count = 0; count < strlen (string); count++)
    {
      string[count] = tolower (string[count]);
    }
}

/*
 * Function : void print_connection(int connection, char *string)
 * Purpose  : outputs info string for a connection
 * Parms    : int connection - numbre of connection
 *            char *string - text to output
 */
void
print_connection (int connection, char *string)
{
  printf (gettext ("Connection"));
  printf (" #%d: %s", connection, string);
}

/*
 * Function: void EOC (int connection)
 * Purpose : terminates connection gracefully
 * Params  : int connection - connection number to terminate
 */
void
EOC (int connection)
{
  debug_printf (">EOC(%d)\n", connection);
  debug_printf ("connection_status[%d]=%d\n",
		connection, connection_status[connection]);
  
  client_socket_is_free[connection] = 1;
  FD_CLR (client_socket[connection], &mask);
  FD_CLR (client_socket[connection], &rmask);
  close (client_socket[connection]);

  if (connection_status[connection] == BICONNECTED)
    {
      debug_printf ("connection_status[connection] == BICONNECTED\n");
      
      FD_CLR (proxy_socket[connection], &mask);
      FD_CLR (proxy_socket[connection], &rmask);
      close (proxy_socket[connection]);
    }
  connection_status[connection] = TO_RESET;
  print_connection (connection, gettext ("end of connection\n"));

  debug_printf ("EOC>\n");
}

/*
 * Function : mark_all_client_sockets_as_free (void)
 * Purpose  : initializes client sockets, marking all of them as free
 * Params   : none
 */
void
mark_all_client_sockets_as_free (void)
{
  int connection;

  debug_printf (">mark_all_client_sockets_as_free ()\n");
  for (connection = 0; connection < MAX_CONNECTIONS; connection++)
    client_socket_is_free[connection] = 1;
  debug_printf ("mark_all_client_sockets_as_free>\n");
}

/*
 * Function : print_program_version(char *PROGRAM_NAME, char *PROGRAM_VERSION);
 * Purpose  : outputs program name & version "window"
 * Params   : char *PROGRAM_NAME - program name
 *            char *PROGRAM_VERSION - program version
 */
void
print_program_version (char *PROGRAM_NAME, char *PROGRAM_VERSION)
{
  int count;

  printf ("\n-----------------------------------\n");
  printf ("%s", PROGRAM_NAME);
  for (count = 0; count < 25 - strlen (PROGRAM_NAME); count++)
    printf (" ");
  for (count = 0; count < 10 - strlen (PROGRAM_VERSION); count++)
    printf (" ");
  printf ("%s\n\n", PROGRAM_VERSION);
  printf ("(C) 2003 Miguelanxo Otero Salgueiro\n");
  printf ("-----------------------------------\n\n");
}

/*
 * Function : void strnsend (int fd, char *string, int len)
 * Purpose  : almost same as write () + debugging
 * Params   : fd - file descriptor
 *          : char *string - data to write
 *          : int len - length of data
 */
void
strnsend (int fd, char *string, int len)
{

  debug_printf (">strnsend(%d,%s,%d)\n", fd, string, len);
/*	int towrite,index=0;

	towrite=len;
	while (towrite>BUFFSIZE) {
		memcpy(&buffer,&string[index],BUFFSIZE);
		write(fd,buffer,BUFFSIZE);
		towrite=towrite-BUFFSIZE;
		index=index+BUFFSIZE;
	}
	memcpy(&buffer,&string[index],towrite);
	write(fd,buffer,towrite);
	if (DEBUG)
	{
		write(1,buffer,towrite);
		fflush(stdout);
	}
*/
  write (fd, string, len);
  debug_printf ("strnsend>\n");
}

/*
 * Function : void strsend(int fd, char *string)
 * Purpose  : send null terminated string to file descriptor
 * Params   : int fd - file descriptor
 *          : char *string - data to send
 */
void
strsend (int fd, char *string)
{
  strnsend (fd, string, strlen (string));
}

/*
 * Function : char *parse_HTTP_return_code(void)
 * Purpose  : gets a parsed HTTP return code from line in buffer
 *          : (returns a pointer to global variable HTTP_return_code)
 * Params   : none
 */
char *
parse_HTTP_return_code (void)
{
  int count;

  debug_printf (">parse_HTTP_return_code\n");
  /*
   * initialize HTTP_return_code to XXX (undefined/error)
   */
  strcpy (HTTP_return_code, "XXX");

  if (!memcmp (buffer, "HTTP", 4))
    {
      for (count = 0; buffer[count] != ' '; count++)
	if (count == BUFFER_SIZE)
	  break;
      if (count < BUFFER_SIZE)
	{
	  memcpy (HTTP_return_code, &buffer[count + 1], 3);
	  HTTP_return_code[3] = 0;
	  debug_printf ("parse_HTTP_return_code>\n");
	  return HTTP_return_code;
	}
    }
  printf ("parse_HTTP_return_code:");
  printf (gettext ("bad proxy response.\n"));
  exit (1);
}

/*
 *  Function : void wait_for_crlf(int fd)
 *  Purpose  : reads data from file descriptor until sequence CR LF found
 *           : (data is stored in global variable buffer)
 *  Params   : int fd - file descriptor
 *  Returns  : 0 if Ok, -1 otherwise
 */
int
wait_for_crlf (int fd)
{
  unsigned char previous_byte = 0;
  int count;

  debug_printf (">wait_for_crfl(%d)\n", fd);
  count = 0;
  while (1)
    {
      read (fd, &buffer[count], 1);
      debug_printf ("%c", buffer[count]);
      if ((buffer[count] == '\n') && (previous_byte == '\r'))
	break;
      if (count == BUFFER_SIZE)
	{
	  printf (" (CASCA)\n\n");
	  printf ("wait_for_crlf: BUFFER OVERFLOW!\n");
	  return (-1);
	}
      previous_byte = buffer[count];
      count++;
    }
  buffer[count + 1] = 0;
  debug_printf ("wait_for_crfl>\n");
  return (0);
}

/*
 * Function : void wait_for_2crlf(int fd)
 * Purpose  : reads file descriptor until sequence CR LR CR LF found
 *          : (that sequence is used to mark HTTP header end)
 * Params   : int fd - file descriptor to read from
 * Returns  : 0 if Ok, -1 otherwise
 */
int
wait_for_2crlf (int fd)
{
  debug_printf (">wait_for_2crlf\n");
  while (memcmp (buffer, "\r\n", 2))
    {
      if (wait_for_crlf (fd) < 0)
        {
	  return (-1);
	}
    }
  debug_printf ("wait_for_2crlf>\n");
  return (0);
}


void init_http_header(char* string, char* remote_host, char *remote_port){
  char User_Agent[256];
  strcpy (string, "CONNECT ");
  strcat (string, remote_host);
  strcat (string, ":");
  strcat (string, remote_port);
  strcat (string, " HTTP/1.1\r\nHost: ");
  strcat (string, remote_host);
  strcat (string, ":");
  strcat (string, remote_port);
  strcat (string, "\r\nUser-Agent: ");
  if (getenv ("USER_AGENT") != NULL)
    {
       strncpy (User_Agent, getenv ("USER_AGENT"), 255);
    }
  else
    {
       strcpy (User_Agent, "Mozilla/4.0 (compatible; MSIE 5.5; Windows 98)");
    }
  strcat (string, User_Agent);
  
}



#define MD5_HASHLEN (16)
static void dump_hash(char *buf, const unsigned char *hash) 
{
	int i;

	for (i = 0; i < MD5_HASHLEN; i++) {
		buf += sprintf(buf, "%02x", hash[i]);
	}

	*buf = 0;
}

/*
	Taken from darkk/redsocks
*/
uint32_t red_randui32()
{
	uint32_t ret;
	evutil_secure_rng_get_bytes(&ret, sizeof(ret));
	return ret;
}

/*
	Based on the corresponding function on darkk/redsocks
*/

char* digest_authentication_encode(char* user, char* realm, char* passwd, char * method, char * path, char* nc, char* nonce, char* cnonce, char* qop){

	
	/* calculate the digest value */
	md5_state_t ctx;
	md5_byte_t hash[MD5_HASHLEN];
	char a1buf[MD5_HASHLEN * 2 + 1], a2buf[MD5_HASHLEN * 2 + 1];
	char response[MD5_HASHLEN * 2 + 1];
	/* A1 = username-value ":" realm-value ":" passwd */
	md5_init(&ctx);
	md5_append(&ctx, (md5_byte_t*)user, strlen(user));
	md5_append(&ctx, (md5_byte_t*)":", 1);
	md5_append(&ctx, (md5_byte_t*)realm, strlen(realm));
	md5_append(&ctx, (md5_byte_t*)":", 1);
	md5_append(&ctx, (md5_byte_t*)passwd, strlen(passwd));
	md5_finish(&ctx, hash);
	dump_hash(a1buf, hash);

	/* A2 = Method ":" digest-uri-value */
	
	md5_init(&ctx);
	md5_append(&ctx, (md5_byte_t*)method, strlen(method));
	md5_append(&ctx, (md5_byte_t*)":", 1);
	md5_append(&ctx, (md5_byte_t*)path, strlen(path));
	md5_finish(&ctx, hash);
	dump_hash(a2buf, hash);
	
	
	
	
	/* qop set: request-digest = H(A1) ":" nonce-value ":" nc-value ":" cnonce-value ":" qop-value ":" H(A2) */
	/* not set: request-digest = H(A1) ":" nonce-value ":" H(A2) */
	md5_init(&ctx);
	md5_append(&ctx, (md5_byte_t*)a1buf, strlen(a1buf));
	md5_append(&ctx, (md5_byte_t*)":", 1);
	md5_append(&ctx, (md5_byte_t*)nonce, strlen(nonce));
	md5_append(&ctx, (md5_byte_t*)":", 1);
	if (qop) {
		md5_append(&ctx, (md5_byte_t*)nc, strlen(nc));
		md5_append(&ctx, (md5_byte_t*)":", 1);
		md5_append(&ctx, (md5_byte_t*)cnonce, strlen(cnonce));
		md5_append(&ctx, (md5_byte_t*)":", 1);
		md5_append(&ctx, (md5_byte_t*)qop, strlen(qop));
		md5_append(&ctx, (md5_byte_t*)":", 1);
	}
	md5_append(&ctx, (md5_byte_t*)a2buf, strlen(a2buf));
	md5_finish(&ctx, hash);
	dump_hash(response, hash);

	/* prepare the final string */
	int len = 256;
	len += strlen(user);
	len += strlen(realm);
	len += strlen(nonce);
	len += strlen(path);
	len += strlen(response);

	if (qop) {
		len += strlen(qop);
		len += strlen(nc);
		len += strlen(cnonce);
	}



	char *res = (char*)malloc(len);
	sprintf(res, "username=\"%s\", realm=\"%s\", nonce=\"%s\", uri=\"%s\", response=\"%s\", qop=%s, nc=%s, cnonce=\"%s\"",
			user, realm, nonce, path, response, qop, nc, cnonce);
	



	
	return res;
}
#define BUFSIZE 4096
typedef struct proxyauth{
	char realm[BUFSIZE];
	char nonce[33];
	char qop[BUFSIZE];
	int stale;
}proxyauth;


proxyauth* process_line(char* line){
	
	proxyauth* pa = (proxyauth *)malloc(sizeof(proxyauth));
	// for now only looking for nonce and realm
	char* where = strstr(line, "nonce");
	if(where != NULL)strncpy(pa->nonce, where + 7, 32);
	where = strstr(line, "realm");
	if(where != NULL)sscanf(where + 7, "%[^\"]", pa->realm);
	return pa;
}


proxyauth * get_param(char * buffer){
	char line[BUFSIZE] = "";
	int cur = 0;
	int blen = strlen(buffer);
	const char pauth[20] = "Proxy-Authenticate:";
	const char conclose[18] = "Connection: close";
	while(cur < blen){
		sscanf(buffer + cur, " %[^\n]", line);
		
		cur += strlen(line) + 1;
		
		if(strncmp(line, pauth, strlen(pauth)) == 0){
			return process_line(line);
		}
		if(strncmp(line, conclose, strlen(conclose)) == 0)break;
	}
	return NULL;
	
}









/*
 * Function : int connect_host_to_proxy(int connection, char *remote_host
 *          : char *remote_port)
 * Purpose  : connects to remote_host:remote_port
 *          : trough proxy_host:proxy_port 
 * Params   : int connection - number of connection in use
 *          : char *remote_host - remote host (name or IP as string)
 *          : char *remote_port - remote port (number as string)
 */
int
connect_host_to_proxy (int connection, char *remote_host, char *remote_port)
{
  int count;
  char proxy_user[256];
  char User_Agent[256];

  debug_printf (">connect_host_to_proxy(%d,%s,%s)\n", connection, remote_host,
		remote_port);
  debug_printf (">socket(AF_INET,SOCK_STREAM,0)\n");
  if ((proxy_socket[connection] = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror ("socket");
      return -1;
    }
  debug_printf ("socket> (%d)\n", proxy_socket[connection]);
  if ((proxy_hostent = gethostbyname (proxy_host)) == NULL)
    {
      switch (h_errno)
	{
	case TRY_AGAIN:
	  {
	    printf ("gethostbyname:");
	    printf (gettext (" temporary error"));
	    printf (gettext (" in name resolution\n"));
	    break;
	  }
	case HOST_NOT_FOUND:
	  {
	    printf ("gethostbyname:");
	    printf (gettext (" unknown host\n"));
	    break;
	  }
	default:
	  {
	    printf ("gethostbyname:");
	    printf (gettext (" non-recoverable"));
	    printf (gettext (" name server error\n"));
	  }
	}
      return -2;
    }
  memset (&proxy, 0, sizeof proxy);
  proxy.sin_family = AF_INET;
  memcpy (&proxy.sin_addr, proxy_hostent->h_addr, proxy_hostent->h_length);
  proxy.sin_port = htons (atoi (proxy_port));
  debug_printf (">connect\n");
  if (connect (proxy_socket[connection],
	       (struct sockaddr *) &proxy, sizeof proxy) < 0)
    {
      perror ("connect");
      return -3;
    }
  debug_printf ("connect>\n");
  status = PROXY_OK;
  init_http_header(string, remote_host, remote_port);
  strcat (string, "\r\n\r\n");
  strsend (proxy_socket[connection], string);
  int tries = 0;
  while (status == PROXY_OK)
    {
      tries += 1;
      if(tries > 2){
	perror ("failed");
	return -4;
      }
      if (wait_for_crlf (proxy_socket[connection]) < 0)
      {
	      EOC (connection);
	      return -4;
      }
      parse_HTTP_return_code ();
      if (!strcmp (HTTP_return_code, "200"))
	status = BICONNECTED;
      else if(!strcmp (HTTP_return_code, "407")){
	  read (proxy_socket[connection], buffer, sizeof (buffer)); // hopefully buffer is big enough to receive the http header
	  if(strstr(buffer, "Proxy-Authenticate: Basic") != NULL){
	    debug_printf("connect_host_to_proxy> Proxy-Authenticate: Basic\n");
	    
	    init_http_header(string, remote_host, remote_port);
	    if (getenv ("PROXY_USER") != NULL)
	    {
	      char proxy_authorization_base64[257];

	      strncpy (proxy_user, getenv ("PROXY_USER"), 255);
	      base64_encode (proxy_user, proxy_authorization_base64);
	      strcat (string, "\r\nProxy-authorization: Basic ");
	      strcat (string, proxy_authorization_base64);
	      //debug_printf ("Proxy-authorization: Basic %s\n", proxy_authorization_base64);
	      strcat (string, "\r\n\r\n");
	      if ((proxy_socket[connection] = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	      {
		perror ("socket");
		return -1;
	      }
	      
	      
	      if (connect (proxy_socket[connection],
			 (struct sockaddr *) &proxy, sizeof proxy) < 0)
	      {
		perror ("connect");
		return -3;
	      }
	      
	      
	      strsend (proxy_socket[connection], string);
	    }
	    else{
	      status = PROXY_FAULT;
	    }
	  }
	  
	  else if(strstr(buffer, "Proxy-Authenticate: Digest") != NULL){
	    
	    debug_printf("connect_host_to_proxy> Proxy-Authenticate: Digest\n");
	    
	    proxyauth* pa = get_param(buffer);
	    if(pa == NULL){
		perror ("proxy");
		return -1;
	    }
	    if(strlen(pa->nonce) != 32){
		perror("proxy(nonce)");
		return -1;
	    }
	    
	    init_http_header(string, remote_host, remote_port);
	    if (getenv ("PROXY_USER") != NULL)
	    {
	      char up[256];
	      strncpy (up, getenv ("PROXY_USER"), 255);
	      char user[256] = "";
	      sscanf(up, "%[^:]", user);
	      char passwd[256] = "";
	      sscanf(up + strlen(user) + 1, "%s", passwd);
	      char method[32] = "CONNECT";
	      strcat(string, "\r\nProxy-Authorization: Digest ");
	      
	      
	      /* prepare an random string for cnounce */
	      char cnonce[17];
	      snprintf(cnonce, sizeof(cnonce), "%08x%08x", red_randui32(), red_randui32());
	      
	      char nc[17];
	      snprintf(nc, sizeof(nc), "%08x", 1);
	      
	      
	      
	      
	      char* auth_string = digest_authentication_encode(
			      user, pa->realm, passwd, //user, realm, pass
			      method, "/", nc, pa->nonce, cnonce, "auth"); // method, path, nc, cnonce
	      
	      strcat(string, auth_string);
	      
	      
	      
	      strcat (string, "\r\n\r\n");
	      if ((proxy_socket[connection] = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	      {
		perror ("socket");
		return -1;
	      }
	      
	      
	      if (connect (proxy_socket[connection],
			 (struct sockaddr *) &proxy, sizeof proxy) < 0)
	      {
		perror ("connect");
		return -3;
	      }
	      
	      
	      strsend (proxy_socket[connection], string);
	    }
	    else{
	      status = PROXY_FAULT;
	    }
	  }
	  else{
	    status = PROXY_FAULT;
	  }
	}
	
      else
	status = PROXY_FAULT;
    }
  if (status == PROXY_FAULT)
    {
      /*
       * if PROXY_FAULT then write HTTP response to stdout
       */
      printf("HTTP CODE=%s\n", HTTP_return_code);
      while ((count = read (proxy_socket[connection],
			    buffer, sizeof (buffer))) != 0)
	write (1, buffer, count);
      return -5;
    }
  /*
   * discard the rest of HTTP header until CR LF CR LF
   * (that is, to the beginning of the real connection)
   */
  if (wait_for_2crlf (proxy_socket[connection]) < 0)
    {
      return -6;
    }
  print_connection (connection,
		    gettext ("bidirectional connection stablished\n\n"));
  if (proxy_socket[connection] > maxfd)
    maxfd = proxy_socket[connection];
  FD_SET (proxy_socket[connection], &mask);
  debug_printf ("connect_host_to_proxy> (0)\n");
  return 0;
}

/*
 * Function : void initialize_gettext(void)
 * Purpose  : initializes gettext (i18n) extension
 * Params   : none
 */
void
initialize_gettext (void)
{
  debug_printf (">initialize_gettext()\n");
  setlocale (LC_ALL, "");
  bindtextdomain ("desproxy", LOCALEDIR);
  textdomain ("desproxy");
  debug_printf ("initilize_gettext>\n");
}

/*
 * Function : int bind_UDP_port(unsigned int request_port)
 * Purpose  : binds requested UDP port
 * Params   : unsigned int request_port - requested port
 */
int
bind_UDP_port (unsigned int request_port)
{
  int UDP_socket;

  debug_printf ("bind_UDP_port(%d)\n", request_port);
  if ((UDP_socket = socket (PF_INET, SOCK_DGRAM, 0)) < 0)
    {
      perror ("socket");
      exit (1);
    }
  memset (&server, 0, sizeof server);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (request_port);
  if ((bind (UDP_socket, (struct sockaddr *) &server, sizeof server)) < 0)
    {
      perror ("bind");
      exit (1);
    }
  printf (gettext ("UDP port "));
  printf ("%d", request_port);
  printf (gettext (" Bound\n"));
  debug_printf ("bind_UDP_port> (%d)\n", UDP_socket);
  return (UDP_socket);
}

/*
 * Function : int listen_in_TCP_port(unsigned int request_port)
 * Purpose  : listens in requested TCP port for incoming connections
 * Parmas   : unsigned int request_port - requested port
 */
int
listen_in_TCP_port (unsigned int request_port)
{
  int request_socket;

  debug_printf ("listen_in_TCP_port(%d)\n", request_port);
  if ((request_socket = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror ("socket");
      exit (1);
    }
  memset (&server, 0, sizeof server);
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = htonl (INADDR_ANY);
  server.sin_port = htons (request_port);
  if ((bind (request_socket, (struct sockaddr *) &server, sizeof server)) < 0)
    {
      perror ("bind");
      exit (1);
    }
  if (listen (request_socket, SOMAXCONN) < 0)
    {
      perror ("listen");
      exit (1);
    }
  printf (gettext ("TCP port "));
  printf ("%d", request_port);
  printf (gettext (" Bound & Listening\n"));
  debug_printf ("listen_in_TCP_port> (%d)\n", request_socket);
  return (request_socket);
}

/*
 * Function : int look_for_desproxy_conf(void)
 * NYI
 */
int
look_for_desproxy_conf (void)
{
  FILE *desproxy_conf;

  if ((desproxy_conf = fopen ("desproxy.conf", "r")) == NULL)
    {
      return 0;
    }
  fclose (desproxy_conf);
  return (1);
}

/*
 * Function : void turn_console_echo_off(void)
 * Purpose  : turns off console echo
 *          : stores old tty status in global variable old_tty
 * Params   : none
 */
void
turn_console_echo_off (void)
{
  struct termios tty;

  /*
   * Save the old tty settings
   */
  tcgetattr (0, &old_tty);

  /*
   * get rid of echo for the new tty settings.
   *
   * (from man tcgetattr)
   *
   * ICANON Enable canonical mode.  This  enables  the  special
   *        characters  EOF,  EOL,  EOL2,  ERASE,  KILL, LNEXT,
   *        REPRINT, STATUS, and WERASE, and buffers by  lines.
   *
   * ECHO   Echo input characters.
   * 
   * ECHOE  If ICANON is also set, the ERASE  character  erases
   *        the  preceding  input  character, and WERASE erases
   *        the preceding word.
   *
   * ECHOK  If ICANON is also set, the  KILL  character  erases
   *        the current line.
   *
   * ECHONL If  ICANON  is also set, echo the NL character even
   *        if ECHO is not set.
   */
  tty = old_tty;
  tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL);

  /*
   * set new tty settings 
   *
   * (from man tcsetattr)
   *
   * TCSAFLUSH
   *
   *  the  change  occurs after all output written to the
   *  object referred by fd has been transmitted, and all
   *  input  that  has been received but not read will be
   *  discarded before the change is made.
   */
  tcsetattr (0, TCSAFLUSH, &tty);
}

/*
 * Function : void turn_console_echo_on(void)
 * Purpose  : turns on console echo
 *          : restores old tty status stored in global variable old_tty
 * Params   : none
 */
void
turn_console_echo_on (void)
{
  /*
   * Now reset the old settings
   */
  tcsetattr (0, TCSAFLUSH, &old_tty);
}

/*
 * Function : void get_username_and_password(void)
 * Purpose  : requests user to give username and password
 *          : strores them in global variables username & password
 * Params   : none
 */
void
get_username_and_password (void)
{
  strcpy (console_line, "");

  while (!strcmp (console_line, ""))
    {
      printf (gettext ("Username: "));
      strcpy (username, get_console_line ());
    }

  printf (gettext ("Password: "));

  /*
   * turn out echo, so password is not displayed when typed
   */
  turn_console_echo_off ();
  strcpy (password, get_console_line ());

  /*
   * turn on echo again
   */
  turn_console_echo_on ();

  /*
   * send \n because the one in the passwd didn't echo :)
   */
  printf ("\n");
}
