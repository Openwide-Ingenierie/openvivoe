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
#include <ctype.h>
#include "../include/mibParameters.h"

/* Define the number of groups in the configuration file
 * It should correspond to the number of primary nodes in the VIVOE MIB
 */
#define NUM_GROUP 5

/* Define the number of subNode in the deviceInfo section of VIVOE MIB*/
#define NUM_DEVICEINFO_PARAM 10


int check_MAC_format(u_char** mac){
    int i=0;
    int j =0;
    int Mac_size;
    /* The number of Mac addresses passed into the configuration file have already been tested at this point
     * there are no need to check it again. However, for every one of them we should check their size and their content
     */
     for(i=0; i<deviceInfo.ethernetIfNumber; i++){
        /*get the size of a mac address, should be 18*/
        Mac_size = strlen( (const char*) mac[i]);
        if(Mac_size != 17){
            return EXIT_FAILURE;
        }else{
            /*if we are here, the length should be good, but we need to check if it's parsed correctly*/
            if( mac[i][2] != ':' || mac[i][5] != ':' || mac[i][8] != ':' || mac[i][11] != ':' ||mac[i][14] != ':'){
                return EXIT_FAILURE;
            }else{
                for(j=0; j<Mac_size; j=j+3){
                    if(mac[i][j] < '0' || (mac[i][j] > '9' && toupper(mac[i][j]) < 'A') || toupper(mac[i][j]) > 'F')
                        return EXIT_FAILURE;
                    else if (mac[i][j+1] < '0' || (mac[i][j+1] > '9' && toupper(mac[i][j+1]) < 'A') || toupper(mac[i][j+1]) > 'F')
                        return EXIT_FAILURE;
                }
            }
        }
    }
    return EXIT_SUCCESS;
}

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

    /*loop variable*/
    int i=0;

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
        fprintf (stderr, "Could not read config file %s\n%s\n",CONFIG_FILE,error->message);
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

    /* Defined an array containing the group names that should be present into the configuration file
     * to check if there are all present
     */
    const gchar* groupsName_vector[NUM_GROUP] = {   (const gchar*)"deviceInfo",
                                                (const gchar*)"VideoFormatInfo",
                                                (const gchar*)"ChannelControl",
                                                (const gchar*)"VivoeNotification",
                                                (const gchar*)"VivoeGroups" };

    for(i=0; i< NUM_GROUP; i++) {
        if( !(g_strv_contains((const gchar* const*) groups,  groupsName_vector[i]))){
            fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n", "deviceInfo", "deviceInfo");
            return EXIT_FAILURE;
        }
    }

    /* Once we have check that all groups were present in the configuration file
     * we assign the values to the different
     * MIB's parameters
     */
#if 0
    /* Defined an array containing the MIB's parameters name that should be present into the configuration file
     * to check if there are all present
     */
    const gchar* deviceInfoName_vector[NUM_DEVICEINFO_PARAM] ={  (const gchar*)"deviceDesc",
                                               (const gchar*)"deviceManufacturer",
                                               (const gchar*)"devicePartNumber",
                                               (const gchar*)"deviceSerialNumber",
                                               (const gchar*)"deviceHardwareVersion",
                                               (const gchar*)"deviceSoftwareVersion",
                                               (const gchar*)"deviceFirmwareVersion",
                                               (const gchar*)"deviceMibVersion",
                                               (const gchar*)"deviceUserDesc",
                                               (const gchar*)"ethernetIfNumber",
                                            };

    /* Get the value of all parameter that are present into the configuration file*/
    for(i=0; i< NUM_DEVICEINFO_PARAM; i++) {
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) deviceInfoName_vector[i] , &error)){
        deviceInfo.deviceDesc = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceDesc", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceDesc", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }

    }
#endif

    /*deviceDesc - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceDesc", &error)){
        deviceInfo.deviceDesc = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceDesc", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceDesc", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }

    /*deviceManufacturer - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceManufacturer", &error)){
        deviceInfo.deviceManufacturer = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceManufacturer", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceManufacturer", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*devicePartNumber - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*)"devicePartNumber", &error)){
        deviceInfo.devicePartNumber = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*)"devicePartNumber", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*)"devicePartNumber", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
   /*deviceSerialNumber - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceSerialNumber", &error)){
        deviceInfo.deviceSerialNumber = (char*) g_key_file_get_string(gkf, groups[0], (const char*) "deviceSerialNumber", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const char*) "deviceSerialNumber", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*deviceHardwareVersion - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*)"deviceHardwareVersion", &error)){
        deviceInfo.deviceHardwareVersion = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*)"deviceHardwareVersion", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*)"deviceHardwareVersion", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*deviceSoftwareVersion - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceSoftwareVersion", &error)){
        deviceInfo.deviceSoftwareVersion = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceSoftwareVersion", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceSoftwareVersion", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*deviceFirmwareVersion - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceFirmwareVersion", &error)){
        deviceInfo.deviceFirmwareVersion = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceFirmwareVersion", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceFirmwareVersion", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*deviceMibVersion - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceMibVersion", &error)){
        deviceInfo.deviceMibVersion = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceMibVersion", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceMibVersion", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*deviceType - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceType", &error)){
        deviceInfo.deviceType = (int) g_key_file_get_integer(gkf, groups[0], (const gchar*) "deviceType", &error);
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
        deviceInfo.deviceUserDesc = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceUserDesc", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceUserDesc", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*ethernetIfNumber - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfNumber", &error)){
        deviceInfo.ethernetIfNumber = (int) g_key_file_get_integer(gkf, groups[0], (const gchar*) "ethernetIfNumber", &error);
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
        deviceInfo.ethernetIfSpeed = (int*) g_key_file_get_integer_list(gkf, groups[0], (const gchar*) "ethernetIfSpeed", &length,  &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "ethernetIfSpeed", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
        if(length != deviceInfo.ethernetIfNumber){
            fprintf(stderr, "Invalid number of values for %s, there should be %ld value(s)\n", "ethernetIfSpeed", deviceInfo.ethernetIfNumber);
            error_occured = TRUE; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }

    /*ethernetIfMacAddress - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfMacAddress", &error)){
        deviceInfo.ethernetIfMacAddress = (u_char**) g_key_file_get_string_list(gkf, groups[0], (const gchar*) "ethernetIfMacAddress", &length, &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "ethernetIfMacAddress", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
        if(length != deviceInfo.ethernetIfNumber){
            fprintf(stderr, "Invalid number of values for %s, there should be %ld value(s)\n", "ethernetIfMacAddress", deviceInfo.ethernetIfNumber);
            error_occured = TRUE; /*set the error boolean to*/
        }
        else if(check_MAC_format(deviceInfo.ethernetIfMacAddress)){
            fprintf(stderr, "Invalid format for %s, it should something like: XX:XX:XX:XX:XX:XX\n", "ethernetIfMacAddress");
            error_occured = TRUE; /*set the error boolean to*/
        /* For every MAC address we check the format of the string passed into the config file: it should be a 18 bytes string
          * including the '\0' character, and parsed every two characters with ':'
          */
          }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }

    /*ethernetIfIpAddress - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfIpAddress", &error)){
        deviceInfo.ethernetIfIpAddress=  (char**) g_key_file_get_string_list(gkf, groups[0], (const gchar*) "ethernetIfIpAddress", &length, &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "ethernetIfIpAddress", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
        if(length != deviceInfo.ethernetIfNumber){
            fprintf(stderr, "Invalid number of values for %s, there should be %ld value(s)\n", "ethernetIfIpAddress",deviceInfo.ethernetIfNumber);
            error_occured = TRUE; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }

    /*ethernetIfSubnetMask - compulsory*/
    if(g_key_file_has_key(gkf, groups[0],(const gchar*) "ethernetIfSubnetMask", &error)){
        deviceInfo.ethernetIfSubnetMask = (char**) g_key_file_get_string_list(gkf, groups[0], (const gchar*) "ethernetIfSubnetMask", &length, &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "ethernetIfSubnetMask", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
        if(length != deviceInfo.ethernetIfNumber){
            fprintf(stderr, "Invalid number of values for %s, there should be %ld value(s)\n", "ethernetIfSubnetMask", deviceInfo.ethernetIfNumber);
            error_occured = 1; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }
    /*ethernetIfIpAddressConflict - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "ethernetIfIpAddressConflict", &error)){
        deviceInfo.ethernetIfIpAddressConflict = (char**) g_key_file_get_string_list(gkf, groups[0], (const gchar*) "ethernetIfIpAddressConflict", &length, &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for%s: %s\n", (const gchar*) "ethernetIfIpAddressConflict", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
        if(length != deviceInfo.ethernetIfNumber){
            fprintf(stderr, "Invalid number of values for %s, there should be %ld value(s)\n", (const gchar*) "ethernetIfIpAddressConflict", deviceInfo.ethernetIfNumber);
            error_occured = TRUE; /*set the error boolean to*/
        }
    }else{
        fprintf(stderr, "Parameter %s compulsory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
        return EXIT_FAILURE;
    }

    /*deviceNatoStockNumber - optional*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceNatoStockNumber", &error)){
        deviceInfo.deviceNatoStockNumber = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) "deviceNatoStockNumber", &error);
        if(error != NULL){
            fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) "deviceNatoStockNumber", error->message);
            error = NULL; /* resetting the error pointer*/
            error_occured = TRUE; /*set the error boolean to*/
        }
    }
    /*deviceMode - compulsory*/
    if(g_key_file_has_key(gkf, groups[0], (const gchar*) "deviceMode", &error)){
        deviceInfo.deviceMode = (int) g_key_file_get_integer(gkf, groups[0], (const gchar*) "deviceMode", &error);
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
        deviceInfo.deviceReset = (int) g_key_file_get_integer(gkf, groups[0], (const gchar*) "deviceReset", &error);
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




