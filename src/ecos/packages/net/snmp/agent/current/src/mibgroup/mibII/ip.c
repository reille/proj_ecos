//==========================================================================
//
//      snmp/snmpagent/current/src/mibgroup/mibII/ip.c
//
//
//==========================================================================
// ####ECOSGPLCOPYRIGHTBEGIN####                                            
// -------------------------------------------                              
// This file is part of eCos, the Embedded Configurable Operating System.   
// Copyright (C) 1998, 1999, 2000, 2001, 2002 Free Software Foundation, Inc.
//
// eCos is free software; you can redistribute it and/or modify it under    
// the terms of the GNU General Public License as published by the Free     
// Software Foundation; either version 2 or (at your option) any later      
// version.                                                                 
//
// eCos is distributed in the hope that it will be useful, but WITHOUT      
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or    
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License    
// for more details.                                                        
//
// You should have received a copy of the GNU General Public License        
// along with eCos; if not, write to the Free Software Foundation, Inc.,    
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.            
//
// As a special exception, if other files instantiate templates or use      
// macros or inline functions from this file, or you compile this file      
// and link it with other works to produce a work based on this file,       
// this file does not by itself cause the resulting work to be covered by   
// the GNU General Public License. However the source code for this file    
// must still be made available in accordance with section (3) of the GNU   
// General Public License v2.                                               
//
// This exception does not invalidate any other reasons why a work based    
// on this file might be covered by the GNU General Public License.         
// -------------------------------------------                              
// ####ECOSGPLCOPYRIGHTEND####                                              
//####UCDSNMPCOPYRIGHTBEGIN####
//
// -------------------------------------------
//
// Portions of this software may have been derived from the UCD-SNMP
// project,  <http://ucd-snmp.ucdavis.edu/>  from the University of
// California at Davis, which was originally based on the Carnegie Mellon
// University SNMP implementation.  Portions of this software are therefore
// covered by the appropriate copyright disclaimers included herein.
//
// The release used was version 4.1.2 of May 2000.  "ucd-snmp-4.1.2"
// -------------------------------------------
//
//####UCDSNMPCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    hmt
// Contributors: hmt, Andrew Lunn
// Date:         2000-05-30
// Purpose:      Port of UCD-SNMP distribution to eCos.
// Description:  
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================
/********************************************************************
       Copyright 1989, 1991, 1992 by Carnegie Mellon University
 
                          Derivative Work -
Copyright 1996, 1998, 1999, 2000 The Regents of the University of California
 
                         All Rights Reserved
 
Permission to use, copy, modify and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appears in all copies and
that both that copyright notice and this permission notice appear in
supporting documentation, and that the name of CMU and The Regents of
the University of California not be used in advertising or publicity
pertaining to distribution of the software without specific written
permission.
 
CMU AND THE REGENTS OF THE UNIVERSITY OF CALIFORNIA DISCLAIM ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL CMU OR
THE REGENTS OF THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR ANY SPECIAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
FROM THE LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*********************************************************************/
/* This file was generated by mib2c and is intended for use as a mib module
   for the ucd-snmp snmpd agent. */

#include <sys/param.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <net/if.h>
#ifdef CYGPKG_NET_FREEBSD_STACK
#include <net/if_var.h>
#endif
#include <netinet/in.h>
#include <netinet/if_ether.h>
#include <net/if_arp.h>
#include <netinet/in_var.h>
#include <netinet/ip_var.h>

/* This should always be included first before anything else */
#include <config.h>

/* minimal include directives */
#include "mibincl.h"
#include "mibgroup/util_funcs.h"
#include "mibgroup/mibII/ip.h"

#include <cyg/io/eth/eth_drv.h>

extern struct in_ifaddrhead in_ifaddr;

/* 
 * ip_variables_oid:
 *   this is the top level oid that we want to register under.  This
 *   is essentially a prefix, with the suffix appearing in the
 *   variable below.
 */


oid ip_variables_oid[] = { 1,3,6,1,2,1,4 };

/* 
 * variable4 ip_variables:
 *   this variable defines function callbacks and type return information 
 *   for the ip mib section 
 */

struct variable4 ip_variables[] = {
/*  magic number        , variable type , ro/rw , callback fn  , L, oidsuffix */
#define   IPFORWARDING          1
  { IPFORWARDING        , ASN_INTEGER   , RWRITE, var_ip, 1, { 1 } },
#define   IPDEFAULTTTL          2
  { IPDEFAULTTTL        , ASN_INTEGER   , RWRITE, var_ip, 1, { 2 } },
#define   IPINRECEIVES          3
  { IPINRECEIVES        , ASN_COUNTER   , RONLY , var_ip, 1, { 3 } },
#define   IPINHDRERRORS         4
  { IPINHDRERRORS       , ASN_COUNTER   , RONLY , var_ip, 1, { 4 } },
#define   IPINADDRERRORS        5
  { IPINADDRERRORS      , ASN_COUNTER   , RONLY , var_ip, 1, { 5 } },
#define   IPFORWDATAGRAMS       6
  { IPFORWDATAGRAMS     , ASN_COUNTER   , RONLY , var_ip, 1, { 6 } },
#define   IPINUNKNOWNPROTOS     7
  { IPINUNKNOWNPROTOS   , ASN_COUNTER   , RONLY , var_ip, 1, { 7 } },
#define   IPINDISCARDS          8
  { IPINDISCARDS        , ASN_COUNTER   , RONLY , var_ip, 1, { 8 } },
#define   IPINDELIVERS          9
  { IPINDELIVERS        , ASN_COUNTER   , RONLY , var_ip, 1, { 9 } },
#define   IPOUTREQUESTS         10
  { IPOUTREQUESTS       , ASN_COUNTER   , RONLY , var_ip, 1, { 10 } },
#define   IPOUTDISCARDS         11
  { IPOUTDISCARDS       , ASN_COUNTER   , RONLY , var_ip, 1, { 11 } },
#define   IPOUTNOROUTES         12
  { IPOUTNOROUTES       , ASN_COUNTER   , RONLY , var_ip, 1, { 12 } },
#define   IPREASMTIMEOUT        13
  { IPREASMTIMEOUT      , ASN_INTEGER   , RONLY , var_ip, 1, { 13 } },
#define   IPREASMREQDS          14
  { IPREASMREQDS        , ASN_COUNTER   , RONLY , var_ip, 1, { 14 } },
#define   IPREASMOKS            15
  { IPREASMOKS          , ASN_COUNTER   , RONLY , var_ip, 1, { 15 } },
#define   IPREASMFAILS          16
  { IPREASMFAILS        , ASN_COUNTER   , RONLY , var_ip, 1, { 16 } },
#define   IPFRAGOKS             17
  { IPFRAGOKS           , ASN_COUNTER   , RONLY , var_ip, 1, { 17 } },
#define   IPFRAGFAILS           18
  { IPFRAGFAILS         , ASN_COUNTER   , RONLY , var_ip, 1, { 18 } },
#define   IPFRAGCREATES         19
  { IPFRAGCREATES       , ASN_COUNTER   , RONLY , var_ip, 1, { 19 } },


#define   IPADENTADDR           22
  { IPADENTADDR         , ASN_IPADDRESS , RONLY , var_ipAddrTable, 3, { 20,1,1 } },
#define   IPADENTIFINDEX        23
  { IPADENTIFINDEX      , ASN_INTEGER   , RONLY , var_ipAddrTable, 3, { 20,1,2 } },
#define   IPADENTNETMASK        24
  { IPADENTNETMASK      , ASN_IPADDRESS , RONLY , var_ipAddrTable, 3, { 20,1,3 } },
#define   IPADENTBCASTADDR      25
  { IPADENTBCASTADDR    , ASN_INTEGER   , RONLY , var_ipAddrTable, 3, { 20,1,4 } },
#define   IPADENTREASMMAXSIZE   26
  { IPADENTREASMMAXSIZE , ASN_INTEGER   , RONLY , var_ipAddrTable, 3, { 20,1,5 } },

// ROUTE TABLE is OBSOLETE according to my book
//#define   IPROUTEDEST           29
//  { IPROUTEDEST         , ASN_IPADDRESS , RWRITE, var_ipRouteTable, 3, { 21,1,1 } },
//#define   IPROUTEIFINDEX        30
//  { IPROUTEIFINDEX      , ASN_INTEGER   , RWRITE, var_ipRouteTable, 3, { 21,1,2 } },
//#define   IPROUTEMETRIC1        31
//  { IPROUTEMETRIC1      , ASN_INTEGER   , RWRITE, var_ipRouteTable, 3, { 21,1,3 } },
//#define   IPROUTEMETRIC2        32
//  { IPROUTEMETRIC2      , ASN_INTEGER   , RWRITE, var_ipRouteTable, 3, { 21,1,4 } },
//#define   IPROUTEMETRIC3        33
//  { IPROUTEMETRIC3      , ASN_INTEGER   , RWRITE, var_ipRouteTable, 3, { 21,1,5 } },
//#define   IPROUTEMETRIC4        34
//  { IPROUTEMETRIC4      , ASN_INTEGER   , RWRITE, var_ipRouteTable, 3, { 21,1,6 } },
//#define   IPROUTENEXTHOP        35
//  { IPROUTENEXTHOP      , ASN_IPADDRESS , RWRITE, var_ipRouteTable, 3, { 21,1,7 } },
//#define   IPROUTETYPE           36
//  { IPROUTETYPE         , ASN_INTEGER   , RWRITE, var_ipRouteTable, 3, { 21,1,8 } },
//#define   IPROUTEPROTO          37
//  { IPROUTEPROTO        , ASN_INTEGER   , RONLY , var_ipRouteTable, 3, { 21,1,9 } },
//#define   IPROUTEAGE            38
//  { IPROUTEAGE          , ASN_INTEGER   , RWRITE, var_ipRouteTable, 3, { 21,1,10 } },
//#define   IPROUTEMASK           39
//  { IPROUTEMASK         , ASN_IPADDRESS , RWRITE, var_ipRouteTable, 3, { 21,1,11 } },
//#define   IPROUTEMETRIC5        40
//  { IPROUTEMETRIC5      , ASN_INTEGER   , RWRITE, var_ipRouteTable, 3, { 21,1,12 } },
//#define   IPROUTEINFO           41
//  { IPROUTEINFO         , ASN_OBJECT_ID , RONLY , var_ipRouteTable, 3, { 21,1,13 } },

#define   IPNETTOMEDIAIFINDEX   44
  { IPNETTOMEDIAIFINDEX , ASN_INTEGER     , RWRITE, var_ipNetToMediaTable, 3, { 22,1,1 } },
#define   IPNETTOMEDIAPHYSADDRESS  45
  { IPNETTOMEDIAPHYSADDRESS, ASN_OCTET_STR, RWRITE, var_ipNetToMediaTable, 3, { 22,1,2 } },
#define   IPNETTOMEDIANETADDRESS  46
  { IPNETTOMEDIANETADDRESS, ASN_IPADDRESS , RWRITE, var_ipNetToMediaTable, 3, { 22,1,3 } },
#define   IPNETTOMEDIATYPE      47
  { IPNETTOMEDIATYPE    , ASN_INTEGER     , RWRITE, var_ipNetToMediaTable, 3, { 22,1,4 } },

#define   IPROUTINGDISCARDS     48
  { IPROUTINGDISCARDS   , ASN_COUNTER   , RONLY , var_ip, 1, { 23 } },

};
/*    (L = length of the oidsuffix) */


/*
 * init_ip():
 *   Initialization routine.  This is called when the agent starts up.
 *   At a minimum, registration of your variables should take place here.
 */
void init_ip(void) {


  /* register ourselves with the agent to handle our mib tree */
  REGISTER_MIB("ip", ip_variables, variable4,
               ip_variables_oid);


  /* place any other initialization junk you need here */
}


/*
 * var_ip():
 *   This function is called every time the agent gets a request for
 *   a scalar variable that might be found within your mib section
 *   registered above.  It is up to you to do the right thing and
 *   return the correct value.
 *     You should also correct the value of "var_len" if necessary.
 *
 *   Please see the documentation for more information about writing
 *   module extensions, and check out the examples in the examples
 *   and mibII directories.
 */
unsigned char *
var_ip(struct variable *vp, 
                oid     *name, 
                size_t  *length, 
                int     exact, 
                size_t  *var_len, 
                WriteMethod **write_method)
{


    /* variables we may use later */
    static long long_ret;

    if (header_generic(vp,name,length,exact,var_len,write_method)
        == MATCH_FAILED )
        return NULL;

    switch(vp->magic) {

    case IPFORWARDING:
        *write_method = write_ipForwarding;
        long_ret = ipforwarding ? 1 : 2;
        return (unsigned char *) &long_ret;

    case IPDEFAULTTTL:
        *write_method = write_ipDefaultTTL;
        long_ret = ip_defttl;
        return (unsigned char *) &long_ret;

    case IPINRECEIVES:
        long_ret = ipstat.ips_total;
        return (unsigned char *) &long_ret;

    case IPINHDRERRORS:
        long_ret = ipstat.ips_badsum
            + ipstat.ips_badhlen
            + ipstat.ips_badlen
            + ipstat.ips_badoptions
            + ipstat.ips_badvers
#ifdef CYGPKG_NET_OPENBSD_STACK
            + ipstat.ips_badfrags
#endif
            + ipstat.ips_toolong
            ;
        return (unsigned char *) &long_ret;

    case IPINADDRERRORS:
        long_ret = ipstat.ips_cantforward;
        return (unsigned char *) &long_ret;

    case IPFORWDATAGRAMS:
        long_ret = ipstat.ips_forward;
        return (unsigned char *) &long_ret;

    case IPINUNKNOWNPROTOS:
        long_ret = ipstat.ips_noproto;
        return (unsigned char *) &long_ret;

    case IPINDISCARDS:
        long_ret = ipstat.ips_total - ipstat.ips_delivered -
            (
              ipstat.ips_badsum
            + ipstat.ips_badhlen
            + ipstat.ips_badlen
            + ipstat.ips_badoptions
            + ipstat.ips_badvers
#ifdef CYGPKG_NET_OPENBSD_STACK
            + ipstat.ips_badfrags
#endif
            + ipstat.ips_toolong
                ) -
            ipstat.ips_cantforward;
        if ( 0 > long_ret )
            long_ret = 0;
        return (unsigned char *) &long_ret;

    case IPINDELIVERS:
        long_ret = ipstat.ips_delivered;
        return (unsigned char *) &long_ret;

    case IPOUTREQUESTS:
        long_ret = ipstat.ips_localout;
        return (unsigned char *) &long_ret;

    case IPOUTDISCARDS:
        long_ret = ipstat.ips_odropped;
        return (unsigned char *) &long_ret;

    case IPOUTNOROUTES:
        long_ret = ipstat.ips_noroute;
        return (unsigned char *) &long_ret;

    case IPREASMTIMEOUT:
        long_ret = 0; //FIXME
        return (unsigned char *) &long_ret;

    case IPREASMREQDS:
        long_ret = ipstat.ips_fragments;
        return (unsigned char *) &long_ret;

    case IPREASMOKS:
        long_ret = ipstat.ips_reassembled;
        return (unsigned char *) &long_ret;

    case IPREASMFAILS:
        long_ret = ipstat.ips_fragments - 
            (ipstat.ips_fragdropped + ipstat.ips_fragtimeout);
        return (unsigned char *) &long_ret;

    case IPFRAGOKS:
        long_ret = ipstat.ips_fragmented;
        return (unsigned char *) &long_ret;

    case IPFRAGFAILS:
        long_ret = ipstat.ips_cantfrag;
        return (unsigned char *) &long_ret;

    case IPFRAGCREATES:
        long_ret = ipstat.ips_ofragments;
        return (unsigned char *) &long_ret;

    case IPROUTINGDISCARDS:
        long_ret = ipstat.ips_noroute;
        return (unsigned char *) &long_ret;

    default:
      ERROR_MSG("");
  }
  return NULL;
}


/*
 * var_ipAddrTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for var_ipAddrTable above.
 */
unsigned char *
var_ipAddrTable(struct variable *vp,
                oid     *name,
                size_t  *length,
                int     exact,
                size_t  *var_len,
                WriteMethod **write_method)
{
    /*
     * object identifier is of form:
     * 1.3.6.1.2.1.4.20.1.?.A.B.C.D,  where A.B.C.D is IP address.
     * IPADDR starts at offset 10.
     */
    oid			    lowest[14];
    oid			    current[14], *op;
    u_char		    *cp;

    static long long_ret;
    static unsigned char string[SPRINT_MAX_LEN];

    register struct in_ifaddr *ia;
    register struct in_ifaddr *low_ia = NULL;

    register struct ifnet *ifp;
    int interface_count = 1;

    /* fill in object part of name for current (less sizeof instance part) */
    memcpy( (char *)current,(char *)vp->name, (int)vp->namelen * sizeof(oid));

    for (
#ifdef CYGPKG_NET_OPENBSD_STACK
	 ia = in_ifaddr.tqh_first; ia; 
	 ia = ia->ia_list.tqe_next
#endif
#ifdef CYGPKG_NET_FREEBSD_STACK
	 ia = in_ifaddrhead.tqh_first; ia; 
	 ia = ia->ia_link.tqe_next
#endif
	 ) {
	cp = (u_char *)&(ia->ia_addr.sin_addr.s_addr);

	op = current + 10;
	*op++ = *cp++;
	*op++ = *cp++;
	*op++ = *cp++;
	*op++ = *cp++;
	if (exact){
	    if (snmp_oid_compare(current, 14, name, *length) == 0) {
		memcpy( (char *)lowest,(char *)current, 14 * sizeof(oid));
		low_ia = ia;
                break;	/* no need to search further */
	    }
	} else {
	    if ((snmp_oid_compare(current, 14, name, *length) > 0) &&
                (!low_ia || (snmp_oid_compare(current, 14, lowest, 14) < 0))) {
		/*
		 * if new one is greater than input and closer to input than
		 * previous lowest, save this one as the "next" one.
		 */
		memcpy( (char *)lowest,(char *)current, 14 * sizeof(oid));
		low_ia = ia;
	    }
	}
    }

    if ( ! low_ia )
        return NULL;

    memcpy( (char *)name,(char *)lowest, 14 * sizeof(oid));
    *length = 14;
    *write_method = 0;
    *var_len = sizeof(long_return);

    /* 
     * this is where we do the value assignments for the mib results.
     */
    switch(vp->magic) {

    case IPADENTADDR:
        cp = (u_char *)&(low_ia->ia_addr.sin_addr.s_addr);
        string[0] = *cp++;
        string[1] = *cp++;
        string[2] = *cp++;
        string[3] = *cp++;
        *var_len = 4;
        return (unsigned char *) string;

    case IPADENTIFINDEX:
        ifp =  ifnet.tqh_first;
        while (ifp && ifp->if_index != low_ia->ia_ifa.ifa_ifp->if_index) {
            interface_count++;
            ifp = ifp->if_list.tqe_next;
        }
        if (!ifp) {
            return NULL;
        }
        long_ret = interface_count;
        return (unsigned char *) &long_ret;

    case IPADENTNETMASK:
        cp = (u_char *)&(low_ia->ia_subnetmask);
        string[0] = *cp++;
        string[1] = *cp++;
        string[2] = *cp++;
        string[3] = *cp++;
        *var_len = 4;
        return (unsigned char *) string;

    case IPADENTBCASTADDR:
        long_ret = 1;
        return (unsigned char *) &long_ret;

    case IPADENTREASMMAXSIZE:
        long_ret = IP_MAXPACKET;
        return (unsigned char *) &long_ret;

    default:
      ERROR_MSG("");
    }
    return NULL;
}




/*
 * var_ipNetToMediaTable():
 *   Handle this table separately from the scalar value case.
 *   The workings of this are basically the same as for var_ip above.
 */

// According to sections 6.1.5 (ip) and 6.1.4 (at) pp.130-138 of the book
// by William Stallings, this lists *our* interfaces only, not the ARP
// table.  The MIBs are rather ambiguous, as is Mark A. Miller's book also.
// 
// Specifically, the indexing by interface Id suggests there should only be
// one entry per interface.

unsigned char *
var_ipNetToMediaTable(struct variable *vp,
    	    oid     *name,
    	    size_t  *length,
    	    int     exact,
    	    size_t  *var_len,
    	    WriteMethod **write_method)
{
    static long long_ret;
    static unsigned char string[SPRINT_MAX_LEN];
    /*
     * IP Net to Media table object identifier is of form:
     * 1.3.6.1.2.1.4.22.1.?.interface.A.B.C.D,  where A.B.C.D is IP address.
     * Interface is at offset 10,
     * IPADDR starts at offset 11.
     */
    u_char		    *cp;
    oid			    *op;
    oid			    lowest[16];
    oid			    current[16];

    register struct in_ifaddr *ia;
    register struct in_ifaddr *low_ia = NULL;

    /* fill in object part of name for current (less sizeof instance part) */
    memcpy((char *)current, (char *)vp->name, (int)vp->namelen * sizeof(oid));

    for (
#ifdef CYGPKG_NET_OPENBSD_STACK
	 ia = in_ifaddr.tqh_first; ia; 
	 ia = ia->ia_list.tqe_next
#endif
#ifdef CYGPKG_NET_FREEBSD_STACK	 
	   ia = in_ifaddrhead.tqh_first; ia; 
	 ia = ia->ia_link.tqe_next
#endif
	 ) {
        // interface number
        current[10] = ia->ia_ifa.ifa_ifp->if_index;
        // IP address
	cp = (u_char *)&(ia->ia_addr.sin_addr.s_addr);
	op = current + 11;
	*op++ = *cp++;
	*op++ = *cp++;
	*op++ = *cp++;
	*op++ = *cp++;
        
	if (exact){
	    if (snmp_oid_compare(current, 15, name, *length) == 0){
		memcpy( (char *)lowest,(char *)current, 15 * sizeof(oid));
		low_ia = ia;
		break;	/* no need to search further */
	    }
	} else {
	    if ((snmp_oid_compare(current, 15, name, *length) > 0) &&
                ((!low_ia) || (snmp_oid_compare(current, 15, lowest, 15) < 0))) {
		/*
		 * if new one is greater than input and closer to input than
		 * previous lowest, save this one as the "next" one.
		 */
		memcpy( (char *)lowest,(char *)current, 15 * sizeof(oid));
		low_ia = ia;
	    }
	}
    }
    if ( ! low_ia )
	return(NULL);

    memcpy( (char *)name,(char *)lowest, 15 * sizeof(oid));
    *length = 15;
    *write_method = 0;
    *var_len = sizeof(long_return);

    /* 
     * this is where we do the value assignments for the mib results.
     */
    switch(vp->magic) {
    case IPNETTOMEDIAIFINDEX:
        //NOTSUPPORTED: *write_method = write_ipNetToMediaIfIndex;
        long_ret = low_ia->ia_ifa.ifa_ifp->if_index;
        return (unsigned char *) &long_ret;

    case IPNETTOMEDIAPHYSADDRESS: {
        struct eth_drv_sc *sc = low_ia->ia_ifa.ifa_ifp->if_softc;
        if (!sc) {
            // No hardware associated with this device.
            return(NULL);
        }
        bcopy(&sc->sc_arpcom.ac_enaddr, string, ETHER_ADDR_LEN);
        *var_len = ETHER_ADDR_LEN;
        //NOTSUPPORTED: *write_method = write_ipNetToMediaPhysAddress;
        return (unsigned char *) string;
    }
    case IPNETTOMEDIANETADDRESS:
        //NOTSUPPORTED: *write_method = write_ipNetToMediaNetAddress;
	cp = (u_char *)&(low_ia->ia_addr.sin_addr.s_addr);
        string[0] = *cp++;
        string[1] = *cp++;
        string[2] = *cp++;
        string[3] = *cp++;
        *var_len = 4;
        return (unsigned char *) string;

    case IPNETTOMEDIATYPE:
        //NOTSUPPORTED: *write_method = write_ipNetToMediaType;
        long_ret = 4; // Static mapping
        return (unsigned char *) &long_ret;

    default:
        ERROR_MSG("");
    }
    return NULL;
}




int
write_ipForwarding(int      action,
            u_char   *var_val,
            u_char   var_val_type,
            size_t   var_val_len,
            u_char   *statP,
            oid      *name,
            size_t   name_len)
{
    static long setval;
    
    switch ( action ) {
    case RESERVE1:
        if (var_val_type != ASN_INTEGER){
            fprintf(stderr, "write to ipForwarding not ASN_INTEGER\n");
            return SNMP_ERR_WRONGTYPE;
        }
        if (var_val_len > sizeof(setval)){
            fprintf(stderr,"write to ipForwarding: bad length\n");
            return SNMP_ERR_WRONGLENGTH;
        }
        setval = *(long *)var_val;
        if ( 1 != setval && 2 != setval )
            return SNMP_ERR_WRONGVALUE;
        break;

    case RESERVE2:
    case FREE:
    case ACTION:
    case UNDO:
        break;

    case COMMIT:
        ipforwarding = (setval == 1);
        break;
    }
    return SNMP_ERR_NOERROR;
}




int
write_ipDefaultTTL(int      action,
            u_char   *var_val,
            u_char   var_val_type,
            size_t   var_val_len,
            u_char   *statP,
            oid      *name,
            size_t   name_len)
{
    static long setval;

    switch ( action ) {
    case RESERVE1:
        if (var_val_type != ASN_INTEGER){
            fprintf(stderr, "write to ipDefaultTTL not ASN_INTEGER\n");
            return SNMP_ERR_WRONGTYPE;
        }
        if (var_val_len > sizeof(setval)){
            fprintf(stderr,"write to ipDefaultTTL: bad length\n");
            return SNMP_ERR_WRONGLENGTH;
        }
        setval = *(long *)var_val;
        break;

    case RESERVE2:
    case FREE:
    case ACTION:
    case UNDO:
        break;

    case COMMIT:
        ip_defttl = setval;
        break;
    }
    return SNMP_ERR_NOERROR;
}


// ---------------------------------------------------------------------------
// writing these is not supported.  The templates from mib2c are retained.
//
//NOTSUPPORTED:
#if 0
int
write_ipNetToMediaIfIndex(int      action,
            u_char   *var_val,
            u_char   var_val_type,
            size_t   var_val_len,
            u_char   *statP,
            oid      *name,
            size_t   name_len)
{
  static long *long_ret;
  int size;


  switch ( action ) {
        case RESERVE1:
          if (var_val_type != ASN_INTEGER){
              fprintf(stderr, "write to ipNetToMediaIfIndex not ASN_INTEGER\n");
              return SNMP_ERR_WRONGTYPE;
          }
          if (var_val_len > sizeof(long_ret)){
              fprintf(stderr,"write to ipNetToMediaIfIndex: bad length\n");
              return SNMP_ERR_WRONGLENGTH;
          }
          break;


        case RESERVE2:
          size = var_val_len;
          long_ret = (long *) var_val;


          break;


        case FREE:
             /* Release any resources that have been allocated */
          break;


        case ACTION:
             /* The variable has been stored in long_ret for
             you to use, and you have just been asked to do something with
             it.  Note that anything done here must be reversable in the UNDO case */
          break;


        case UNDO:
             /* Back out any changes made in the ACTION case */
          break;


        case COMMIT:
             /* Things are working well, so it's now safe to make the change
             permanently.  Make sure that anything done here can't fail! */
          break;
  }
  return SNMP_ERR_NOERROR;
}




int
write_ipNetToMediaPhysAddress(int      action,
            u_char   *var_val,
            u_char   var_val_type,
            size_t   var_val_len,
            u_char   *statP,
            oid      *name,
            size_t   name_len)
{
  static unsigned char string[SPRINT_MAX_LEN];
  int size;


  switch ( action ) {
        case RESERVE1:
          if (var_val_type != ASN_OCTET_STR){
              fprintf(stderr, "write to ipNetToMediaPhysAddress not ASN_OCTET_STR\n");
              return SNMP_ERR_WRONGTYPE;
          }
          if (var_val_len > sizeof(string)){
              fprintf(stderr,"write to ipNetToMediaPhysAddress: bad length\n");
              return SNMP_ERR_WRONGLENGTH;
          }
          break;


        case RESERVE2:
          size = var_val_len;
          //string = (char *) var_val;


          break;


        case FREE:
             /* Release any resources that have been allocated */
          break;


        case ACTION:
             /* The variable has been stored in string for
             you to use, and you have just been asked to do something with
             it.  Note that anything done here must be reversable in the UNDO case */
          break;


        case UNDO:
             /* Back out any changes made in the ACTION case */
          break;


        case COMMIT:
             /* Things are working well, so it's now safe to make the change
             permanently.  Make sure that anything done here can't fail! */
          break;
  }
  return SNMP_ERR_NOERROR;
}




int
write_ipNetToMediaNetAddress(int      action,
            u_char   *var_val,
            u_char   var_val_type,
            size_t   var_val_len,
            u_char   *statP,
            oid      *name,
            size_t   name_len)
{
  static unsigned char string[SPRINT_MAX_LEN];
  int size;


  switch ( action ) {
        case RESERVE1:
          if (var_val_type != ASN_IPADDRESS){
              fprintf(stderr, "write to ipNetToMediaNetAddress not ASN_IPADDRESS\n");
              return SNMP_ERR_WRONGTYPE;
          }
          if (var_val_len > sizeof(string)){
              fprintf(stderr,"write to ipNetToMediaNetAddress: bad length\n");
              return SNMP_ERR_WRONGLENGTH;
          }
          break;


        case RESERVE2:
          size = var_val_len;
          //string = (char *) var_val;


          break;


        case FREE:
             /* Release any resources that have been allocated */
          break;


        case ACTION:
             /* The variable has been stored in string for
             you to use, and you have just been asked to do something with
             it.  Note that anything done here must be reversable in the UNDO case */
          break;


        case UNDO:
             /* Back out any changes made in the ACTION case */
          break;


        case COMMIT:
             /* Things are working well, so it's now safe to make the change
             permanently.  Make sure that anything done here can't fail! */
          break;
  }
  return SNMP_ERR_NOERROR;
}




int
write_ipNetToMediaType(int      action,
            u_char   *var_val,
            u_char   var_val_type,
            size_t   var_val_len,
            u_char   *statP,
            oid      *name,
            size_t   name_len)
{
  static long *long_ret;
  int size;


  switch ( action ) {
        case RESERVE1:
          if (var_val_type != ASN_INTEGER){
              fprintf(stderr, "write to ipNetToMediaType not ASN_INTEGER\n");
              return SNMP_ERR_WRONGTYPE;
          }
          if (var_val_len > sizeof(long_ret)){
              fprintf(stderr,"write to ipNetToMediaType: bad length\n");
              return SNMP_ERR_WRONGLENGTH;
          }
          break;


        case RESERVE2:
          size = var_val_len;
          long_ret = (long *) var_val;


          break;


        case FREE:
             /* Release any resources that have been allocated */
          break;


        case ACTION:
             /* The variable has been stored in long_ret for
             you to use, and you have just been asked to do something with
             it.  Note that anything done here must be reversable in the UNDO case */
          break;


        case UNDO:
             /* Back out any changes made in the ACTION case */
          break;


        case COMMIT:
            /* Things are working well, so it's now safe to make the change
                permanently.  Make sure that anything done here can't fail! */
            break;
  }
  return SNMP_ERR_NOERROR;
}

#endif
// ---------------------------------------------------------------------------

// EOF ip.c
