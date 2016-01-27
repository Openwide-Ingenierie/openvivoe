/*
 * Note: this file originally auto-generated by mib2c using
 *        $
 */

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "../include/deviceReset.h"
#include "../include/mibParameters.h"


/** Initializes the deviceReset module */
void
init_deviceReset(void)
{
    const oid deviceReset_oid[] = { 1,3,6,1,4,1,35990,3,1,1,15 };

  DEBUGMSGTL(("deviceReset", "Initializing\n"));

    netsnmp_register_instance(
        netsnmp_create_handler_registration("deviceReset", handle_deviceReset,
                               deviceReset_oid, OID_LENGTH(deviceReset_oid),
                               HANDLER_CAN_RWRITE
        ));
}

int
handle_deviceReset(netsnmp_mib_handler *handler,
                          netsnmp_handler_registration *reginfo,
                          netsnmp_agent_request_info   *reqinfo,
                          netsnmp_request_info         *requests)
{
    int * old_deviceReset = NULL;
    int ret;
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                     &(deviceInfo.deviceReset), 2);
            break;

        /*
         * SET REQUEST
         *
         * multiple states in the transaction.  See:
         * http://www.net-snmp.org/tutorial-5/toolkit/mib_module/set-actions.jpg
         */
        case MODE_SET_RESERVE1:
            /* or you could use netsnmp_check_vb_type_and_size instead */
            ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
            if ( ret != SNMP_ERR_NOERROR ) {
                netsnmp_set_request_error(reqinfo, requests, ret );
            }
            break;

        case MODE_SET_RESERVE2:
            /* XXX malloc "undo" storage buffer */
            old_deviceReset =  (int*) netsnmp_memdup((int *) & (deviceInfo.deviceReset),  sizeof(deviceInfo.deviceReset));
            if (old_deviceReset == NULL) {
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_RESOURCEUNAVAILABLE);
            }else{
                *old_deviceReset = deviceInfo.deviceReset;
            }
            /*checking type as it is done in MODE_SET_RESERVE1
             * is not enough, here we also want to check if
             * the integer is 1, 2 or 3. Otherwise, we return an error
             * indicating to the user that the integer used is a ba value
             * for our deviceReset parameter.
             */
            if( *(requests->requestvb->val.integer) <=0 || *(requests->requestvb->val.integer) >2)
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_BADVALUE);
            break;

        case MODE_SET_FREE:
            /* XXX: free resources allocated in RESERVE1 and/or
            RESERVE2.  Something failed somewhere, and the states
            below won't be called. */
            free(old_deviceReset); /*free resources allocated in RESERVE2*/
            break;

        case MODE_SET_ACTION:
            /* XXX: perform the value change here */
            deviceInfo.deviceReset = *(requests->requestvb->val.integer);
            /*Send a message during debug to inform the update had been performed*/
            DEBUGMSGTL(("deviceReset", "updated delay_time -> %d\n", deviceInfo.deviceReset));
            /*get possible errors*/
            ret = netsnmp_check_requests_error(requests);
            if (ret != SNMP_ERR_NOERROR) {
                netsnmp_set_request_error(reqinfo, requests, ret);
            }
            break;

        case MODE_SET_COMMIT:
            /* XXX: delete temporary storage
            if ( XXX: error? ) {
                 try _really_really_ hard to never get to this point
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_COMMITFAILED);
            }*/
            break;

        case MODE_SET_UNDO:
            /* XXX: UNDO and return to previous value for the object */
            memcpy(&(deviceInfo.deviceReset), old_deviceReset, 2);
            ret = netsnmp_check_requests_error(requests);
            if (ret != SNMP_ERR_NOERROR) {
                /* try _really_really_ hard to never get to this point */
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_UNDOFAILED);
            }
            break;

        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_deviceReset\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}
