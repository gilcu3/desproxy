/*
 * desproxy.h: Main header file for desproxy
 *
 * Copyright (C) 2003 Miguelanxo Otero Salgueiro
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the tems of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <locale.h>
#include "util.h"

#ifdef HAVE_CONFIG_H
#include "../config.h"
#endif

#ifndef HAVE_LIBINTL_H
#warning
#warning ****************************
#warning *  Not using GNU gettext   *
#warning * for internationalization *
#warning *  (libintl.h not found)   *
#warning ****************************
#warning
#define gettext(A) A
#define bindtextdomain(A,B) ""
#define textdomain(A) ""
#else
#include <libintl.h>
#endif

#define PROXY_OK 0
#define BICONNECTED 1
#define PROXY_FAULT 2
#define END_OF_CONNECTION 3
#define NOTHING_READ 4
#define PROTOCOL_V5_OK 5
#define PROTOCOL_V4_OK 6
#define NMETHODS_READ 7
#define LOOKING_FOR_METHODS 8
#define METHOD_ACCEPTED_V4 9
#define METHOD_ACCEPTED_V5 10
#define TO_RESET 11

#define MAX_CONNECTIONS 10

#define BUFFER_SIZE 1500
#define VERSION "0.1.0-pre3"
/*
 * Debug level, 0 = none, 1 = basic, >1 = full
 */
#define DEBUG 0

int status, fd, maxfd;
int client_socket[MAX_CONNECTIONS];
int proxy_socket[MAX_CONNECTIONS];
int connection_status[MAX_CONNECTIONS];

char *proxy_host;
char *proxy_port;
char *dns_server;
char *local_port;

char username[256], password[256];
unsigned char string[256], console_line[256], HTTP_return_code[4];
unsigned char buffer[BUFFER_SIZE], client_socket_is_free[MAX_CONNECTIONS];

fd_set mask, rmask;

struct sockaddr_in proxy;
struct hostent *proxy_hostent;
struct sockaddr_in server;
struct sockaddr_in client;
struct sockaddr_in proxy;
struct sockaddr_in remote;

struct termios old_tty;

int client_length;
int server_length;
int proxy_length;
int remote_length;
