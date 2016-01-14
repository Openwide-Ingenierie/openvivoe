/*
 * Licence: GPL
 * Created: Thu, 14 Jan 2016 12:05:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */


#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include "../include/nstAgentSubagentObject.h"

/*
 * the variable we want to tie an OID to.  The agent will handle all
 * * GET and SET requests to this variable changing it's value as needed.
 */

static long      nstAgentSubagentObject = 2;

/*
 * our initialization routine, automatically called by the agent 
 * (to get called, the function name must match init_FILENAME()) 
 */
void
init_nstAgentSubagentObject(void)
{
    static oid      nstAgentSubagentObject_oid[] =
        { 1, 3, 6, 1, 4, 1, 8072, 2, 4, 1, 1, 2, 0 };

    /*
     * a debugging statement.  Run the agent with -DnstAgentSubagentObject to see
     * the output of this debugging statement. 
     */
    DEBUGMSGTL(("nstAgentSubagentObject",
                "Initializing the nstAgentSubagentObject module\n"));
    
    /*
     * the line below registers our variables defined above as
     * accessible and makes it writable.  A read only version of any
     * of these registration would merely call
     * register_read_only_long_instance() instead.  The functions
     * called below should be consistent with your MIB, however.
     * 
     * If we wanted a callback when the value was retrieved or set
     * (even though the details of doing this are handled for you),
     * you could change the NULL pointer below to a valid handler
     * function. 
     */
    DEBUGMSGTL(("nstAgentSubagentObject",
                "Initalizing nstAgentSubagentObject scalar integer.  Default value = %d\n",
                nstAgentSubagentObject));

    netsnmp_register_long_instance("nstAgentSubagentObject",
                                  nstAgentSubagentObject_oid,
                                  OID_LENGTH(nstAgentSubagentObject_oid),
                                  &nstAgentSubagentObject, NULL);

    DEBUGMSGTL(("nstAgentSubagentObject",
                "Done initalizing nstAgentSubagentObject module\n"));
}