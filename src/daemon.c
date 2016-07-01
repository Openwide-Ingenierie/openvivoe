/*
 * Licence: GPL
 * Created: Tue, 09 Feb 2016 11:54:15 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */


#include <stdio.h>

/*
 * Compulsory header - for SNMP or other purposes (but mainly SNMP)
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <libgen.h>/*for basename*/
#include <net-snmp/library/vacm.h> /*for init_vacms_vars()*/
#include <net-snmp/library/snmpusm.h> /*for init_usmUser()*/
#include <signal.h>

#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
/*
 * mib parameters
 */
#include "mibParameters.h"

/*
 * DeviceInfo header
 */
#include "deviceInfo/deviceDesc.h"
#include "deviceInfo/deviceManufacturer.h"
#include "deviceInfo/devicePartNumber.h"
#include "deviceInfo/deviceSerialNumber.h"
#include "deviceInfo/deviceHardwareVersion.h"
#include "deviceInfo/deviceSoftwareVersion.h"
#include "deviceInfo/deviceFirmwareVersion.h"
#include "deviceInfo/deviceMibVersion.h"
#include "deviceInfo/deviceType.h"
#include "deviceInfo/deviceUserDesc.h"
#include "deviceInfo/ethernetIfNumber.h"
#include "deviceInfo/ethernetIfTable.h"
#include "deviceInfo/deviceNatoStockNumber.h"
#include "deviceInfo/deviceMode.h"
#include "deviceInfo/deviceReset.h"
/*
 * videoFormatInfo header
 */
#include "videoFormatInfo/videoFormatNumber.h"
#include "videoFormatInfo/videoFormatTable.h"
/*
 * channelControl header
 */
#include "channelControl/channelNumber.h"
#include "channelControl/channelReset.h"
#include "channelControl/channelTable.h"

/*
 * SAP/SDP announcement
 */
#include "announcement/sap.h"

/*
 * Configuration - Initialization of the MIB header
 */
#include "conf/mib-conf.h"

/*
 * Header of this file
 */
#include "daemon.h"

/**
 * \brief start deamon for the MIB subAgent
 * \param deamon_name the name of the deamon (openvivoe)
 * \return EXIT_FAILURE or EXIt_SUCCESS
 */
int open_vivoe_daemon (char* deamon_name) {
    int syslog = 0; /* change this if you want to use syslog */
    /* print log errors to syslog or stderr */
    if (syslog)
        snmp_enable_calllog();
    else
        snmp_enable_stderrlog();
	/* make us a agentx client. */
    netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);

    /* Before starting the agent, we should initialize the MIB's parameters
     * from the configuration file vivoe-mib.conf
     */
    if ( !init_mib_content() )
        return EXIT_FAILURE;

    /* initialize the agent library */
    init_agent(basename(deamon_name));

    /* initialize mib code here
     * this is where we initialize each init_parameterOfMIB routine to be handle by the agent
     */
    init_deviceDesc();
    init_deviceManufacturer();
    init_devicePartNumber();
    init_deviceSerialNumber();
    init_deviceHardwareVersion();
    init_deviceSoftwareVersion();
    init_deviceFirmwareVersion();
    init_deviceMibVersion();
    init_deviceType();
    init_deviceUserDesc();
    init_ethernetIfNumber();
    init_ethernetIfTable();
    init_deviceNatoStockNumber();
    init_deviceMode();
    init_deviceReset();
	init_channelNumber();
	init_channelReset();
	init_sap_multicast();
	init_channelTable();
	init_videoFormatNumber();
	init_videoFormatTable();

	/*
	 *Now that streams for SP/SU have been loaded, close configuration file
	 */
	close_mib_configuration_file( gkf_conf_file );

	/* openvivoe-demon will be used to read openvivoe-demon.conf files. */
	g_debug("init SNMP");
	init_snmp(basename(deamon_name));

	g_info("%s is up and running.", basename(deamon_name));
	return EXIT_SUCCESS;

}

gboolean handle_snmp_request( void ){
    agent_check_and_process(0); /* 0 == don't block */
	return TRUE;
}
