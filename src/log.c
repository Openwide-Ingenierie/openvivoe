/*
 * Licence: GPL
 * Created: Mon, 04 Apr 2016 16:35:18 +0200
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */
#include <sys/types.h>
#include <unistd.h>
#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <stdio.h>
#include <string.h>
#include "../include/trap/deviceError.h"


/* For a radix of 8 we need at most 3 output bytes for 1 input
 * byte. Additionally we might need up to 2 output bytes for the
 * readix prefix and 1 byte for the trailing NULL.
 */
#define FORMAT_UNSIGNED_BUFSIZE ((GLIB_SIZEOF_LONG * 3) + 3)

static void
format_unsigned (gchar  *buf,
		 gulong  num,
		 guint   radix)
{
  gulong tmp;
  gchar c;
  gint i, n;

  /* we may not call _any_ GLib functions here (or macros like g_return_if_fail()) */

  if (radix != 8 && radix != 10 && radix != 16)
    {
      *buf = '\000';
      return;
    }

  if (!num)
    {
      *buf++ = '0';
      *buf = '\000';
      return;
    }
  if (radix == 16)
    {
      *buf++ = '0';
      *buf++ = 'x';
    }
  else if (radix == 8)
    {
      *buf++ = '0';
    }

  n = 0;
  tmp = num;
  while (tmp)
    {
      tmp /= radix;
      n++;
    }

  i = n;

  /* Again we can't use g_assert; actually this check should _never_ fail. */
  if (n > FORMAT_UNSIGNED_BUFSIZE - 3)
    {
      *buf = '\000';
      return;
    }

  while (num)
    {
      i--;
      c = (num % radix);
      if (c < 10)
	buf[i] = c + '0';
      else
	buf[i] = c + 'a' - 10;
      num /= radix;
    }

  buf[n] = '\000';
}

/* string size big enough to hold level prefix */
#define	STRING_BUFFER_SIZE	(FORMAT_UNSIGNED_BUFSIZE + 32)

#define	ALERT_LEVELS		(G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING)

/* these are emitted by the default log handler */
#define DEFAULT_LEVELS (G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING | G_LOG_LEVEL_MESSAGE)
/* these are filtered by G_MESSAGES_DEBUG by the default log handler */
#define INFO_LEVELS (G_LOG_LEVEL_INFO | G_LOG_LEVEL_DEBUG)

static FILE *
mklevel_prefix (gchar          level_prefix[STRING_BUFFER_SIZE],
		GLogLevelFlags log_level)
{
  gboolean to_stdout = TRUE;

  /* we may not call _any_ GLib functions here */

  switch (log_level & G_LOG_LEVEL_MASK)
    {
    case G_LOG_LEVEL_ERROR:
      strcpy (level_prefix, "ERROR");
      to_stdout = FALSE;
      break;
    case G_LOG_LEVEL_CRITICAL:
      strcpy (level_prefix, "CRITICAL");
      to_stdout = FALSE;
      break;
    case G_LOG_LEVEL_WARNING:
      strcpy (level_prefix, "WARNING");
      to_stdout = FALSE;
      break;
    case G_LOG_LEVEL_MESSAGE:
      strcpy (level_prefix, "Message");
      to_stdout = FALSE;
      break;
    case G_LOG_LEVEL_INFO:
      strcpy (level_prefix, "INFO");
      break;
    case G_LOG_LEVEL_DEBUG:
      strcpy (level_prefix, "DEBUG");
      break;
    default:
      if (log_level)
	{
	  strcpy (level_prefix, "LOG-");
	  format_unsigned (level_prefix + 4, log_level & G_LOG_LEVEL_MASK, 16);
	}
      else
	strcpy (level_prefix, "LOG");
      break;
    }
  if (log_level & G_LOG_FLAG_RECURSION)
    strcat (level_prefix, " (recursed)");
  if (log_level & ALERT_LEVELS)
    strcat (level_prefix, " **");

  return to_stdout ? stdout : stderr;

}

static void
write_string (FILE        *stream,
	      const gchar *string)
{
  fputs (string, stream);
}

void
log_handler (const gchar   *log_domain,
		       GLogLevelFlags log_level,
		       const gchar   *message,
		       gpointer	      unused_data)
{
	gchar level_prefix[STRING_BUFFER_SIZE], *string;
	GString *gstring;
	FILE *stream;
	const gchar *domains;

	if ((log_level & DEFAULT_LEVELS) || (log_level >> G_LOG_LEVEL_USER_SHIFT))
    	goto emit;

	domains = g_getenv ("G_MESSAGES_DEBUG");
	if (((log_level & INFO_LEVELS) == 0) ||
			domains == NULL ||
			(strcmp (domains, "all") != 0 && (!log_domain || !strstr (domains, log_domain))))
		return;

emit:
	stream = mklevel_prefix (level_prefix, log_level);

	gstring = g_string_new (NULL);
	if (!log_domain)
		g_string_append (gstring, "** ");

	if (( (log_level & G_LOG_LEVEL_MASK) ))
	{
		const gchar *prg_name = g_get_prgname ();

		if (!prg_name)
			g_string_append_printf (gstring, "(process:%lu): ", (gulong)getpid ());
		else
			g_string_append_printf (gstring, "(%s:%lu): ", prg_name, (gulong)getpid ());
	}

	if (log_domain)
	{
		g_string_append (gstring, log_domain);
		g_string_append_c (gstring, '-');
	}
	g_string_append (gstring, level_prefix);

	g_string_append (gstring, ": ");
	if (!message)
		g_string_append (gstring, "(NULL) message");
	else
		g_string_append (gstring, message);

	g_string_append (gstring, "\n");

	string = g_string_free (gstring, FALSE);

	write_string (stream, string);
	g_free (string);
}

/**
 * \brief link element, and display error message if it fails
 * \param GstElement the first element
 * \param GstElement the second element
 * \param TRUE on success, FALSE on failure
 */
gboolean gst_element_link_log(	GstElement *element1 ,  GstElement *element2 ){

		/* link element1 to element2 payloader */
	if ( !gst_element_link(element1,element2 )){
		send_deviceError_trap();
		g_critical("failed to link %s to %s", GST_ELEMENT_NAME(element1), GST_ELEMENT_NAME(element2));
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
			g_critical("cannot create element %s named %s", element , name);
		else
			g_critical("cannot create element, NULL values given");
			send_deviceError_trap( );
		return NULL;
	}

	return return_elt;

}

void init_logger(){
	g_log_set_default_handler (log_handler , NULL );
}

