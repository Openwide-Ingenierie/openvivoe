/*
 * Licence: GPL
 * Created: Wed, 02 Mar 2016 09:12:37 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef SAP_H
# define SAP_H
gboolean prepare_socket(struct channelTable_entry * entry );
gboolean send_announcement(gpointer entry);

#endif /* SAP_H */
