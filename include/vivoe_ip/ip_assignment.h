/*
 * Licence: GPL
 * Created: Mon, 23 May 2016 10:54:49 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef IP_ASSIGNMENT_H
#define IP_ASSIGNMENT_H

gboolean set_static_ip( const gchar *interface, const gchar *ip) ;
gboolean assign_default_ip( const gchar *interface) ;
in_addr_t random_ip_for_conflict( gchar *interface) ;

#endif /* IP_ASSIGNMENT_H */

