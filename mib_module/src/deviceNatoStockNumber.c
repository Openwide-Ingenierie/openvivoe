/*
 * Note: this file originally auto-generated by mib2c using
 *        $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "../include/deviceNatoStockNumber.h"
#include "../include/macro.h"
#include "../include/mibParameters.h"



/** Initializes the deviceNatoStockNumber module */
void
init_deviceNatoStockNumber(void)
{
    const oid deviceNatoStockNumber_oid[] = { 1,3,6,1,4,1,35990,3,1,1,13 };

  DEBUGMSGTL(("deviceNatoStockNumber", "Initializing\n"));

    netsnmp_register_read_only_instance(
                                        netsnmp_create_handler_registration("deviceNatoStockNumber", handle_deviceNatoStockNumber,
                                        deviceNatoStockNumber_oid, OID_LENGTH(deviceNatoStockNumber_oid),
                                        HANDLER_CAN_RONLY ));
}

int
handle_deviceNatoStockNumber(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(   requests->requestvb, ASN_OCTET_STR,
                                        deviceInfo.deviceNatoStockNumber, MIN(strlen(deviceInfo.deviceNatoStockNumber), 32));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_deviceNatoStockNumber\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
