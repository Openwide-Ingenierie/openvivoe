/*
 * Licence: GPL
 * Created: Thu, 18 Jan 2016 12:15:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "include/deviceInfo/deviceDesc.h"
#include "include/mibParameters.h"
#include "include/handler.h"


/**
 * \brief Initializes the deviceDesc module
 */
void init_deviceDesc(void)
{
    const oid deviceDesc_oid[] = { 1,3,6,1,4,1,35990,3,1,1,1 };

    g_debug("deviceDesc initializing");
    netsnmp_register_read_only_instance(
                    netsnmp_create_handler_registration(
                            "deviceDesc", handle_deviceDesc,
                            deviceDesc_oid, OID_LENGTH(deviceDesc_oid),
                    HANDLER_CAN_RONLY));
}

/**
 * \brief calls appropriate handler for this parameter
 * \param handler the specific handler for this item
 * \param reqinfo the SNMP request
 * \param reuests the resuest information
 * \param mib_parameter the parameter of the MIB
 * \return SNMP_ERR_NOERROR or approriate code error
 */
int handle_deviceDesc(  netsnmp_mib_handler *handler,
                        netsnmp_handler_registration *reginfo,
                        netsnmp_agent_request_info   *reqinfo,
                        netsnmp_request_info         *requests)
{
        return handle_ROstring32(handler, reginfo, reqinfo, requests, &deviceInfo.parameters[num_DeviceDesc]);

}
