/*
 * desproxy-socksserver.c: Socks Version 4 & 5 server for HTTP proxies
 *
 * Copyright (C) 2003 Miguelanxo Otero Salgueiro
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the tems of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "desproxy.h"
#define PROGRAM_NAME "desproxy-socksserver"
#define PROGRAM_VERSION VERSION

int request_socket, request_length;
char remote_host[256], remote_port[6];
unsigned char nmethods[MAX_CONNECTIONS];
unsigned char methods[MAX_CONNECTIONS][256];

/*
 * Parses command line
 */
void
parse_command_line (int argc, char **argv)
{
  debug_printf (">parse_command_line(COMMAND_LINE)\n");
  if (argc != 4)
    {
      printf (gettext ("Usage: desproxy-socksserver proxy_host"
                       " proxy_port local_port\n\n"));
      exit (1);
    }

  proxy_host = argv[1];
  proxy_port = argv[2];
  local_port = argv[3];
  debug_printf ("parse_command_line>\n");
}

/*
 * invoked for the first time something is read
 */
void
nothing_read (connection)
{
  debug_printf (">nothing_read()\n");
  if (read (client_socket[connection], buffer, 1) == -1)
    {
      perror ("read");
      EOC (connection);
      return;
    }
  
  if (buffer[0] == 4)
    {
      connection_status[connection] = PROTOCOL_V4_OK;
      debug_printf ("nothing_read>\n");
      return;
    }

  if (buffer[0] == 5)
    {
      connection_status[connection] = PROTOCOL_V5_OK;
      debug_printf ("nothing_read>\n");
      return;
    }

   printf (gettext ("invalid client protocol version ("));
   printf ("%d)\n", buffer[0]);

   /*
    * send general SOCKS error
    */
   strnsend (client_socket[connection], "\x05\x01\x00\x00", 4);
   EOC (connection);
   return;
}

/*
 * invoked when we read the protocol header and it is 4
 */
void
protocol_v4_ok (connection)
{
  debug_printf (">protocol_v4_ok()\n");
  if (read (client_socket[connection], buffer, 1) == -1)
    {
      perror ("read");
      EOC (connection);
      return;
    }
  if (buffer[0] != 1)
    {
      debug_printf ("Method != 1 (CONNECT) => method refused\n");
      strnsend (client_socket[connection], "\x00\x5B\x00\x00\x00\x00\x00\x00",
		8);
      EOC (connection);
    }
  else
    {
      debug_printf ("Method 1 (CONNECT) accepted\n");
      connection_status[connection] = METHOD_ACCEPTED_V4;
    }
  debug_printf ("protocol_v4_ok>\n");
  return;
}

/*
 * invoked when we read the protocol header and it is 5
 */
void
protocol_v5_ok (connection)
{
  debug_printf (">protocol_v5_ok()\n");
  if (read (client_socket[connection], buffer, 1) == -1)
    {
      perror ("read");
      EOC (connection);
      return;
    }
  nmethods[connection] = buffer[0];
  connection_status[connection] = NMETHODS_READ;	//NMETHODS is read
  debug_printf ("Number of methods: %d\n", nmethods[connection]);
  debug_printf ("protocol_v5_ok>\n");
  return;
}

/*
 * Invoked when the method is accepted for protocol version 4
 */
void
method_accepted_v4 (connection)
{
  debug_printf (">method_accepted()\n");
  if (read (client_socket[connection], buffer, 6) == -1)
    {
      perror ("read");
      EOC (connection);
      return;
    }
  debug_printf ("IP ");
  debug_printf ("(%d.%d.%d.%d:%d)\n", buffer[2], buffer[3], buffer[4],
		buffer[5], buffer[0] * 256 + buffer[1]);
  sprintf (remote_host, "%d.%d.%d.%d", buffer[2], buffer[3], buffer[4],
	   buffer[5]);
  sprintf (remote_port, "%d", buffer[0] * 256 + buffer[1]);
  buffer[0] = 0xff;
  while (buffer[0] != 0)
    {
      if (read (client_socket[connection], buffer, 1) == -1)
	{
	  perror ("read");
	  EOC (connection);
	  return;
	}
    }
  if (connect_host_to_proxy (connection, remote_host, remote_port) != 0)
    {
      strnsend (client_socket[connection], "\x00\x5B\x00\x00\x00\x00\x00\x00",
		8);
      printf ("connect_host_to_proxy: ERROR\n");
      EOC (connection);
      return;
    }
  else
    {
      /*
       * Send success
       */
      strnsend (client_socket[connection],
		"\x00\x5A\x00\x00\x00\x00\x00\x00", 8);
      connection_status[connection] = BICONNECTED;
    }
}

/*
 * Invoked when the method is accepted for protocol version 5
 */
void
method_accepted_v5 (connection)
{
  unsigned char index;

  debug_printf (">method_accepted()\n");
  if (read (client_socket[connection], buffer, 4) == -1)
    {
      perror ("read");
      EOC (connection);
      return;
    }
  debug_printf ("VER %d\n", buffer[0]);
  debug_printf ("CMD %d\n", buffer[1]);
  debug_printf ("RSV %d\n", buffer[2]);
  debug_printf ("ATYP %d\n", buffer[3]);
  if (buffer[1] != 1)
    {
      debug_printf ("Command %d not supported\n", buffer[1]);
      /*
       * Send "Command not supported"
       */
      strnsend (client_socket[connection], "\x05\x07\x00\x01", 4);
      strnsend (client_socket[connection], (char *) &server.sin_port, 2);
      strnsend (client_socket[connection], (char *) &server.sin_addr.s_addr, 4);
      EOC (connection);
      return;
    }
  if (buffer[3] == 1)
    {
      if (read (client_socket[connection], buffer, 6) == -1)
	{
	  perror ("read");
	  EOC (connection);
	  return;
	}
      debug_printf ("ATYP = IP ");
      debug_printf ("(%d.%d.%d.%d:%d)\n", buffer[0], buffer[1], buffer[2],
		    buffer[3], buffer[4] * 256 + buffer[5]);
      sprintf (remote_host, "%d.%d.%d.%d", buffer[0], buffer[1], buffer[2],
	       buffer[3]);
      sprintf (remote_port, "%d", buffer[4] * 256 + buffer[5]);
    }
  if (buffer[3] == 3)
    {
      if (read (client_socket[connection], buffer, 1) == -1)
	{
	  perror ("read");
	  EOC (connection);
	  return;
	}
      debug_printf ("ATYP = DOMAINNAME\n");
      strcpy (remote_host, "");
      for (index = buffer[0]; index > 0; index--)
	{
	  if (read (client_socket[connection], buffer, 1) == -1)
	    {
	      perror ("read");
	      EOC (connection);
	      return;
	    }
	  strncat (remote_host, buffer, 1);
	}
      strncat (remote_host, "\x00", 1);
      debug_printf ("remote_host %s\n", remote_host);
      if (read (client_socket[connection], buffer, 2) == -1)
	{
	  perror ("read");
	  EOC (connection);
	  return;
	}
      sprintf (remote_port, "%d", buffer[0] * 256 + buffer[1]);
      debug_printf ("remote_port %s\n", remote_port);
    }
  if (connect_host_to_proxy (connection, remote_host, remote_port) != 0)
    {
      /*
       * Send "Host unreachable"
       */
      strnsend (client_socket[connection], "\x05\x04\x00\x01", 4);
      printf ("connect_host_to_proxy: ERROR\n");
      EOC (connection);
      return;
    }
  else
    {
      /*
       * Send "Success"
       */
      strnsend (client_socket[connection], "\x05\x00\x00\x01", 4);
      strnsend (client_socket[connection], (char *) &server.sin_port, 2);
      strnsend (client_socket[connection], (char *) &server.sin_addr.s_addr, 4);
      connection_status[connection] = BICONNECTED;
    }
}

void
nmethods_read (connection)
{
  unsigned char index;
  debug_printf (">nmethods_read()\n");
  if (read
      (client_socket[connection], &methods[connection][0],
       nmethods[connection]) == -1)
    {
      perror ("read");
      EOC (connection);
      return;
    }
  connection_status[connection] = LOOKING_FOR_METHODS;
  for (index = 0; index < nmethods[connection]; index++)
    {
      /*
       * if there is one method 0 (NOAUTH) then accept it
       */
      if (methods[connection][index] == 0)
	{
	  connection_status[connection] = METHOD_ACCEPTED_V5;
	  strnsend (client_socket[connection], "\x05\x00", 2);
	  debug_printf ("Selected method 0\n");
	}
    }
  if (connection_status[connection] != METHOD_ACCEPTED_V5)
    {
      debug_printf (gettext ("All methods rejected\n"));
      strnsend (client_socket[connection], "\x05\xff", 2);
      EOC (connection);
      return;
    }
  debug_printf ("nmethods_read>\n");
}

/*
 * Invoked for transcactions when the client is accepted yet
 */
void
biconnected (connection)
{
  int count;
  debug_printf (">biconnected()\n");
  debug_printf ("client -> proxy (%d) ", connection);
  if ((count = read (client_socket[connection], buffer, sizeof (buffer))) == -1)
    {
      debug_printf ("-1 bytes\n");
      perror ("read");
      EOC (connection);
      return;
    }
  if (count == 0)
    {
      debug_printf ("0 bytes\n");
      EOC (connection);
    }
  else
    {
      if ((count = write (proxy_socket[connection], buffer, count)) == -1)
	{
	  perror ("write");
	  EOC (connection);
	  return;
	}
      debug_printf ("%d bytes\n", count);
    }
  debug_printf ("biconnected>\n");
}

/*
 * Processes connection requests
 */
void
process_connection_request (void)
{
  int connection;

  debug_printf (">process_connection_request()\n");
  for (connection = 0; connection < MAX_CONNECTIONS; connection++)
    if (client_socket_is_free[connection])
      break;
  if (connection == MAX_CONNECTIONS)
    {
      printf (gettext ("There are no connections available at this time\n"));
    }
  else
    {
      client_length = sizeof (client);
      client_socket[connection] =
	accept (request_socket, (struct sockaddr *) &client, &client_length);
      if (client_socket[connection] < 0)
	{
	  perror ("accept");
	  EOC (connection);
	  return;
	}
      debug_printf ("accept> (%d)\n", client_socket[connection]);
      client_socket_is_free[connection] = 0;
      printf (gettext ("Connection request from %s, port %d\n"),
	      inet_ntoa (client.sin_addr), ntohs (client.sin_port));
      if (client_socket[connection] > maxfd)
	maxfd = client_socket[connection];
      FD_SET (client_socket[connection], &mask);
      connection_status[connection] = NOTHING_READ;
    }
  debug_printf ("process_connection_request>\n");
}

int
main (int argc, char **argv)
{
  int connection;
  int count;
  int nfound;
  struct timeval timeout;

  signal (SIGPIPE, SIG_IGN);
  initialize_gettext ();
  print_program_version (PROGRAM_NAME, PROGRAM_VERSION);
  parse_command_line (argc, argv);

  request_socket = listen_in_TCP_port (atoi (local_port));
  printf (gettext ("Press <Control+C> to Quit\n\n"));
  mark_all_client_sockets_as_free ();

  FD_ZERO (&mask);
  FD_SET (request_socket, &mask);
  maxfd = request_socket;
  while (1)
    {
      rmask = mask;
      timeout.tv_sec = 5;
      timeout.tv_usec = 0;
      nfound = select (maxfd + 1, &rmask, NULL, NULL, &timeout);
      if (DEBUG > 1)
      debug_printf ("nfound> (%d)\n", nfound);
      if (nfound < 0)
	{
	  if (errno == EINTR)
	    {
	      printf (gettext ("Interrupted by system call\n"));
	      continue;
	    }
	  perror ("select");
	  exit (1);
	}

      /*
       * Proccess connection request
       */
      if (FD_ISSET (request_socket, &rmask))
	{
	  process_connection_request ();
	}

      /*
       * Incoming connections
       */
      for (connection = 0; connection < MAX_CONNECTIONS; connection++)
	{
	  if (DEBUG > 1)
	    debug_printf ("connection #%d\n", connection);

	  if (!client_socket_is_free[connection])
	    {
	      debug_printf ("client_socket_is_free[%d]=%d\n", connection,
			    client_socket_is_free[connection]);
	      if (FD_ISSET (client_socket[connection], &rmask))
		{
		  debug_printf ("FD_ISSET(client_socket[%d],&rmask\n",
				connection);
		  if (connection_status[connection] == BICONNECTED)
		    {
		      biconnected (connection);
		    }
		  if (connection_status[connection] == METHOD_ACCEPTED_V4)
		    {
		      method_accepted_v4 (connection);
		    }
		  if (connection_status[connection] == METHOD_ACCEPTED_V5)
		    {
		      method_accepted_v5 (connection);
		    }
		  if (connection_status[connection] == NMETHODS_READ)
		    {
		      nmethods_read (connection);
		    }
		  if (connection_status[connection] == PROTOCOL_V4_OK)
		    {
		      protocol_v4_ok (connection);
		    }
		  if (connection_status[connection] == PROTOCOL_V5_OK)
		    {
		      protocol_v5_ok (connection);
		    }
		  if (connection_status[connection] == NOTHING_READ)
		    {
		      nothing_read (connection);
		    }
		}
	      if (connection_status[connection] == BICONNECTED)
		{
		  if (FD_ISSET (proxy_socket[connection], &rmask))
		    {
		      debug_printf ("proxy -> client (%d) ", connection);
		      if ((count =
			   read (proxy_socket[connection], buffer,
				 sizeof (buffer))) == -1)
			{
			  perror ("read");
			  EOC (connection);
			}
		      else
			{
			  if (count == 0)
			    {
			      EOC (connection);
			    }
			  else
			    {
			      if ((count =
				   write (client_socket[connection], buffer,
					  count)) == -1)
				{
				  perror ("write");
				  EOC (connection);
				}
			    }
			  debug_printf ("%d bytes\n", count);
			}
		    }
		}
	    }
	}

      if (DEBUG > 1)
	{
	  for (connection = 0; connection < MAX_CONNECTIONS; connection++)
	    {
	      debug_printf ("connection_status[%d]=%d\n", connection,
		      connection_status[connection]);
	      if (FD_ISSET (proxy_socket[connection], &rmask))
		{
		  debug_printf ("FD_ISSET(proxy_socket[%d],&rmask)\n", connection);
		}
	      if (FD_ISSET (client_socket[connection], &rmask))
		{
		  debug_printf ("FD_ISSET(client_socket[%d],&rmask)\n", connection);
		}
	    }
	}
    }
  exit (0);
}
