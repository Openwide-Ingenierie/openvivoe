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
    /*Declaration of a  gsize variable, that will be used to get the size of the list associated to some keys*/
     gsize length;

    /* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
     * declared in the configuration file
     */
    gchar** groups;

    /* Define the error pointer we will be using to check for errors in the configuration file */
    GError* error = NULL;
    /* This is a boolean used to check if everything went ok, otherwise the function will return EXIT_FAILURE
     * This will allow to print all the synthax errors to the user before exiting the program
     */
    int error_occured = FALSE;

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
    if( !(g_strv_contains(groups, (const gchar*) "deviceInfo"))){
        fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n", "deviceInfo", "deviceInfo");
        return EXIT_FAILURE;
    }

    if( !(g_strv_contains(groups , (const gchar*) "VideoFormatInfo"))){
        fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n", "VideoFormatInfo", "VideoFormatInfo");
        return EXIT_FAILURE;
    }

    if( !(g_strv_contains(groups , (const gchar*) "ChannelControl"))){
        fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n", "ChannelControl", "ChannelControl");
        return EXIT_FAILURE;
    }

    if( !(g_strv_contains(groups , (const gchar*) "VivoeNotification"))){
        fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n", "VivoeNotification", "VivoeNotification");
        return EXIT_FAILURE;
    }

    if( !(g_strv_contains(groups , (const gchar*) "VivoeGroups"))){
        fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n", "VivoeGroups", "VivoeGroups");
        return EXIT_FAILURE;
    }

    /* Once we have check that all groups were present in the configuration file
     * we assign the values to the different MIB's parameters
     */
    /*deviceDesc - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceDesc", &error)){
        deviceDesc = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceDesc", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceDesc", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }

    /*deviceManufacturer - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceManufacturer", &error)){
        deviceManufacturer = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceManufacturer", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceManufacturer", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*devicePartNumber - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*)"devicePartNumber", &error)){
        devicePartNumber = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*)"devicePartNumber", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*)"devicePartNumber", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
   /*deviceSerialNumber - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceSerialNumber", &error)){
        deviceSerialNumber = (char*) g_key_file_get_string(gkf, groups[0], (const char*) "deviceSerialNumber", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const char*) "deviceSerialNumber", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*deviceHardwareVersion - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*)"deviceHardwareVersion", &error)){
        deviceHardwareVersion = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*)"deviceHardwareVersion", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*)"deviceHardwareVersion", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*deviceSoftwareVersion - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceSoftwareVersion", &error)){
        deviceSoftwareVersion = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceSoftwareVersion", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceSoftwareVersion", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*deviceFirmwareVersion - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceFirmwareVersion", &error)){
        deviceFirmwareVersion = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceFirmwareVersion", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceFirmwareVersion", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*deviceMibVersion - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceMibVersion", &error)){
        deviceMibVersion = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceMibVersion", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceMibVersion", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*deviceType - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceType", &error)){
        deviceType = (int) g_key_file_get_integer(gkf, groups[0], (const gchar*) "deviceType", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceType", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }

    /*deviceUserDesc - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceUserDesc", &error)){
        deviceUserDesc = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceUserDesc", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceUserDesc", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*ethernetIfNumber - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfNumber", &error)){
        ethernetIfNumber = (int) g_key_file_get_integer(gkf, groups[0], (const gchar*) "ethernetIfNumber", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "ethernetIfNumber", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }

    /* Depending on the number of interface associated to the VIVOE protocol
     * we check for all the compulsory parameters:
     * _ ethernetIfSpeed
     * _ ethernetIfMacAddress
     * _ ethernetIfIpAddress
     * _ ethernetIfSubnetMask
     * The different values for each Ethernet Interface's parameter must be set
     * int the configuration as a list of values separated by ";"
     */

    /*First we defined what separator will be used*/
    g_key_file_set_list_separator (gkf, (gchar) ';');

    /* After each initialization:
     * Check if one parameter is missing for one of the Ethernet Interface associated to the VIVOE protocol
     * As explained before, all the parameters: ethernetIfSpeed, ethernetIfMacAddress, ethernetIfIpAddress, ethernetIfSubnetMask
     * should be initialize for all ethernet Interface that is used by the VIVOE protocol. So there should be as many
     * values in the list of the parameters described above as the number stored by ethernetIfNumber.
     */

    /*ethernetIfSpeed - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfSpeed", &error)){
        ethernetIfSpeed = (long*) g_key_file_get_integer_list(gkf, groups[0], (const gchar*) "ethernetIfSpeed", &length,  &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "ethernetIfSpeed", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
        if(length != ethernetIfNumber){
            fprintf(stderr, "Invalid number of values for %s, there should be %ld value(s)\n", "ethernetIfSpeed", ethernetIfNumber);
            error_occured = TRUE; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }

    /*ethernetIfMacAddress - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfMacAddress", &error)){
        ethernetIfMacAddress = (u_char**) g_key_file_get_string_list(gkf, groups[0], (const gchar*) "ethernetIfMacAddress", &length, &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "ethernetIfMacAddress", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
        if(length != ethernetIfNumber){
            fprintf(stderr, "Invalid number of values for %s, there should be %ld value(s)\n", "ethernetIfMacAddress", ethernetIfNumber);
            error_occured = TRUE; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }

    /*ethernetIfIpAddress - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfIpAddress", &error)){
        ethernetIfIpAddress=  (char**) g_key_file_get_string_list(gkf, groups[0], (const gchar*) "ethernetIfIpAddress", &length, &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "ethernetIfIpAddress", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
        if(length != ethernetIfNumber){
            fprintf(stderr, "Invalid number of values for %s, there should be %ld value(s)\n", "ethernetIfIpAddress", ethernetIfNumber);
            error_occured = TRUE; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }

    /*ethernetIfSubnetMask - compulsory*/
    if(g_key_file_has_key(gkf, groups[0],(const gchar*) "ethernetIfSubnetMask", &error)){
        ethernetIfSubnetMask = (char**) g_key_file_get_string_list(gkf, groups[0], (const gchar*) "ethernetIfSubnetMask", &length, &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "ethernetIfSubnetMask", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
        if(length != ethernetIfNumber){
            fprintf(stderr, "Invalid number of values for %s, there should be %ld value(s)\n", "ethernetIfSubnetMask", ethernetIfNumber);
            error_occured = 1; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }
    /*ethernetIfIpAddressConflict - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfIpAddressConflict", &error)){
        ethernetIfIpAddressConflict = (char**) g_key_file_get_string_list(gkf, groups[0], (const gchar*) "ethernetIfIpAddressConflict", &length, &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "ethernetIfIpAddressConflict", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
        if(length != ethernetIfNumber){
            fprintf(stderr, "Invalid number of values for %s, there should be %ld value(s)\n", (const gchar*) "ethernetIfIpAddressConflict", ethernetIfNumber);
            error_occured = TRUE; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }

    /*deviceNatoStockNumber - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceNatoStockNumber", &error)){
        deviceNatoStockNumber = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceNatoStockNumber", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceNatoStockNumber", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*deviceMode - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceMode", &error)){
        deviceMode = (int) g_key_file_get_integer(gkf, groups[0], (const gchar*) "deviceMode", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceMode", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }

    /*deviceReset - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceReset", &error)){
        deviceReset = (int) g_key_file_get_integer(gkf, groups[0], (const gchar*) "deviceReset", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceReset", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }

    /* free the GKeyFile before leaving the function*/
    g_key_file_free (gkf);


    if(error_occured==FALSE){
        return EXIT_FAILURE;
    }else{
        return EXIT_SUCCESS;
    }
}
