/*
 * Licence: GPL
 * Created: Fri, 12 Feb 2016 12:48:48 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */

#include "../../include/conf/mib-conf.h"
#include <stdio.h>
#include <glib-2.0/glib.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../../include/mibParameters.h"


/* Define the number of groups in the configuration file
 * It should correspond to the number of primary nodes in the VIVOE MIB
 */
#define NUM_GROUP 5

int check_MAC_format(u_char** mac, int ethernetIf_Number){
    int i=0;
    int j =0;
    int Mac_size;
    /* The number of Mac addresses passed into the configuration file have already been tested at this point
     * there are no need to check it again. However, for every one of them we should check their size and their content
     */
     for(i=0; i< ethernetIf_Number; i++){
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
            fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n",groupsName_vector[i] ,groupsName_vector[i] );
            return EXIT_FAILURE;
        }
    }

    /* Once we have check that all groups were present in the configuration file
     * we assign the values to the different
     * MIB's parameters
     */

    /*
     *--------------------------------deviceInfo--------------------------------
     */
    /*creation of the deviceInfo's parameter*/
    parameter deviceDesc                    = {"deviceDesc",                    STRING,     1};
    parameter deviceManufacturer            = {"deviceManufacturer",            STRING,     1};
    parameter devicePartNumber              = {"devicePartNumber",              STRING,     1};
    parameter deviceSerialNumber            = {"deviceSerialNumber",            STRING,     1};
    parameter deviceHardwareVersion         = {"deviceHardwareVersion",         STRING,     1};
    parameter deviceSoftwareVersion         = {"deviceSoftwareVersion",         STRING,     1};
    parameter deviceFirmwareVersion         = {"deviceFirmwareVersion",         STRING,     1};
    parameter deviceMibVersion              = {"deviceMibVersion",              STRING,     1};
    parameter deviceType                    = {"deviceType",                    INTEGER,    0};
    parameter deviceUserDesc                = {"deviceUserDesc",                STRING,     1};
    parameter ethernetIfNumber              = {"ethernetIfNumber",              INTEGER,    0};
    parameter ethernetIfSpeed               = {"ethernetIfSpeed",               T_INTEGER,  0};
    parameter ethernetIfMacAddress          = {"ethernetIfMacAddress",          T_STRING,   0};
    parameter ethernetIfIpAddress           = {"ethernetIfIpAddress",           T_STRING,   0};
    parameter ethernetIfSubnetMask          = {"ethernetIfSubnetMask",          T_STRING,   0};
    parameter ethernetIfIpAddressConflict   = {"ethernetIfIpAddressConflict",   T_STRING,   0};
    parameter deviceNatoStockNumber         = {"deviceNatoStockNumber",         STRING,     1};
    parameter deviceMode                    = {"deviceMode",                    INTEGER,    1};
    parameter deviceReset                   = {"deviceReset",                   STRING,     0};



    parameter deviceInfo_parameters[DEVICEINFO_NUM_PARAM] = {  deviceDesc,            deviceManufacturer,
                                                               devicePartNumber,      deviceSerialNumber,
                                                               deviceHardwareVersion, deviceSoftwareVersion,
                                                               deviceFirmwareVersion, deviceMibVersion,
                                                               deviceType,            deviceUserDesc,
                                                               ethernetIfNumber,      ethernetIfSpeed,
                                                               ethernetIfMacAddress,  ethernetIfIpAddress,
                                                               ethernetIfSubnetMask,  ethernetIfIpAddressConflict,
                                                               deviceNatoStockNumber, deviceMode,
                                                               deviceReset   };
   
   
   /* Defined what separator will be used in the list when the parameter can have several values (for a table for example)*/
    g_key_file_set_list_separator (gkf, (gchar) ';');

    /* Get the value of all parameter that are present into the configuration file*/
    for(i=0; i<DEVICEINFO_NUM_PARAM; i++) {
        if(g_key_file_has_key(gkf, groups[0], (const gchar*) deviceInfo_parameters[i]._name , &error)){
            switch(deviceInfo_parameters[i]._type){
                    case INTEGER:
                            deviceInfo_parameters[i]._value.int_val = (int) g_key_file_get_integer(gkf, groups[0], (const gchar*) deviceInfo_parameters[i]._name, &error);
                            if(error != NULL){
                                fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) deviceInfo_parameters[i]._name, error->message);
                                error = NULL; /* resetting the error pointer*/
                                error_occured = TRUE; /*set the error boolean to*/
                            }
                            break;
                    case STRING:
                            deviceInfo_parameters[i]._value.string_val = (char*) g_key_file_get_string(gkf, groups[0], (const gchar*) deviceInfo_parameters[i]._name, &error);
                            if(error != NULL){
                                fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) deviceInfo_parameters[i]._name, error->message);
                                error = NULL; /* resetting the error pointer*/
                                error_occured = TRUE; /*set the error boolean to*/
                            }
                            break;
                    case T_INTEGER:
                             /*Only case of int* in deviceInfo is ethernetIfSpeed number*/
                            deviceInfo_parameters[i]._value.array_int_val = (int*) g_key_file_get_integer_list(gkf, groups[0], (const gchar*) deviceInfo_parameters[i]._name, &length,  &error);
                            if(error != NULL){
                                fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) deviceInfo_parameters[i]._name, error->message);
                                error = NULL; /* resetting the error pointer*/
                                error_occured = TRUE; /*set the error boolean to*/
                            }
                            if(length !=  deviceInfo_parameters[num_ethernetIFnumber]._value.int_val ){
                                fprintf(stderr, "Invalid number of values for %s, there should be %ld value(s)\n", deviceInfo_parameters[i]._name,(long) deviceInfo_parameters[num_ethernetIFnumber]._value.int_val );
                                error_occured = TRUE; /*set the error boolean to*/
                            }
                            break;
                    case T_STRING:
                            deviceInfo_parameters[i]._value.array_string_val =  (char**) g_key_file_get_string_list(gkf, groups[0], (const gchar*) deviceInfo_parameters[i]._name, &length, &error);
                            if(error != NULL){
                                fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) deviceInfo_parameters[i]._name, error->message);
                                error = NULL; /* resetting the error pointer*/
                                error_occured = TRUE; /*set the error boolean to*/
                            }
                            if(length != deviceInfo_parameters[num_ethernetIFnumber]._value.int_val){
                                fprintf(stderr, "Invalid number of values for %s, there should be %d value(s)\n", deviceInfo_parameters[i]._name, deviceInfo_parameters[num_ethernetIFnumber]._value.int_val);
                                error_occured = TRUE; /*set the error boolean to*/
                            }
                            else if((   !strcmp("ethernetIfMacAddress",deviceInfo_parameters[i]._name)) &&
                                        check_MAC_format((u_char**) deviceInfo_parameters[i]._value.array_string_val,  deviceInfo_parameters[num_ethernetIFnumber]._value.int_val)){
                                fprintf(stderr, "Invalid format for %s, it should something like: XX:XX:XX:XX:XX:XX\n", "ethernetIfMacAddress");
                                error_occured = TRUE; /*set the error boolean to*/
                                    /* For every MAC address we check the format of the string passed into the config file: it should be a 18 bytes string
                                    * including the '\0' character, and parsed every two characters with ':'
                                    */
                                }
                            break;
            }
        }else{
            /*if the parameter is mandatory, print an error message to the user, and exit the program*/
            if(!deviceInfo_parameters[i]._optional){
                fprintf(stderr, "Parameter %s mandatory and not found: %s\n", (const gchar*) "deviceUserDesc", error->message);
                return EXIT_FAILURE;
            }
            /*if the parameter is optional, do not do anything special, just keep running!*/
        }
    }

    deviceInfo.number       = DEVICEINFO_NUM_PARAM;
    deviceInfo.parameters  = (parameter*) malloc(deviceInfo.number*sizeof(parameter));
    memcpy(deviceInfo.parameters ,deviceInfo_parameters, sizeof(deviceInfo_parameters));
	
    /* free the GKeyFile before leaving the function*/
    g_key_file_free (gkf);


    if(error_occured==FALSE){
        return EXIT_FAILURE;
    }else{
        return EXIT_SUCCESS;
    }
}
