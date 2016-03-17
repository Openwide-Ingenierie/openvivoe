/*
 * Note: this file originally auto-generated by mib2c using
 *        $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "../../include/channelControl/channelReset.h"
#include "../../include/handler.h"
#include "../../include/mibParameters.h"

/** Initializes the channelReset module */
void
init_channelReset(void)
{
    const oid channelReset_oid[] = { 1,3,6,1,4,1,35990,3,1,3,1 };

  DEBUGMSGTL(("channelReset", "Initializing\n"));

  netsnmp_register_instance(
        netsnmp_create_handler_registration("channelReset", handle_channelReset,
                               channelReset_oid, OID_LENGTH(channelReset_oid),
                               HANDLER_CAN_RWRITE
        ));

}

int
handle_channelReset(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
        return handle_RWinteger(handler, reginfo, reqinfo, requests, channelReset._name, &(channelReset._value.int_val) );
}

