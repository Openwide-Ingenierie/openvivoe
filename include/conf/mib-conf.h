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
 * \brief the name of the Gstreamer command line used by the user to give the input video to vivoe
 */
#define GST_SOURCE_CMDLINE 	"gst_source"

/*functions' definitions*/
int init_mib_content(); /*check the groups, key and values of the MIB's parameters*/
gchar 	*init_sources_from_conf(int index); /* get the command line used to initiate the streams */
gchar* get_desc_from_conf(int index); /* get the channelUserDesc associated to the stream */
#endif /* MIB_CONF_H */

