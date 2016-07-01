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
#include "mibParameters.h"
#include "conf/stream-conf.h"

/**
 * \brief the number of video formats supported in VIVOE ( for now 3: RAW, MPEG-4, JPEG2000)
 */
#define NUM_KEYS 	3

/*
 * \brief open the configuration GKeyfile
 * \return A pointer to the openned GKeyfile
 */
GKeyFile* open_configuration_file(){

/* Declaration of a pointer that will contain our configuration file*/
	GKeyFile* gkf;

	/* Define the error pointer we will be using to check for errors in the configuration file */
	GError* error = NULL;

	/*define a pointer to the full path were the vivoe-mib.conf file is located*/
	gchar* gkf_path = NULL;

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

/*
 * \brief close the configuration GKeyfile
 * \param A pointer to the openned GKeyfile
 */
void close_configuration_file(	GKeyFile* gkf){
	/* free the GKeyFile before leaving the function*/
	g_key_file_free (gkf);
}

/**
 * \brief get the expected list of values of a Key
 * \param a pointer to the openned GKeyfile
 * \param key_name the name of the key
 * \param group_name the group to which the key belong
 * \return the list as an array of strings values, each element is a value
 */
static gchar** get_list_value(	GKeyFile* gkf, const gchar* key_name, const char* group_name ){
	
	gchar **list_returned;

	/*Declaration of a  gsize variable, that will be used to get the size of the list associated to some keys*/
	gsize length;
	/* Define the error pointer we will be using to check for errors in the configuration file */
	GError* error = NULL;
	/* return variable */
	if(g_key_file_has_key(gkf, group_name, key_name, &error)){
		list_returned = g_key_file_get_string_list( gkf, group_name, key_name, &length, &error);
	}else
		return NULL;

	/* 
	 * Now we remove all whitspace from the list: indeed a user could have left a space at the end pf the line
	 * or separate each delimiter of the list with spaces
	 */
	int k = 0; 
	for ( int i = 0 ; i < length; i ++ ){
		for ( int j = 0 ; j < strlen(list_returned[i]) + 1; j++){
			if (list_returned[i][j] != ' ')  
				list_returned[i][k++] = list_returned[i][j];  
		}
	}

	return list_returned;

}

/**
 * \brief specify if group name is present in configuration file
 * \param gkf the GKeyfile openned
 * \param group_name the name of the group to check for its presence
 * \return TRUE if the group is found, FALSE otherwise
 */
gboolean vivoe_use_format(GKeyFile* gkf, const char* group_name){
	if(	!g_key_file_has_group (gkf, group_name )){
		fprintf (stderr, "%s: WARNING: VIVOE is not using %s\n",CONFIG_FILE_NAME, group_name);
		return FALSE;
	}
	else
		return TRUE;
}

/**
 * \brief get the list  of encodings to use for RAW video
 * \param gkf the GKeyfile openned
 * \return the encodings to use in a string array
 */
gchar** get_raw_encoding(GKeyFile* gkf){
	return get_list_value(gkf, "encoding",  RAW_GROUP_NAME ) ;
}

/**
 * \brief get the list  of resolutions to use for RAW video
 * \param gkf the GKeyfile openned
 * \return the resolutions to use in a string array
 */
gchar** get_raw_res(GKeyFile* gkf){
	return get_list_value(gkf, "resolution", RAW_GROUP_NAME );
}

/**
 * \brief get the list  of resolutions to use for MPEG-4 video
 * \param gkf the GKeyfile openned
 * \return the resolutions to use in a string array
 */
gchar** get_mp4_res(GKeyFile* gkf){
	return get_list_value(gkf, "resolution", MPEG4_GROUP_NAME ); 
}

/**
 * \brief get the list  of resolutions to use for JPEG2000 video
 * \param gkf the GKeyfile openned
 * \return the resolutions to use in a string array
 */
gchar** get_j2k_res(GKeyFile* gkf){
	return get_list_value(gkf, "resolution", J2K_GROUP_NAME);	
}
