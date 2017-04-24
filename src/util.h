/*
 * util.h: misc functions common to all desproxy binaries
 *
 * Copyright (C) 2003 Miguelanxo Otero Salgueiro
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the tems of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/*
 * Function : base64_encode (const char *string, char *p)
 * Purpose  : create a base 64 encoded string
 * Params   : p - string to encode
 *          : s - string buffer to write into
 *
 * Note     : Taken from transconnect (http://transconnect.sourceforge.net/)
 */
//static void encode_base64 (const char *s, char *p);

/*
 * Function : debug_printf(char *fmt, ...)
 * Purpose  : prints arguments like printf, but adds fflush(stdout)
 * Params   : same as printf, as defined in <stdio.h>
 *        
 * Note     : this function obtained via IRC chat in 
 *          : irc.openprojects.net, #c channel.
 */
int debug_printf (const char *fmt, ...);

/*
 *  Function : char *get_console_line(void)
 *  Purpose  : gets a line from console to char console_line[256]
 *           : (returns a pointer to global variable console_line)
 *  Params   : none
 */
char *get_console_line (void);

/*
 * Function : void strtolower(char *string)
 * Purpose  : changes char *string to lowercase
 * Params   : char *string - string to lowercase
 */
void strtolower (char *string);

/*
 * Function: void EOC (int connection)
 * Purpose : terminates connection gracefully
 * Params  : int connection - connection number to terminate
 */
void
EOC (int connection);

/*
 * Function : void mark_all_client_sockets_as_free(void)
 * Purpose  : initializes client sockets, marking all of them as free
 * Params   : none
 */
void mark_all_client_sockets_as_free (void);

/*
 * Function : void strnsend(int fd, char *string, int len)
 * Purpose  : almost same as write() + debugging
 * Params   : fd - file descriptor
 *          : char *string - data to write
 *          : int len - length of data
 */
void strnsend (int fd, char *string, int len);

/*
 * Function : void strsend(int fd, char *string)
 * Purpose  : send null terminated string to file descriptor
 * Params   : int fd - file descriptor
 *          : char *string - data to send
 */
void strsend (int fd, char *string);

/*
 * Function : void print_program_version(char *PROGRAM_NAME,
 *          : char *PROGRAM_VERSION);
 * Purpose  : outputs program name & version "window"
 * Params   : char *PROGRAM_NAME - program name
 *          : char *PROGRAM_VERSION - program version
 */
void print_program_version (char *PROGRAM_NAME, char *PROGRAM_VERSION);

/*
 * Function : char *parse_HTTP_return_code(void)
 * Purpose  : gets a parsed HTTP return code from line in buffer
 *          : (returns a pointer to global variable HTTP_return_code)
 * Params   : none
 */
char *parse_HTTP_return_code (void);

/*
 * Function : void wait_for_crlf(int fd)
 * Purpose  : reads data from file descriptor until sequence CR LF found
 *          : (data is stored in global variable buffer)
 * Params   : int fd - file descriptor
 * Returns  : 0 if Ok, -1 otherwise
 */
int wait_for_crlf (int fd);

/*
 * Function : void wait_for_2crlf(int fd)
 * Purpose  : reads file descriptor until sequence CR LR CR LF found
 *          : (that sequence is used to mark HTTP header end)
 * Params   : int fd - file descriptor to read from
 * Returns  : 0 if Ok, -1 otherwise
 */
int wait_for_2crlf (int fd);

/*
 * Function : int connect_host_to_proxy(int connection, char *remote_host
 *          : char *remote_port)
 * Purpose  : connects to remote_host:remote_port
 *          : trough proxy_host:proxy_port 
 * Params   : int connection - number of connection in use
 *          : char *remote_host - remote host (name or IP as string)
 *          : char *remote_port - remote port (number as string)
 */
int connect_host_to_proxy (int connection, char *remote_host,
			   char *remote_port);

/*
 * Function : void initialize_gettext(void)
 * Purpose  : initializes gettext (i18n) extension
 * Params   : none
 */
void initialize_gettext (void);

/*
 * Function : int listen_in_TCP_port(unsigned int request_port)
 * Purpose  : listens in requested TCP port for incoming connections
 * Params   : unsigned int request_port - requested port
 */
int listen_in_TCP_port (unsigned int request_port);

/*
 * Function : int bind_UDP_port(unsigned int request_port)
 * Purpose  : binds requested UDP port
 * Params   : unsigned int request_port - requested port
 */
int bind_UDP_port (unsigned int request_port);

/*
 * Function : void print_connection(int connection, char *string)
 * Purpose  : outputs info string for a connection
 * Params   : int connection - numbre of connection
 *          : char *string - text to output
 */
void print_connection (int connection, char *string);

/*
 * Function : int look_for_desproxy_conf(void)
 * NYI
 */
int look_for_desproxy_conf (void);

/*
 * Function : void get_username_and_password(void)
 * Purpose  : requests user to give username and password
 *          : strores them in global variables username & password
 * Params   : none
 */
void get_username_and_password (void);
