/*
 * Licence: GPL
 * Created: Thu, 21 Jan 2016 18:39:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */
# ifndef MIBPARAMETERS_H
# define MIBPARAMETERS_H


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
    int     int_val;
    int*    array_int_val;
    char*   string_val;
    char**  array_string_val;
}value;

typedef struct {
    char*   _name; /*name of the parameter as it is stored in the vivoe-mib.conf file*/
    int     _type; /*type of the parameter: INT or STRING*/
    int     _optional;/*Specify of the parameter is optional or not (can be leaved empty, or should have a default value: 1 it is optional, 0 it is mand.*/
    value   _value; /*the value of tha parameter*/
}parameter;

/*
 * Define the structure for the MIB Groups: deviceInfo, VideoControl, ChannelGroup, VivoeNotification
 */
typedef struct {
    parameter*  parameters; /*an array of the different parameter that belong to the group*/
    int         number; /*the number of parameters in that group*/
}MIB_group;

/* --------------------------------- Group DeviceInfo --------------------------------- */
/* define the number of deviceInfo's parameters in a MACRO, so it is more evolutive! */
#define DEVICEINFO_NUM_PARAM 				19

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
#define num_ethernetIFnumber            	10
#define num_ethernetIfSpeed             	11
#define num_ethernetIfMacAddress        	12
#define num_ethernetIfIpAddress         	13
#define num_ethernetIfSubnetMask        	14
#define num_ethernetIfIpAddressConflict 	15
#define num_DeviceNato                  	16
#define num_DeviceMode                  	17
#define num_DeviceReset                 	18

MIB_group deviceInfo;
/* --------------------------------- end Group DeviceInfo --------------------------------- */


/* --------------------------------- Group videoFormatInfo --------------------------------- */
/* define the number of videoFormatInfo's parameters in a MACRO, so it is more evolutive! */
#define VIDEOFORMAT_NUM_PARAM 				20

/*set the declaration of the position of each parameter in the array parameters of videoFormatInfo MIB_group*/
#define num_videoFormatIndex            	0
#define num_videoFormatType             	1
#define num_videoFormatStatus           	2
#define num_videoFormatBase             	3
#define num_videoFormatSampling         	4
#define num_videoFormatBitDepth         	5
#define num_videoFormatFps              	6
#define num_videoFormatColorimetry      	7
#define num_videoFormatInterlaced       	8
#define num_videoFormatCompressionFactor 	9
#define num_videoFormatCompressionRate  	10
#define num_videoFormatMaxHorzRes       	11
#define num_videoFormatMaxVertRes       	12
#define num_videoFormatRoiHorzRes       	13
#define num_videoFormatRoiVertRes       	14
#define num_videoFormatRoiOriginTop 		15
#define num_videoFormatRoiOriginLeft    	16
#define num_videoFormatRoiExtentBottom  	17
#define num_videoFormatRoiExtentRight   	18
#define num_videoFormatRtpPt            	19

MIB_group videoFormatInfo;
/* --------------------------------- end Group videoFormatInfo --------------------------------- */


#endif /* MIBPARAMETERS_H */

