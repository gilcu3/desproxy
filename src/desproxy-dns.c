/*
 * desproxy-dns.c: DNS forwarder through HTTP proxy
 *
 * Copyright (C) 2003 Miguelanxo Otero Salgueiro
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the tems of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include "desproxy.h"
#include <fcntl.h>

#define DNSPORT 53
#define MAXREQUESTLEN 512
#define PROGRAM_NAME "desproxy-dns"
#define PROGRAM_VERSION VERSION

#define UDP_CONNECTION (MAX_CONNECTIONS - 1)

char UDP_buffer[MAXREQUESTLEN];
int request_socket, maxfd;

struct request
{
  unsigned char buffer[MAXREQUESTLEN];
  unsigned short int size;
  unsigned short int bib;	// Bytes In Buffer
} requests[MAX_CONNECTIONS];

/*
 * Function: void EOC_dns(int connection)
 * Purpose : terminates connection gracefully
 * Params  : int connection
 */
void
EOC_dns (int connection)		// terminates connection
{
  debug_printf (">EOC_dns(%d)\n", connection);
  if (connection != UDP_CONNECTION)
    {
      FD_CLR (client_socket[connection], &mask);
      FD_CLR (client_socket[connection], &rmask);
      close (client_socket[connection]);
    }
  FD_CLR (proxy_socket[connection], &mask);
  FD_CLR (proxy_socket[connection], &rmask);
  close (proxy_socket[connection]);
  client_socket_is_free[connection] = 1;
  debug_printf ("EOC_dns>\n");
}

int
answer_request (int connection, int size)
{
  int count;
  int htons_size;
  char buffer[MAXREQUESTLEN + 2];

  debug_printf (">answer_request(%d,%d)\n", connection, size);
  if (connection == UDP_CONNECTION)
    {
      memcpy (&buffer[2], UDP_buffer, size);
      htons_size = htons (size);
      memcpy (buffer, &htons_size, 2);
      debug_printf ("UDP\n");
    }
  else
    {
      memcpy (buffer, requests[connection].buffer, size + 2);
      debug_printf ("TCP\n");
    }

  debug_printf ("proxy_host %s\n", proxy_host);
  debug_printf ("proxy_port %s\n", proxy_port);
  if (connect_host_to_proxy (connection, dns_server, "53") != 0)
    {
      printf ("connect_host_to_proxy: ERROR\n");
      return (-1);
    }

  if ((count = write (proxy_socket[connection], buffer, size + 2)) == -1)
    {
      perror ("write");
      return (-2);
    }
  debug_printf ("#BEGIN OF REQUEST#");
  if (DEBUG)
    {
      write (1, buffer, count);
      fflush (stdout);
    }
  debug_printf ("#END OF REQUEST#");
  debug_printf ("LOCAL->PROXY(%d)\n", count);
  if ((count = read (proxy_socket[connection], buffer, 2)) == -1)
    {
      perror ("read");
      return (-3);
    }
  size = ntohs (*((unsigned short int *) buffer));
  debug_printf ("size=%d\n", size);
  if ((count = read (proxy_socket[connection], &buffer[2], size)) == -1)
    {
      perror ("write");
      return (-4);
    }
  debug_printf ("PROXY->LOCAL(%d)\n", count);
  if (connection == UDP_CONNECTION)
    {
      /*size = count - 2;*/
      /*memcpy (buffer, &buffer[2], size);*/
      if ((count =
	   sendto (client_socket[connection], &buffer[2], size, 0,
		   (struct sockaddr *) &client, client_length)) != size)
	{
	  perror ("sendto");	//short write
	  return (-5);
	}
    }
  else
    {
      if ((count = write (client_socket[connection], buffer, size + 2)) == -1)
	{
	  perror ("write");
	  return (-6);
	}
    }
  debug_printf ("LOCAL->CLIENT(%d)\n", count);
  debug_printf ("answer_request>\n");
  return (0);
}

void
parse_command_line (int argc, char **argv)
{
  debug_printf (">parse_command_line(COMMAND_LINE)\n");
  if (argc != 4)
    {
      printf (gettext
	      ("Usage: desproxy-dns dns_server proxy_host proxy_port\n\n"));
      exit (1);
    }
  dns_server = argv[1];
  proxy_host = argv[2];
  proxy_port = argv[3];
  debug_printf ("parse_command_line>\n");
}

void
process_connection_request (void)
{
  int connection;

  debug_printf (">process_connection_request()\n");
  for (connection = 0; connection < MAX_CONNECTIONS - 1; connection++)
    if (client_socket_is_free[connection])
      break;
  if (connection == MAX_CONNECTIONS - 1)
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
	  EOC_dns (connection);
	  return;
	}
      client_socket_is_free[connection] = 0;
      printf (gettext ("TCP request from %s, port %d\n"),
	      inet_ntoa (client.sin_addr), ntohs (client.sin_port));
      if (client_socket[connection] > maxfd)
	maxfd = client_socket[connection];
      FD_SET (client_socket[connection], &mask);
      requests[connection].bib = 0;	// initializes requests[connection] 
    }
  debug_printf ("process_connection_request>\n");
}

int
main (int argc, char **argv)
{
  int connection;
  int nfound;
  int count;
  struct timeval timeout;

  signal (SIGPIPE, SIG_IGN);
  initialize_gettext ();
  print_program_version (PROGRAM_NAME, PROGRAM_VERSION);
  parse_command_line (argc, argv);

  request_socket = listen_in_TCP_port (DNSPORT);
  client_socket[UDP_CONNECTION] = bind_UDP_port (DNSPORT);

  if (fcntl (client_socket[UDP_CONNECTION], F_SETFL, O_NONBLOCK) < 0)	// mark UDP socketet as NON BLOCKING
    {
      perror ("fnctl");
      exit (1);
    }

  printf (gettext ("Press <Control+C> to Quit\n\n"));
  mark_all_client_sockets_as_free ();

  FD_ZERO (&mask);
  FD_SET (request_socket, &mask);
  maxfd = request_socket;
  while (1)
    {
      rmask = mask;
      timeout.tv_sec = 0;
      timeout.tv_usec = 500000;
      nfound = select (maxfd + 1, &rmask, NULL, NULL, &timeout);
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

      if (FD_ISSET (request_socket, &rmask))	// processes connection requests
	{
	  process_connection_request ();
	}

      for (connection = 0; connection < MAX_CONNECTIONS - 1; connection++)	// TCP connections 
	{
	  if (!client_socket_is_free[connection])
	    {
	      debug_printf ("client_socket_is_free[%d]=1\n", connection);
	      if (FD_ISSET (client_socket[connection], &rmask))
		{
		  if ((count =
		       read (client_socket[connection], buffer,
			     BUFFER_SIZE)) == -1)
		    {
		      perror ("read");
		      exit (1);
		    }
		  if (count == 0)
		    EOC_dns (connection);
		  else
		    {
		      memcpy (&requests[connection].
			      buffer[requests[connection].bib], buffer,
			      count);
		      requests[connection].bib =
			requests[connection].bib + count;
		      if (requests[connection].bib > 2)	// if at least 2 bib (Bytes In Buffer), we have request size
			{
			  requests[connection].size =
			    htons (*
				   ((unsigned short int *)
				    &requests[connection].buffer[0]));
			  if (requests[connection].size ==
			      requests[connection].bib - 2)
			    {
			      if (answer_request
				  (connection, requests[connection].size) < 0)
				{
				  EOC_dns (connection);
				}
			    }
			}
		    }
		}
	    }

	}
      client_length = sizeof (client);
      memset (&client, 0, sizeof client);
      if ((count = recvfrom (client_socket[UDP_CONNECTION], UDP_buffer, MAXREQUESTLEN, 0, (struct sockaddr *) &client, &client_length)) < 1)	// UDP connections
	{
	  if (errno != EAGAIN)
	    {
	      perror ("recvfrom");
	      exit (1);
	    }
	}
      else
	{
	  printf (gettext ("UDP request from %s, port %d\n"),
		  inet_ntoa (client.sin_addr), ntohs (client.sin_port));
	  answer_request (UDP_CONNECTION, count);
	    {
	      EOC_dns (UDP_CONNECTION);
	    }
	}
      fflush (stdout);
    }
  exit (0);
}
