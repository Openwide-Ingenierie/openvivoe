/*
 * Licence: GPL
 * Created: Thu, 28 Jan 2016 10:33:41 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#include <glib-2.0/glib.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "../include/mibParameters.h"
#include "../include/handler.h"

/*For 16 bytes string handler*/
#define DislayString16 16

/*For 32 bytes string handler*/
#define DislayString32 32

/*For 64 bytes string handler*/
#define DislayString64 64

/**
 * \brief return appropriate code value in case object should modify only in maintenance mode
 * \param reginfo same as the handler's one
 * \param reqinfo same as the handler's one
 * \param requests same as the handler's one
 * \param parameter the given parameter to test
 */
static void check_maintenance( 	netsnmp_handler_registration *reginfo,
                     			netsnmp_agent_request_info   *reqinfo,
								netsnmp_request_info         *requests,
								parameter *mib_param){
	int ret = SNMP_ERR_NOERROR;
	if ( mib_param->_mainenance_group && deviceInfo.parameters[num_DeviceMode]._value.int_val	!= maintenanceMode){
		ret = SNMP_ERR_RESOURCEUNAVAILABLE;
		netsnmp_set_request_error(reqinfo, requests, ret );
	}
}

/**
 * \brief check if the index given to the function is out of the range of the table
 * \param reginfo same as the handler's one
 * \param reqinfo same as the handler's one
 * \param requests same as the handler's one
 * \param parameter the given parameter to test
 */
gboolean index_out_of_range( 	netsnmp_handler_registration 	*reginfo,
                     		netsnmp_agent_request_info   	*reqinfo,
							netsnmp_request_info         	*requests,
							netsnmp_table_request_info 		*table_info,
							int max_index){

	if ( *(table_info->indexes->val.integer) > max_index){
		netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_NOSUCHNAME );
		return TRUE;
	}else
		return FALSE;
} 


/*--------------------------------INTEGER--------------------------------*/

/*this is the handler for all RO integer in the MIB*/
int handle_ROinteger(netsnmp_mib_handler *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info *reqinfo,
                     netsnmp_request_info *requests,
                     parameter *mib_param)
{
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */
    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER, (void*) &(mib_param->_value.int_val) ,sizeof(int));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in %s\n", reqinfo->mode, mib_param->_name);
            return SNMP_ERR_GENERR;
    }
    return SNMP_ERR_NOERROR;
}

int old_integer = -1;
/*this is the handler for all RW integer in the MIB*/
int handle_RWinteger(netsnmp_mib_handler *handler,
                     netsnmp_handler_registration *reginfo,
                     netsnmp_agent_request_info   *reqinfo,
                     netsnmp_request_info         *requests,
					 parameter *mib_param
                     )
{
    int ret;
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_INTEGER,
                                     &(mib_param->_value.int_val), sizeof(int));
            break;

        /*
         * SET REQUEST
         */
        case MODE_SET_RESERVE1:
            ret = netsnmp_check_vb_type(requests->requestvb, ASN_INTEGER);
            if ( ret != SNMP_ERR_NOERROR ) {
                netsnmp_set_request_error(reqinfo, requests, ret );
            }
			check_maintenance( reginfo, reqinfo, requests, mib_param);
            break;

        case MODE_SET_RESERVE2:
            /* XXX malloc "undo" storage buffer */	
            break;

        case MODE_SET_FREE:
            /* XXX: free resources allocated in RESERVE1 and/or
            RESERVE2.  Something failed somewhere, and the states
            below won't be called. */
            break;

        case MODE_SET_ACTION:
            /* XXX: perform the value change here */
            old_integer =  mib_param->_value.int_val;
			mib_param->_value.int_val = *(requests->requestvb->val.integer);
            /*Send a message during debug to inform the update had been performed*/
            DEBUGMSGTL((mib_param->_name, "updated value -> %d\n",mib_param->_value.int_val ));
            /*get possible errors*/
            ret = netsnmp_check_requests_error(requests);
            if (ret != SNMP_ERR_NOERROR) {
                netsnmp_set_request_error(reqinfo, requests, ret);
            }
            break;

        case MODE_SET_COMMIT:
            break;

        case MODE_SET_UNDO: /* will be executed only if some part of MODE_SET_ACTION fails */
			mib_param->_value.int_val = old_integer;
            ret = netsnmp_check_requests_error(requests);
            if (ret != SNMP_ERR_NOERROR) {
                /* try _really_really_ hard to never get to this point */
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_UNDOFAILED);
            }
            break;

        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in handle_%s\n",reqinfo->mode ,mib_param->_name);
            return SNMP_ERR_GENERR;
    }
    return SNMP_ERR_NOERROR;
}


/*--------------------------------STRING--------------------------------*/

/*this is the handler for all RO strings in the program*/
static int handle_ROstring( netsnmp_mib_handler *handler,
       			            netsnmp_handler_registration *reginfo,
                    		netsnmp_agent_request_info   *reqinfo,
                    		netsnmp_request_info         *requests,
							parameter *mib_param,
 							int 		length
                    )
{

    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                      mib_param->_value.string_val,MIN(strlen(mib_param->_value.string_val), length));
            break;


        default:
            /* we should never get here, so this is a really bad error */
            snmp_log(LOG_ERR, "unknown mode (%d) in %s\n", reqinfo->mode,mib_param->_name );
            return SNMP_ERR_GENERR;
    }

    return SNMP_ERR_NOERROR;
}

/*this is the handler for all 16 bytes RO strings in the program*/
int handle_ROstring16(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests,
					parameter *mib_param)
{
    return  handle_ROstring(handler, reginfo, reqinfo, requests, mib_param, DislayString16);
}

/*this is the handler for all 32 bytes RO strings in the program*/
int handle_ROstring32(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests,
					parameter *mib_param
                    )
{
    return  handle_ROstring(handler, reginfo, reqinfo, requests,mib_param , DislayString32);
}


/*this is the handler for all 64 bytes RO strings in the program*/
int handle_ROstring64(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests,
					parameter *mib_param
                    )
{
    return  handle_ROstring(handler, reginfo, reqinfo, requests,mib_param , DislayString64);
}

/**
 * \brief a global string to save the old values of the MIB when a set is performed. Max size a of string in MIB is 64 bytes, so we're cool.
 */
char old_string[64];

int
handle_RWstring(netsnmp_mib_handler *handler,
                netsnmp_handler_registration *reginfo,
                netsnmp_agent_request_info   *reqinfo,
                netsnmp_request_info         *requests,
                parameter *mib_param,
                int length)
{
    int ret;
    /* We are never called for a GETNEXT if it's registered as a
       "instance", as it's "magically" handled for us.  */

    /* a instance handler also only hands us one request at a time, so
       we don't need to loop over a list of requests; we'll only get one. */

    switch(reqinfo->mode) {

        case MODE_GET:
            snmp_set_var_typed_value(requests->requestvb, ASN_OCTET_STR,
                                     mib_param->_value.string_val,MIN(strlen(mib_param->_value.string_val), length));
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
			check_maintenance( reginfo, reqinfo, requests, mib_param);
            break;

        case MODE_SET_RESERVE2:
            /* XXX malloc "undo" storage buffer */
			if( strlen((char*) requests->requestvb->val.string) > length )
                netsnmp_set_request_error(reqinfo, requests, SNMP_ERR_TOOBIG);
			break;

        case MODE_SET_FREE:
            break;

        case MODE_SET_ACTION:
            /* XXX: perform the value change here */
            /*realloc to the size of the string entered bu the user*/
			strcpy( old_string, mib_param->_value.string_val);
			strcpy(mib_param->_value.string_val,(char*)requests->requestvb->val.string); 
            /*Send a message during debug to inform the update had been performed*/
           DEBUGMSGTL((mib_param->_name, "updated delay_time -> %s\n",mib_param->_value.string_val ));
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
            strcpy(mib_param->_value.string_val, old_string);
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
					parameter *mib_param
                    )
{
    return  handle_RWstring(handler, reginfo, reqinfo, requests, mib_param, DislayString16);
}

/*this is the handler for all 32 bytes RW strings in the program*/
int handle_RWstring32(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests,
					parameter *mib_param
                    )
{
    return  handle_RWstring(handler, reginfo, reqinfo, requests,mib_param , DislayString32);
}


/*this is the handler for all 64 bytes RW strings in the program*/
int handle_RWstring64(netsnmp_mib_handler *handler,
                    netsnmp_handler_registration *reginfo,
                    netsnmp_agent_request_info   *reqinfo,
                    netsnmp_request_info         *requests,
					parameter *mib_param
                    )
{
    return  handle_RWstring(handler, reginfo, reqinfo, requests,mib_param , DislayString64);
}
