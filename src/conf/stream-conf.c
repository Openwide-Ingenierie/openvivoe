/*
 * Licence: GPL
 * Created: Fri, 12 Feb 2016 12:51:27 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */

#include <stdio.h>
#include <glib-2.0/glib.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../../include/mibParameters.h"
#include "../../include/conf/stream-conf.h"


#define NUM_KEYS 	3
/*
 * This function is used to generates filter for the RTP module
 * In case the user does not want to use all video formats supported 
 * by the VIVOE norm
 */  
GKeyFile* open_configuration_file(){

/* Declaration of a pointer that will contain our configuration file*/
	GKeyFile* gkf; 

	/* Define the error pointer we will be using to check for errors in the configuration file */
	GError* error = NULL;

	/*define a pointer to the full path were the vivoe-mib.conf file is located*/
	gchar* gkf_path = NULL;

	/*defined the paths from were we should retrieve our configuration files */
	const gchar* search_dirs[] = {CONFIG_FILE_1, CONFIG_FILE_2, CONFIG_FILE_3, CONFIG_FILE_4, NULL};

	/*initialization of the variable, this perform a malloc, do not forget to free it when closing file! */
	gkf = g_key_file_new();

	/*
	 * Loads the conf file and tests that everything went OK. Problems can occur if the file doesn't exist,
	 * or if user doesn't have read permission to it.
	 * If a problem occurs, print it and exit.
	 */
	if (!g_key_file_load_from_dirs(gkf,CONFIG_FILE_NAME, search_dirs, &gkf_path,G_KEY_FILE_NONE, &error)){
		fprintf (stderr, "Could not read config file %s\n%s\nDefault settings taken for streaming\n",CONFIG_FILE_NAME,error->message);
		return NULL;
	}

	/* Defined what separator will be used in the list when the parameter can have several values (for a table for example)*/
	g_key_file_set_list_separator (gkf, (gchar) ',');
	return gkf;
}

void close_configuration_file(	GKeyFile* gkf){
	/* free the GKeyFile before leaving the function*/
	g_key_file_free (gkf);
}

static gchar** get_list_value(	GKeyFile* gkf, const gchar* key_name, const char* group_name){
	/*Declaration of a  gsize variable, that will be used to get the size of the list associated to some keys*/
	gsize length;
	/* Define the error pointer we will be using to check for errors in the configuration file */
	GError* error = NULL;
	/* return variable */
	gchar** temp ;	
	gchar** return_value = NULL;
	if(g_key_file_has_key(gkf, group_name, key_name, &error)){
		return g_key_file_get_string_list( gkf, group_name, key_name, &length, &error);
	}else
		return NULL;

}

/*
 * Specify if group name is present in configuration file
 */
gboolean vivoe_use_format(GKeyFile* gkf, const char* group_name){
	if(	!g_key_file_has_group (gkf, group_name )){
		fprintf (stderr, "%s: WARNING: VIVOE is not using %s\n",CONFIG_FILE_NAME, group_name);
		return FALSE;
	}
	else
		return TRUE;
}

gchar** get_raw_encoding(GKeyFile* gkf){
	return get_list_value(gkf, "encoding", "RAW");
}

gchar** get_raw_res(GKeyFile* gkf){
	return get_list_value(gkf, "resolution", "RAW");	
}


gchar** get_mp4_res(GKeyFile* gkf){
	return get_list_value(gkf, "resolution", "MPEG-4");	
}

gchar** get_j2k_res(GKeyFile* gkf){
	return get_list_value(gkf, "resolution", "JPEG2000");	
}

