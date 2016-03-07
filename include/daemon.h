/*
 * Licence: GPL
 * Created: Tue, 09 Feb 2016 11:58:17 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef DAEMON_H
# define DAEMON_H

gboolean handle_snmp_request();
int open_vivoe_daemon (char* deamon_name);

#endif /* DEAMON_H */

