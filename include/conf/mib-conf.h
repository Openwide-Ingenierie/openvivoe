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
#define CONFIG_FILE 		"vivoe-mib.conf"

/**
 * \brief the name of the Gstreamer command line used by the user to give the input video to vivoe
 */
#define GST_SOURCE_CMDLINE 	"gst_source"

/*functions' definitions*/
int init_mib_content(); /*check the groups, key and values of the MIB's parameters*/
gchar 	*init_sources_from_conf(int index); /* get the command line used to initiate the streams */

#endif /* MIB_CONF_H */

