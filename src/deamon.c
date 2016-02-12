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

/*
 * DeviceInfo header
 */
#include "../include/deviceInfo/deviceDesc.h"
#include "../include/deviceInfo/deviceManufacturer.h"
#include "../include/deviceInfo/devicePartNumber.h"
#include "../include/deviceInfo/deviceSerialNumber.h"
#include "../include/deviceInfo/deviceHardwareVersion.h"
#include "../include/deviceInfo/deviceSoftwareVersion.h"
#include "../include/deviceInfo/deviceFirmwareVersion.h"
#include "../include/deviceInfo/deviceMibVersion.h"
#include "../include/deviceInfo/deviceType.h"
#include "../include/deviceInfo/deviceUserDesc.h"
#include "../include/deviceInfo/ethernetIfNumber.h"
#include "../include/deviceInfo/ethernetIfTable.h"
#include "../include/deviceInfo/deviceNatoStockNumber.h"
#include "../include/deviceInfo/deviceMode.h"
#include "../include/deviceInfo/deviceReset.h"
/*
 * videoFormatInfo header
 */
#include "../include/videoFormatInfo/videoFormatNumber.h"
#include "../include/videoFormatInfo/videoFormatTable.h"

/*
 * Configuration - Initialization of the MIB header
 */
#include "../include/conf/config.h"


/*
 * Header of this file
 */
#include "../include/deamon.h"

/* main deamon for the MIB */

static int keep_running;

RETSIGTYPE
stop_server(int a) {
    keep_running = 0;
}

int deamon (char* deamon_name) {
    int agentx_subagent=1; /* change this if you want to be a SNMP master agent */
    int background = 0; /* change this if you want to run in the background */
    int syslog = 0; /* change this if you want to use syslog */

    /* print log errors to syslog or stderr */
    if (syslog)
        snmp_enable_calllog();
    else
        snmp_enable_stderrlog();

  /* we're an agentx subagent? */

    if (agentx_subagent) {
        /* make us a agentx client. */
        netsnmp_ds_set_boolean(NETSNMP_DS_APPLICATION_ID, NETSNMP_DS_AGENT_ROLE, 1);
    }

    /* run in background, if requested */
    if (background && netsnmp_daemonize(1, !syslog))
        exit(1);

    /* Before starting the agent, we should initialize the MIB's parameters
     * from the configuration file vivoe-mib.conf
     */
    if ( !get_check_configuration() ){
        return EXIT_FAILURE;
    }
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
	init_videoFormatNumber();
	init_videoFormatTable();

  /* initialize vacm/usm access control  */
  if (!agentx_subagent) {
      init_vacm_vars();
      init_usmUser();
  }

  /* example-demon will be used to read example-demon.conf files. */
  init_snmp(basename(deamon_name));

  /* If we're going to be a snmp master agent, initial the ports */
  if (!agentx_subagent)
    init_master_agent();  /* open the port to listen on (defaults to udp:161) */

  /* In case we receive a request to stop (kill -TERM or kill -INT) */
  keep_running = 1;
  signal(SIGTERM, stop_server);
  signal(SIGINT,  stop_server);

  snmp_log(LOG_INFO,"mib_module is up and running.\n");

  /* your main loop here... */
  while(keep_running) {
    /* if you use select(), see snmp_select_info() in snmp_api(3) */
    /*     --- OR ---  */
    agent_check_and_process(1); /* 0 == don't block */
  }

  /* at shutdown time */
  snmp_shutdown("mib_module");

  return EXIT_SUCCESS;
}
