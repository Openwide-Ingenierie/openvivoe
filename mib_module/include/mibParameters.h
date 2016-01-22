/*
 * Licence: GPL
 * Created: Thu, 21 Jan 2016 18:39:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */
#ifndef MIBPARAMETERS_H
# define MIBPARAMETERS_H

    /*value of parameter*/
    /*only use 32 bytes strings*/
    char* deviceDesc =  "default";

    /*value of parameter*/
    /*only use 16 bytes strings*/
    char* deviceFirmwareVersion =  "default";

    /*value of parameter*/
    /*only use 16 bytes strings*/
    char* deviceHardwareVersion =  "default";

    /*value of deviceManufacturer*/
    /* It should be a string less or equal to 64 bytes*/
    char* deviceManufacturer = "default";

    /*value of parameter*/
    /*only use 16 bytes strings*/
    char* deviceMibVersion =  "default";

    /*value of deviceMode*/
    /* It should be an integer*/
    /* normal(1),
     * defaultStartUp(2),
     * maintenanceMode(3)
     */
     /*by default it is set to normal mode*/
    int deviceMode = 1;

    /*value of deviceNatoStockNumber*/
    /* It should be a string less or equal to 32 bytes*/
    char* deviceNatoStockNumber = "default";

    /*value of parameter*/
    /*only use 32 bytes strings*/
    char* devicePartNumber =  "default";

    /*value of deviceReset*/
    /* It should be an integer*/
    /* normal(1),
     * reset(2)
     */
     /*by default it is set to normal mode*/
    int deviceReset = 1;

    /*value of parameter*/
    /*only use 32 bytes strings*/
    char* deviceSerialNumber =  "default";

    /*value of parameter*/
    /*only use 16 bytes strings*/
    char* deviceSoftwareVersion =  "default";

    /*value of deviceType*/
    /* It should be an integer
     * serviceProvider(1),
     * serviceUser(2),
     * both(3)
     */
     /*by default it is set to normal serviceProvider*/
    static int deviceType = 1;

    /*value of parameter*/
    /*only use 64 bytes strings*/
    char* deviceUserDesc;

    /*value of ethernetIfNumber*/
    /* By default, it is set to one*/
    long int ethernetIfNumber = 1;

     /* This is the initialization of the default values content
     * of the ethernetIfTable. This is only temporary. In a further
     * implementation, those values will be set from the .conf file
     * associated to the subagent
     */
    long ethernetIfIndex =                      1; /*default index*/
    long ethernetIfSpeed =                      1000; /*default speed value, set here as an example*/
    u_char ethernetIfMacAddress[6] =            {0x00,0x00,0x00,0x00,0x00,0x00};
    size_t ethernetIfMacAddress_len =           6; /*default size for representing ethernet MAC addresses, it should always be set to 6, otherwise an error will occur*/
    u_char ethernetIfIpAddress[4] =             {127, 0, 0, 1}; /*by default, loopback address is used*/
    size_t ethernetIfIpAddress_len =            4; /*default size for representing Internet IP addresses, it should always be set to 4, otherwise an error will occur*/
    u_char ethernetIfSubnetMask[4] =            {255, 255, 255, 0};
    size_t ethernetIfSubnetMask_len =           4; /*default size for representing Internet IP addresses, it should always be set to 4, otherwise an error will occur*/
    u_char ethernetIfIpAddressConflict[4] =     {0, 0, 0, 0};
    size_t ethernetIfIpAddressConflict_len =    4; /*default size for representing Internet IP addresses, it should always be set to 4, otherwise an error will occur*/




#endif /* MIBPARAMETERS_H */

