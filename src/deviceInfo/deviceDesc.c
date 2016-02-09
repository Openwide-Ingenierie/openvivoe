/*
 * Licence: GPL
 * Created: Thu, 18 Jan 2016 12:15:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "../../include/deviceInfo/deviceDesc.h"
#include "../../include/handler.h"
#include "../../include/mibParameters.h"


/** Initializes the deviceDesc module */
void init_deviceDesc(void)
{
    const oid deviceDesc_oid[] = { 1,3,6,1,4,1,35990,3,1,1,1 };

    DEBUGMSGTL(("deviceDesc", "Initializing\n"));
    netsnmp_register_read_only_instance(
                    netsnmp_create_handler_registration(
                            "deviceDesc", handle_deviceDesc,
                            deviceDesc_oid, OID_LENGTH(deviceDesc_oid),
                    HANDLER_CAN_RONLY));
}

int handle_deviceDesc(  netsnmp_mib_handler *handler,
                        netsnmp_handler_registration *reginfo,
                        netsnmp_agent_request_info   *reqinfo,
                        netsnmp_request_info         *requests)
{
        printf("%s\n", deviceInfo.parameters[num_DeviceDesc]._value.string_val);
        return handle_ROstring32(handler, reginfo, reqinfo, requests, "deviceDesc" , deviceInfo.parameters[num_DeviceDesc]._value.string_val);

}
