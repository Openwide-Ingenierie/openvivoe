/*
 * Licence: GPL
 * Created: Mon, 23 May 2016 10:54:49 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef IP_ASSIGNMENT_H
#define IP_ASSIGNMENT_H


/**
 * \brief the default static IP that shoud be assgigned to a device
 */
#define DEFAULT_STATIC_IP 	"192.168.204.254"

/**
 * \brief a global variable to store the conflicting tested IP
 */
struct in_addr table_ip_tested [ RANDOM_MAX_SUFFIX - RANDOM_MIN_SUFFIX ];

gboolean assign_default_ip( const gchar *interface) ;
struct in_addr random_ip_for_conflict() ;


#endif /* IP_ASSIGNMENT_H */

