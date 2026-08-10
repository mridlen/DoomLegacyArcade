// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id: i_tcp.c 1548 2020-09-29 10:26:04Z wesleyjohnson $
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
// $Log: i_tcp.c,v $
// Revision 1.40  2004/04/20 00:34:26  andyp
// Linux compilation fixes and string cleanups
//
// Revision 1.39  2003/05/04 02:31:53  sburke
// Added many #ifdef, #ifndef SOLARIS.
//
// Revision 1.38  2003/01/19 21:24:26  bock
// Make sources buildable on FreeBSD 5-CURRENT.
//
// Revision 1.37  2001/08/26 15:27:29  bpereira
// added fov for glide and fixed newcoronas code
//
// Revision 1.36  2001/08/21 21:53:37  judgecutor
// Fixed incorect place of #include "d_main.h"
//
// Revision 1.35  2001/08/20 20:40:39  metzgermeister
//
// Revision 1.34  2001/05/16 22:33:34  bock
// Initial FreeBSD support.
//
// Revision 1.33  2001/02/24 13:35:20  bpereira
// Revision 1.32  2001/02/10 12:27:13  bpereira
//
// Revision 1.31  2001/01/05 18:17:43  hurdler
// fix master server bug
//
// Revision 1.30  2000/11/26 00:46:31  hurdler
// Revision 1.29  2000/10/21 08:43:29  bpereira
// Revision 1.28  2000/10/16 20:02:29  bpereira
// Revision 1.27  2000/10/08 13:30:00  bpereira
//
// Revision 1.26  2000/10/01 15:20:23  hurdler
// Add private server
//
// Revision 1.25  2000/09/28 20:57:15  bpereira
// Revision 1.24  2000/09/15 19:49:22  bpereira
// Revision 1.23  2000/09/10 10:43:21  metzgermeister
//
// Revision 1.22  2000/09/08 22:28:30  hurdler
// merge masterserver_ip/port in one cvar, add -private
//
// Revision 1.21  2000/09/01 18:23:42  hurdler
// fix some issues with latest network code changes
//
// Revision 1.20  2000/08/31 14:30:55  bpereira
//
// Revision 1.19  2000/08/29 15:53:47  hurdler
// Remove master server connect timeout on LAN (not connected to Internet)
//
// Revision 1.18  2000/08/21 11:06:44  hurdler
// Add ping and some fixes
//
// Revision 1.17  2000/08/17 23:18:05  hurdler
// fix bad port sent to master server when using -udpport
//
// Revision 1.16  2000/08/16 23:39:41  hurdler
// fix a bug with windows sockets
//
// Revision 1.15  2000/08/16 17:21:50  hurdler
// update master server code (bis)
//
// Revision 1.14  2000/08/16 15:44:18  hurdler
// update master server code
//
// Revision 1.13  2000/08/16 14:10:01  hurdler
// add master server code
//
// Revision 1.12  2000/08/10 14:55:56  ydario
// OS/2 port
//
// Revision 1.11  2000/08/10 14:08:48  hurdler
// Revision 1.10  2000/08/03 17:57:42  bpereira
//
// Revision 1.9  2000/04/21 13:03:27  hurdler
// apply Robert's patch for SOCK_Get error. Boris, can you verify this?
//
// Revision 1.8  2000/04/21 00:01:45  hurdler
// apply Robert's patch for SOCK_Get error. Boris, can you verify this?
//
// Revision 1.7  2000/04/16 18:38:07  bpereira
// Revision 1.6  2000/03/29 19:39:48  bpereira
//
// Revision 1.5  2000/03/08 14:44:52  hurdler
// fix "select" problem under linux
//
// Revision 1.4  2000/03/07 03:32:24  hurdler
// fix linux compilation
//
// Revision 1.3  2000/03/06 15:46:43  hurdler
// Revision 1.2  2000/02/27 00:42:10  hurdler
// Revision 1.1.1.1  2000/02/22 20:32:32  hurdler
// Initial import into CVS (v1.29 pr3)
//
//
// DESCRIPTION:
// NOTE:    This is not realy Os dependant because all Os have the same Socket api
//          Just use '#ifdef' for Os dependant stuffs
//
//-----------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef __OS2__
  // sys/types.h is also included unconditionally by doomincl.h
# include <sys/types.h>
  // [MB] 2020-06-18: Maybe required for old Unix too
# include <sys/time.h>
#endif // __OS2__

#include "doomincl.h"

#ifdef __WIN32__
# include <winsock2.h>
# include <ws2tcpip.h>
# ifdef USE_IPX
# include <wsipx.h>
# endif // USE_IPX

#else
  // Not Windows

# if !defined(SCOUW2) && !defined(SCOUW7) && !defined(__OS2__)
#  include <arpa/inet.h>
# endif

// non-windows includes
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
  // [MB] 2020-06-18: For fcntl()
#include <unistd.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <time.h>


// Enable debug_Printf stmts
//#define NET_NODE_DEBUG  1

// Just some standard length for a char string
#define STD_STRING_LEN    256

// [WDJ] FIXME: Add some test of IPX headers being present.
// When someone puts an IPX package on their system, this will prevent
// using it.

#ifdef SOLARIS
// Previous code: Solaris did not have IPX.
# ifdef USE_IPX
#   undef USE_IPX
# endif
#endif

#ifdef NETBSD
// NetBSD does not have IPX.
# ifdef USE_IPX
#   undef USE_IPX
# endif
#endif

// Reported to be __OpenBSD__ , but it should be all caps and I am paranoid.
#if defined( __OpenBSD__ ) || defined( __OPENBSD__ )
// OpenBSD does not have IPX.
# ifdef USE_IPX
#   undef USE_IPX
# endif
#endif

#ifdef __DJGPP__
#include <lsck/lsck.h>
#ifdef USE_IPX
//#define strerror  lsck_strerror
// ipx not yet supported in libsocket (cut and pasted from wsipx.h (winsock)
// [WDJ] Updated to stdint
typedef struct sockaddr_ipx {
    int16_t   sa_family;
    char  sa_netnum[4];
    char  sa_nodenum[6];
    uint16_t  sa_socket;
} SOCKADDR_IPX, *PSOCKADDR_IPX;
#define NSPROTO_IPX      1000
#endif // USE_IPX
#endif // djgpp


#ifdef __OS2__
#ifdef USE_IPX
// ipx not yet supported in libsocket (cut and pasted from wsipx.h (winsock)
#define AF_IPX          23              /* Novell Internet Protocol */
// [WDJ] Updated to stdint.
typedef struct sockaddr_ipx {
    int16_t   sa_family;
    char  sa_netnum[4];
    char  sa_nodenum[6];
    uint16_t  sa_socket;
} SOCKADDR_IPX, *PSOCKADDR_IPX;
#define NSPROTO_IPX      1000
#endif // USE_IPX
#endif // os2

#ifdef MACOS_DI
// [WDJ] 4/2015 Merged macos/i_tcp.c here, very few significant differences.
// They did not have any IPX code.  It would need includes.
# undef USE_IPX
#endif

#ifdef LINUX
# include <sys/time.h>
# ifdef USE_IPX
#  ifdef __GLIBC__
#   include <netipx/ipx.h>
#  else
#   ifdef FREEBSD
#    include <netipx/ipx.h>
#   else
#    include <linux/ipx.h>
#   endif
#  endif // glibc
    typedef struct sockaddr_ipx SOCKADDR_IPX, *PSOCKADDR_IPX;
# define NSPROTO_IPX      PF_IPX
# endif // USE_IPX
#endif // linux

  // END of Not Windows
#endif // win32

#ifdef __APPLE__
// IPX is not supported on macOS
# ifdef USE_IPX
#   undef USE_IPX
# endif
#endif

#include "i_system.h"
#include "i_net.h"
#include "d_net.h"
#include "m_argv.h"
#include "command.h"
#include "d_main.h"

#include "doomstat.h"
#include "mserv.h" //Hurdler: support master server

#ifdef __WIN32__
    // some undefined under win32
# ifdef errno
#  undef errno
# endif
   // some very strange things happen when not use h_error ?!?
#  define errno         h_errno
#else
   // linux, djgpp, os2, non-windows
#  define  SOCKET int
#  define  INVALID_SOCKET -1
#endif

#if defined( WIN32) || defined( __DJGPP__ ) 
   // win32 or djgpp
    // winsock stuff (in winsock a socket is not a file)
#  define ioctl ioctlsocket
#  define close closesocket
#endif

#ifdef USE_IPX
static boolean  ipx_select;
#endif

// Set default sock_port to 5029, which is necessary to find a server on the network.
// If this actually ever moved, we would have to ensure uniformity over all systems.
#ifdef IPPORT_USERRESERVED
# if IPPORT_USERRESERVED != 5000
#   warning IPPORT_USERRESERVED non-standard, DoomLegacy uses sock port 5029.
# endif
#else
# define IPPORT_USERRESERVED 5000
#endif
// [WDJ] If there are variations, set it to 5029 to keep all servers uniform.
//#define IPPORT_SERVER       5029
#define IPPORT_SERVER       (IPPORT_USERRESERVED +0x1d )

// Enable use of alternate client port range.
#define CLIENT_SOCK_PORT_SEARCH     1
// Alternate port addresses for clients.
#define IPPORT_CLIENT       7000
#define IPPORT_CLIENT_MAX   7099


// IP port numbers are 16 bit.
uint16_t server_sock_port = IPPORT_SERVER;
static uint16_t client_sock_port = IPPORT_SERVER;  // default
static uint16_t my_sock_port = 0;  // From UDP_Socket or IPX_Socket

static SOCKET   mysocket = -1;


// A network address, kept in network byte order.
typedef union {
        struct sockaddr_in  ip;
#ifdef USE_IPX
        struct sockaddr_ipx ipx;
#endif
}  mysockaddr_t;

// Player and additional net nodes.
static mysockaddr_t clientaddress[MAX_CON_NETNODE];


// Enables node_hash functions.
// When off, use a generic node hash.
#define NODE_ADDR_HASHING

// Contains address hash, is 0 when unused.  Hash is not allowed to be 0.
// Node connected when node_hash > 0.
// To receive, node_hash must have been set to hash of clientaddress.
// To send, any hash value > 0 will enable.  Using 1 will not allow receive.
static byte     node_hash[MAX_CON_NETNODE];

#ifdef NODE_ADDR_HASHING
// Customized hash function.
byte    (*SOCK_hashaddr) (mysockaddr_t *a);
#else
// For all network types.
static
byte  generic_hashaddr( mysockaddr_t *a )
{
    // For any structure.
    // Not allowed to be 0. Always node_hash is connected flag.
    byte * p = &(a->ip);
    return (p[0] ^ p[3]) | 0x80;
}
#endif



// Network is big-endian, 386,486,586 PC are little-endian.
// htons: host to net byte order
// ntohs: net to host byte order

#if defined( SOLARIS)
// [WDJ] Was using init_aton defined below ..., DoomLegacy 1.48 and before.
// [MB] 2020-06-18: Use native inet_aton() on Solaris
// Solaris has inet_aton() in libresolv since version 2.6 from 1997
#endif
#if defined( WIN32) || defined( __OS2__)
// [WDJ] Also defined in mserv.c, but too small, will be inlined anyway.
static inline
int inet_aton(const char *hostname,
              /* OUT */ struct in_addr *addr)
{
    // [WDJ] This cannot handle 255.255.255.255, which == INADDR_NONE.
    addr->s_addr = inet_addr(hostname);
    return ( addr->s_addr != INADDR_NONE );
}   
#endif

// To print error messages
char *SOCK_AddrToStr(mysockaddr_t *sk)
{
    static char s[50];

    if( sk->ip.sin_family==AF_INET)
    {
        // Internet address
        sprintf(s,"%d.%d.%d.%d:%d",((byte *)(&(sk->ip.sin_addr.s_addr)))[0],
                                   ((byte *)(&(sk->ip.sin_addr.s_addr)))[1],
                                   ((byte *)(&(sk->ip.sin_addr.s_addr)))[2],
                                   ((byte *)(&(sk->ip.sin_addr.s_addr)))[3],
                                   ntohs(sk->ip.sin_port));
    }
#ifdef USE_IPX
    else
#ifdef LINUX
    if( sk->ipx.sipx_family==AF_IPX )
    {
# ifdef FREEBSD
        // FreeBSD IPX
        sprintf(s,"%s", ipx_ntoa(sk->ipx.sipx_addr));
# else
        // Linux IPX, but Not FreeBSD
        sprintf(s,"%08x.%02x%02x%02x%02x%02x%02x:%d",
#  if 1
                  ntohl(sk->ipx.sipx_network),  // big_endian, 32bit
#  else
                  sk->ipx.sipx_network,
#  endif
                  (byte)sk->ipx.sipx_node[0],
                  (byte)sk->ipx.sipx_node[1],
                  (byte)sk->ipx.sipx_node[2],
                  (byte)sk->ipx.sipx_node[3],
                  (byte)sk->ipx.sipx_node[4],
                  (byte)sk->ipx.sipx_node[5],
                  sk->ipx.sipx_port);
# endif
    }
#else
    // IPX Windows, OS2, DJGPP
    if( sk->ipx.sa_family==AF_IPX )
    {
        // IPX address
        sprintf(s,"%02x%02x%02x%02x.%02x%02x%02x%02x%02x%02x:%d",
                  (byte)sk->ipx.sa_netnum[0],
                  (byte)sk->ipx.sa_netnum[1],
                  (byte)sk->ipx.sa_netnum[2],
                  (byte)sk->ipx.sa_netnum[3],
                  (byte)sk->ipx.sa_nodenum[0],
                  (byte)sk->ipx.sa_nodenum[1],
                  (byte)sk->ipx.sa_nodenum[2],
                  (byte)sk->ipx.sa_nodenum[3],
                  (byte)sk->ipx.sa_nodenum[4],
                  (byte)sk->ipx.sa_nodenum[5],
                  sk->ipx.sa_socket);
    }
#endif // linux
#endif // USE_IPX
    else
        sprintf(s,"Unknown type");
    return s;
}

#ifdef USE_IPX
boolean IPX_cmpaddr(mysockaddr_t *a, mysockaddr_t *b)
{
#ifdef LINUX
# ifdef FREEBSD
    // FreeBSD: IPX address compare
    return ipx_neteq( a->ipx.sipx_addr, b->ipx.sipx_addr) &&
           ipx_hosteq( a->ipx.sipx_addr, b->ipx.sipx_addr );
# else
    // Linux (except FreeBSD): IPX address compare
    return ((memcmp(&(a->ipx.sipx_network) ,&(b->ipx.sipx_network) ,4)==0) &&
            (memcmp(&(a->ipx.sipx_node),&(b->ipx.sipx_node),6)==0));
# endif
#else
    // Windows, OS2, DJGPP: IPX address compare
    return ((memcmp(&(a->ipx.sa_netnum) ,&(b->ipx.sa_netnum) ,4)==0) &&
            (memcmp(&(a->ipx.sa_nodenum),&(b->ipx.sa_nodenum),6)==0));
#endif // linux
}

#ifdef NODE_ADDR_HASHING
byte  IPX_hashaddr(mysockaddr_t *a)
{
    // Not allowed to be 0.
    // Big endian, want final addr byte.
#ifdef LINUX
# ifdef FREEBSD
    return ((byte)(a->ipx.sipx_addr.x_host.c_host[5])) | 0x80;
# else
    // Linux: IPX address hash
    return ((byte)(a->ipx.sipx_node[5])) | 0x80;
# endif
#else
    // Windows, OS2, DJGPP: IPX address hash
    return ((byte)(a->ipx.sa_nodenum[5])) | 0x80;
#endif // linux
}
#endif

#endif // USE_IPX

boolean UDP_cmpaddr(mysockaddr_t *a, mysockaddr_t *b)
{
#ifdef MACOS_DI   
    return (a->ip.sin_addr.s_addr == b->ip.sin_addr.s_addr);
#else
    return (a->ip.sin_addr.s_addr == b->ip.sin_addr.s_addr
            && a->ip.sin_port == b->ip.sin_port);
#endif
}

#ifdef NODE_ADDR_HASHING
byte  UDP_hashaddr(mysockaddr_t *a)
{
    // Not allowed to be 0.
    // Big endian, want final addr byte.
    return ((byte*)(&(a->ip.sin_addr.s_addr)))[3] | 0x80;
}
#endif


// Indirect function for net address compare.
boolean (*SOCK_cmpaddr) (mysockaddr_t *a, mysockaddr_t *b);


// Set address and port of utility net nodes.
//  saddr: IP address in network byte order
//  port: port number in host byte order
// Called by: Bind_Node_str, mserv:MS_open_UDP_Socket
void UDP_Bind_Node( int nnode, unsigned int saddr, uint16_t port )
{
    clientaddress[nnode].ip.sin_family      = AF_INET;
    clientaddress[nnode].ip.sin_port        = htons(port);
    clientaddress[nnode].ip.sin_addr.s_addr = saddr;
#ifdef NODE_ADDR_HASHING
    node_hash[nnode] = UDP_hashaddr( &clientaddress[nnode] );
#else
    node_hash[nnode] = generic_hashaddr( &clientaddress[nnode] );
#endif
}


// Setup broadcast address to BROADCAST_NODE entry.
// To send broadcasts, PT_ASKINFO.
// [WDJ] Broadcast address for network "192.168.1.x" is "192.168.1.255".
// INADDR_BROADCAST is "255.255.255.255" which gives network unreachable.

// By Client.
// IPX or inet.
// Bind an inet or ipx address string to a net node.
//  addrstr: net address in special format
//  port: port number in host byte order
// Called by: CL_Broadcast_AskInfo
boolean  Bind_Node_str( int nnode, char * addrstr, uint16_t port )
{
    mysockaddr_t address;

    if( addrstr == NULL )   goto addr_fail;
    if( addrstr[0] < '0' )  goto addr_fail;

#ifdef USE_IPX
    if(ipx_select)
    {
        // Network byte order is big-endian first.
        int  cnt;

# ifdef LINUX
        // IPX address format (HEX) "7F20540F:5C0020040101"
        // Keep it big-endian order, so do not have to convert.
        byte   ba[11];
        cnt = sscanf(addrstr,
              "%02hhx%02hhx%02hhx%02hhx.%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
            &ba[0], &ba[1], &ba[2], &ba[3],
            &ba[4], &ba[5], &ba[6], &ba[7], &ba[8], &ba[9] );
        if( cnt != 10 )  goto addr_fail;  // need exactly 10 digits

        clientaddress[nnode].ipx.sipx_family = AF_IPX;
        clientaddress[nnode].ipx.sipx_port = htons(port);
#  ifdef FREEBSD
        // FreeBSD
        // network: ipx.sipx_addr.xnet.s_net[0..1]   16 bit, big endian
        // addr: ipx.sipx_addr.x_host.c_host[0..5]   8 bit
        memcpy(&clientaddress[nnode].ipx.sipx_addr.x_net.s_net[0], &ba[0], 4 );
        memcpy(&clientaddress[nnode].ipx.sipx_addr.x_host.c_host[0], &ba[4], 6 );
#  else
        // Linux, but Not FreeBSD
        // network: ipx.sipx_network   32 bit, big endian
        // addr: ipx.sipx_node[0..5]   8 bit
        memcpy(&clientaddress[nnode].ipx.sipx_network, &ba[0], 4 );
        memcpy(&clientaddress[nnode].ipx.sipx_node[0], &ba[4], 6 );
#  endif
# else
        // Windows, etc.
        // IPX address format (HEX) "7F20540F:5C0020040101"
        // Keep it big-endian order, so do not have to convert.
        // Windows is missing the hh conversion, at least in MINGW.
        int  i;
        int  ib[4], ic[6];
        cnt = sscanf(addrstr,
              "%02x%02x%02x%02x.%02x%02x%02x%02x%02x%02x",
            &ib[0], &ib[1], &ib[2], &ib[3],
            &ic[0], &ic[1], &ic[2], &ic[3], &ic[4], &ic[5] );
        if( cnt != 10 )  goto addr_fail;  // need exactly 10 digits

        // Convert int to byte, keeping BIG endian.
        // network: ipx.sa_netnum[0..3]   8 bit
        // addr:   ipx.sa_nodenum[0..5]   8 bit
        for( i=0; i<4; i++ )
            clientaddress[nnode].ipx.sa_netnum[i] = ib[i];
        for( i=0; i<6; i++ )
            clientaddress[nnode].ipx.sa_nodenum[i] = ic[i];
# endif // linux

#if 0       
# ifdef NODE_ADDR_HASHING
        node_hash[nnode] = IPX_hashaddr( &clientaddress[nnode] );
# else
        node_hash[nnode] = generic_hashaddr( &clientaddress[nnode] );
# endif
#else
        node_hash[nnode] = 1;  // send only
#endif
    }
    else
#endif // IPX
    {
        // INET
        if( ! inet_aton( addrstr, &address.ip.sin_addr ) )
           goto addr_fail;

        UDP_Bind_Node( nnode, address.ip.sin_addr.s_addr, port );
    }
#ifdef NET_NODE_DEBUG
    // DEBUG
    debug_Printf( "Bind Node %d to %s\n", nnode,
               SOCK_AddrToStr( &clientaddress[nnode] ) );
#endif
    return true;

addr_fail:
    return false;
}


  // Return net node.  When nodes full, return 255.
static byte get_freenode( void )
{
    byte nn;

    // Only this range is dynamically allocated, the others are preallocated.
    for( nn=1; nn<MAXNETNODES; nn++)  // self is not free
    {
        if( node_hash[nn] == 0 )
        {
            node_hash[nn]=1;  // enable send, but hash is needed to receive
            return nn;
        }
    }
    return 255;
}

// Function for I_NetFreeNode().
// IPX or inet.
static
void SOCK_FreeNode( byte nnode )
{
    // MAX_CON_NETNODE is preallocated.
    // can't disconnect to self :)
    if( (nnode == 0) || (nnode >= MAXNETNODES) )
        return;

#ifdef DEBUGFILE
    if( debugfile )
    {
        fprintf(debugfile,"Free node %d (%s)\n",
                nnode, SOCK_AddrToStr(&clientaddress[nnode]));
    }
#endif

    // Disconnect and invalidate address.
    node_hash[nnode] = 0;
    memset(&clientaddress[nnode], 0, sizeof(clientaddress[nnode]));
}


//Hurdler: something is wrong with Robert's patch and win2k
// Function for I_NetGet().
// IPX or inet.  Server and client.
// Return packet into doomcom struct.
// Return 0 when got packet, else net_error.  Error in net_error.
// Called by: HGetPacket
static
byte  SOCK_Get(void)
{
    uint32_t errno2;
    byte  nnode;  // index to net_nodes[]
#ifdef NODE_ADDR_HASHING
    byte  hashaddr;
#endif
    int   rcnt;  // data bytes received
#ifdef MACOS_DI
    size_t        fromlen;
#else
    socklen_t     fromlen;
#endif
    mysockaddr_t  fromaddress;

    fromlen = sizeof(fromaddress);  // num bytes of addr for OUT
    // fromaddress: OUT the actual address.
    // fromlen: IN sizeof fromaddress, OUT the actual length of the address.
#ifdef LINUX
    rcnt = recvfrom(mysocket,
                    &doomcom->data,  // packet
                    MAXPACKETLENGTH,  // packet length
                    0,  // flags
                    /*OUT*/ (struct sockaddr *)&fromaddress,  // net address
                    /*IN,OUT*/ &fromlen );  // net address length
#else
    // winsock.h  recvfrom(SOCKET, char*, int, int, struct sockaddr*, int*)
    rcnt = recvfrom(mysocket,
                    // Some other port requires (char*), undocumented.
                    (char *)&doomcom->data,
                    MAXPACKETLENGTH,  // packet length
                    0,  // flags
                    /*OUT*/ (struct sockaddr *)&fromaddress,  // net address
                    /*IN,OUT*/ &fromlen );  // net address length
#endif
    if(rcnt < 0)  goto recv_err;
    
//    DEBFILE(va("Get from %s\n",SOCK_AddrToStr(&fromaddress)));

    // Find remote node number, player nodes only.
#ifdef NODE_ADDR_HASHING
    hashaddr = SOCK_hashaddr( &fromaddress );  // hash != 0
    // debug_Printf( "hashaddr=%d\n", hashaddr );
#else
    hashaddr = generic_hashaddr( &fromaddress );  // hash != 0
#endif
    for (nnode=0; nnode<MAXNETNODES; nnode++)
    {
        // [WDJ] avoid testing null addresses.
        if( node_hash[nnode] != hashaddr )  continue;

        if( SOCK_cmpaddr(&fromaddress, &(clientaddress[nnode])) )
             goto return_node;  // found match
    }

    // Net node not found.
    nnode = get_freenode();  // Find a free node.
    if(nnode >= MAXNETNODES)  goto no_nodes;

    // clientaddress is IP addr and sock port.
    // SOCK_Send will use nnode to send back to this clientaddress.
    memcpy(&clientaddress[nnode], &fromaddress, fromlen);

    // Set node_hash[nnode] to enable receive.
    node_hash[nnode] = hashaddr;

#ifdef DEBUGFILE
    if( debugfile )
    {
        fprintf(debugfile,"New node detected: node:%d address:%s\n",
                nnode, SOCK_AddrToStr(&clientaddress[nnode]));
    }
#endif

return_node:
    doomcom->remotenode = nnode; // good packet from a game player
    doomcom->datalength = rcnt;
    return NE_success;

    // Rare errors
recv_err:
    errno2 = errno; // save it, print will overwrite it
    // Send failed, determine the error.
#ifdef __WIN32__
    if(errno2 == WSAEWOULDBLOCK || errno2 == WSATRY_AGAIN )  // no message
#else
    if(errno2 == EWOULDBLOCK || errno2 == EAGAIN)   // no message
#endif
    {
        net_error = NE_empty;   // no packet
        goto no_packet;
    }

#ifdef __WIN32__
    if( (errno2 == WSAEMSGSIZE)   // message too large
        || (errno2 == WSAECONNREFUSED) )  // connection refused
#else
    if( (errno2 == EMSGSIZE)   // message too large
        || (errno2 == ECONNREFUSED) )  // connection refused
#endif
    {
        net_error = NE_fail;
        goto no_packet;
    }
   
    I_SoftError("SOCK_Get: %s \n    Error %x (%d)\n",
                strerror(errno2), errno2, errno2);

    // Recoverable conditions.
#ifdef __WIN32__
    if( errno2 == WSAECONNRESET || errno2 == WSAECONNABORTED )
#else
    // Linux
    if( errno2 == ECONNRESET || errno2 == ECONNABORTED )
#endif
    {
        net_error = NE_network_reset;  // transient condition
        goto no_packet;
    }

#ifdef __WIN32__
    if( errno2 == WSAENETUNREACH || errno2 == WSAEFAULT || errno2 == WSAEBADF )
#else
    if( errno2 == ENETUNREACH || errno2 == EFAULT || errno2 == EBADF )
#endif   
    {
        // network unreachable
        net_error = NE_network_unreachable; // allows test net without crashing
        goto no_packet;
    }
    // Many other errors.
    // Because of new errors, give it a chance to recover or reset.
    net_error = NE_unknown_net_error;
    goto no_packet;

no_nodes:
#ifdef DEBUGFILE
    // node table full
    if( debugfile )
        fprintf(debugfile,"SOCK_Get: Free nodes all used.\n");
#endif
    net_error = NE_nodes_exhausted;
    goto no_packet;

no_packet:
    doomcom->remotenode = -1;  // no packet
    return net_error;
}


static fd_set  write_set;  // Linux: modified by select

// Function for I_NetCanSend().
// Check if we can send (to save a buffer transfer).
static
boolean SOCK_CanSend(void)
{
    // [WDJ] Linux: select modifies timeval, so it must be init with each call.
    struct timeval timeval_0 = {0,0};  // immediate
    int stat;

    // [WDJ] Linux: write_set is modified by the select call, so it must
    // be init each call.  Smaller code with write_set static global.
    FD_ZERO(&write_set);
    FD_SET(mysocket, &write_set);
    // huh Boris, are you sure about the 1th argument:
    // it is the highest-numbered descriptor in any of the three
    // sets, plus 1 (I suppose mysocket + 1).
    // BP:ok, no prob since it is ignored in windows :)
    // Linux: select man page specifies (highest file descriptor + 1).
    // winsock.h: select(int, fd_set*, fd_set*, fd_set*, const struct timeval*)
    // [WDJ] MACOS_DI had select( 1, ... ) but I do not think that was correct.
    stat = select(mysocket + 1,
                  NULL,  // read fd
                  /*IN,OUT*/ &write_set,  // write fd to watch
                  NULL,  // exceptions
                  /*IN,OUT*/ &timeval_0   // timeout
                 );
    return ( stat > 0 );
}


// Function for I_NetSend().
// IPX or inet.
// Send packet from within doomcom struct.
// Return 0 when got packet, else net_error.  Error in net_error.
byte  SOCK_Send(void)
{
    uint32_t errno2;
    byte  nnode = doomcom->remotenode;
    int  cnt;  // chars sent
                         
    if( node_hash[nnode] == 0 )   goto node_unconnected;

    // sockaddr is defined in sys/socket.h
    // MSG_DONTROUTE: Do not use a gateway, local network only.
    // MSG_DONTWAIT: Do not block.
#ifdef LINUX
    cnt = sendto(mysocket,
                &doomcom->data, doomcom->datalength,  // packet
                0,  // flags
                (struct sockaddr *)&clientaddress[nnode],  // net address
                sizeof(struct sockaddr));  // net address length
#else
    // winsock.h: sendto(SOCKET, char*, int, int, struct sockaddr*, int)
    cnt = sendto(mysocket,
                // Some other port requires (char*), undocumented.
                (char *)&doomcom->data, doomcom->datalength,  // packet
                0,  // flags
                (struct sockaddr *)&clientaddress[nnode],  // net address
                sizeof(struct sockaddr));  // net address length
#endif

//    DEBFILE(va("send to %s\n",SOCK_AddrToStr(&clientaddress[doomcom->remotenode])));
    if( cnt < 0 )  goto send_err;
    return NE_success;
   
    // Rare error.
send_err:
    errno2 = errno; // save it, print will overwrite it
//  if( errno2 == ENOBUFS )  // out of buffer space
#ifdef __WIN32__
    if( errno2 == WSAEWOULDBLOCK || errno2 == WSATRY_AGAIN )
#else
    // Linux
    // ECONNREFUSED can be got in linux port.
    if( errno2 == ECONNREFUSED || errno2 == EWOULDBLOCK || errno2 == EAGAIN )
#endif
    {
        net_error = NE_refused_again;  // silent
        goto err_return;
    }

    I_SoftError("SOCK_Send to node %d (%s): %s\n    Error %x (%d)\n",
                 nnode, SOCK_AddrToStr(&clientaddress[nnode]),
                 strerror(errno2), errno2, errno2);

    // Recoverable conditions.
#ifdef __WIN32__
    if( errno2 == WSAECONNRESET || errno2 == WSAECONNABORTED )
#else
    // Linux
    if( errno2 == ECONNRESET || errno2 == ECONNABORTED )
#endif
    {
        net_error = NE_network_reset;  // transient condition
        goto err_return;
    }

//#if defined(WIN32) && defined(__MINGW32__)
#ifdef __WIN32__
    if( errno2 == WSAENETUNREACH || errno2 == WSAEFAULT || errno2 == WSAEBADF )
#else
    // Linux
    if( errno2 == ENETUNREACH || errno2 == EFAULT || errno2 == EBADF )
#endif
    {
        // network unreachable
        net_error = NE_network_unreachable; // allows test net without crashing
        goto err_return;
    }
    // Many other errors.
#if 1
    // Because of new errors, give it a chance to recover or reset.
    net_error = NE_unknown_net_error;
    goto err_return;
#else
    I_Error("SOCK_Send\n");
#endif

node_unconnected:
    net_error = NE_node_unconnected;
    goto err_return;

err_return:
    return net_error;
}



// UDPsocket
// Server or Client.
//
// TCP, UDP: Called by SOCK_OpenSocket
//   Called by: CL_Update_ServerList, Command_connect
static SOCKET  UDP_Socket (void)
{
    const char * msg = "";
    SOCKET s;
    struct sockaddr_in  address;
#if defined(WIN32) && defined(__MINGW32__)
    u_long trueval = true;
#else
    int    trueval = true;
#endif
    int optval;
    socklen_t optlen;
    int stat;

    // allocate a socket
    s = socket (PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s<0 || s==INVALID_SOCKET)
    {
        msg = "Create socket failed";
        goto no_socket;
    }

    memset (&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    
#ifdef CLIENT_SOCK_PORT_SEARCH
retry_bind:
#endif

    my_sock_port = (server)? server_sock_port : client_sock_port;
#ifdef NET_NODE_DEBUG
    debug_Printf( "UDP_Socket  my_sock_port = %d\n", my_sock_port );
#endif
    address.sin_port = htons(my_sock_port);
    stat = bind (s, (struct sockaddr *)&address, sizeof(address) );
    if (stat == -1)
    {
#ifdef CLIENT_SOCK_PORT_SEARCH
#ifdef WIN32
        if( (errno == WSAEADDRINUSE)
#else
        if( (errno == EADDRINUSE)
#endif
            && ! server
            && (client_sock_port < IPPORT_CLIENT_MAX) )
        {
            // Try alternative client sock ports.
            client_sock_port = (client_sock_port < IPPORT_CLIENT)? IPPORT_CLIENT : client_sock_port+1;
            goto retry_bind;
        }
#endif
        msg = "Bind failed";
        goto close_socket;
    }
    CONS_Printf("Network port: %d\n", my_sock_port);

    // make it non blocking
#ifdef LINUX
    // [MB] 2020-06-18: Use portable POSIX way to enable non-blocking mode
    // https://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html
    // [WDJ] POSIX.1-2001 : F_SETFL
    // Windows: Winsock does not have fcntl.
    {
        int res = fcntl(s, F_SETFL, O_NONBLOCK);
        if( res == -1 )
        {
            msg = "Switch to non-blocking mode failed";
            goto close_socket;
        }
    }
#else
    // [WDJ] DoomLegacy 1.48.4 and before.
    // Does not work on: SunOS (SmartOS and maybe Solaris).
    // Linux: still defined in asm-generic/ioctls.h
    // Needed for Windows, and older ports.
    ioctl (s, FIONBIO, &trueval);
#endif

    // make it broadcastable
#ifdef LINUX
    stat = setsockopt(s, SOL_SOCKET, SO_BROADCAST,
                      &trueval,  // option value
                      sizeof(trueval));  // length of value
#else
    // [WDJ] They must treat optval as a char buffer.
    // winsock.h:  getsockopt(SOCKET, int, int, char*, int*)
    stat = setsockopt(s, SOL_SOCKET, SO_BROADCAST,
                      // Some other port requires (char*), undocumented.
                      (char *)&trueval,  // option value
                      sizeof(trueval));  // length of value
#endif

#ifdef NET_NODE_DEBUG
    // Set SO_DEBUG
#ifdef LINUX
    stat = setsockopt(s, SOL_SOCKET, SO_DEBUG,
                      &trueval,  // option value
                      sizeof(trueval));  // length of value
#else
    // winsock.h:  getsockopt(SOCKET, int, int, char*, int*)
    stat = setsockopt(s, SOL_SOCKET, SO_DEBUG,
                      // Some other port requires (char*), undocumented.
                      (char *)&trueval,  // option value
                      sizeof(trueval));  // length of value
#endif
#endif

    // Set Network receive buffer size.
    optlen=sizeof(optval);  // Linux: gets modified
    // optval: gets the value of the option
    // optlen: gets the actual length of the option
#ifdef LINUX
    stat = getsockopt(s, SOL_SOCKET, SO_RCVBUF,
                      /* OUT */ &optval,  // option value
                      /* IN,OUT */ &optlen);  // available length
#else
    // FIXME: so an int value is written to a (char *); portability!!!!!!!
    // [WDJ] They must treat optval as a char buffer.
    // winsock.h:  getsockopt(SOCKET, int, int, char*, int*)
    stat = getsockopt(s, SOL_SOCKET, SO_RCVBUF,
                      // Some other port requires (char*), undocumented.
                      /* OUT */ (char *)&optval,  // option value
                      /* IN,OUT */ &optlen);  // available length
#endif
    CONS_Printf("Network receive buffer: %dKb\n", optval>>10);

    if(optval < (64<<10)) // 64k
    {
        optval = (64<<10);
#ifdef LINUX
        stat = setsockopt(s, SOL_SOCKET, SO_RCVBUF,
                          &optval,
                          sizeof(optval));
#else
        // winsock.h  setsockopt(SOCKET, int, int, const char*, int)
        stat = setsockopt(s, SOL_SOCKET, SO_RCVBUF,
                          // Some other port requires (char*), undocumented.
                          (char *)&optval,
                          sizeof(optval));
#endif
        if( stat < 0 )
            CONS_Printf("Network receive buffer: Failed to set buffer to 64k.\n");
        else
            CONS_Printf("Network receive buffer: set to %dKb\n", optval>>10);
    }

    // ip + udp
    net_packetheader_length = 20 + 8; // for stats

    // should not receive from self, but will set it up anyway.
    clientaddress[0].ip.sin_family      = AF_INET;
    clientaddress[0].ip.sin_port        = htons(my_sock_port);
#ifdef MACOS_DI
    clientaddress[0].ip.sin_addr.s_addr = inet_addr("127.0.0.1");
#else
    clientaddress[0].ip.sin_addr.s_addr = INADDR_LOOPBACK;
                                  // inet_addr("127.0.0.1");
#endif

#ifdef NODE_ADDR_HASHING
    node_hash[0] = UDP_hashaddr( &clientaddress[0] );
#else
    node_hash[0] = generic_hashaddr( &clientaddress[0] );
#endif

    // [WDJ] Broadcast is now setup at use by CL_Broadcast_AskInfo.

    doomcom->extratics=1; // internet is very high ping

    SOCK_cmpaddr=UDP_cmpaddr;
#ifdef NODE_ADDR_HASHING
    SOCK_hashaddr=UDP_hashaddr;
#endif
    return s;

close_socket:
    // Print error msg in no_socket.
#ifdef __DJGPP__
    // bug in libsocket 0.7.4 beta 4 onder winsock 1.1 (win95)
#else
    close(s);
#endif
   
no_socket:
    // [WDJ] Common error, to control bloat.
    I_SoftError("UDP_socket: %s: %s\n", msg, strerror(errno));
    return -1;
}


#ifdef USE_IPX
// Server or Client.
static SOCKET  IPX_Socket (void)
{
    SOCKET s;
    SOCKADDR_IPX  address;
#if defined(WIN32) && defined(__MINGW32__)
    u_long trueval = true;
#else
    int    trueval = true;
#endif
    int    optval;
    socklen_t optlen;
    int  stat;

    // allocate a socket
    s = socket (AF_IPX, SOCK_DGRAM, NSPROTO_IPX);
    if (s<0 || s==INVALID_SOCKET)
    {
        I_SoftError("IPX_socket: Create socket failed: %s\n", strerror(errno));
        goto no_ipx;
    }

#ifdef CLIENT_SOCK_PORT_SEARCH
retry_bind:
#endif

    memset (&address, 0, sizeof(address));
    my_sock_port = (server)? server_sock_port : client_sock_port;
#ifdef NET_NODE_DEBUG
    debug_Printf( "IPX_Socket  my_sock_port = %d\n", my_sock_port );
#endif
#ifdef LINUX
    address.sipx_family = AF_IPX;
    address.sipx_port = htons(my_sock_port);
#else
    address.sa_family = AF_IPX;
    address.sa_socket = htons(my_sock_port);
#endif // linux

    stat = bind (s, (struct sockaddr *)&address, sizeof(address));
    if( stat == -1)
    {
#ifdef CLIENT_SOCK_PORT_SEARCH
#ifdef WIN32
        if( (errno == WSAEADDRINUSE)
#else
        if( (errno == EADDRINUSE)
#endif
            && ! server
            && (client_sock_port < IPPORT_CLIENT_MAX) )
        {
            client_sock_port = (client_sock_port < IPPORT_CLIENT)? IPPORT_CLIENT : client_sock_port+1;
            goto retry_bind;
        }
#endif
        I_SoftError("IPX_Socket: Bind failed: %s\n", strerror(errno));
        goto close_socket;
    }
    CONS_Printf("Network port: %d\n", my_sock_port);

    // make it non blocking
    ioctl (s, FIONBIO, &trueval);

    // make it broadcastable
#ifdef LINUX
    stat = setsockopt(s, SOL_SOCKET, SO_BROADCAST,
                      &trueval,  // option value
                      sizeof(trueval));
#else
    // winsock.h:  setsockopt(SOCKET, int, int, const char*, int)
    stat = setsockopt(s, SOL_SOCKET, SO_BROADCAST,
                      // Some other port requires (char*), undocumented.
                      (char *)&trueval,  // option value
                      sizeof(trueval));
#endif

    // Set Network receive buffer size.
    optlen=sizeof(optval);  // gets modified
    // optval: gets the value of the option
    // optlen: gets the actual length of the option
#ifdef LINUX
    stat = getsockopt(s, SOL_SOCKET, SO_RCVBUF,
                      /* OUT */ &optval,  // option value
                      /* IN,OUT */ &optlen);  // available length
#else
    // FIXME: so an int value is written to a (char *); portability!!!!!!!
    // winsock.h  getsockopt(SOCKET, int, int, char*, int*)
    stat = getsockopt(s, SOL_SOCKET, SO_RCVBUF,
                      // Some other port requires (char*), undocumented.
                      /* OUT */ (char *)&optval,  // option value
                      /* IN,OUT */ &optlen);  // available length
#endif
    // [WDJ] Had remnants of 64K.  Set to 128K.
    CONS_Printf("Network receive buffer: %dKb\n", optval>>10);
    if(optval < (128<<10)) // 128K
    {
        optval = (128<<10);
        stat = setsockopt(s, SOL_SOCKET, SO_RCVBUF,
                          (char *)&optval,
                          sizeof(optval));
        if( stat < 0 )
            CONS_Printf("Network receive buffer: Failed to set buffer to 128k.\n");
        else
            CONS_Printf("Network receive buffer: set to %dKb\n", optval>>10);
    }

    // ipx header
    net_packetheader_length=30; // for stats

    // [WDJ] Broadcast is now setup at use by CL_Broadcast_AskInfo.

    SOCK_cmpaddr=IPX_cmpaddr;
#ifdef NODE_ADDR_HASHING
    SOCK_hashaddr=IPX_hashaddr;
#endif
    return s;

close_socket:
#ifdef __DJGPP__
    // bug in libsocket 0.7.4 beta 4 onder winsock 1.1 (win95)
#else
    close(s);
#endif
   
no_ipx:
    return -1;
}
#endif // USE_IPX

//Hurdler: temporary addition and changes for master server

static byte TCP_driver_flag = 0;

void I_Init_TCP_Driver(void)
{
    if (!TCP_driver_flag)
    {
#ifdef __WIN32__
        WSADATA winsockdata;
        if( WSAStartup(MAKEWORD(1,1),&winsockdata) )
            I_Error("No TCP/IP driver detected");
#endif
#ifdef __DJGPP_
        if( !__lsck_init() )
            I_Error("No TCP/IP driver detected");
#endif
        TCP_driver_flag = 1;
    }
}


// Function for I_NetCloseSocket().
// IPX or inet.
static
void SOCK_CloseSocket( void )
{
    if( mysocket>=0 )
    {
        //if( server )
        //    UnregisterServer(); 
#ifdef __DJGPP__
// quick fix bug in libsocket 0.7.4 beta 4 onder winsock 1.1 (win95)
#else
        // Not DJGPP
        close(mysocket);
#endif
        mysocket = -1;
    }
}

void I_Shutdown_TCP_Driver(void)
{
    //A.J. possible bug fix. I_ShutdownTcpDriver never used in Mac version   
    if( mysocket!=-1 )
        SOCK_CloseSocket();

    if ( TCP_driver_flag )
    {
#ifdef __WIN32__
        WSACleanup();
#endif
#ifdef __DJGPP__
        __lsck_uninit();
#endif
        TCP_driver_flag = 0;
    }
}


// Function for I_NetMakeNode().
//   Make a node for server address.
// IPX or inet.
// Called by CL_Update_ServerList
//   port is in the hostname
// Called by Command_connect
//   connect to server at hostname (which may have port)
// Called by mserv: MS_open_UDP_Socket if using player socket to contact MasterServer.
//   port is ping_port which is in the hostname.
// Param:
//   hostname : string with network address of remote server
//      example "192.168.127.34:5034"
//      example "doomservers.net"
// Return the net node number, or network_error_e > MAXNETNODES.
static
byte  SOCK_NetMakeNode (char *hostname)
{
    byte newnode;
    mysockaddr_t  newaddr;
    char * namestr;  // owned copy of hostname string
    char *portchar;
    int portnum = server_sock_port;  // target server port

    // [WDJ] From command line can get "192.168.127.34:5234:"
    // From console only get ""192.168.127.34", the port portion is stripped.
    namestr = strdup(hostname);
    //debug_Printf( "Parm hostname=%s\n", namestr );
    // Split into ip address and port.
    char * st = namestr;
    strtok(st,":");  // overwrite the colon with a 0.
    portchar = strtok(NULL,":");
    if( portchar )
    {
        portnum = atoi(portchar);
    }
//    debug_Printf( "  hostname=%s  port=%i\n", namestr, portnum );
    CONS_Printf( "  hostname=%s  port=%i\n", namestr, portnum );

    // server address only in ip
#ifdef USE_IPX
    if(ipx_select)
    {
        // ipx only
        free(namestr);
        return BROADCAST_NODE;
    }
#endif

    // TCP/IP
    // Previous operation on namestr already parsed out the ip addr.
    //debug_Printf( "  ip hostname=%s\n", namestr );

    // Too early, but avoids resolving names we cannot use.
    newnode = get_freenode();
    if( newnode >= MAXNETNODES )
        goto no_nodes;  // out of nodes

    // Find the IP of the server.
    // [WDJ] This cannot handle addr 255.255.255.255 which == INADDR_NONE.
    newaddr.ip.sin_addr.s_addr = inet_addr(namestr);
    if(newaddr.ip.sin_addr.s_addr==INADDR_NONE) // not a ip, ask the dns
    {
        struct hostent * hostentry;      // host information entry
        CONS_Printf("Resolving %s\n",namestr);
        hostentry = gethostbyname (namestr);
        if (!hostentry)
        {
            CONS_Printf ("%s unknown\n", namestr);
            I_NetFreeNode(newnode);  // release the newnode
            goto abort_makenode;
        }
        newaddr.ip.sin_addr.s_addr = *(int *)hostentry->h_addr_list[0];
    }
    CONS_Printf("Resolved %s\n",
                 inet_ntoa(*(struct in_addr *)&newaddr.ip.sin_addr.s_addr));

    // Commit to the new node.
    clientaddress[newnode].ip.sin_family      = AF_INET;
    clientaddress[newnode].ip.sin_port        = htons(portnum);
    clientaddress[newnode].ip.sin_addr.s_addr = newaddr.ip.sin_addr.s_addr;
#ifdef NODE_ADDR_HASHING
    node_hash[newnode] = SOCK_hashaddr( &newaddr );  // hash != 0
#else
    node_hash[newnode] = generic_hashaddr( &newaddr );  // hash != 0
#endif

clean_ret:
    free(namestr);
    return newnode;

    // Rare errors.
abort_makenode:
    newnode = NE_fail;  //  > MAXNETNODES
    goto clean_ret;

no_nodes:
    newnode = NE_nodes_exhausted;  //  > MAXNETNODES
    goto clean_ret;
}


// Function for I_NetOpenSocket().
// Server or Client.
// IPX or inet.
// Called by: CL_Update_ServerList, Command_connect, SV_SpawnServer
static
boolean SOCK_OpenSocket( void )
{
    memset(clientaddress,0,sizeof(clientaddress));
#if 0   
    memset(node_hash, 0, sizeof(node_hash));
#else
    int i;
    for(i=1; i<MAX_CON_NETNODE; i++)
        node_hash[i] = 0;
#endif
    node_hash[0] = 0x81; // always connected to self
   
    I_NetSend        = SOCK_Send;
    I_NetGet         = SOCK_Get;
    I_NetCloseSocket = SOCK_CloseSocket;
    I_NetFreeNode    = SOCK_FreeNode;
    I_NetMakeNode    = SOCK_NetMakeNode;

#ifdef __WIN32__
    // seem like not work with libsocket nor linux :(
    I_NetCanSend  = SOCK_CanSend;
#else
    // [WDJ] Fixed, Linux can change select time.
    I_NetCanSend  = SOCK_CanSend;
#endif

    // build the socket
    // Setup up Broadcast.
#ifdef USE_IPX
    if(ipx_select) {
        mysocket = IPX_Socket ();
        net_bandwidth = 800000;
        hardware_MAXPACKETLENGTH = MAXPACKETLENGTH;
    }
    else
#endif // USE_IPX
    {
        // TCP, UDP
        mysocket = UDP_Socket ();
       // if (server && cv_internetserver.value)
       //     RegisterServer(mysocket, sock_port);
    }

    return (mysocket >= 0);
}


// Called by D_Startup_NetGame
// IPX or inet.
// Called by: D_Startup_NetGame, MS_Connect
void I_Init_TCP_Network( void )
{
    char     serverhostname[255];

#ifdef USE_IPX
    ipx_select = M_CheckParm("-ipx");
#endif
    
    // Initialize the driver.
    I_Init_TCP_Driver();
#ifdef MACOS_DI
    // [WDJ] Do not know why they did not use I_Shutdown_TCP_Driver.
#else
    I_AddExitFunc (I_Shutdown_TCP_Driver);
#endif

    if ( M_CheckParm ("-udpport") )
    {
        if( !M_IsNextParm() )
            I_Error("syntax : -udpport <portnum>");
        server_sock_port = atoi(M_GetNextParm());
#ifdef NET_NODE_DEBUG
        debug_Printf( "Init_TCP  server_sock_port = %d\n", server_sock_port );       
#endif
    }
   
    //[WDJ]: Moved here so can be used for IPX too.
    //Hurdler: I'd like to put a server and a client on the same computer
    //BP: in fact for client we can use any free port we want, i have read 
    //    in some doc that connect in udp can do it for us...
    if( M_CheckParm ("-clientport") )
    {
        if( !M_IsNextParm() )
            I_Error("syntax : -clientport <portnum>");
        client_sock_port = atoi(M_GetNextParm());
        // If this this client port is in use, it may be changed.
    }
   
    // parse network game options,
    if ( server )
    {
        // FIXME:
        // ??? and now ?
        // server on a big modem ??? 4*isdn
        net_bandwidth = 16000;

#ifdef MACOS_DI
        hardware_MAXPACKETLENGTH = 512;
#else
        hardware_MAXPACKETLENGTH = INETPACKETLENGTH;
#endif
    }
    else if( M_CheckParm ("-connect") )
    {
        if(M_IsNextParm())
            strcpy(serverhostname,M_GetNextParm());
        else
            serverhostname[0]=0; // assuming server in the LAN, use broadcast to detect it

        // server address only in ip
        if(serverhostname[0]
#ifdef USE_IPX	   
           && !ipx_select
#endif
           )
        {
            COM_BufAddText(va("connect \"%s\"\n", serverhostname ));

            // probably modem
            hardware_MAXPACKETLENGTH = INETPACKETLENGTH;
        }
        else
        {
            // so we're on a LAN
            COM_BufAddText("connect any\n");

            net_bandwidth = 800000;
            hardware_MAXPACKETLENGTH = MAXPACKETLENGTH;
        }
    }

    I_NetOpenSocket = SOCK_OpenSocket;
}
