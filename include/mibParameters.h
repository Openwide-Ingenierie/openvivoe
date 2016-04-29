/*
 * Licence: GPL
 * Created: Thu, 21 Jan 2016 18:39:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */
#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>

# ifndef MIBPARAMETERS_H
# define MIBPARAMETERS_H

/* define general parameters for the MIB_management */
#define DisplayString16 			16
#define DisplayString32 			32
#define DisplayString64 			64

/* size of IP addresses used in this MIB in bytes - 32bits/4bytes*/
#define IPV4_SIZE  					4

/* defines default parameters for multicasting*/
#define DEFAULT_MULTICAST_PORT		5004
#define DEFAULT_MULTICAST_CHANNEL	"1"
#define DEFAULT_MULTICAST_ADDR		"239.192.1.254"
#define DEFAULT_MULTICAST_IFACE		"enp2s0"

/* Definition of value to create entries in table */

/* values for deviceInfo */
#define device_SP 					1
#define device_SU 					2
#define device_both					3

/* values for devideMode */
#define normal 						1
#define defaultStartUp 				2
#define maintenanceMode 			3


/* values for videoFormatType */
#define videoChannel 				1
#define roi 						2
#define serviceUser 				3
#define rawData 					4
#define otherwise 					5

/*values for videoFormatStatus */
#define enable 						1
#define disable 					2

/*values for videoForamtInterleaced*/
/*#define vivoe_interlaced 			1
#define vivoe_progressive 			2
#define vivoe_none 					3*/
typedef enum {vivoe_interlaced = 1, vivoe_progressive,  vivoe_none }interlaced_mode;

/* values for channelStatus */
#define start 						1
#define stop 						2
#define singleFrame 				3
#define default_SAP_interval 		1000

/* value for the SAP/SDP Announcement */
#define network_type 			"IN"
#define address_type 			"IP4"
#define address_ttl 			15
#define media_type 				"video"
#define transport_proto 		"RTP/AVP"
#define sap_multi_addr 			"224.2.127.254"
#define sap_port_num 			"9875"

/* define string value for the videoFormat names */
#define RAW_NAME 				"RAW"
#define MPEG4_NAME 				"MP4V-ES"
#define J2K_NAME 				"JPEG2000"

/* removing row from table in this MIB */
#define ALLOW_REMOVING_ROW 			0
/*
 * Enum for the type of MIB's parameter
 */

typedef enum {INTEGER, STRING, T_INTEGER, T_STRING} type;

/*
 * Structure of a MIB's parameter
 */

 /*
  *  The member "_value" of the structure parameter should be an union as it can be several different type, int, int*, char*, char**
  *  When the code is extand to initialize the MIB, it is adviced to add the differents type of value needed in this union
  */

typedef union{
    int     	int_val;
    int*    	array_int_val;
    char*   	string_val;
    char**  	array_string_val;
}value;

typedef struct {
    char*   	_name; /*name of the parameter as it is stored in the vivoe-mib.conf file*/
    int     	_type; /*type of the parameter: INT or STRING*/
    int     	_optional;/*Specify of the parameter is optional or not (can be leaved empty, or should have a default value: 1 it is optional, 0 it is mand.*/
    value   	_value; /*the value of tha parameter*/
	gboolean 	_mainenance_group;
}parameter;

/*
 * Define the structure for the MIB Groups: deviceInfo, VideoControl, ChannelGroup, VivoeNotification
 */
typedef struct {
    parameter*  parameters; /*an array of the different parameter that belong to the group*/
    int         number; /*the number of parameters in that group*/
}MIB_group;

/* --------------------------------- Group DeviceInfo --------------------------------- */
/*set the declaration of the position of each parameter in the array parameters of deviceInfo MIB_group*/
#define num_DeviceDesc                  	0
#define num_DeviceManu                  	1
#define num_DevicePartNum               	2
#define num_deviceSN                    	3
#define num_DeviceHV                    	4
#define num_DeviceSWV                   	5
#define num_DeviceFV                    	6
#define num_DeviceMibV                  	7
#define num_DeviceType                  	8
#define num_DeviceUD                    	9
#define num_DeviceNato                  	10
#define num_DeviceMode                  	11
#define num_DeviceReset                 	12
#define num_ethernetInterface             	13
#define num_ethernetIFnumber 				14

/* define the number of deviceInfo's parameters in a MACRO, so it is more evolutive! */
#define DEVICEINFO_NUM_PARAM 				(num_ethernetIFnumber + 1)

MIB_group deviceInfo;
/* --------------------------------- end Group DeviceInfo --------------------------------- */

/* --------------------------------- Group videoFormatInfo -------------------------------- */
/* Initiation of videoFormatNumber to 0, it will be automatically updated each 
 * time a new videoFormat is detected on the display
 */
extern parameter videoFormatNumber; /*initialize in viodeFormatNumber.c*/
/* --------------------------------- end Group videoFormatInfo ---------------------------- */

/* --------------------------------- Group channelControl --------------------------------- */



/**
 * \brief a structure to registeer the information to send the udp datagram
 */
typedef struct {
	char 	*udp_payload;
	int 	udp_payload_length;
}sap_data;

extern parameter channelReset;
extern parameter channelNumber;

/* --------------------------------- end Group channelControl ----------------------------- */

/**
 * \brief a structure to register the information relative to redirection 
 */
typedef struct {
	long 		channel_SU_index;
	GstElement 	*pipeline_SU;
	gchar 		*gst_sink;
	long 		video_SP_index;
	GstElement 	*pipeline_SP;
	gchar 		*gst_source;
}redirect_data;

/**
 * \brief a NULL terminated array that contains all the redirection_data of the current program
 */
typedef struct {
	int 			size;
	redirect_data 	*(*redirect_channels);
}redirection_str;

extern redirection_str redirection;

/**
 * \brief a structure to save the data related to ROI
 */
typedef struct{
	long 		roi_width;
	long 		roi_height;
	long 		roi_top;
	long 		roi_left;
	long 		roi_extent_bottom;
	long 		roi_extent_right;
	gboolean 	scalable;
	int 		video_SP_index;
	gchar 		*gst_before_roi_elt;
	gchar 		*gst_after_roi_elt;
}roi_data;


/**
 * \brief a NULL terminated array that contains all the redirection_data of the current program
 */
typedef struct {
	int 			size;
	roi_data 	*(*roi_datas);
}roi_str;

extern roi_str roi_table ;


/** 
 * \brief The main loop of the program, because to be able to acces it everywhere in the program
 */
GMainLoop 	*main_loop;

/** 
 * \brief a boolean to save the fact that a Gstreamer occured before starting runnig the Main Loop (for example: v4l2src, no such device )
 */
extern gboolean 	internal_error;

/** 
 * \brief a boolean to save the fact the main loop was runnig: this is mostly to be able to perform typefind even when the MainLoop is already started
 */
extern gboolean 	was_running;
#endif /* MIBPARAMETERS_H */
