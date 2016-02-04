/*
 * Licence: GPL
 * Created: Wed, 03 Feb 2016 11:47:34 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#ifndef STREAMING_H
# define STREAMING_H

/* Handle bus messages */ 
static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data);
/* Check validity of parameters passed to program */
static int check_param(int argc, char* argv[], char** ip, gint *port, char** format);

#endif /* STREAMING_H */

