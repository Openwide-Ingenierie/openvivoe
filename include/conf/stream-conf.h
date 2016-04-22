/*
 * Licence: GPL
 * Created: Fri, 12 Feb 2016 12:47:57 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef STREAM_CONF_H
# define STREAM_CONF_H

/*
 * Name of the configuration file
 */
#define CONFIG_FILE_NAME "vivoe-stream.conf"

/*
 * This is a list of path where the configuration should be found
 */
#define WORKING_DIR 	"."
#define DEBUG_DIR 		WORKING_DIR"/bin/Debug/"

#define CONFIG_FILE_1 	WORKING_DIR"/conf"
#define CONFIG_FILE_2 	DEBUG_DIR"/conf"
#define CONFIG_FILE_3 	"/usr/share/vivoe/conf"
#define CONFIG_FILE_4 	"$HOME/.vivoe/conf"

#define RAW_GROUP_NAME 	"RAW"
#define RAW_GROUP_NAME 	"MPEG-4"
#define RAW_GROUP_NAME 	"JPEG2000"

GKeyFile* open_configuration_file();
void close_configuration_file(	GKeyFile* gkf);
gboolean vivoe_use_format(GKeyFile* gkf, const char* group_name);
gchar** get_raw_encoding(GKeyFile* gkf);
gchar** get_raw_res(GKeyFile* gkf);
gchar** get_mp4_res(GKeyFile* gkf);
gchar** get_j2k_res(GKeyFile* gkf);
	
#endif /* STREAM_CONF_H */
