/*
 * Licence: GPL
 * Created: Fri, 12 Feb 2016 12:48:48 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */

#include <stdio.h>
#include <glib-2.0/glib.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../../include/mibParameters.h"
#include "../../include/conf/mib-conf.h"

/**
 * \brief initialize the deviceInfo section from paramters enter in configuration file
 * \param gkf the GKeyFile which is the configuration_file
 * \param groups name of the groups present in the configuration file
 * \param error a Gerror to check if errors occurs
 * \return EXIT_FAILURE (1) on failure, EXIT_SUCCES (0) on sucess
 */
static int init_deviceInfo(GKeyFile* gkf, gchar* group_name, GError* error, gsize *length){
    /* This is a boolean used to check if everything went ok, otherwise the function will return EXIT_FAILURE
     * This will allow to print all the synthax errors to the user before exiting the program
     */
	int error_occured = FALSE;
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
    parameter deviceNatoStockNumber         = {"deviceNatoStockNumber",         STRING,     1};
    parameter deviceMode                    = {"deviceMode",                    INTEGER,    1};
    parameter deviceReset                   = {"deviceReset",                   STRING,     0};
    parameter ethernetInterface             = {"ethernetInterface",             T_STRING,   1};
    parameter ethernetIfNumber       	    = {"ethernetIfNumber",             	INTEGER,    1};


    parameter deviceInfo_parameters[DEVICEINFO_NUM_PARAM] = {  deviceDesc,            deviceManufacturer,
                                                               devicePartNumber,      deviceSerialNumber,
                                                               deviceHardwareVersion, deviceSoftwareVersion,
                                                               deviceFirmwareVersion, deviceMibVersion,
                                                               deviceType,            deviceUserDesc,
                                                               deviceNatoStockNumber, deviceMode,
                                                               deviceReset, 		  ethernetInterface,
															   ethernetIfNumber	
															};
   
    /* Get the value of all parameter that are present into the configuration file*/
    for(int i=0; i<DEVICEINFO_NUM_PARAM - 1; i++) {
		if(g_key_file_has_key(gkf, group_name, (const gchar*) deviceInfo_parameters[i]._name , &error)){
            switch(deviceInfo_parameters[i]._type){
                    case INTEGER:
                            deviceInfo_parameters[i]._value.int_val = (int) g_key_file_get_integer(gkf, group_name, (const gchar*) deviceInfo_parameters[i]._name, &error);
                            if(error != NULL){
                                fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) deviceInfo_parameters[i]._name, error->message);
                                error = NULL; /* resetting the error pointer*/
                                error_occured = TRUE; /*set the error boolean to*/
                            }
                            break;
                    case STRING:
                            deviceInfo_parameters[i]._value.string_val = (char*) g_key_file_get_string(gkf, group_name, (const gchar*) deviceInfo_parameters[i]._name, &error);
                            if(error != NULL){
                                fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) deviceInfo_parameters[i]._name, error->message);
                                error = NULL; /* resetting the error pointer*/
                                error_occured = TRUE; /*set the error boolean to*/
                            }
                            break;
                    case T_INTEGER:
                            deviceInfo_parameters[i]._value.array_int_val = (int*) g_key_file_get_integer_list(gkf, group_name, (const gchar*) deviceInfo_parameters[i]._name, length,  &error);
                            if(error != NULL){
                                fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) deviceInfo_parameters[i]._name, error->message);
                                error = NULL; /* resetting the error pointer*/
                                error_occured = TRUE; /*set the error boolean to*/
                            }
                            break;
                    case T_STRING: /* only case here is ethernetInterface, from that we should compute ethernetIfNumber */
                            deviceInfo_parameters[i]._value.array_string_val =  (char**) g_key_file_get_string_list(gkf, group_name, (const gchar*) deviceInfo_parameters[i]._name, length, &error);
                            if(error != NULL){
                                fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) deviceInfo_parameters[i]._name, error->message);
                                error = NULL; /* resetting the error pointer*/
                                error_occured = TRUE; /*set the error boolean to*/
                            }
							/* check that the key is indeed ethernetInterface */
							if( !strcmp(deviceInfo_parameters[i]._name, "ethernetInterface"))
								deviceInfo_parameters[num_ethernetIFnumber]._value.int_val = (int) *length;
                            break;
            }
        }else{
            /*if the parameter is mandatory, print an error message to the user, and exit the program*/
            if(!deviceInfo_parameters[i]._optional){
                fprintf(stderr, "Parameter %s mandatory and not found: %s\n", (const gchar*)deviceInfo_parameters[i]._name, error->message);
                return EXIT_FAILURE;
            }
            /*if the parameter is optional, do not do anything special, just keep running!*/
        }
    }

    deviceInfo.number      = DEVICEINFO_NUM_PARAM;
    deviceInfo.parameters  = (parameter*) malloc(deviceInfo.number*sizeof(parameter));
    memcpy(deviceInfo.parameters ,deviceInfo_parameters, sizeof(deviceInfo_parameters));
    if(error_occured==FALSE){
        return EXIT_FAILURE;
    }else{
        return EXIT_SUCCESS;
    }
}

/**
 * \breif intiatition of constant paramerters of channelNumber
 */
parameter channelNumber = {"nb_screens", INTEGER, 0};

/**
 * \brief intialize the globla variable paramer channelNumber 
 * \param gkf the GkeyFile configuration File vivoe_mib.conf
 * \param group_name the name of the group in which the value of channelNumber should be found
 * \param error a object to store errors when they occurs
 * \return EXIT_SUCCESS on success, EXIT_FAILURE on FAILURE
 */
static int init_channelNumber(GKeyFile* gkf, gchar* group_name, GError* error){
	if(g_key_file_has_key(gkf, group_name, (const gchar*) channelNumber._name , &error)){
		channelNumber._value.int_val = (int) g_key_file_get_integer(gkf, group_name, (const gchar*)channelNumber._name, &error);
		 if(error != NULL){
			fprintf(stderr, "Invalid format for %s: %s\n", (const gchar*) channelNumber._name, error->message);
			return EXIT_FAILURE;
		}
	}else{
		/*if the parameter is mandatory, print an error message to the user, and exit the program*/
		if(!channelNumber._optional){
			fprintf(stderr, "Parameter mandatory and not found: %s\n", (const gchar*)channelNumber._name);
			return EXIT_FAILURE;
		}
		/*if the parameter is optional, do not do anything special, just keep running!*/
	}
	return	EXIT_SUCCESS;
}
/**
 * \biref the number of group in vivoe_mib.conf
 */
#define NUM_GROUP 	2
/**
 * \brief this function is used to get the information to place in the VIVOE MIB
 * from the configuration file associated to it: "vivoe-mib.conf". It will
 * get the values of the different parameters of the MIB, check their validity
 * \return error messages for the parameters that are not Valid, and initialize the other
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

	/* Defined what separator will be used in the list when the parameter can have several values (for a table for example)*/
    g_key_file_set_list_separator (gkf, (gchar) ';');

    /*first we load the different Groups of the configuration file
     * second parameter "gchar* length" is optional*/
    groups = g_key_file_get_groups(gkf, NULL);

	gchar* needed[NUM_GROUP] = {"deviceInfo", "channelControl"};
	/* check that the groups are present in configuration file */
	for (int i=0; i<NUM_GROUP; i++){
		if( !(g_strv_contains((const gchar* const*) groups, needed[i] ))){
			fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n",needed[i] , needed[i]);
			return EXIT_FAILURE;
    	}
	}

	if (!init_deviceInfo(gkf, groups[0], error, &length))
		return EXIT_FAILURE;

	if ( init_channelNumber(gkf,groups[1], error))
		return EXIT_FAILURE;

    /* free the GKeyFile before leaving the function*/
    g_key_file_free (gkf);

	return EXIT_SUCCESS;
}
