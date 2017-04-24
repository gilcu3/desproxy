/*
 * desproxy.c: Main program file for desproxy
 *
 * Copyright (C) 2003 Miguelanxo Otero Salgueiro
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the tems of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "desproxy.h"
#define PROGRAM_NAME "desproxy"
#define PROGRAM_VERSION VERSION

int request_socket;
int connection_status[MAX_CONNECTIONS];
char *remote_host, *remote_port;

void
client_to_proxy (int connection)
{
  int count;

  print_connection (connection, "client -> proxy");
  if ((count = read (client_socket[connection], buffer, sizeof (buffer))) == -1)
    {
      perror ("read");
      EOC (connection);
      return;
    }
  printf (" %4d bytes read\n", count);
  if (count == 0)
    {
      EOC (connection);
      return;
    }
  else
    {
      if ((count = write (proxy_socket[connection], buffer, count)) == -1)
	{
	  perror ("write");
	  EOC (connection);
	}
    }
}

void
proxy_to_client (int connection)
{
  int count;

  print_connection (connection, "proxy -> client");
  if ((count = read (proxy_socket[connection], buffer, sizeof (buffer))) == -1)
    {
      perror ("read");
      EOC (connection);
      return;
    }
  printf (" %4d bytes read\n", count);
  if (count == 0)
    {
      EOC (connection);
      return;
    }
  else
    {
      if ((count = write (client_socket[connection], buffer, count)) == -1)
	{
	  perror ("write");
	  EOC (connection);
	}
    }
}

void
parse_command_line (int argc, char **argv)
{
  debug_printf (">parse_command_line(COMMAND_LINE)\n");
  if (argc != 6)
    {
      printf (gettext
	      ("Usage: desproxy remote_host remote_port proxy_host"
	       " proxy_port local_port\n\n"));
      exit (1);
    }
  remote_host = argv[1];
  remote_port = argv[2];
  proxy_host = argv[3];
  proxy_port = argv[4];
  local_port = argv[5];
  debug_printf ("parse_command_line>\n");
}

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
      client_socket_is_free[connection] = 0;
      printf (gettext ("Connection request from %s, port %d\n"),
	      inet_ntoa (client.sin_addr), ntohs (client.sin_port));
      if (client_socket[connection] > maxfd)
	maxfd = client_socket[connection];
      FD_SET (client_socket[connection], &mask);
      printf (gettext ("Connecting to http proxy (%s:%s)\n"), proxy_host,
	      proxy_port);
      if (connect_host_to_proxy (connection, remote_host, remote_port) != 0)
	{
	  printf ("connect_host_to_proxy: ERROR\n");
	  EOC (connection);
	  return;
	}
      else
	{
	  connection_status[connection] = BICONNECTED;
	}
    }
  debug_printf ("process_connection_request>\n");
}

int
main (int argc, char **argv)
{

  int connection, nfound;
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
      debug_printf ("%d", nfound);
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

      if (FD_ISSET (request_socket, &rmask))
	{
	  process_connection_request ();
	}

      for (connection = 0; connection < MAX_CONNECTIONS; connection++)
	{
	  if (!client_socket_is_free[connection])
	    {
	      debug_printf ("client_socket[%d]=%d\n", connection,
			    client_socket[connection]);
	      debug_printf ("connection_status[%d]=%d\n", connection,
			    connection_status[connection]);
	      if (FD_ISSET (client_socket[connection], &rmask))
		{
		  client_to_proxy (connection);
		}
	      if (connection_status[connection] == BICONNECTED)
		{
		  if (FD_ISSET (proxy_socket[connection], &rmask))
		    {
		      proxy_to_client (connection);
		    }
		}
	    }
	}
      fflush (stdout);
    }
}
