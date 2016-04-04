/*
 * Licence: GPL
 * Created: Mon, 04 Apr 2016 16:35:11 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef LOG_H
# define LOG_H

gboolean gst_element_link_log(	GstElement *element1 ,  GstElement *element2 );
GstElement *gst_element_factory_make_log( const gchar *element,  const gchar *name );

#endif /* LOG_H */

