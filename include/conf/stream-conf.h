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

#define RAW_GROUP_NAME 		"RAW"
#define MPEG4_GROUP_NAME 	"MPEG-4"
#define J2K_GROUP_NAME 		"JPEG2000"

GKeyFile* open_configuration_file();
void close_configuration_file(	GKeyFile* gkf);
gboolean vivoe_use_format(GKeyFile* gkf, const char* group_name);
gchar** get_raw_encoding(GKeyFile* gkf);
gchar** get_raw_res(GKeyFile* gkf);
gchar** get_mp4_res(GKeyFile* gkf);
gchar** get_j2k_res(GKeyFile* gkf);

#endif /* STREAM_CONF_H */
