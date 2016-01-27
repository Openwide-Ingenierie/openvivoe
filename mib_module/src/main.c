/*
 * Licence: GPL
 * Created: Thu, 14 Jan 2016 12:05:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */
#include <stdio.h>

/*
 * Compulsory header - for SNMP or other purposes (but mainly SNMP)
 */
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <signal.h>

/*
 * Personal header
 */
#include "../include/deviceDesc.h"
#include "../include/deviceManufacturer.h"
#include "../include/devicePartNumber.h"
#include "../include/deviceSerialNumber.h"
#include "../include/deviceHardwareVersion.h"
#include "../include/deviceSoftwareVersion.h"
#include "../include/deviceFirmwareVersion.h"
#include "../include/deviceMibVersion.h"
#include "../include/deviceType.h"
#include "../include/deviceUserDesc.h"
#include "../include/ethernetIfNumber.h"
#include "../include/ethernetIfTable.h"

/*#include "../include/ethernetIfSpeed.h"
#include "../include/ethernetIfMacAddress.h"
#include "../include/ethernetIfIpAddress.h"
#include "../include/ethernetIfSubnetMask.h"
#include "../include/ethernetIfIpAddressConflict.h"*/

#include "../include/deviceNatoStockNumber.h"
#include "../include/deviceMode.h"
#include "../include/deviceReset.h"
#include "../include/config.h"


/* main deamon for the MIB */

static int keep_running;

RETSIGTYPE
stop_server(int a) {
    keep_running = 0;
}

int
main (int argc, char **argv) {
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
    init_agent("mib_module");

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





  /* initialize vacm/usm access control  */
  if (!agentx_subagent) {
      init_vacm_vars();
      init_usmUser();
  }

  /* example-demon will be used to read example-demon.conf files. */
  init_snmp("mib_module");

  /* If we're going to be a snmp master agent, initial the ports */
  if (!agentx_subagent)
    init_master_agent();  /* open the port to listen on (defaults to udp:161) */

  /* In case we receive a request to stop (kill -TERM or kill -INT) */
  keep_running = 1;
  signal(SIGTERM, stop_server);
  signal(SIGINT, stop_server);

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


