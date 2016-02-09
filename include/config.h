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

/*functions' definitions*/
int get_check_configuration(); /*check the groups, key and values of the MIB's parameters*/

#endif /* CONFIG_H */

