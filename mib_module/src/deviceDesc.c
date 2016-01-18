/*
 * Licence: GPL
 * Created: Thu, 18 Jan 2016 12:15:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "../include/deviceDesc.h"

/* Function to pad the string use as a value for deviceDesc to 32 bytes*/
void process_value(char * string){
    char * pad = "                                ";
    if(strlen(string) < 32)
        strncat(string, pad, 32 - strlen(string));
}

/** Initializes the deviceDesc module */
void init_deviceDesc(void)
{
    const oid deviceDesc_oid[] = { 1,3,6,1,4,1,35990,3,1,1,1 };

    /*check the value we want to assigned to deviceDesc*/
   // process_value(deviceDesc);

    DEBUGMSGTL(("deviceDesc", "Initializing\n"));
    netsnmp_register_read_only_instance( netsnmp_create_handler_registration("deviceDesc", handle_deviceDesc, deviceDesc_oid, OID_LENGTH(deviceDesc_oid), HANDLER_CAN_RONLY));
}

int handle_deviceDesc(  netsnmp_mib_handler *handler,
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
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR, deviceDesc, 32);
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_deviceDesc\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
