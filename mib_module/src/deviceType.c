/*
 * Licence: GPL
 * Created: Thu, 14 Jan 2016 12:05:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */

#include "../include/deviceType.h"

/*
 * the variable we want to tie an OID to.  The agent will handle all
 * * GET and SET requests to this variable changing it's value as needed.
 */

static long      deviceType = 2;

/*
 * our initialization routine, automatically called by the agent 
 * (to get called, the function name must match init_FILENAME()) 
 */
void init_deviceType(void)
{   
    /*
    * this is the oid that we want to modify
    */
    static oid      deviceType_oid[] = { 1, 3, 6, 1, 4, 1, 35990, 3, 1, 1, 9};
    
    /*
     * a debugging statement.  Run the agent with -DdeviceType to see
     * the output of this debugging statement. 
     */
    DEBUGMSGTL(("deviceType",
                "Initializing the deviceType module\n"));
    
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
    DEBUGMSGTL(("deviceType",
                "Initalizing deviceType scalar integer.  Default value = %d\n",
                deviceType));

    netsnmp_register_long_instance("deviceType", deviceType_oid,OID_LENGTH(deviceType_oid), &deviceType, NULL);

    DEBUGMSGTL(("deviceType",
                "Done initalizing deviceType module\n"));
}