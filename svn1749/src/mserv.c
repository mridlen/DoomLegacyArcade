// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id: mserv.c 1651 2023-11-14 09:03:39Z wesleyjohnson $
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//
// $Log: mserv.c,v $
// Revision 1.33  2003/06/05 20:34:48  hurdler
// new master server host
//
// Revision 1.32  2003/05/04 02:34:17  sburke
// Define inet_aton() for Solaris, and make hostname arg const char *.
//
// Revision 1.31  2001/05/14 19:02:58  metzgermeister
//   * Fixed floor not moving up with player on E3M1
//   * Fixed crash due to oversized string in screen message ... bad bug!
//   * Corrected some typos
//   * fixed sound bug in SDL
//
// Revision 1.30  2001/05/12 16:33:08  hurdler
// fix master server registration problem under some OS
//
// Revision 1.29  2001/03/03 19:44:50  ydario
// Added OS/2 headers
//
// Revision 1.28  2001/02/24 13:35:20  bpereira
//
// Revision 1.27  2001/02/16 01:17:55  hurdler
// No need to convert msg->id
//
// Revision 1.26  2001/02/16 00:45:07  hurdler
//
// Revision 1.25  2001/01/11 01:15:57  hurdler
// Fix Little/Big Endian issue
//
// Revision 1.24  2001/01/05 18:17:43  hurdler
// fix master server bug
//
// Revision 1.23  2000/11/26 00:46:31  hurdler
// Revision 1.22  2000/10/22 00:38:22  hurdler
//
// Revision 1.21  2000/10/22 00:20:53  hurdler
// Updated for the latest master server code
//
// Revision 1.20  2000/10/21 23:21:56  hurdler
// Revision 1.19  2000/10/21 08:43:29  bpereira
//
// Revision 1.18  2000/10/17 10:09:27  hurdler
// Update master server code for easy connect from menu
//
// Revision 1.17  2000/10/16 20:02:29  bpereira
// Revision 1.16  2000/10/08 13:30:01  bpereira
//
// Revision 1.15  2000/10/07 18:36:50  hurdler
// fix a bug with Win2k
//
// Revision 1.14  2000/10/01 15:20:23  hurdler
// Add private server
//
// Revision 1.13  2000/09/14 10:39:59  hurdler
// Fix compiling problem under win32
//
// Revision 1.12  2000/09/10 10:45:14  metzgermeister
//
// Revision 1.11  2000/09/08 22:28:30  hurdler
// merge masterserver_ip/port in one cvar, add -private
//
// Revision 1.10  2000/09/02 15:38:24  hurdler
// Add master server to menus (temporaray)
//
// Revision 1.9  2000/09/01 18:23:42  hurdler
// fix some issues with latest network code changes
//
// Revision 1.8  2000/08/29 15:53:47  hurdler
// Remove master server connect timeout on LAN (not connected to Internet)
//
// Revision 1.7  2000/08/21 12:44:45  hurdler
// fix SOCKET not defined under some OS
//
// Revision 1.6  2000/08/21 11:06:44  hurdler
// Add ping and some fixes
//
// Revision 1.5  2000/08/16 23:39:41  hurdler
// fix a bug with windows sockets
//
// Revision 1.4  2000/08/16 17:21:50  hurdler
// update master server code (bis)
//
// Revision 1.3  2000/08/16 16:24:45  ydario
// OS/2 also needs inet_aton
//
// Revision 1.2  2000/08/16 15:44:18  hurdler
// update master server code
//
// Revision 1.1  2000/08/16 14:04:57  hurdler
// add master server code
//
//
//
// DESCRIPTION:
//      Commands used for communicate with the master server
//
//-----------------------------------------------------------------------------

#include "doomincl.h"

#ifdef WIN32
# include <winsock2.h>
  // socket(),...
# include <ws2tcpip.h>
  // socklen_t
#else
# include <unistd.h>
# ifdef __OS2__
#  include <sys/types.h>
     // [MB] 2020-06-16: Maybe required for old Unix too
# endif
# include <sys/socket.h>
  // socket(),...
# include <sys/time.h>
  // timeval,... (TIMEOUT)
# include <netinet/in.h>
  // sockaddr_in
# include <arpa/inet.h>
  // inet_addr(),...
# include <fcntl.h>
  // [MB] 2020-06-16: For fcntl()
# include <netdb.h>
  // gethostbyname(),...
# include <sys/ioctl.h>
# include <errno.h>
//#include <string.h>
  // memset(),...
//#include <sys/types.h>
  // socket(),...
#endif

#include "doomdata.h"
#include "command.h"
#include "console.h"
#include "mserv.h"
#include "i_tcp.h"
#include "i_net.h"
#include "i_system.h"
#include "d_clisrv.h"
#include "d_net.h"

// ================================ DEFINITIONS ===============================

#define  MS_NO_ERROR                   0
#define  MS_SOCKET_ERROR            -201
#define  MS_CONNECT_ERROR           -203
#define  MS_WRITE_ERROR             -210
#define  MS_READ_ERROR              -211
#define  MS_CLOSE_ERROR             -212
#define  MS_GETHOSTBYNAME_ERROR     -220
#define  MS_GETHOSTNAME_ERROR       -221
#define  MS_TIMEOUT_ERROR           -231

#if 0
// [WDJ] Unused
#define HEADER_MSG_POS      0
#define IP_MSG_POS         16
#define PORT_MSG_POS       32
#define HOSTNAME_MSG_POS   40
#endif

#ifndef SOCKET
#define SOCKET int
#endif

// --- Master Server message format

#define MS_PACKET_SIZE 1024

// see master server code for the values
enum {
   MSGTYPE_ADD_SERVER_MSG       = 101,
   MSGTYPE_REMOVE_SERVER_MSG    = 103,
   MSGTYPE_GET_SERVER_MSG       = 200,
   MSGTYPE_GET_SHORT_SERVER_MSG = 205
} master_server_msgtype_e;

// MS_Write modifies type, length to network byte order.
// [WDJ] Fix sizes to stdint.  This seems to be a master server msg format.
typedef struct {
    int32_t id;
    int32_t type;    // from master_server_msgtype_e
    int32_t length;  // when <0 then MS_Write will use strlen
    char    buffer[MS_PACKET_SIZE];
} msg_t;

//#define MS_HEADER_SIZE ((long)sizeof(long)*3)
#define MS_HEADER_SIZE ((int32_t)sizeof(int32_t)*3)
//#define MS_HEADER_SIZE  (offsetof(msg_t, buffer))


// ---

struct Copy_CVarMS_t
{
    char ip[64];
    char port[8];
    char name[64];
} registered_server;

// win32 or djgpp
#if defined( WIN32) || defined( __DJGPP__ ) 
#define ioctl ioctlsocket
#define close closesocket
#endif

#if defined( SOLARIS)
// [MB] 2020-06-16: Use native inet_aton() on Solaris
// Solaris has inet_aton() in libresolv since version 2.6 from 1997
#endif
#if defined( WIN32) || defined( __OS2__)
// it seems windows doesn't define that... maybe some other OS? OS/2
static inline
int inet_aton(const char *hostname,
              /* OUT */ struct in_addr *addr)
{
    // [WDJ] This cannot handle 255.255.255.255, which == INADDR_NONE.
    return ( (addr->s_addr=inet_addr(hostname)) != INADDR_NONE );
}   
#endif

static void Command_Listserv_f(void);
//TODO: when we change the port or ip, unregister to the old master server, register to the new one

#define MS_DEFAULT_PORT "28910"
consvar_t cv_internetserver= {"sv_public", "No", CV_SAVE, CV_YesNo };
consvar_t cv_masterserver  = {"masterserver", "doomlegacy.dyndns.org:28910", CV_SAVE, NULL };
consvar_t cv_servername    = {"sv_name", "Doom Legacy server", CV_SAVE, NULL };

typedef enum{
    MSCS_NONE,
    MSCS_WAITING,
    MSCS_REGISTERED,
    MSCS_FAILED
} con_state_e;

static con_state_e  con_state = MSCS_NONE;

// Using SOCK UDP for pinging the MasterServer.
// MS_PINGNODE: use this special node so to not tie up a player net node.
// This allows having all 32 players.

static byte msnode = 255;  // net node for pinging the MasterServer.

// MasterServer communications.
static SOCKET               ms_socket_fd = -1;  // TCP/IP socket
static struct sockaddr_in   ms_addr;

#define  ASYNC   1
static int  MS_Connect(char *ip_addr, char *str_port, int async_flag);
static boolean  MS_Connect_MasterServer(void);
static int  MS_Read(msg_t *msg);
static int  MS_Write(msg_t *msg);
static int  MS_GetIP(char *);

// Called by D_Register_ClientCommands.
void MS_Register_Commands(void)
{
    CV_RegisterVar(&cv_internetserver);
    CV_RegisterVar(&cv_masterserver);
    CV_RegisterVar(&cv_servername);
    COM_AddCommand("listserv", Command_Listserv_f, CC_net);
}

static void MS_Close_socket(void)
{
    if (ms_socket_fd > 0)
        close(ms_socket_fd);
    ms_socket_fd = -1;
}

static int GetServersList(void)
{
    msg_t   msg;
    int     count = 0;

    msg.type = MSGTYPE_GET_SERVER_MSG;
    msg.length = 0;
    if (MS_Write(&msg) < 0)
        return MS_WRITE_ERROR;

    while (MS_Read(&msg) >= 0)
    {
        if (msg.length == 0)
        {
            if (!count)
                GenPrintf(EMSG_hud, "No server currently running.\n");
            return MS_NO_ERROR;
        }
        count++;
        GenPrintf(EMSG_hud, msg.buffer);
    }
    return MS_READ_ERROR;
}

#define NUM_LIST_SERVER 10

// Return NULL when no servers list gotten.
msg_server_t * MS_Get_ShortServersList(void)
{
    static msg_server_t server_list[NUM_LIST_SERVER+1]; // +1 for easy test
    msg_t  msg;
    int    i;

    //arf, we must be connected to the master server before writing to it
    if( ! MS_Connect_MasterServer() )
        return NULL;

    msg.type = MSGTYPE_GET_SHORT_SERVER_MSG;
    msg.length = 0;
    if (MS_Write(&msg) < 0)
        return NULL;

    for (i=0; i<NUM_LIST_SERVER; i++)
    {
        if( MS_Read(&msg) < 0 )  goto read_err;
        if (msg.length == 0)  break;
        memcpy(&server_list[i], msg.buffer, sizeof(msg_server_t));
        server_list[i].header[0] = 1;
    }
    // End of server list.
    server_list[i].header[0] = 0;
    MS_Close_socket();
    return server_list;

read_err:
    MS_Close_socket();
    return NULL;
}

static void Command_Listserv_f(void)
{
    if (con_state == MSCS_WAITING)
    {
        GenPrintf(EMSG_hud, "Not yet registered to the master server.\n");
        return;
    }

    GenPrintf(EMSG_hud, "Retrieving server list ...\n");

    if( ! MS_Connect_MasterServer() )
        return;

    if (GetServersList())
        GenPrintf(EMSG_warn, "Cannot get server list.\n");

    MS_Close_socket();
}

static char *int2str(int n)
{
    int         i;
    static char res[16];

    res[15] = '\0';
    res[14] = (n%10)+'0';
    for (i=13; (n /= 10); i--)
        res[i] = (n%10)+'0';

    return &res[i+1];
}

// Return MS error code.
static int MS_failed_connect(void)
{
    con_state = MSCS_FAILED;
    GenPrintf(EMSG_error, "Connection to master server failed\n");
    MS_Close_socket();
    return MS_CONNECT_ERROR;
}

// Return MS error code.
static int  send_master_server( char * name, int32_t msgtype )
{
    msg_t           msg;
    msg_server_t  * info = (msg_server_t *) msg.buffer;

    char wbuf[40];

    strcpy(info->header, "");
    strcpy(info->ip,     "");
    strcpy(info->port,   int2str(server_sock_port));
    strncpy(info->name,   name, 31);
    info->name[31] = 0;

    // write to larger buffer, because gcc complains about overrun
    snprintf(wbuf, 39, "%1d.%2d.%2d", VERSION/100, VERSION%100, REVISION);
    wbuf[7] = 0;
    memcpy(info->version, wbuf, 8);  // length 8

    msg.type = msgtype;
    msg.length = sizeof(msg_server_t);
    if (MS_Write(&msg) < 0)
        return MS_WRITE_ERROR;

    return MS_NO_ERROR;
}

// Return MS error code.
static int RegisterInfo_on_MasterServer(void)
{
    static int      retry = 0;
    // [WDJ] Linux: select alters the timeout, it must be init each usage.
    struct timeval  select_timeout = {0,0};
    fd_set          write_set;  // modified by select
    int             res;
    int             optval;
    socklen_t       optlen;


    // [WDJ] Simpler, smaller code to setup write_set on usage.
    FD_ZERO(&write_set);
    FD_SET(ms_socket_fd, &write_set);  
    res = select(ms_socket_fd+1,
                 NULL,  // read fd
                 /*IN,OUT*/ &write_set,  // write fd to watch
                 NULL,  // exceptions
                 /*IN,OUT*/ &select_timeout);  // modified
    if (res == 0)
    {
        if (retry++ > 30) // an about 30 second timeout
        {
            retry = 0;
            GenPrintf(EMSG_error, "Timeout on masterserver\n");
            return MS_failed_connect();
        }
        return MS_CONNECT_ERROR;
    }
    retry = 0;
    if (res < 0)
    {
        GenPrintf(EMSG_error, "Masterserver register: %s\n", strerror(errno));
        return MS_failed_connect();
    }
    
    // so, the socket is writable, but what does that mean, that the connection is
    // ok, or bad... let see that!
    optlen = 4;  // Linux alters this value.
#ifdef LINUX   
    getsockopt(ms_socket_fd, SOL_SOCKET, SO_ERROR, &optval, &optlen);
#else
    getsockopt(ms_socket_fd, SOL_SOCKET, SO_ERROR, (char *)&optval, &optlen);
#endif
    if (optval != 0) // it was bad
    {
        GenPrintf(EMSG_error, "Masterserver register getsockopt: %s\n", strerror(errno));
        return MS_failed_connect();
    }

    strcpy(registered_server.name, cv_servername.string);
    if( send_master_server( cv_servername.string, MSGTYPE_ADD_SERVER_MSG )  < 0 )
        return MS_failed_connect();

    GenPrintf(EMSG_hud, "This server has been registered on the master server.\n");
    con_state = MSCS_REGISTERED;
    MS_Close_socket();

    return MS_NO_ERROR;
}

// Return MS error code.
static int RemoveInfo_from_MasterServer(void)
{
    return  send_master_server( registered_server.name, MSGTYPE_REMOVE_SERVER_MSG );
}

// Return ptr to port number string.
char * MS_Get_MasterServerPort(void)
{
    char *t = cv_masterserver.string;

    while ((*t != ':') && (*t != '\0'))
        t++;

    if (*t)
        return ++t;

    return MS_DEFAULT_PORT;  // default MasterServer port
}

// Return ptr to IP address string.
char * MS_Get_MasterServerIP(void)
{
    static char str_ip[64];
    char        *t = str_ip;

    if (strstr(cv_masterserver.string, "doomlegacy.dhs.org"))
    {
        // replace it with the current default one
        CV_Set(&cv_masterserver, cv_masterserver.defaultvalue);
    }
    strcpy(t, cv_masterserver.string);

    while ((*t != ':') && (*t != '\0'))
        t++;
    *t = '\0';
    
    return str_ip;
}



static void MS_open_UDP_Socket()
{
    // Setup ping UDP addr from MasterServer IP, MasterServer port + 1.
    uint32_t  ping_port = atoi(MS_Get_MasterServerPort()) + 1;

    if( I_NetMakeNode )  // UDP functions connected
    {
#ifdef MS_PINGNODE
        // Using a special node.
        msnode = MS_PINGNODE;
        UDP_Bind_Node(MS_PINGNODE, ms_addr.sin_addr.s_addr, ping_port );
#else
        // Using a player node.  This ties up a player node.
        char hostname[24];

        sprintf(hostname, "%s:%d", inet_ntoa(ms_addr.sin_addr), ping_port );
        msnode = I_NetMakeNode(hostname);
          // errors are > MAXNETNODES+2
#endif
    }
    else
        msnode = 255;
}

// By Server.
// MasterServer address is in cv_masterserver.
void MS_RegisterServer(void)
{
    GenPrintf(EMSG_hud, "Registering this server to the master server ...\n");

    strcpy(registered_server.ip, MS_Get_MasterServerIP());
    strcpy(registered_server.port, MS_Get_MasterServerPort());

    if (MS_Connect(registered_server.ip, registered_server.port, ASYNC) < 0)
    {
        GenPrintf(EMSG_hud, "Cannot connect to the master server\n");
        return;
    }
    MS_open_UDP_Socket();

    // Keep the TCP connection open until RegisterInfo_on_MasterServer() is completed;
}

// By Server.
// Called by NetUpdate.
void MS_SendPing_MasterServer( tic_t cur_time )
{
    static tic_t   next_time = 0;

    if (cur_time > next_time) // ping every 2 second if possible
    {
        next_time = cur_time + (2*TICRATE);  // 2 sec

        if (con_state == MSCS_WAITING)
            RegisterInfo_on_MasterServer();

        if (con_state != MSCS_REGISTERED)
            return;

        // Keep-alive tick to the MasterServer on MasterServer port+1.
        // cur_time is just a dummy data to send
        if( msnode > MAXNETNODES+1 )
            return;  // no UDP connection

        *((tic_t *)netbuffer) = cur_time;
        doomcom->datalength = sizeof(cur_time);
        doomcom->remotenode = msnode;
        I_NetSend();  // to packet port
    }
}

void MS_UnregisterServer()
{
    if (con_state != MSCS_REGISTERED)
    {
        con_state = MSCS_NONE;
        MS_Close_socket();
        return;
    }
    con_state = MSCS_NONE;

    GenPrintf(EMSG_hud, "Unregistering this server to the master server...\n");

    if (MS_Connect(registered_server.ip, registered_server.port, 0) < 0)
    {
        GenPrintf(EMSG_error, "  Cannot connect to the master server\n");
        return;
    }

    if (RemoveInfo_from_MasterServer() < 0)
        GenPrintf(EMSG_error, "Cannot remove this server from the master server\n");

    MS_Close_socket();
    I_NetFreeNode( msnode );  // can be used on special net nodes too
}

/*
** MS_GetIP()
*/
static int MS_GetIP(char *hostname)
{
    struct hostent *host_ent;

    if (!inet_aton(hostname, &ms_addr.sin_addr))
    {
        //TODO: only when we are connected to Internet, or use a non bloking call
        host_ent = gethostbyname(hostname);
        if (host_ent==NULL)
            return MS_GETHOSTBYNAME_ERROR;

        memcpy(&ms_addr.sin_addr, host_ent->h_addr_list[0], sizeof(struct in_addr));
    }
    return 0;
}


/*
** MS_Connect()
*/
static int MS_Connect(char *ip_addr, char *str_port, int async_flag)
{
    int res;
   
    memset(&ms_addr, 0, sizeof(ms_addr));
    ms_addr.sin_family = AF_INET;
    I_Init_TCP_Driver(); // this is done only if not already done

    // TCP connection socket.
    ms_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(ms_socket_fd < 0)
        return MS_SOCKET_ERROR;

    if( MS_GetIP(ip_addr) == MS_GETHOSTBYNAME_ERROR)
        return MS_GETHOSTBYNAME_ERROR;

    ms_addr.sin_port = htons( atoi(str_port) );

    if(async_flag) // do asynchronous connection
    {
        // Set ms_socket as non-blocking.
#ifdef LINUX
        // [WDJ] DoomLegacy 1.48.6
        // [MB] 2020-06-16: Use portable POSIX way to enable non-blocking mode
        // https://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html
        // [WDJ] POSIX.1-2001 : F_SETFL
        // Windows: Winsock does not have fcntl.
        res = fcntl(ms_socket_fd, F_SETFL, O_NONBLOCK);
        if( res == -1 )
            goto fail_close;

#elif  defined( WIN32 )
        // winsock.h:  int ioctlsocket(SOCKET,long,u_long *);
        u_long test = 1; // [smite] I have no idea what this type is supposed to be
        ioctlsocket(ms_socket_fd, FIONBIO, &test);
#else
        // Does not work on: SunOS (SmartOS and maybe Solaris).
        // Linux: still defined in asm-generic/ioctls.h
        // Needed for older ports.
        res = 1;  // non-blocking true
        ioctl(ms_socket_fd, FIONBIO, &res);
#endif
        res = connect(ms_socket_fd, (struct sockaddr *) &ms_addr, sizeof(ms_addr));
        if (res < 0)
        {
#ifdef WIN32
            // Winsock uses WSA prefix.
	    // Socket returns EWOULDBLOCk, different value than EINPROGRESS.
            if (WSAGetLastError() != WSAEWOULDBLOCK)
                goto fail_close;
#else
            // Linux: EWOULDBLOCK has same value as EINPROGRESS (pref).
            if (errno != EINPROGRESS)
                goto fail_close; // Socket is nonblock, and operation cannot be completed immediately.
#endif
        }
        con_state = MSCS_WAITING;
    }
    else
    {
        res = connect(ms_socket_fd, (struct sockaddr *) &ms_addr, sizeof(ms_addr));
        if (res < 0)
            return MS_CONNECT_ERROR;
    }

    return 0;

fail_close:
    con_state = MSCS_FAILED;
    MS_Close_socket();
    return MS_CONNECT_ERROR;
}


static boolean MS_Connect_MasterServer(void)
{
    if( MS_Connect(MS_Get_MasterServerIP(), MS_Get_MasterServerPort(), 0) < 0)
    {
        GenPrintf(EMSG_error, "Cannot connect to the master server.\n");
        return false;
    }
    return true;
}


/*
 * MS_Write():
 */
static int MS_Write(msg_t *msg)
{
    int len, cnt;

    if (msg->length < 0)
        msg->length = strlen(msg->buffer);
    len = msg->length+MS_HEADER_SIZE;

    // htonl has uint32_t operand and result.
    //msg->id = htonl(msg->id);
    msg->type = htonl(msg->type);
    msg->length = htonl(msg->length);
   
    cnt = send(ms_socket_fd, (char*)msg, len, 0);
    if ( cnt != len)
        return MS_WRITE_ERROR;

    return 0;
}


/*
 * MS_Read():
 */
static int MS_Read(msg_t *msg)
{
    int cnt, msglen;
   
    cnt = recv(ms_socket_fd, (char*)msg, MS_HEADER_SIZE, 0);
    if( cnt != MS_HEADER_SIZE)
        return MS_READ_ERROR;

    // ntohl has uint32_t operand and result.
    //msg->id = ntohl(msg->id);
    msg->type = ntohl(msg->type);
    msg->length = ntohl(msg->length);

    // If only a header and no data, do not read the data.
    if (!msg->length) //Hurdler: fix a bug in Windows 2000
        return 0;

    // Protection against attackers.
    msglen = msg->length;
    if( msglen > MS_PACKET_SIZE )
        msglen = MS_PACKET_SIZE;  // limit, to not overrun buffer

    cnt = recv(ms_socket_fd, (char*)msg->buffer, msglen, 0);
    if(cnt != msglen)
        return MS_READ_ERROR;

    return 0;
}
