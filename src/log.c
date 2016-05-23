/*
 * Licence: GPL
 * Created: Mon, 04 Apr 2016 16:35:18 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#include <glib-2.0/glib.h>

#include "../include/deviceError.h"
#include <gstreamer-1.0/gst/gst.h>
#include <stdio.h>
/**
 * \brief link element, and display error message if it fails
 * \param GstElement the first element
 * \param GstElement the second element
 * \param TRUE on success, FALSE on failure
 */
gboolean gst_element_link_log(	GstElement *element1 ,  GstElement *element2 ){

		/* link element1 to element2 payloader */
	if ( !gst_element_link(element1,element2 )){
		send_deviceError_trap( );
		g_warning("failed to link %s to %s\n", GST_ELEMENT_NAME(element1), GST_ELEMENT_NAME(element2));
		return FALSE;
	}

	return TRUE;
}
/**
 * \brief link element, and display error message if it fails
 * \param GstElement the first element
 * \param GstElement the second element
 * \param TRUE on success, FALSE on failure
 */
GstElement *gst_element_factory_make_log( const gchar *element,  const gchar *name ){

	/* make an elment */
	GstElement *return_elt =	gst_element_factory_make( element , name );

	/* check if everything went ok, if not, display an error message */
	if ( !return_elt ){
		if ( element && name )
			g_warning("cannot create element %s named %s\n", element , name);
		else
			g_warning("cannot create element, NULL values given\n");
			send_deviceError_trap( );
		return NULL;
	}

	return return_elt;

}
