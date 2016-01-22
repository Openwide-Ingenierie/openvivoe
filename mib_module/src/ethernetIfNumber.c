/*
 * Note: this file originally auto-generated by mib2c using
 *        $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "../include/ethernetIfNumber.h"
#include "../include/mibParameters.h"


/*value of ethernetIfNumber*/
/* By default, it is set to one*/
//long int ethernetIfNumber = 1;

/** Initializes the ethernetIfNumber module */
void init_ethernetIfNumber(void)
{
    const oid ethernetIfNumber_oid[] = { 1,3,6,1,4,1,35990,3,1,1,11 };

  DEBUGMSGTL(("ethernetIfNumber", "Initializing\n"));
    netsnmp_register_read_only_instance(
        netsnmp_create_handler_registration("ethernetIfNumber", handle_ethernetIfNumber, ethernetIfNumber_oid, OID_LENGTH(ethernetIfNumber_oid), HANDLER_CAN_RONLY)
    );
}

int handle_ethernetIfNumber(netsnmp_mib_handler *handler, netsnmp_handler_registration *reginfo, netsnmp_agent_request_info *reqinfo, netsnmp_request_info *requests)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, &ethernetIfNumber,4);
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_ethernetIfNumber\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
