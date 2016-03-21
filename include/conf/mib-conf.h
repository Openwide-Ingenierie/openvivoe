/*
 * Licence: GPL
 * Created: Fri, 12 Feb 2016 12:47:49 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef MIB_CONF_H
# define MIB_CONF_H

/**
 * \brief This is the name of the configuration file to use for the subAgent handling the VIVOE MIB
 */
#define CONFIG_FILE 				"vivoe-mib.conf"

/**
 * \brief the names of the groups that should be found in the configuration file
 */
#define GROUP_NAME_DEVICEINFO		"deviceInfo"
#define GROUP_NAME_CHANNELCONTROL	"channelControl"

/**
 * \brief the names of the groups that could be found in the configuration file
 */
#define GROUP_NAME_SOURCE			"source_"
#define GROUP_NAME_RECEIVER			"receiver_"

/**
 * \brief the names of the keys that could be found in the configuration file under group [source_x]
 */
#define KEY_NAME_CHANNEL_DESC		"channelUserDesc"
/**
 * \brief the names of the keys that could be found in the configuration file under group [source_x]
 */
#define KEY_NAME_DEFAULT_IP			"defaultReceiveIP"

/**
 * \brief the name of the Gstreamer command line used by the user to give the input video to vivoe
 */
#define GST_SOURCE_CMDLINE 			"gst_source"

/*functions' definitions*/
int 	init_mib_content(); /*check the groups, key and values of the MIB's parameters*/
gchar 	*init_sources_from_conf(int index); /* get the command line used to initiate the streams */
gchar 	*get_desc_from_conf(int index); /* get the channelUserDesc associated to the stream */
gchar 	*get_default_IP_from_conf(int index); /* get the defaultReceiveIP for the defaultStartUp mode for Service User */
void 	set_default_IP_from_conf(int index, const char* new_default_ip); /* set the defaultReceiveIP for the defaultStartUp mode for Service User */
#endif /* MIB_CONF_H */

