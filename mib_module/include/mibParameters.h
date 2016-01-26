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
    char* deviceDesc;

    /*value of parameter*/
    /*only use 16 bytes strings*/
    char* deviceFirmwareVersion;

    /*value of parameter*/
    /*only use 16 bytes strings*/
    char* deviceHardwareVersion ;

    /*value of deviceManufacturer*/
    /* It should be a string less or equal to 64 bytes*/
    char* deviceManufacturer;

    /*value of parameter*/
    /*only use 16 bytes strings*/
    char* deviceMibVersion ;

    /*value of deviceMode*/
    /* It should be an integer*/
    /* normal(1),
     * defaultStartUp(2),
     * maintenanceMode(3)
     */
     /*by default it is set to normal mode*/
    int deviceMode;

    /*value of deviceNatoStockNumber*/
    /* It should be a string less or equal to 32 bytes*/
    char* deviceNatoStockNumber;

    /*value of parameter*/
    /*only use 32 bytes strings*/
    char* devicePartNumber;

    /*value of deviceReset*/
    /* It should be an integer*/
    /* normal(1),
     * reset(2)
     */
     /*by default it is set to normal mode*/
    int deviceReset;

    /*value of parameter*/
    /*only use 32 bytes strings*/
    char* deviceSerialNumber;

    /*value of parameter*/
    /*only use 16 bytes strings*/
    char* deviceSoftwareVersion;

    /*value of deviceType*/
    /* It should be an integer
     * serviceProvider(1),
     * serviceUser(2),
     * both(3)
     */
     /*by default it is set to normal serviceProvider*/
    int deviceType;

    /*value of parameter*/
    /*only use 64 bytes strings*/
    char* deviceUserDesc;

    /*value of ethernetIfNumber*/
    /* By default, it is set to one*/
    long int ethernetIfNumber;

    /* This is the initialization of the default values content
    * of the ethernetIfTable. This is only temporary. In a further
    * implementation, those values will be set from the .conf file
    * associated to the subagent
    */
    int* ethernetIfSpeed;                           /*default speed value, set here as an example*/
    u_char** ethernetIfMacAddress;                   /* This is the default subNet Mask of the internet Interface*/
    char** ethernetIfIpAddress;                      /*by default, loopback address is used*/
    char** ethernetIfSubnetMask;                     /*this is the SubnetMask used for this network interface*/
    char** ethernetIfIpAddressConflict;              /* This is the default Ip Conflict address, normally, it should not be initialize at the start-up of the subAgent*/


#endif /* MIBPARAMETERS_H */

