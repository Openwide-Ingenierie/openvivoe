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
#include <arpa/inet.h>
#include <netinet/in.h>

#include "mibParameters.h"
#include "conf/mib-conf.h"

/**
 * \brief the number of parameters in the maintenance group
 */
#define MAINTENANCE_GROUP_SIZE 	19

/**
 * \brief the path where to search for configuration files
 */
const gchar* search_dirs[] = {(gchar*)"./conf",(gchar*)"/etc/openvivoe",(gchar*)".", NULL};

/**
 * \brief set the mainenance flag to one for the given parameter if necesseray, otherwise, set it to false
 * \param param the MIB parameter that need to be modify
 * \return TRUE if the param is in the maintenance group, FALSE otherwise
 */
static gboolean set_maintenencef_flag(parameter *param){

	 /* maintenance group */
const gchar* maintenance_group[MAINTENANCE_GROUP_SIZE]= { 	"deviceUserDesc", 				"ethernetIfIpAddress",
															"ethernetIfSubnetMask", 		"ethernetIfIpAddressConflict",
															"deviceReset", 					"videoFormatCompressionRate",
															"videoFormatRoiHorzRes", 		"videoFormatRoiVertRes",
															"videoFormatRoiOriginTop", 		"videoFormatRoiOriginLeft",
															"videoFormatRoiExtentBottom", 	"videoFormatRoiExtentRight",
															"videoFormatRtpPt", 			"channelReset",
															"channelUserDesc", 				"channelInterPacketDelay",
															"channelSapMessageInterval", 	"channelDefaultVideoFormatIndex",
															"channelDefaultReceiveIpAddress"};
	for(int i = 0 ; i < MAINTENANCE_GROUP_SIZE; i++){
		/* if the name of param is in maintenance group, set maintenance flag to TRUE and return */
		if ( !strcmp( param->_name , maintenance_group[i] )){
			param->_mainenance_group = TRUE;
			return TRUE;
		}
	}
	return FALSE;

}

/**
 * \brief open the vivoe-mib.conf configuration file
 * \param error the Gerror to store error when they happened
 * \gkf_path a set of path into which search for the path
 * \return the GKeyFile if it has been successfully openned, NULL otherwise
 */
static GKeyFile * open_mib_configuration_file(GError* error, gchar** gkf_path){

	GKeyFile* gkf;

    /*initialization of the variable */
    gkf = g_key_file_new();

    /*
     * Loads the conf file and tests that everything went OK. Problems can occur if the file doesn't exist,
     * or if user doesn't have read permission to it.
     * If a problem occurs, print it and exit.
     */
    if (!g_key_file_load_from_dirs( gkf , CONFIG_FILE , search_dirs , gkf_path, G_KEY_FILE_KEEP_COMMENTS , &error )){
        fprintf (stderr, "Could not read config file %s\n%s\n",CONFIG_FILE,error->message);
        return NULL;
    }

	return gkf;

}

/**
 * \brief close the gkf configuration file
 * \param gkf the GKeyFile openned
 */
void close_mib_configuration_file(GKeyFile *gkf){

    /* free the GKeyFile before leaving the function*/
    g_key_file_free (gkf);
}


/**
 * \biref the number of group in vivoe_mib.conf
 */
#define NUM_GROUP 	1

/**
 * \brief check if mandatory groups are present
 * \param gkf the configuration file
 * \param groups an array of string to store the groups' names found in the configuration file
 * \param error an Gerror to save errors if theeu occurs
 */
gchar** check_mib_group(GKeyFile *gkf, GError *error){

	/*Declaration of a  gsize variable, that will be used to get the size of the list associated to some keys*/
     gsize length;

	 /* the array of string to return */
	 gchar **groups;

	/*first we load the different Groups of the configuration file
     * second parameter "gchar* length" is optional*/
	groups = g_key_file_get_groups(gkf, &length);

	gchar* needed[NUM_GROUP] = {GROUP_NAME_DEVICEINFO };
	/* check that the groups are present in configuration file */
	for (int i=0; i<NUM_GROUP; i++){
		if( !(g_strv_contains((const gchar* const*) groups, needed[i] ))){
			fprintf (stderr, "Group %s not found in configuration file\nIt should be written in the form [%s]\n",needed[i] , needed[i]);
			return NULL;
    	}
	}
	return groups;
}


/**
 * \brief initialize the deviceInfo section from paramters enter in configuration file
 * \param gkf the GKeyFile which is the configuration_file
 * \param groups name of the groups present in the configuration file
 * \param error a Gerror to check if errors occurs
 * \return EXIT_FAILURE (1) on failure, EXIT_SUCCES (0) on sucess
 */
static gboolean init_deviceInfo(GKeyFile* gkf, gchar* group_name, GError* error){

	/*Declaration of a  gsize variable, that will be used to get the size of the list associated to some keys*/
     gsize length;


    /* This is a boolean used to check if everything went ok, otherwise the function will return EXIT_FAILURE
     * This will allow to print all the synthax errors to the user before exiting the program
     */
	int error_occured = FALSE;
    /* Once we have check that all groups were present in the configuration file
     * we assign the values to the different
     * MIB's parameters
     */
  /* Define the parameters that should be in deviceInfo group of the configuration file
     * the parameters should be ordered the same as as the MIB, and the same way as the
     * the configuration file. Indeed, this will allow the user to refer only the MIB's parameter
     * he wants to initialize. If he does not need to initialize some parameters, he will just no
     * put them as keys into the MIB.
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
    parameter deviceReset                   = {"deviceReset",                   INTEGER,    0};
    parameter ethernetInterface             = {"ethernetInterface",             T_STRING,   0};
    parameter ethernetIfNumber       	    = {"ethernetIfNumber",             	STRING, 	1};


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
    for(int  i=0; i < DEVICEINFO_NUM_PARAM - 1; i++) {
		/* set maintenance flag for each parameter */
		set_maintenencef_flag( &deviceInfo_parameters[i] );

		if(g_key_file_has_key(gkf, group_name, (const gchar*) deviceInfo_parameters[i]._name , &error)){
            switch(deviceInfo_parameters[i]._type){
                    case INTEGER:
                            deviceInfo_parameters[i]._value.int_val = (int) g_key_file_get_integer(gkf, group_name, (const gchar*) deviceInfo_parameters[i]._name, &error);
                            if(error != NULL){
                                g_critical("Invalid format for %s: %s\n", (const gchar*) deviceInfo_parameters[i]._name, error->message);
                                error = NULL; /* resetting the error pointer*/
                                error_occured = TRUE; /*set the error boolean to*/
                            }
                            break;
                    case STRING:
                            deviceInfo_parameters[i]._value.string_val = (char*) g_key_file_get_string(gkf, group_name, (const gchar*) deviceInfo_parameters[i]._name, &error);
                            if(error != NULL){
                                g_critical("Invalid format for %s: %s\n", (const gchar*) deviceInfo_parameters[i]._name, error->message);
                                error = NULL; /* resetting the error pointer*/
                                error_occured = TRUE; /*set the error boolean to*/
                            }
                            break;
                    case T_INTEGER:
                            deviceInfo_parameters[i]._value.array_int_val = (int*) g_key_file_get_integer_list(gkf, group_name, (const gchar*) deviceInfo_parameters[i]._name, &length,  &error);
                            if(error != NULL){
                                g_critical("Invalid format for %s: %s\n", (const gchar*) deviceInfo_parameters[i]._name, error->message);
                                error = NULL; /* resetting the error pointer*/
                                error_occured = TRUE; /*set the error boolean to*/
                            }
                            break;
                    case T_STRING: /* only case here is ethernetInterface, from that we should compute ethernetIfNumber */
                            deviceInfo_parameters[i]._value.array_string_val =  (char**) g_key_file_get_string_list(gkf, group_name, (const gchar*) deviceInfo_parameters[i]._name, &length, &error);
                            if(error != NULL){
                                g_critical("Invalid format for %s: %s\n", (const gchar*) deviceInfo_parameters[i]._name, error->message);
                                error = NULL; /* resetting the error pointer*/
                                error_occured = TRUE; /*set the error boolean to*/
                            }
							/* check that the key is indeed ethernetInterface */
							if( !strcmp(deviceInfo_parameters[i]._name, "ethernetInterface"))
								deviceInfo_parameters[num_ethernetIFnumber]._value.int_val = (int) length;
                            break;
            }
        }else{
            /*if the parameter is mandatory, print an error message to the user, and exit the program*/
            if(!deviceInfo_parameters[i]._optional){
                g_critical("Parameter %s mandatory and not found", (const gchar*)deviceInfo_parameters[i]._name);
                return FALSE;
            }
            /*if the parameter is optional, do not do anything special, just keep running!*/
        }
    }

    deviceInfo.number      = DEVICEINFO_NUM_PARAM;
    deviceInfo.parameters  = (parameter*) malloc(deviceInfo.number*sizeof(parameter));
    memcpy(deviceInfo.parameters ,deviceInfo_parameters, sizeof(deviceInfo_parameters));
    if(error_occured==FALSE){
        return TRUE;
    }else{
        return FALSE;
    }
}

/**
 * \breif intiatition of constant paramerters of channelNumber
 */
parameter channelNumber = {"nb_screens", INTEGER, 0};

/**
 * \brief intialize the global variable paramer channelNumber
 * \param gkf the GkeyFile configuration File vivoe_mib.conf
 * \param group_name the name of the group in which the value of channelNumber should be found
 * \param error a object to store errors when they occurs
 * \return EXIT_SUCCESS on success, EXIT_FAILURE on FAILURE
 */
static gboolean init_channelNumber_param(GKeyFile* gkf, gchar **groups, GError* error){

	/* set maintenance flag for channelNumber */
	set_maintenencef_flag( &channelNumber );

	char *receiver_prefix = "receiver_";
	char *receiver_name = (char*) malloc( strlen(receiver_prefix)+2 * sizeof(char));

	int i=1;
	for (i=1; i<g_strv_length (groups); i++){
		/* Build the name that the group should have */
		sprintf(receiver_name, "%s%d",receiver_prefix , i);
		if ( !g_strv_contains((const gchar * const *) groups,receiver_name ))
			break;
	}

	free(receiver_name);
	channelNumber._value.int_val = i -1 ;
	if( channelNumber._value.int_val < 0 )
		return FALSE;
	else
		return TRUE;

}

parameter videoFormatNumber = {"videoFormatNumber", INTEGER, 0, {0} };
parameter channelReset 		= {"channelReset", 		INTEGER, 0, {0} };

/**
 * \brief intialize the global variable paramer videoFormatNumber
 * \param gkf the GkeyFile configuration File vivoe_mib.conf
 * \param group_name the name of the group in which the value of videoFormatNumber should be found
 * \param error a object to store errors when they occurs
 * \return TRUE on success, FALSE on FAILURE
 */
static gboolean init_videoFormatNumber_param(GKeyFile *gkf, gchar **groups, GError *error){

	/* set maintenance flag for videoForamatNumber */
	set_maintenencef_flag( &videoFormatNumber );

	char *source_prefix = "source_";
	char *source_name = (char*) malloc( strlen(source_prefix)+2 * sizeof(char));
	int i=1;
	for (i=1; i<g_strv_length (groups); i++){
		/* Build the name that the group should have */
		sprintf(source_name, "%s%d", source_prefix, i);
		if ( !g_strv_contains((const gchar * const *) groups, source_name))
			break;
	}

	videoFormatNumber._value.int_val = i -1 ;

	free(source_name);
	if ( videoFormatNumber._value.int_val < 0 )
		return FALSE;
	else
		return TRUE;

}

/* static definition and initiation of ethernetIpAssignment value */
parameter ethernetIpAssignment = {"ethernetIpAssignment", STRING, 1 };
/**
 * \brief intialize the global variable value ethernetIpAssignment
 * \param gkf the GkeyFile configuration File vivoe_mib.conf
 * \param group_name the name of the group in which the value of ethernetIpAssignment should be found
 * \param error a object to store errors when they occurs
 * \return TRUE on success, FALSE on FAILURE
 */
static gboolean init_ethernetIpAssignment_param(GKeyFile *gkf, gchar *group_name, GError *error){

	ethernetIpAssignment._value.string_val = (char*) g_key_file_get_string(gkf, group_name , (const gchar*) ethernetIpAssignment._name, &error);
	if(error != NULL){
		g_warning("Invalid format for %s: %s\n", (const gchar*) ethernetIpAssignment._name, error->message);
		error = NULL; /* resetting the error pointer*/
	}

	if ( 	strcmp( ethernetIpAssignment._value.string_val , KEY_ETHERNET_IP_ASSIGNMENT_DEFAULT )
		   	&& strcmp ( ethernetIpAssignment._value.string_val , KEY_ETHERNET_IP_ASSIGNMENT_VIVOE	)  ){
		g_warning("Invalid key value for %s, can only be set to %s or %s\n",
			   	(const gchar*) ethernetIpAssignment._name ,
			   	KEY_ETHERNET_IP_ASSIGNMENT_DEFAULT ,
				KEY_ETHERNET_IP_ASSIGNMENT_VIVOE  );
		return FALSE;
	}
	else
		return TRUE;

}



/**
 * \brief initalize global variable which value cannot be found in vivoe_mib.conf
 * \param void
 */
static void init_mib_global_parameter(){

	/* set maintenance flag for videoForamatNumber */
	set_maintenencef_flag( &channelReset );

}

/**
 * \brief give the vlaue of vivoe-element's property, "", if the element is ine cmdline but no property is set, NULL otherwise
 * \param the cmdline to analyze
 * \return the vlaue of vivoe-element's property, "", if the element is ine cmdline but no property is set, NULL otherwise
 */
static gchar *get_vivoe_element_with_property ( gchar *cmdline, gchar *vivoe_element_name, gchar *property ) {

	if ( cmdline == NULL )
		return NULL;

	gchar **splitted, **gst_elements;

	gchar *property_name = g_strdup ( property );

	/* parse entirely the command line */
	splitted = g_strsplit ( cmdline , "!", -1);

	for (int i = 0 ; i < g_strv_length(splitted); i++){

		gst_elements = g_strsplit ( splitted[i] , " ", -1);

		/* check if the gst element mention contains the redirection element */
		if (g_strv_contains ((const gchar * const *) gst_elements, vivoe_element_name )){
			/* if so splitted[i] should also contained a string with "name=..." where ... is the name given to the corresponding elements */
			property_name = realloc( property_name, strlen("="));
			strcat ( property_name , "=");
			if ( !property_name )
				return "";
			property_name = g_strdup( g_strrstr ( splitted[i] , property_name ) );
			g_strstrip( property_name );

			/* free splitted */
			g_strfreev(splitted);

			return property_name;
		}
	}

	/* free splitted */
	g_strfreev(splitted);

	return NULL;

}

/**
 * \brief specify if the cmdline is a redirection, if so returns the "name" parameter
 * \param the cmdline to analyze
 * \return the "name" argument or null if no name argument or if cmdline is not a redirection
 */
static gchar *vivoe_redirect(gchar *cmdline){

	gchar *return_value = get_vivoe_element_with_property ( cmdline, VIVOE_REDIRECT_NAME, VIVOE_REDIRECT_PROPERTY_NAME );

	/*
	 * For vivoe-redirect the element should have a propoerty, otherwise, it is an error, so if vivoe-redirect has been
	 * found with no name=... property, return NULL
	 */

	if ( (return_value != NULL ) && (! strcmp(return_value , "" ) ) )
		return NULL;

	return return_value;

}

/**
 * \brief check if the group contains the given key, get the corresponding char value or display appropriate error to the user
 * \param gkf the GKeyFile openned
 * \param groups the names of groups present in configuration file
 * \param group_name the group's name in which we are interested
 * \param key_name the name of the key we are loooking for
 * \param error a variable to store errors
 * \return gchar* the value of the found key or NULL is the key has not been found
 */
static gchar *get_key_value_string(GKeyFile* gkf, const gchar* const* groups ,char *group_name, const gchar *key_name, GError* error){

	gchar *key_value; /* a variable to store the key value */

	if( !(g_strv_contains(groups, group_name )) ){
		g_critical ("Group %s not found in configuration file - It should be written in the form [%s]", group_name ,group_name);
		return NULL;
	}

	if(g_key_file_has_key(gkf,group_name,key_name, &error)){
		key_value = (char*) g_key_file_get_string(gkf,group_name , key_name , &error);
		if(error != NULL)
			g_warning("Invalid format for key %s: %s", key_name , error->message);
	}
	else {
		g_warning("key not found %s for group: %s", key_name ,group_name );
		return NULL;
	}

	if ( !strcmp( key_value, "") ){
		g_warning("invalid key value for %s in %s", key_name ,group_name );
		return NULL;
	}

	return key_value;
}

#if 0
/**
 * \brief check if the group contains the given key, get the corresponding list of values or display appropriate error to the user
 * \param gkf the GKeyFile openned
 * \param groups the names of groups present in configuration file
 * \param group_name the group's name in which we are interested
 * \param key_name the name of the key we are loooking for
 * \param length a variable to store the length of the list
 * \param error a variable to store errors
 * \return gchar* the value of the found key or NULL is the key has not been found
 */
static gchar **get_key_string_list(GKeyFile* gkf, const gchar* const* groups ,char *group_name, const gchar *key_name, gsize *length, GError* error){

	gchar **key_value; /* a variable to store the key value */

	if( !(g_strv_contains(groups, group_name )) ){
		g_critical ("Group %s not found in configuration file - It should be written in the form [%s]", group_name ,group_name);
		return NULL;
	}

	if(g_key_file_has_key(gkf,group_name,key_name, &error)){
		key_value = g_key_file_get_string_list(gkf,group_name , key_name , length , &error);
		if(error != NULL)
			g_warning("Invalid format for key %s: %s", key_name , error->message);
	}
	else {
		g_warning("key not found %s for group: %s", key_name ,group_name );
		return NULL;
	}

	if ( *length == 0 ){
		g_warning("invalid key value for %s in %s", key_name ,group_name );
		return NULL;
	}

	return key_value;
}
#endif //if 0

/**
 * \brief check if the group contains the given key, get the corresponding integer value or display appropriate error to the user
 * \param gkf the GKeyFile openned
 * \param groups the names of groups present in configuration file
 * \param group_name the group's name in which we are interested
 * \param key_name the name of the key we are loooking for
 * \param error a variable to store errors
 * \param optional specify if the key is optional and should be present or optional
 * \return the integer value or -1 if an error occurs
 */
static int get_key_value_int(GKeyFile* gkf, const gchar* const* groups ,char *group_name, const gchar *key_name, GError* error, gboolean optional){

	int key_value; /* a variable to store the key value */

	if( !(g_strv_contains(groups, group_name )) ){
		g_critical ("Group %s not found in configuration file - It should be written in the form [%s]", group_name ,group_name);
		return -1;
	}

	if(g_key_file_has_key(gkf,group_name,key_name, &error)){
		key_value = (int) g_key_file_get_integer(gkf,group_name , key_name , &error);
		if(error != NULL)
			g_warning("Invalid format for key %s: %s", key_name , error->message);
	}
	else {
		if ( !optional )
			g_warning("key not found %s for group: %s", key_name ,group_name );
		return -1;
	}

	if ( key_value < 0  ){
		g_warning("invalid key value for %s in %s", key_name ,group_name );
		return -1;
	}

	return key_value;
}


/**
 * \brief check if the group contains the given key, set the corresponding value or display appropriate error to the user, save the new version of conf file
 * \param gkf the GKeyFile openned
 * \param groups the names of groups present in configuration file
 * \param group_name the group's name in which we are interested
 * \param key_name the name of the key we are looking for
 * \param error a variable to store errors
 * \return gchar* the value of the found key or NULL is the key has not been found
 */
gboolean set_key_value(GKeyFile* gkf, const gchar* const* groups ,char *group_name, gchar* gkf_path,  const gchar *key_name,const gchar *new_value, GError* error){

	if( !(g_strv_contains((const gchar* const*) groups, group_name )))
		 g_critical("Group %s not found in configuration file - It should be written in the form [%s]",group_name,group_name );
	if(g_key_file_has_key(gkf,group_name,key_name , &error)){
		g_key_file_set_string(gkf,group_name ,key_name ,new_value );
		g_key_file_save_to_file(gkf, gkf_path , &error);
		if(error != NULL){
			g_warning("failed to write to configuration file %s: %s",CONFIG_FILE , error->message);
			return FALSE;
		}
	}
	else{
		g_warning("key not found %s for group: %s",key_name ,group_name );
		return FALSE;
	}

	return TRUE;

}

/**
 * \brief check if the group contains the given key, set the corresponding value or display appropriate error to the user, save the new version of conf file
 * \param gkf the GKeyFile openned
 * \param index the index of the SP or SU to which belong the gst_src or gst_sink command line
 * \return gchar* the value of the found key, it should be a gstreamer command line
 */
gchar *get_source_cmdline(GKeyFile 	*gkf, int index){

	/* Define the error pointer we will be using to check for errors in the configuration file */
    GError 		*error 	= NULL;

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	gchar 		**groups;

	/*
	 * the command line string retreive from the configuration file
	 */
	gchar 		*cmdline;

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups = g_key_file_get_groups(gkf, NULL);

	char *source_prefix = "source_";
	char *source_name = (char*) malloc( strlen(source_prefix)+2 * sizeof(char));
	/* Build the name that the group should have */
	sprintf(source_name, "%s%d", source_prefix, index);

	cmdline = get_key_value_string(gkf,(const gchar* const*) groups , source_name ,GST_SOURCE_CMDLINE , error);

	free(source_name);
	return cmdline;

}

/**
 * \brief get the command line entered by the user in configuration file to get the video source
 * \param index the index of the initiated stream
 * \return gchar* the gst_source command line found in a string form or NULL if no ones has been found
 */
gchar *init_sources_from_conf(int index){
	/* Define the error pointer we will be using to check for errors in the configuration file */
    GError 		*error 	= NULL;

	 /* Declaration of a pointer that will contain our configuration file*/

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	gchar 		**groups;

	/*
	 * the command line string retreive from the configuration file
	 */
	gchar 		*cmdline;

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups = g_key_file_get_groups(gkf_conf_file, NULL);

	char *source_prefix = "source_";
	char *source_name = (char*) malloc( strlen(source_prefix)+2 * sizeof(char));
	/* Build the name that the group should have */
	sprintf(source_name, "%s%d", source_prefix, index);

	cmdline = get_key_value_string(gkf_conf_file,(const gchar* const*) groups , source_name ,GST_SOURCE_CMDLINE , error);

	free(source_name);

	return cmdline;
}

/**
 * \brief get the command line entered by the user in configuration file to use as a sink for the received stream
 * \param index the index of the initiated stream
 * \return gchar* the gst_sink command line found in a string form or NULL if no ones has been found
 */
gchar *get_sink_cmdline(GKeyFile 	*gkf, int index){

	/* Define the error pointer we will be using to check for errors in the configuration file */
    GError 		*error 	= NULL;

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	gchar 		**groups;

	/*
	 * the command line string retreive from the configuration file
	 */
	gchar 		*cmdline;

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups = g_key_file_get_groups(gkf, NULL);

	char *receiver_prefix = "receiver_";
	char *receiver_name = (char*) malloc( strlen(receiver_prefix)+2 * sizeof(char));
	/* Build the name that the group should have */
	sprintf(receiver_name, "%s%d", receiver_prefix, index);

	cmdline = get_key_value_string(gkf,(const gchar* const*) groups , receiver_name ,GST_SINK_CMDLINE , error);

	free(receiver_name);
	return cmdline;
}

/**
 * \brief get the command line entered by the user in configuration file to get the video source
 * \param index the index of the initiated stream
 * \return gchar* the command line found in a string form or NULL if no ones has been found
 */
gchar *init_sink_from_conf(int index){
	/* Define the error pointer we will be using to check for errors in the configuration file */
    GError 		*error 	= NULL;

	/* a location to store the path of the openned configuration file */
	gchar* gkf_path = NULL;

	 /* Declaration of a pointer that will contain our configuration file*/
	GKeyFile 	*gkf 	= open_mib_configuration_file(error, &gkf_path);

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	gchar 		**groups;

	/*
	 * the command line string retreive from the configuration file
	 */
	gchar 		*cmdline;

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups = g_key_file_get_groups(gkf, NULL);

	char *receiver_prefix = "receiver_";
	char *receiver_name = (char*) malloc( strlen(receiver_prefix)+2 * sizeof(char));
	/* Build the name that the group should have */
	sprintf(receiver_name, "%s%d", receiver_prefix, index);

	cmdline = get_key_value_string(gkf,(const gchar* const*) groups , receiver_name ,GST_SINK_CMDLINE , error);

	free(receiver_name);
	close_mib_configuration_file(gkf);
	return cmdline;
}


/**
 * \brief returned the channel User Description enter by the user to described the channel
 * \param the corresponding number of the source to refer it in the configuration file
 */
gchar* get_desc_from_conf(int index){
	/* Define the error pointer we will be using to check for errors in the configuration file */
    GError 		*error 	= NULL;

	/* a location to store the path of the openned configuration file */
	gchar* gkf_path = NULL;

	 /* Declaration of a pointer that will contain our configuration file*/
	GKeyFile 	*gkf 	= open_mib_configuration_file(error, &gkf_path);

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	gchar 		**groups;

	/*
	 * the channelUserDesc retreive from the configuration file
	 */
	gchar 		*channelUserDesc;

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups = g_key_file_get_groups(gkf, NULL);

	char *source_prefix = "source_";
	char *source_name = (char*) malloc( strlen(source_prefix)+2 * sizeof(char));
	/* Build the name that the group should have */
	sprintf(source_name, "%s%d", source_prefix, index);

	channelUserDesc = get_key_value_string(gkf,(const gchar* const*) groups , source_name ,KEY_NAME_CHANNEL_DESC , error);

	free(source_name);
	close_mib_configuration_file(gkf);
	return channelUserDesc;
}


/**
 * \brief save the value of ChannelUserDesc enter by the user in configuration file
 * \param index the corresponding number of the source to refer it in the configuration file
 * \param new_default_ip the new value of DefaultReceiveIPaddress
 */
void set_desc_to_conf(int index, const char* new_desc){
	/* Define the error pointer we will be using to check for errors in the configuration file */
    GError 		*error 	= NULL;

	/* a location to store the path of the openned configuration file */
	gchar* gkf_path = NULL;

	 /* Declaration of a pointer that will contain our configuration file*/
	GKeyFile 	*gkf 	= open_mib_configuration_file(error, &gkf_path);

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	gchar 		**groups;

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups = g_key_file_get_groups(gkf, NULL);

	char *source_prefix = "source_";
	char *source_name = (char*) malloc( strlen(source_prefix)+2 * sizeof(char));
	/* Build the name that the group should have */
	sprintf(source_name, "%s%d", source_prefix, index);

	set_key_value(gkf,(const gchar* const*) groups , source_name , gkf_path ,KEY_NAME_CHANNEL_DESC  ,new_desc , error);

	free(source_name);
	close_mib_configuration_file(gkf);
}

/**
 * \brief returned the DefaultIPaddress enter by the user to use for SU in defaultStartUPMode
 * \param the corresponding number of the source to refer it in the configuration file
 * \return the value of the defaultReceiveIp key or NULL if not found
 */
gchar *get_default_IP_from_conf(int index){
	/* Define the error pointer we will be using to check for errors in the configuration file */
    GError 		*error 	= NULL;

	/* a location to store the path of the openned configuration file */
	gchar* gkf_path = NULL;

	 /* Declaration of a pointer that will contain our configuration file*/
	GKeyFile 	*gkf 	= open_mib_configuration_file(error, &gkf_path);

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	gchar 		**groups;

	/*
	 * the default receive IP retreive from the configuration file
	 */
	gchar 		*default_receive_ip;

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups = g_key_file_get_groups(gkf, NULL);

	char *receiver_prefix = "receiver_";
	char *receiver_name = (char*) malloc( strlen(receiver_prefix)+2 * sizeof(char));
	/* Build the name that the group should have */
	sprintf(receiver_name, "%s%d", receiver_prefix, index);

	default_receive_ip = get_key_value_string(gkf,(const gchar* const*) groups ,receiver_name, KEY_NAME_DEFAULT_IP , error);

	free(receiver_name);
	close_mib_configuration_file(gkf);
	return default_receive_ip;
}

/**
 * \brief save the value of DefaultReceiveIPaddress enter by the user to use for SU in defaultStartUPMode in configuration file
 * \param index the corresponding number of the source to refer it in the configuration file
 * \param new_default_ip the new value of DefaultReceiveIPaddress
 */
void set_default_IP_from_conf(int index, const char* new_default_ip){
	/* Define the error pointer we will be using to check for errors in the configuration file */
    GError 		*error 	= NULL;

	/* a location to store the path of the openned configuration file */
	gchar* gkf_path = NULL;

	/* Declaration of a pointer that will contain our configuration file*/
	GKeyFile 	*gkf 	= open_mib_configuration_file(error, &gkf_path);

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	gchar 		**groups;

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups = g_key_file_get_groups(gkf, NULL);
	char *receiver_prefix = "receiver_";
	char *receiver_name = (char*) malloc( strlen(receiver_prefix)+2 * sizeof(char));
	/* Build the name that the group should have */
	sprintf(receiver_name, "%s%d", receiver_prefix, index);

	set_key_value(gkf,(const gchar* const*) groups , receiver_name , gkf_path , KEY_NAME_DEFAULT_IP ,new_default_ip , error);

	free(receiver_name);
	close_mib_configuration_file(gkf);

}

static gboolean check_assignedIP_validity( gchar *ip_value){

	in_addr_t ip 		= inet_addr( ip_value );

	/* start by checking prefix */
	in_addr_t ip_prefix 		= inet_addr( RANDOM_IP_PREFIX );

	/* chekc if IP start with 192.168.204 */
	if ( ( htonl(ip) & 0xFFFFFF00 )  != ( htonl (ip_prefix ) & 0xFFFFFF00) ) {
		return FALSE;
	}

	/* check if its last byte is not 192.168.2042.54 */
	return ( ip !=  inet_addr(DEFAULT_STATIC_IP ) );

}

/**
 * \brief returned the assignedIP saved in the configuration file
 * \return the value of the assigned key or NULL if not found
 */
gchar *get_static_assigned_IP_from_conf( const gchar *if_name){
	/* Define the error pointer we will be using to check for errors in the configuration file */
    GError 		*error 	= NULL;

	/* a location to store the path of the openned configuration file */
	gchar* gkf_path = NULL;

	 /* Declaration of a pointer that will contain our configuration file*/
	GKeyFile 	*gkf 	= open_mib_configuration_file(error, &gkf_path);

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	gchar 		**groups;

	/*
	 * the default receive IP retreive from the configuration file
	 */
	gchar 		*assigned_ip;

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups = g_key_file_get_groups(gkf, NULL);

	/* build key name from KEY_NAME_ASSIGNED_IP and if_name */
	gchar *key_name = g_strconcat ( KEY_NAME_ASSIGNED_IP , "_", if_name ,  NULL);

	assigned_ip = get_key_value_string(gkf,(const gchar* const*) groups , GROUP_NAME_DEVICEINFO , key_name , error);

	/* check if the IP is comprise between 192.168.204.1 and  192.168.204.253 */
	if ( ! check_assignedIP_validity(assigned_ip) ){
		/* if assigned is not valid, stop the program, there are no point to continue */
		g_error ("value of %s is not a valid IP for a VIVOE device",key_name );
	}

	close_mib_configuration_file(gkf);
	g_free ( key_name );
	return assigned_ip;

}

/**
 * \brief save the value of the key "assignedIP" set by the manager in MIB into the configuration file
 * \param new_ip the new value of assignedIP
 */
void set_static_assigned_IP_to_conf(const gchar *if_name , const char* new_ip ){
	/* Define the error pointer we will be using to check for errors in the configuration file */
    GError 		*error 	= NULL;

	/* a location to store the path of the openned configuration file */
	gchar* gkf_path = NULL;

	/* Declaration of a pointer that will contain our configuration file*/
	GKeyFile 	*gkf 	= open_mib_configuration_file(error, &gkf_path);

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	gchar 		**groups;

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups = g_key_file_get_groups(gkf, NULL);

	/* build key name from KEY_NAME_ASSIGNED_IP and if_name */
	gchar *key_name = g_strconcat ( KEY_NAME_ASSIGNED_IP , "_", if_name , NULL);


	set_key_value(gkf,(const gchar* const*) groups , GROUP_NAME_DEVICEINFO , gkf_path , key_name ,  new_ip , error);

	close_mib_configuration_file(gkf);

	g_free ( key_name );

}

/**
 * \brief register into the structure roi_data the MIB ROI parameters' values extract from the configuration file
 * \param index the source index from which we should be looking for ROI parameters
 * \param roi_datas the roi_data structure to fill
 * \return TRUE if ROI values are valid, FALSE otherwise
 */
gboolean
get_roi_parameters_for_sources (
		int index, 						gboolean scalable,
		long *roi_width_ptr , 			long *roi_height_ptr ,
	   	long *roi_top_ptr, 				long *roi_left_ptr,
		long *roi_extent_bottom_ptr,   	long *roi_extent_right_ptr ){

	/* Define the error pointer we will be using to check for errors in the configuration file */
	GError 		*error 	= NULL;

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	gchar 		**groups;

	/*
	 * the roi parameters to retrieve from the configuration file
	 */
	long 		roi_width;
	long 		roi_height;
	long 		roi_top;
	long 		roi_left;
	long 		roi_extent_bottom;
	long 		roi_extent_right;

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups 		= g_key_file_get_groups(gkf_conf_file, NULL);


	char *source_prefix = "source_";
	char *source_name = (char*) malloc( strlen(source_prefix)+2 * sizeof(char));
	/* Build the name that the group should have */
	sprintf(source_name, "%s%d", source_prefix, index);

	roi_width 			= get_key_value_int(gkf_conf_file,(const gchar* const*) groups , source_name , ROI_WIDTH, 			error, TRUE );
	roi_height 			= get_key_value_int(gkf_conf_file,(const gchar* const*) groups , source_name , ROI_HEIGHT, 			error, TRUE );
	roi_top 			= get_key_value_int(gkf_conf_file,(const gchar* const*) groups , source_name , ROI_ORIGIN_TOP ,		error, TRUE );
	roi_left 			= get_key_value_int(gkf_conf_file,(const gchar* const*) groups , source_name , ROI_ORIGIN_LEFT,		error, TRUE );
	roi_extent_bottom 	= get_key_value_int(gkf_conf_file,(const gchar* const*) groups , source_name , ROI_EXTENT_BOTTOM, 	error, TRUE );
	roi_extent_right 	= get_key_value_int(gkf_conf_file,(const gchar* const*) groups , source_name , ROI_EXTENT_RIGHT, 	error, TRUE );

	/*
	 * check if values given in configuration file are correct
	 */

	/*
	 * if origin parameters set, but no ROI resolution specified
	 */
	if ( ( roi_top != -1 || roi_left != -1 ) && ( roi_width == -1 && roi_height ==-1) ){

		g_warning ( "[source_%d] ROI's origin specified but no ROI resolution found\n", index );
		return FALSE;

	}

	if  ( scalable )
	{

		/* if extent object are set but not origin object */
		if ( (roi_extent_bottom != -1 || roi_extent_right != -1) && (roi_top == -1 && roi_left==-1)  ){
			g_warning ( " [source_%d] scalable ROI cannot have %s and %s set if %s and %s are not set too", index, ROI_EXTENT_BOTTOM , ROI_EXTENT_RIGHT , ROI_ORIGIN_TOP , ROI_ORIGIN_LEFT );
			return FALSE;
		}

	}

	if ( roi_width == -1 )
		roi_width = 0 ;
	if ( roi_height == -1 )
		roi_height = 0 ;
	if (roi_top  == -1 )
		roi_top = 0 ;
	if ( roi_left == -1 )
		roi_left = 0 ;
	if ( roi_extent_bottom == -1 )
		roi_extent_bottom = 0 ;
	if ( roi_extent_right == -1 )
		roi_extent_right = 0 ;

	/*
	 * save those values to roi_datas
	 */
	*roi_width_ptr			= roi_width;
	*roi_height_ptr 		= roi_height;
	*roi_top_ptr 			= roi_top;
	*roi_left_ptr 			= roi_left;
	*roi_extent_bottom_ptr 	= roi_extent_bottom;
	*roi_extent_right_ptr 	= roi_extent_right;

	free(source_name);

	return TRUE;

}

/**
 * \brief
 * \param
 * \return
 */
gboolean get_roi_parameters_for_sink(int index , gboolean scalable,
		long *roi_width_ptr , 			long *roi_height_ptr ,
	   	long *roi_top_ptr, 				long *roi_left_ptr,
		long *roi_extent_bottom_ptr,   	long *roi_extent_right_ptr ){
	/* Define the error pointer we will be using to check for errors in the configuration file */
    GError 		*error 	= NULL;

	/* a location to store the path of the openned configuration file */
	gchar* gkf_path = NULL;

	 /* Declaration of a pointer that will contain our configuration file*/
	GKeyFile 	*gkf 	= open_mib_configuration_file(error, &gkf_path);

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	gchar 		**groups;

	/*
	 * the roi parameters to retrieve from the configuration file
	 */
	long 		roi_width;
	long 		roi_height;
	long 		roi_top;
	long 		roi_left;
	long 		roi_extent_bottom;
	long 		roi_extent_right;

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups 		= g_key_file_get_groups(gkf, NULL);

	char *receiver_prefix = "receiver_";
	char *receiver_name = (char*) malloc( strlen(receiver_prefix)+2 * sizeof(char));
	/* Build the name that the group should have */
	sprintf(receiver_name, "%s%d", receiver_prefix, index);

	roi_width 			= get_key_value_int(gkf,(const gchar* const*) groups , receiver_name , ROI_WIDTH, 			error, TRUE );
	roi_height 			= get_key_value_int(gkf,(const gchar* const*) groups , receiver_name , ROI_HEIGHT, 			error, TRUE );
	roi_top 			= get_key_value_int(gkf,(const gchar* const*) groups , receiver_name , ROI_ORIGIN_TOP ,		error, TRUE );
	roi_left 			= get_key_value_int(gkf,(const gchar* const*) groups , receiver_name , ROI_ORIGIN_LEFT,		error, TRUE );
	roi_extent_bottom 	= get_key_value_int(gkf,(const gchar* const*) groups , receiver_name , ROI_EXTENT_BOTTOM, 	error, TRUE );
	roi_extent_right 	= get_key_value_int(gkf,(const gchar* const*) groups , receiver_name , ROI_EXTENT_RIGHT, 	error, TRUE );

	/*
	 * check if values given in configuration file are correct
	 */

	/*
	 * if origin parameters set, but no ROI resolution specified
	 */
	if ( ( roi_top != -1 || roi_left != -1 ) && ( roi_width == -1 && roi_height ==-1) ){

		g_warning ( "[receiver_%d] ROI's origin specified but no ROI resolution found\n", index );
		return FALSE;

	}

	if  ( scalable )
	{

		/* if extent object are set but not origin object */
		if ( (roi_extent_bottom != -1 || roi_extent_right != -1) && (roi_top == -1 && roi_left==-1)  ){
			g_warning ( "[receiver_%d] scalable ROI cannot have %s and %s set if %s and %s are not set too", index, ROI_EXTENT_BOTTOM , ROI_EXTENT_RIGHT , ROI_ORIGIN_TOP , ROI_ORIGIN_LEFT );
			return FALSE;
		}

	}

	if ( roi_width == -1 )
		roi_width = 0 ;
	if ( roi_height == -1 )
		roi_height = 0 ;
	if (roi_top  == -1 )
		roi_top = 0 ;
	if ( roi_left == -1 )
		roi_left = 0 ;
	if ( roi_extent_bottom == -1 )
		roi_extent_bottom = 0 ;
	if ( roi_extent_right == -1 )
		roi_extent_right = 0 ;

	/*
	 * save those values to roi_datas
	 */
	*roi_width_ptr			= roi_width;
	*roi_height_ptr 		= roi_height;
	*roi_top_ptr 			= roi_top;
	*roi_left_ptr 			= roi_left;
	*roi_extent_bottom_ptr 	= roi_extent_bottom;
	*roi_extent_right_ptr 	= roi_extent_right;

	free(receiver_name);

	return TRUE;

}

/**
 * \brief definition of the redirection structure that will contains the redirection data
 */
redirection_str redirection;

/**
 * \brief create the mapping to link redirection between service provider and service user
 * \param gkf a openned GKeyFile
 * \return TRUE on success, FALSE on FAILURE
 */
static gboolean init_redirection_data(GKeyFile* gkf ){

	gchar *cmdline, *name_source, *name_receiver;
	int index_source, index_receiver;

	int i = 0 ;
	int i_old = i ;

	redirection.redirect_channels = (redirect_data**) malloc ( sizeof ( redirect_data* ));

	/* get redirection data for service provider */
	for ( index_source = 1 ; index_source <=  videoFormatNumber._value.int_val ; index_source++){

		cmdline = get_source_cmdline(gkf, index_source );
		name_source = vivoe_redirect(cmdline);

		if( name_source != NULL ){

			/* check if there is a corresponding receiver's channel with same redirection name */
			for (  index_receiver = 1 ; index_receiver <= channelNumber._value.int_val ; index_receiver ++){

				cmdline = get_sink_cmdline(gkf, index_receiver );
				name_receiver = vivoe_redirect(cmdline);

				if( name_receiver != NULL && strcmp(name_receiver,  name_source)==0) {
					redirection.size = i+1 ;
					redirection.redirect_channels[i] =(redirect_data*) malloc ( sizeof (redirect_data)) ;
					redirection.redirect_channels[i]->channel_SU_index 	= index_receiver;
					redirection.redirect_channels[i]->gst_sink 			= init_sink_from_conf( index_receiver );
					redirection.redirect_channels[i]->video_SP_index 	= index_source;
					redirection.redirect_channels[i]->gst_source 		= init_sources_from_conf( index_source );
					i++;
					break;
				}

			}
			/*
			 * if redirect_channels[i] is still NULL, we do not have found a corresponding channel:
			 * --> exit with an error
			 */
			if ( &redirection.redirect_channels[i_old] == NULL ){
				g_warning("redirection of source_%d with name \"%s\" does not have corresponding redirection's receiver\n", index_source, name_source);
				return FALSE;
			}
			i_old = i ;
		}
	}

	return TRUE;

}

/**
 * \brief init the data for the phisical camera control with parameters from the configuration file
 * \param
 * \return the command line to use to control de camera
 */
gchar *get_camera_ctl_cmdline ( int source_index ){

	/* Define the error pointer we will be using to check for errors in the configuration file */
	GError 		*error 	= NULL;

	/* a location to store the path of the openned configuration file */
	gchar* gkf_path = NULL;

	/* Declaration of a pointer that will contain our configuration file*/
	GKeyFile 	*gkf 	= open_mib_configuration_file(error, &gkf_path);

	/* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
	 * declared in the configuration file
	 */
	const gchar* const* groups;

	/*
	 * the cmd line to retrieve that will be used to controle the command line
	 */
	gchar 		*camera_ctl_cmdline;

	/* build the GROUP_NAME */
	gchar *string_index;
   	if ( asprintf( &string_index , "%d", source_index) < 0 )
		return NULL;

	/* build key name from KEY_NAME_ASSIGNED_IP and if_name */
	gchar *group_name = g_strconcat (GROUP_NAME_SOURCE , string_index ,  NULL);

	/*first we load the different Groups of the configuration file
	 * second parameter "gchar* length" is optional*/
	groups =  (const gchar* const*) g_key_file_get_groups(gkf, NULL);

	/* get the commandline and its parameters */
	camera_ctl_cmdline = get_key_value_string( gkf,  groups, group_name , CAMERA_CTL_CMDLINE , error);

	close_mib_configuration_file(gkf);
	return camera_ctl_cmdline;

}

/**
 * \brief this function is used to get the information to place in the VIVOE MIB
 * from the configuration file associated to it: "vivoe-mib.conf". It will
 * get the values of the different parameters of the MIB, check their validity
 * \return FALSE if failure, TRUE if success
 */
gboolean init_mib_content(){

	/* Define the error pointer we will be using to check for errors in the configuration file */
    GError* error = NULL;
    /* Declaration of an array of gstring (gchar**) that will contain the name of the different groups
     * declared in the configuration file
     */
    gchar** groups;

	/* a location to store the path of the openned configuration file */
	gchar* gkf_path = NULL;

	/* Declaration of a pointer that will contain our configuration file*/
     gkf_conf_file =  open_mib_configuration_file(error, &gkf_path);

	/* check if the MIB groups are present if so we know that GROUP_NAME_DEVICEINFO and FROUP_NAME_CHANNELCONTROL are indeed present*/
	groups = check_mib_group(gkf_conf_file, error);

	if ( groups == NULL)
		return EXIT_FAILURE;

	/* Defined what separator will be used in the list when the parameter can have several values (for a table for example)*/
    g_key_file_set_list_separator (gkf_conf_file, (gchar) ';');

	if ( !init_deviceInfo(gkf_conf_file, GROUP_NAME_DEVICEINFO , error))
		return FALSE;
	if ( !init_channelNumber_param(gkf_conf_file, groups , error))
		return FALSE;
	if ( !init_videoFormatNumber_param(gkf_conf_file, groups, error) )
		return FALSE;
	if ( !init_ethernetIpAssignment_param(gkf_conf_file, GROUP_NAME_DEVICEINFO, error))
		return FALSE;

	/* free the memory allocated for the array of strings groups */
	 g_strfreev(groups);

	 /* initialize global parameter of the mib that value cannot be found inside conguration file */
	init_mib_global_parameter();

	/* parse gstreamer's command line to see if there will be redirection */
	init_redirection_data( gkf_conf_file );

	return TRUE;
}
