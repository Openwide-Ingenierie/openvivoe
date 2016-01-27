/*
 * Licence: GPL
 * Created: Thu, 21 Jan 2016 10:48:55 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */
#ifndef CONFIG_H
# define CONFIG_H

/* This is the name of the configuration file to use for the subAgent
 * handling the VIVOE MIB
 */
#define CONFIG_FILE "vivoe-mib.conf"
#define VERBOSE

/*functions' definitions*/
int get_check_configuration(); /*check the groups, key and values of the MIB's parameters*/

#if 0
typedef struct groups_name{
    const char* deviceInfo;             /*name of VivoeGroups group*/
    const char* VideoFormatInfo;        /*name of VideoFormatInfo group*/
    const char* ChannelControl;         /*name of ChannelControl group*/
    const char* VivoeNotification;      /*name of VivoeNotification group*/
    const char* VivoeGroups;            /*name of VivoeGroups group*/
    int         group_count;            /*count of groups in the config files which should be registered*/
}
#endif


#endif /* CONFIG_H */

