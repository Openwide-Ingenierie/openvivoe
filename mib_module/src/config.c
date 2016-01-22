/*
 * Licence: GPL
 * Created: Thu, 21 Jan 2016 10:48:44 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */

#include "../include/config.h"
#include <stdio.h>
#include <glib-2.0/glib.h>
#include <stdlib.h>
#include <string.h>
#include "../include/mibParameters.h"


/*this function is used to get the information to place in the VIVOE MIB
 * from the configuration file associated to it: "vivoe-mib.conf". It will
 * get the values of the different parameters of the MIB, check their validity
 * return error messages for the parameters that are not Valid, and initialize the other
 */
int get_check_configuration(){

    /* Declaration of a pointer that will contain our configuration file*/
    GKeyFile* gkf;
    /* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
     * declared in the configuration file
     */
    gchar** groups;

    /* Define the error pointer we will be using to check for errors in the configuration file */
    GError* error = NULL;

    /*defined the paths from were we should retrieve our configuration files */

    const gchar* search_dirs[] = {(gchar*)"./conf",(gchar*)"/usr/share/vivoe",(gchar*)"$HOME/.vivoe",(gchar*)".", NULL};
    /*define a pointer to the full path were the vivoe-mib.conf file is located*/
    gchar* gkf_path = NULL;

    /* Define the parameters that should be in deviceInfo group of the configuration file
     * the parameters should be ordered the same as as the MIB, and the same way as the
     * the configuration file. Indeed, this will allow the user to refer only the MIB's parameter
     * he wants to initialize. If he does not need to initialize some parameters, he will just no
     * put them as keys into the MIB.
     */
    //const gchar* deviceInfo_param[] = { (gchar*)"deviceDesc",                   /*0 - sting32*/
    //                                    (gchar*)"deviceManufacturer",           /*1 - string64*/
    //                                    (gchar*)"devicePartNumber",             /*2 - string32*/
    //                                    (gchar*)"deviceSerialNumber",           /*3 - string32*/
    //                                    (gchar*)"deviceHardwareVersion",        /*4 - string16*/
    //                                    (gchar*)"deviceSoftwareVersion",        /*5 - string16*/
    //                                    (gchar*)"deviceFirmwareVersion",        /*6 - string16*/
    //                                    (gchar*)"deviceMibVersion",             /*7 - string16*/
    //                                    (gchar*)"deviceType",                   /*8 - integer*/
    //                                   (gchar*)"deviceUserDesc",               /*9 - string64*/
    //                                    (gchar*)"ethernetIfNumber",             /*10 - integer*/
    //                                    (gchar*)"ethernetIfSpeed",              /*11 - integer*/
    //                                    (gchar*)"ethernetIfMacAddress",         /*12 - string MAC*/
    //                                    (gchar*)"ethernetIfIpAddress",          /*13 - string IP*/
    //                                    (gchar*)"ethernetIfSubnetMask",         /*14 - string IP*/
    //                                    (gchar*)"ethernetIfIpAddressConflict",  /*15 - string IP*/
    //                                    NULL};

    /*initialization of the variable */
    gkf = g_key_file_new();

    /*
     * Loads the conf file and tests that everything went OK. Problems can occur if the file doesn't exist,
     * or if user doesn't have read permission to it.
     * If a problem occurs, print it and exit.
     */

    if (!g_key_file_load_from_dirs(gkf,CONFIG_FILE, search_dirs, &gkf_path,G_KEY_FILE_NONE, &error)){
        fprintf (stderr, "Could not read config file %s\n%s",CONFIG_FILE,error->message);
        return EXIT_FAILURE;
    }

    /*first we load the different Groups of the configuration file
     * second parameter "gchar* length" is optional*/
    groups = g_key_file_get_groups(gkf, NULL);
    /* Check for all the groups that are defined in VIVOE MIB
     * all of them should be here:
     * _deviceInfo
     * _VideoFormatInfo
     * _ChannelControl
     * _ViveoNotification
     * _VivoeGroups
     */
    if( !(g_strv_contains(groups, (gchar*) "deviceInfo"))){
        fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n", "deviceInfo", "deviceInfo");
        return EXIT_FAILURE;
    }

    if( !(g_strv_contains(groups , (gchar*) "VideoFormatInfo"))){
        fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n", "VideoFormatInfo", "VideoFormatInfo");
        return EXIT_FAILURE;
    }

    if( !(g_strv_contains(groups , (gchar*) "ChannelControl"))){
        fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n", "ChannelControl", "ChannelControl");
        return EXIT_FAILURE;
    }

    if( !(g_strv_contains(groups , (gchar*) "VivoeNotification"))){
        fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n", "VivoeNotification", "VivoeNotification");
        return EXIT_FAILURE;
    }

    if( !(g_strv_contains(groups , (gchar*) "VivoeGroups"))){
        fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n", "VivoeGroups", "VivoeGroups");
        return EXIT_FAILURE;
    }

    /* Once we have check that all groups were present in the configuration file
     * we assign the values to the different MIB's parameters
     */
    /*deviceDesc*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceType", &error)){
        deviceDesc = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceType", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "deviceType", error->message);
            }
    }
    /*deviceManufacturer*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceManufacturer", &error)){
        deviceManufacturer = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceManufacturer", &error);
        if(error != NULL){
                printf(stderr, "Invalid format for%s: %s\n", (const gchar*) "deviceManufacturer", error->message);
        }
    }
    /*devicePartNumber*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*)"devicePartNumber", &error)){
        devicePartNumber = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*)"devicePartNumber", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*)"devicePartNumber", error->message);
        }
    }
   /*deviceSerialNumber*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceSerialNumber", &error)){
        deviceSerialNumber = (char*) g_key_file_get_string(gkf, groups[0], (const char*) "deviceSerialNumber", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const char*) "deviceSerialNumber", error->message);
        }
    }
    /*deviceHardwareVersion*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*)"deviceHardwareVersion", &error)){
        deviceHardwareVersion = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*)"deviceHardwareVersion", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*)"deviceHardwareVersion", error->message);
        }
    }
    /*deviceSoftwareVersion*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceSoftwareVersion", &error)){
        deviceSoftwareVersion = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceSoftwareVersion", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "deviceSoftwareVersion", error->message);
        }
    }
    /*deviceFirmwareVersion*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceFirmwareVersion", &error)){
        deviceFirmwareVersion = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceFirmwareVersion", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "deviceFirmwareVersion", error->message);
        }
    }
    /*deviceMibVersion*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceMibVersion", &error)){
        deviceMibVersion = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceMibVersion", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "deviceMibVersion", error->message);
        }
    }
    /*deviceType*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceType", &error)){
        deviceType = (int) g_key_file_get_integer(gkf, groups[0], (const gchar*) "deviceType", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "deviceType", error->message);
        }
    }
    /*deviceUserDesc*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceUserDesc", &error)){
        deviceUserDesc = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceUserDesc", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "deviceUserDesc", error->message);
        }
    }
    /*ethernetIfNumber*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfNumber", &error)){
        ethernetIfNumber = (int) g_key_file_get_integer(gkf, groups[0], (const gchar*) "ethernetIfNumber", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "ethernetIfNumber", error->message);
        }
    }
    /*ethernetIfSpeed*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfSpeed", &error)){
        ethernetIfSpeed = (int) g_key_file_get_integer(gkf, groups[0], (const gchar*) "ethernetIfSpeed", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "ethernetIfSpeed", error->message);
        }
    }
    /*ethernetIfMacAddress*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfMacAddress", &error)){
        strncpy((char*)ethernetIfMacAddress, (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "ethernetIfMacAddress", &error), 6);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "ethernetIfMacAddress", error->message);
        }
    }
    /*ethernetIfIpAddress*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfIpAddress", &error)){
        strncpy(ethernetIfIpAddress, (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "ethernetIfIpAddress", &error), 4);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "ethernetIfIpAddress", error->message);
        }
    }
    /*ethernetIfSubnetMask*/
    if(g_key_file_has_key(gkf, groups[0],(const gchar*) "ethernetIfSubnetMask", &error)){
        strncpy(ethernetIfSubnetMask, (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "ethernetIfSubnetMask", &error), 4);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "ethernetIfSubnetMask", error->message);
        }
    }
    /*ethernetIfIpAddressConflict*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfIpAddressConflict", &error)){
        strncpy(ethernetIfIpAddressConflict,(char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "ethernetIfIpAddressConflict", &error), 4);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "ethernetIfIpAddressConflict", error->message);
        }
    }

    /* free the GKeyFile before leaving the function*/
    g_key_file_free (gkf);

    return EXIT_SUCCESS;

}
