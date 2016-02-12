/*
 * Licence: GPL
 * Created: Fri, 12 Feb 2016 12:47:49 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef MIB_CONF_H
# define MIB_CONF_H

/* This is the name of the configuration file to use for the subAgent
 * handling the VIVOE MIB
 */
#define CONFIG_FILE "vivoe-mib.conf"

/*functions' definitions*/
int get_check_configuration(); /*check the groups, key and values of the MIB's parameters*/

#endif /* MIB_CONF_H */

