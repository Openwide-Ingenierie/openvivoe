/*
 * Licence: GPL
 * Created: Thu, 28 Jan 2016 10:33:41 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */

#include "../include/handler.h"
#include "../include/macro.h"

/*For 16 bytes string handler*/
#define DislayString16 16

/*For 32 bytes string handler*/
#define DislayString32 32

/*For 64 bytes string handler*/
#define DislayString64 64

/*--------------------------------INTEGER--------------------------------*/

/*this is the handler for all RO integer in the MIB*/
int handle_ROinteger(netsnmp_mib_handler *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info *reqinfo,
                     netsnmp_request_info *requests,
                     const char* caller_name,
                     int* value)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, (void*) value ,sizeof(int));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in %s\n", reqinfo->mode, caller_name);
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/*this is the handler for all RW integer in the MIB*/
int handle_RWinteger(netsnmp_mib_handler *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info   *reqinfo,
                     netsnmp_request_info         *requests,
                     const char* caller_name,
                     int* value)
{
    int * old_integer = NULL;
    int ret;
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                     value, sizeof(int));
            break;

        /*
         * SET REQUEST
         */
        case MODE_SET_RESERVE1:
            ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
            if ( ret != SNMP_ERR_NOERROR ) {
                netsnmp_set_request_error(reqinfo, requests, ret );
            }
            break;

        case MODE_SET_RESERVE2:
            /* XXX malloc "undo" storage buffer */
            old_integer =  (int*) netsnmp_memdup((int *) value,  sizeof(int));
            if (old_integer == NULL) {
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_RESOURCEUNAVAILABLE);
            }else{
                *old_integer = *value;
            }

            break;

        case MODE_SET_FREE:
            /* XXX: free resources allocated in RESERVE1 and/or
            RESERVE2.  Something failed somewhere, and the states
            below won't be called. */
            free(old_integer); /*free resources allocated in RESERVE2*/
            break;

        case MODE_SET_ACTION:
            /* XXX: perform the value change here */
            *value = *(requests->requestvb->val.integer);
            /*Send a message during debug to inform the update had been performed*/
            DEBUGMSGTL((caller_name, "updated delay_time -> %d\n", *value));
            /*get possible errors*/
            ret = netsnmp_check_requests_error(requests);
            if (ret != SNMP_ERR_NOERROR) {
                netsnmp_set_request_error(reqinfo, requests, ret);
            }
            break;

        case MODE_SET_COMMIT:
            break;

        case MODE_SET_UNDO:
            memcpy(value, old_integer, sizeof(int));
            ret = netsnmp_check_requests_error(requests);
            if (ret != SNMP_ERR_NOERROR) {
                /* try _really_really_ hard to never get to this point */
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_UNDOFAILED);
            }
            break;

        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_%s\n",reqinfo->mode , caller_name);
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}


/*--------------------------------STRING--------------------------------*

/*this is the handler for all RO strings in the program*/
int handle_ROstring(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests,
                    const char* caller_name,
                    char* value,
                    int length)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                      value,MIN(strlen(value), length));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in %s\n", reqinfo->mode, caller_name);
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/*this is the handler for all 16 bytes RO strings in the program*/
int handle_ROstring16(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests,
                    const char* caller_name,
                    char* value)
{
    return  handle_ROstring(handler, reginfo, reqinfo, requests, caller_name, value, DislayString16);
}

/*this is the handler for all 32 bytes RO strings in the program*/
int handle_ROstring32(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests,
                    const char* caller_name,
                    char* value)
{
    return  handle_ROstring(handler, reginfo, reqinfo, requests, caller_name, value, DislayString32);
}


/*this is the handler for all 64 bytes RO strings in the program*/
int handle_ROstring64(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests,
                    const char* caller_name,
                    char* value)
{
    return  handle_ROstring(handler, reginfo, reqinfo, requests, caller_name, value, DislayString64);
}

int
handle_RWstring(netsnmp_mib_handler *handler,
                netsnmp_handler_registration *reginfo,
                netsnmp_agent_request_info   *reqinfo,
                netsnmp_request_info         *requests,
                const char* caller_name,
                char* value,
                int length)
{
    int ret;
    char * old_string = NULL; /* this will be used to perform UNDO*/
    char * temp_string = NULL; /* this will be used to perform SET ACTION*/

    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     value,MIN(strlen(value), length
                                    ));
            break;

        /*
         * SET REQUEST
         *
         * multiple states in the transaction.  See:
         * http://www.net-snmp.org/tutorial-5/toolkit/mib_module/set-actions.jpg
         */
        case MODE_SET_RESERVE1:
            /* or you could use netsnmp_check_vb_type_and_size instead */
            ret = netsnmp_check_vb_type(requests->requestvb, ASN_OCTET_STR);
            if ( ret != SNMP_ERR_NOERROR ) {
                netsnmp_set_request_error(reqinfo, requests, ret );
            }
            break;

        case MODE_SET_RESERVE2:
            /* XXX malloc "undo" storage buffer */
             old_string =  (char*) netsnmp_memdup((char *) & (value),  strlen(value));
            if (old_string == NULL) {
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_RESOURCEUNAVAILABLE);
            }else{
                old_string = value;
            }
             if( strlen((char*) requests->requestvb->val.string) > length )
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_TOOBIG);
            break;

        case MODE_SET_FREE:

               free(old_string); /*free resources allocated in RESERVE2*/
            break;

        case MODE_SET_ACTION:
            /* XXX: perform the value change here */
            /*realloc to the size of the string entered bu the user*/
            temp_string = (char*) realloc( value, strlen((char*) requests->requestvb->val.string)*sizeof(char));
            if(temp_string == NULL){
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_RESOURCEUNAVAILABLE);
            }else{
                value = temp_string;
                strcpy(value,(char*)requests->requestvb->val.string);
            }
            /*Send a message during debug to inform the update had been performed*/
           DEBUGMSGTL((caller_name, "updated delay_time -> %s\n", value));
            /*get possible errors*/
            ret = netsnmp_check_requests_error(requests);
           if (ret != SNMP_ERR_NOERROR) {
                netsnmp_set_request_error(reqinfo, requests, ret);
            }
            break;

        case MODE_SET_COMMIT:
            break;

        case MODE_SET_UNDO:
            /* XXX: UNDO and return to previous value for the object */
            memcpy(value, old_string, sizeof(*old_string));
            ret = netsnmp_check_requests_error(requests);
            if (ret != SNMP_ERR_NOERROR) {
                /* try _really_really_ hard to never get to this point */
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_UNDOFAILED);
            }
            break;

        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_deviceUserDesc\n", reqinfo->mode );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}


/*this is the handler for all 16 bytes RW strings in the program*/
int handle_RWstring16(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests,
                    const char* caller_name,
                    char* value)
{
    return  handle_RWstring(handler, reginfo, reqinfo, requests, caller_name, value, DislayString16);
}

/*this is the handler for all 32 bytes RW strings in the program*/
int handle_RWstring32(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests,
                    const char* caller_name,
                    char* value)
{
    return  handle_RWstring(handler, reginfo, reqinfo, requests, caller_name, value, DislayString32);
}


/*this is the handler for all 64 bytes RW strings in the program*/
int handle_RWstring64(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests,
                    const char* caller_name,
                    char* value)
{
    return  handle_RWstring(handler, reginfo, reqinfo, requests, caller_name, value, DislayString64);
}
