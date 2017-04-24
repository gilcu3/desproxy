/*
 * desproxy-inetd.c: use desproxy from inetd
 *
 * Copyright (C) 2003 Miguelanxo Otero Salgueiro
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the tems of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "desproxy.h"
#define PROGRAM_NAME "desproxy-inetd"
#define PROGRAM_VERSION VERSION

char *remote_host, *remote_port;

void
client_to_proxy (void)
{
  int count;

  if ((count = read (0, buffer, sizeof (buffer))) == -1)
    {
      perror ("read");
      exit (1);
    }
  if (count == 0)
    {
      exit (0);
    }
  else
    {
      if ((count = write (proxy_socket[0], buffer, count)) == -1)
	{
	  perror ("write");
	  exit (1);
	}
    }
}

void
proxy_to_client (void)
{
  int count;

  if ((count = read (proxy_socket[0], buffer, sizeof (buffer))) == -1)
    {
      perror ("read");
      exit (1);
    }
  if (count == 0)
    {
      exit (0);
    }
  else
    {
      if ((count = write (1, buffer, count)) == -1)
	{
	  perror ("write");
	  exit (1);
	}
    }
}

void
parse_command_line (int argc, char **argv)
{
  debug_printf (">parse_command_line(COMMAND_LINE)\n");
  if (argc != 5)
    {
      print_program_version (PROGRAM_NAME, PROGRAM_VERSION);
      printf (gettext
	      ("Usage: desproxy-inetd remote_host remote_port proxy_host"
	       " proxy_port\n\n"));
      exit (1);
    }
  remote_host = argv[1];
  remote_port = argv[2];
  proxy_host = argv[3];
  proxy_port = argv[4];
  debug_printf ("parse_command_line>\n");
}

void
process_connection_request (void)
{
  if (connect_host_to_proxy (0, remote_host, remote_port) != 0)
    {
      printf ("connect_host_to_proxy: ERROR\n");
      exit (1);
    }
}

int
main (int argc, char **argv)
{

  int nfound;
  struct timeval timeout;

  signal (SIGPIPE, SIG_IGN);
  initialize_gettext ();
  parse_command_line (argc, argv);


  FD_ZERO (&mask);
  FD_SET (0, &mask);

  process_connection_request ();

  while (1)
    {
      rmask = mask;
      timeout.tv_sec = 5;
      timeout.tv_usec = 0;
      nfound = select (maxfd + 1, &rmask, NULL, NULL, &timeout);
      debug_printf ("%d", nfound);
      if (nfound < 0)
	{
	  if (errno != EINTR)
	    {
	      perror ("select");
	      exit (1);
	    }
	}
      if (FD_ISSET (0, &rmask))
	{
	  client_to_proxy ();
	}
      if (FD_ISSET (proxy_socket[0], &rmask))
	{
	  proxy_to_client ();
	}
    }
}
