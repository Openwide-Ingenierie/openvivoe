/*
 * Licence: GPL
 * Created: Thu, 28 Jan 2016 10:33:47 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef HANDLER_H
# define HANDLER_H

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>


/*function declarations*/

/*create an handler for a RO integer*/
int handle_ROinteger(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests, const char* caller_name ,int* value);

/*create an handler for a RW integer*/
int handle_RWinteger(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests, const char* caller_name ,int* value);

/*create an handler for RO strings*/
int handle_ROstring16(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests, const char* caller_name ,char* value);
int handle_ROstring32(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests, const char* caller_name ,char* value);
int handle_ROstring64(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests, const char* caller_name ,char* value);


/*create an handler for RW strings*/
int handle_RWstring16(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info  *reqinfo, netsnmp_request_info *requests, const char* caller_name, char* value);
int handle_RWstring32(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info  *reqinfo, netsnmp_request_info *requests, const char* caller_name, char* value);
int handle_RWstring64(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info  *reqinfo, netsnmp_request_info *requests, const char* caller_name, char* value);

#endif /* HANDLER_H */

