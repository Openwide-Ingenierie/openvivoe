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
#define CONFIG_FILE 					"vivoe-mib.conf"

/**
 * \brief the names of the groups that should be found in the configuration file
 */
#define GROUP_NAME_DEVICEINFO			"deviceInfo"
#define GROUP_NAME_CHANNELCONTROL		"channelControl"

/**
 * \brief the names of the groups that could be found in the configuration file
 */
#define GROUP_NAME_SOURCE				"source_"
#define GROUP_NAME_RECEIVER				"receiver_"


/**
 * \brief the names of the keys that could be found in the configuration file under group [source_x]
 */
#define KEY_NAME_CHANNEL_DESC			"channelUserDesc"

/**
 * \brief the names of the keys that could be found in the configuration file under group [source_x]
 */
#define KEY_NAME_DEFAULT_IP				"defaultReceiveIP"

/**
 * \brief the names of the keys that could be found in the configuration file under group [source_x]
 */
#define ROI_WIDTH 						"ROI_width_default"

/**
 * \brief the names of the keys that could be found in the configuration file under group [source_x]
 */
#define ROI_HEIGHT 						"ROI_height_default"

/**
 * \brief the names of the keys that could be found in the configuration file under group [source_x]
 */
#define ROI_ORIGIN_TOP 					"ROI_top_default"

/**
 * \brief the names of the keys that could be found in the configuration file under group [source_x]
 */
#define ROI_ORIGIN_LEFT 				"ROI_left_default"

/**
 * \brief the names of the keys that could be found in the configuration file under group [source_x]
 */
#define ROI_EXTENT_RIGHT 				"ROI_extentright_default"

/**
 * \brief the names of the keys that could be found in the configuration file under group [source_x]
 */
#define  ROI_EXTENT_BOTTOM				"ROI_extentbottom_default"


/**
 * \brief the name of the Gstreamer command line used by the user to give the input video to vivoe
 */
#define GST_SOURCE_CMDLINE 				"gst_source"

/**
 * \brief the name of the Gstreamer command line used by the user to output the video
 */
#define GST_SINK_CMDLINE 				"gst_sink"

/**
 * \brief the value that should have a gst_source or gst_sink command line to be a redirection
 */
#define VIVOE_REDIRECT_NAME 			"vivoe-redirect"

/**
 * \brief name of vivoe-redirect property
 */
#define VIVOE_REDIRECT_PROPERTY_NAME	"name"

/**
 * \brief the name of the Gstreamer element to use for scaling video on this device
 */
#define VIVOE_ROI_NAME 					"vivoecrop"

/**
 * \brief name of vivoe-roi property
 */
#define VIVOE_ROI_PROPERTY_NAME			"scalable"

/*functions' definitions*/
int 		init_mib_content(); /*check the groups, key and values of the MIB's parameters*/
void 		close_mib_configuration_file(GKeyFile *gkf) ; /* close the openned configuration file */
gboolean get_roi_parameters_for_sources (
		int index, 						gboolean scalable,
		long *roi_width_ptr , 			long *roi_height_ptr ,
	   	long *roi_top_ptr, 				long *roi_left_ptr,
		long *roi_extent_bottom_ptr,   	long *roi_extent_right_ptr );
gboolean get_roi_parameters_for_sink(int index , gboolean scalable,
		long *roi_width_ptr , 			long *roi_height_ptr ,
	   	long *roi_top_ptr, 				long *roi_left_ptr,
		long *roi_extent_bottom_ptr,   	long *roi_extent_right_ptr );
gchar 		*init_sources_from_conf(int index); /* get the command line used to initiate the streams */
gchar 		*init_sink_from_conf(int index); /* get the command line used to sink the streams in ServiceUsers */
gchar 		*get_desc_from_conf(int index); /* get the channelUserDesc associated to the stream */
void 		set_desc_to_conf(int index, const char* new_desc); /* save the new channelUserDesc in configuration file */
gchar 		*get_default_IP_from_conf(int index); /* get the defaultReceiveIP for the defaultStartUp mode for Service User */
void 		set_default_IP_from_conf(int index, const char* new_default_ip); /* set the defaultReceiveIP for the defaultStartUp mode for Service User */

#endif /* MIB_CONF_H */

