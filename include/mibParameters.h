/*
 * Licence: GPL
 * Created: Thu, 21 Jan 2016 18:39:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */
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
#define vivoe_interlaced 			1
#define vivoe_progressive 			2
#define vivoe_none 					3

/* values for channelStatus */
#define start 						1
#define stop 						2
#define singleFrame 				3

/* removing row from table in this MIB */
#define ALLOW_REMOVING_ROW 			0


/**
 * \brief Enum for the type of MIB's parameter
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

extern parameter channelReset;
extern parameter channelNumber;

/* --------------------------------- end Group channelControl ----------------------------- */


#endif /* MIBPARAMETERS_H */

