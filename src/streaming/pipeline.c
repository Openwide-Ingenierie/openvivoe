/*
 * Licence: GPL
 * Created: Mon, 08 Feb 2016 09:32:51 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */

#include <glib-2.0/glib.h>

#include <gstreamer-1.0/gst/gst.h>
#include <gst/video/video.h>
#include <gstreamer-1.0/gst/app/gstappsrc.h>
#include <gstreamer-1.0/gst/app/gstappsink.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

#include <arpa/inet.h>

#include "../../include/mibParameters.h"
#include "../../include/log.h"
#include "../../include/videoFormatInfo/videoFormatTable.h"
#include "../../include/channelControl/channelTable.h"
#include "../../include/streaming/stream_registration.h"
#include "../../include/multicast.h"
#include "../../include/mibParameters.h"
#include "../../include/streaming/filter.h"
#include "../../include/streaming/detect.h"
#include "../../include/streaming/pipeline.h"
#include "../../include/streaming/stream.h"


#define JPEG_FORMAT_NUMBER_SUPPORTED 	3
static const gchar* J2K_STR_NAMES[JPEG_FORMAT_NUMBER_SUPPORTED] = {"image/x-j2c", "image/x-jpc", "image/jp2"};
	
/*
 * \briref This function add the RTP element to the pipeline for service provider 
 */
static GstElement* addRTP( 	GstElement *pipeline, 	GstBus 							*bus,
							guint bus_watch_id, 	GMainLoop 						*loop,
							GstElement* input, 		struct videoFormatTable_entry 	*video_info,
							gpointer stream_datas, 	GstCaps 						*caps){
	/*Create element that will be add to the pipeline */
	GstElement *rtp = NULL;
	GstElement *parser;
	GstStructure *video_caps;

	if (caps == NULL){
		/* Media stream Type detection */
		video_caps = type_detection(GST_BIN(pipeline), input, loop, NULL);

		if ( video_caps == NULL )
			return NULL;

		/* Fill the MIB a first Time */
		fill_entry(video_caps, video_info, stream_datas);

 	}else{

		GstElement *appsrc = gst_bin_get_by_name( GST_BIN ( pipeline ) , "src-redirection" ) ;
		g_object_set ( appsrc , "caps" ,  caps , NULL) ;
		video_caps = gst_caps_get_structure( caps, 0 );
		/* This is a redirection, fill the MIB once for all */
		fill_entry(video_caps, video_info, stream_datas);

	}
	
 	/* in case RAW video type has been detected */
	if ( gst_structure_has_name( video_caps, "video/x-raw") ){

		/* For Raw video */
		rtp 	= gst_element_factory_make_log ("rtpvrawpay", "rtpvrawpay");
		if ( !rtp )
			return NULL;

	}
	/* in case MPEG4 video type has been detected */
	else if  (gst_structure_has_name( video_caps, "video/mpeg")){

		/* 
		 * For MPEG-a videos we need to add a parser before the RTP payloader. However, if caps are NULL i.e. if this pipeline is a Service Provider's pipeline
		 * used for a redirection, we cannot add the mpeg4 parser here because the parser need to be in the same pipeline as the MPEG-4 encoder, otherwise the typefind
		 * cannot be performed. So if caps are NULL then it means that the parser has already been added in the SU's pipeline of the Service Users's part of teh redirection. 
		 * We do not have to add it again 
		 */
		if ( caps == NULL ){
			parser 	= gst_element_factory_make_log ("mpeg4videoparse", "parser");
			if ( !parser )
				return NULL;

			gst_bin_add(GST_BIN(pipeline),parser);

			if ( !gst_element_link_log(input, parser))
				return NULL;

			input = parser;

		}

		rtp 	= gst_element_factory_make_log ("rtpmp4vpay", "rtpmp4vpay");
		if ( !rtp )
			return NULL;

	}

	/* in case J2K video type has been detected */
	else if  ( g_strv_contains ( J2K_STR_NAMES, gst_structure_get_name(video_caps))){

		GstElement *capsfilter = gst_element_factory_make_log("capsfilter", "capsfilter-image/x-jpc");

		GstCaps *caps_jpeg2000 = gst_caps_new_empty_simple("image/x-jpc");
	
		/* Put the source in the pipeline */
		g_object_set (capsfilter, "caps",caps_jpeg2000 , NULL);

		gst_bin_add(GST_BIN(pipeline),capsfilter);

		if ( !gst_element_link_log(input,capsfilter )){
			g_printerr("JPEG2000 format can only be x-jpc\n");
			return NULL;
		}

		input = capsfilter;

		/* For J2K video */
		rtp 	= gst_element_factory_make_log ("rtpj2kpay", "rtpj2kpay");
		if ( !rtp )
			return NULL;
	}
 	/* in case the video type detected is unknown */	
	else
	{
		g_printerr("unknow type of video stream\n");
		return NULL;
	}

	/* add rtp to pipeline */
	gst_bin_add(GST_BIN (pipeline), rtp);

	if (caps == NULL ){
		
		/* Filters out non VIVOE videos, and link input to RTP if video has a valid format*/
		if (!filter_VIVOE(type_detection(GST_BIN(pipeline), input, loop, NULL),input, rtp))
			return NULL;

		/* Now that wa have added the RTP payloader to the pipeline, we can get the new caps of the video stream*/
		/* Media stream Type detection */
		video_caps = type_detection(GST_BIN(pipeline), rtp, loop, NULL);

		if ( video_caps == NULL) 
			return NULL; 

		/*Fill the MIB a second time after creating payload*/
		fill_entry(video_caps, video_info, stream_datas);
	}else{
		
		/* link input to rtp payloader */
		if ( !gst_element_link_log(input, rtp))
		   return NULL;	

		input = rtp ;
		
		video_caps = type_detection(GST_BIN(pipeline), input , loop, NULL);

		/*Fill the MIB a second time after creating payload, this is needed to get rtp_data needed to build SDP files */
		fill_entry(video_caps, video_info, stream_datas);

	}

	/* Finally return*/
	return rtp;
}
		
void set_udpsink_param( GstElement *udpsink, long channel_entry_index){
	/* compute IP */
	long ip 			= define_vivoe_multicast(deviceInfo.parameters[num_ethernetInterface]._value.array_string_val[0], channel_entry_index);

	/* transform IP from long to char * */
	struct in_addr ip_addr;
	ip_addr.s_addr = ip;

	/*Set UDP sink properties */
    g_object_set(   G_OBJECT(udpsink),
                    "host", inet_ntoa(ip_addr),
                    "port", DEFAULT_MULTICAST_PORT,
					"sync", FALSE,
                    NULL);

}

/*
 * This function add the UDP element to the pipeline
 */
static GstElement* addUDP( 	GstElement *pipeline, 	GstBus *bus,
							guint bus_watch_id, 	GMainLoop *loop,
							GstElement *input, 		long channel_entry_index){

	/*Create element that will be add to the pipeline */
	GstElement *udpsink;
		
	/* Create the UDP sink */
    udpsink = gst_element_factory_make_log ("udpsink", "udpsink");

	if ( !udpsink )
		return NULL;


	set_udpsink_param(udpsink, channel_entry_index);

	
	/* add udpsink to pipeline */
	if ( !gst_bin_add(GST_BIN (pipeline), udpsink )){
		g_printerr("Unable to add %s to pipeline", gst_element_get_name(udpsink));
		return NULL;
	}

	/* we link the elements together */
	if ( !gst_element_link_log (input, udpsink))
	    return NULL;

	return udpsink;
}

/**
 * \brief Create the pipeline, add information to MIB at the same time
 * \param pipeline the pipepline of the stream
 * \param bus the bus associated to the pipeline
 * \param bust_watch_id the watch associated to the bus
 * \param loop the GMainLoop
 * \param input the las element of the pipeline, (avenc_mp4 or capsfilter) as we built our own source
 * \param ip the ip to which send the stream on
 * \port the port to use
 * \return GstElement* the last element added to the pipeline (should be udpsink if everything went ok)
 */
GstElement* create_pipeline_videoChannel( 	gpointer stream_datas,
										 	GMainLoop *loop,
											GstElement* input,
											long videoChannelIndex){
	GstElement 		*last;
	stream_data 	*data 	=  stream_datas;
	/* create the empty videoFormatTable_entry structure to intiate the MIB */
	struct videoFormatTable_entry * video_stream_info;
	video_stream_info = SNMP_MALLOC_TYPEDEF(struct videoFormatTable_entry);
	video_stream_info->videoFormatIndex = videoChannelIndex;
	if( !video_stream_info){
		g_printerr("Failed to create temporary empty entry for the table\n");
		return NULL;
	}

	GstElement 	*pipeline 		= data->pipeline;
	GstBus 		*bus 			= data->bus;
    guint 		bus_watch_id 	= data->bus_watch_id;

   	/* Add RTP element */
 	last = addRTP( 	pipeline, 	  bus,
					bus_watch_id, loop,
					input, 		  video_stream_info,
					stream_datas, NULL);
	
	if(last == NULL){
		g_printerr("Failed to create pipeline\n");
		return NULL;
	}

	/*
	 * Before returning the starting the stream
	 * Add the entry to the Table, if necessary.
	 */
	long channel_entry_index = 0;
	if( initialize_videoFormat(video_stream_info, stream_datas,  &channel_entry_index)){
		g_printerr("Failed to add entry in the videoFormatTable or in channelTable\n");
		return NULL;
	}

	/* Add UDP element */
	last = addUDP( 	pipeline, 	  		bus,
					bus_watch_id, 		loop,
					last, 	 			channel_entry_index
				);

	if (last == NULL){
		g_printerr("Failed to create pipeline\n");
		return NULL;
	}

	data->sink = last;
	/* Before returning, free the entry created at the begging*/
	free(video_stream_info);
	return last;

}

/**
 * \brief add elements to the redirection source's pipeline, now that we know the rtp payloader to add
 * \param caps tha video caps of the stream received by the serviceUser to redirect
 * \param channel_entry the service User that receives the stream we need to redirect
 */
GstElement *append_SP_pipeline_for_redirection(GMainLoop *loop, GstCaps *caps, long videoFormatIndex ){

	GstElement 		*last;
	
	
	/* get the entry in videoFormatTable that corresponds to the mapping */
	struct videoFormatTable_entry* videoFormat_entry = videoFormatTable_getEntry(videoFormatIndex);
	
	stream_data 	*data 	=  videoFormat_entry->stream_datas;

	if ( data == NULL ){
		g_printerr("Failed to append pipeline for redirection\n");
		return NULL;
	}


	/* Now build the corresponding pipeline according to the caps*/ 
	/*
	 * As it was convenient, the last element added in a redirection's service provider's pipeline has been stored in
	 * data->sink, so the input element to pass to the addRTP function is the last element added in pipeline ( the bin made from the 
	 * gst_source commmand line given by the user in vivoe-mib.conf configuration file */

	last = addRTP(data->pipeline , data->bus , data->bus_watch_id , loop , data->sink  , videoFormat_entry , data , caps);
	if(last == NULL){
		g_printerr("Failed to append pipeline for redirection\n");
		return NULL;
	}

	/* get the index of the channel corresponding to the videoFormatIndex */
	struct channelTable_entry *channel_entry = channelTable_get_from_VF_index(videoFormatIndex);

	/* fill the channel Table */	
	channelTable_fill_entry ( channel_entry , videoFormat_entry );

	/* Add UDP element */
	last = addUDP(  data->pipeline, 	data->bus,
					data->bus_watch_id, loop,
					last, 	 			channel_entry->channelIndex
				);

	if (last == NULL){
		g_printerr("Failed to create pipeline\n");
		return NULL;
	}

	/* foe convenience sinl contained the source element "intervideosrc" but it is not it purposes */
	data->sink = last;
	channel_entry->stream_datas = data;
	return last;
}

/**
 * \brief This function add the UDP element to the pipeline / fort a ServiceUser channel
 */
static GstElement* addUDP_SU( 	GstElement *pipeline, 	GstBus *bus,
								guint bus_watch_id, 	GMainLoop *loop,
								GstCaps* caps, 			struct channelTable_entry * channel_entry){

	/*Create element that will be add to the pipeline */
	GstElement *udpsrc;
		
	/* Create the UDP sink */
    udpsrc = gst_element_factory_make_log ("udpsrc", "udpsrc");

	if ( !udpsrc )
		return NULL;

	/* get the multicast IP */
	struct in_addr multicast_group;
	multicast_group.s_addr = channel_entry->channelReceiveIpAddress;
	/*Set UDP sink properties */
    g_object_set(   G_OBJECT(udpsrc),
                	"multicast-group", 	inet_ntoa( multicast_group ),
                    "port", 			DEFAULT_MULTICAST_PORT,
					"caps", 			caps, 
                    NULL);

	/* add rtp to pipeline */
	if ( !gst_bin_add(GST_BIN (pipeline), udpsrc )){
		g_printerr("Unable to add %s to pipeline", gst_element_get_name(udpsrc));
		return NULL;
	}
	return udpsrc;
}


/**
 * \brief This function add the RTP element to the pipeline
 * \param pipeline the associated pipeline of the channel
 * \param bus the bus the channel
 * \param bus_watch_id an id watch on the bus
 * \param loop the GMainLoop
 * \param input the gstelement to link in input
 * \param video_info a videoFormatTable_entry to store detected caps 
 * \param stream_data the stream_data associated to the pipeline
 * \param caps the input video caps
 * \return GstElement the last element added in pipeline 
 */
static GstElement* addRTP_SU( 	GstElement *pipeline, 	GstBus *bus,
								guint bus_watch_id, 	GMainLoop *loop,
								GstElement* input, 		struct videoFormatTable_entry *video_info,
								gpointer stream_datas, 	GstCaps *caps){

	/*Create element that will be add to the pipeline */
	GstElement 		*rtp = NULL;
	GstElement 		*last = NULL;
	GstStructure 	*video_caps 	= gst_caps_get_structure(caps, 0);
	char 			*encoding  		= (char*) gst_structure_get_string(video_caps, "encoding-name");
	/* Fill the MIB a first Time */
	fill_entry(video_caps, video_info, stream_datas);
	if ( gst_structure_has_field( video_caps, "encoding-name")){
		/* For Raw video */		
		if ( strcmp( "RAW",encoding) == 0 ){
			rtp 	= gst_element_factory_make_log ("rtpvrawdepay", "rtpvrawdepay");
			/* Check if everything went ok */
			if( rtp == NULL)
				return NULL;

		}else if  ( strcmp( "MP4V-ES",encoding) == 0 ){
			/* For MPEG-4 video */
			rtp 	= gst_element_factory_make_log ("rtpmp4vdepay", "rtpmp4vdepay");
			/* Checek if everything went ok */
			if( rtp == NULL)
				return NULL;

		} 	
		/* For J2K video */		
		else if ( strcmp( "JPEG2000",encoding) == 0 ){
			rtp 	= gst_element_factory_make_log ("rtpj2kdepay", "rtpj2kdepay");
			/* Check if everything went ok */
			if( rtp == NULL)
				return NULL;

		}
		else {
			g_printerr("unknow type of video stream\n");
			return NULL;
		}
	}else{
		g_printerr("ERROR: encoding format not found\n");
		return NULL;
	}

	gst_bin_add(GST_BIN (pipeline), rtp);
	if ( ! gst_element_link_log ( input, rtp ))
		return NULL;
	last = rtp;

	/* Finally return*/
	return last;

}

/* 
 * \brief called when the appsink notifies us that there is a new buffer ready for processing in case of a redirection
 * \param GstElement elt the appsink element from SU pipeline
 * \param GstElement pipeline the pipeline from which we should retrieve appsrc ( SP's pipeline )
 * \return GstFlowReturn
 * */
static GstFlowReturn
	on_new_sample_from_sink (GstElement * appsink, GstElement *pipeline_SP )
{
	GstSample *sample;
	GstElement *appsource;
	GstFlowReturn ret;

	/* get the sample from appsink */
	sample = gst_app_sink_pull_sample (GST_APP_SINK (appsink));

	/* get appsource an push new sample */
	appsource = gst_bin_get_by_name (GST_BIN (pipeline_SP), "src-redirection");
	ret = gst_app_src_push_sample (GST_APP_SRC (appsource), sample); 
	gst_object_unref (appsource);

	/* we don't need the appsink sample anymore */
	gst_sample_unref (sample);

	return ret;
}

static GstElement *handle_redirection_SU_pipeline ( GstElement *pipeline, GstCaps *caps, GstElement *input){

	GstStructure *video_caps = gst_caps_get_structure( caps , 0 );

	/* in case MPEG4 video type has been detected */
	if  (gst_structure_has_name( video_caps, "video/mpeg")){

		/* Add the MPEG-4 parser in SU pipeline */
		GstElement *parser 	= gst_element_factory_make_log ("mpeg4videoparse", "parser");
		if ( !parser )
			return NULL;

		gst_bin_add(GST_BIN(pipeline),parser);

		if ( !gst_element_link_log(input, parser))
			return NULL;

		input = parser;

	}
	/* in case J2K video type has been detected */
	else if  ( g_strv_contains ( J2K_STR_NAMES, gst_structure_get_name(video_caps))){

		GstElement *capsfilter = gst_element_factory_make_log("capsfilter", "capsfilter-image/x-jpc");

		GstCaps *caps_jpeg2000 = gst_caps_new_empty_simple("image/x-jpc");
	
		/* Put the source in the pipeline */
		g_object_set (capsfilter, "caps",caps_jpeg2000 , NULL);

		gst_bin_add(GST_BIN(pipeline),capsfilter);

		if ( !gst_element_link_log(input,capsfilter )){
			g_printerr("JPEG2000 format can only be x-jpc\n");
			return NULL;
		}

		input = capsfilter;
	}

	/* 
	 * Run a new typefind to update caps in order to succeed to run a last typefind in the pipeline of the SP of the 
	 * redirection. Update caps in consequence
	 */
	video_caps = type_detection(GST_BIN(pipeline), input, main_loop, NULL);
	gst_caps_append_structure( caps , gst_structure_copy ( video_caps ) );
	gst_caps_remove_structure ( caps, 0 ) ;
	
	return input;

}


/*
 *\brief  This function add the UDP element to the pipeline / for a ServiceUser channel
 */
static GstElement* addSink_SU( 	GstElement *pipeline, 	GstBus *bus,
								guint bus_watch_id, 	GMainLoop *loop,
								GstElement *input, 		struct channelTable_entry 	*channel_entry,
								gchar *cmdline, 		redirect_data 			 	*redirect, 
								GstCaps 					*caps
								){

	GError 		*error 				= NULL; /* an Object to save errors when they occurs */
	GstElement 	*sink 				= NULL; /* to return last element of pipeline */
	GstElement 	*previous 			= NULL;
	gchar 		*previous_pipeline 	= NULL;


	/*
	 * Classic case
	 */
	if ( !redirect){

		sink  = gst_parse_bin_from_description (cmdline,
												TRUE,
												&error);
		g_object_set(sink, "name", "gst_sink", NULL);		

	}
	else /* redirection case */
	{

		/* add app sink */
		sink = gst_element_factory_make_log("appsink", "redirect-sink");
		
		if ( !sink)
			return NULL;

		/* connect a callback on each sample received */
		g_object_set (G_OBJECT (sink), "emit-signals", TRUE, "sync", TRUE, NULL);
		g_signal_connect (sink, "new-sample",
			G_CALLBACK (on_new_sample_from_sink), redirect->pipeline_SP);

		previous_pipeline = g_strrstr(cmdline, "!");

		/* check if there is another pipeline associated */
		if ( previous_pipeline ){
			gchar *to_parse = g_strndup (cmdline, strlen(cmdline) - strlen(previous_pipeline));

			previous = gst_parse_bin_from_description ( to_parse,
														TRUE,
														&error);
			free(to_parse); 

			/* add rtp to pipeline */
			if ( !gst_bin_add(GST_BIN (pipeline), previous )){
				g_printerr("Unable to add %s to pipeline", gst_element_get_name(sink));
				return NULL;
			}

			/* we link the elements together */
			if ( !gst_element_link_log (input, previous))
			    return NULL;
			
			input = previous;

		}
		
	}

	/* detect the caps of the video after the gstreamer pipeline given by the user in gst_sink command line in vivoe-mib.conf */
	GstStructure *video_caps;
	video_caps = type_detection(GST_BIN(pipeline), input, loop, NULL);
	gst_caps_append_structure( caps , gst_structure_copy ( video_caps ) );
	gst_caps_remove_structure ( caps, 0 ) ;

	GstElement *last = handle_redirection_SU_pipeline(pipeline, caps, input);
	/* update input element to link to appsink */
	input = last ;

	/* Create the sink */
    if(sink == NULL){
       g_printerr ( "error cannot create element for: %s\n","sink");
	   return NULL;
    }


#if 0

	/* add a fake rtp payloader */ 
	GstElement *rtp 	= gst_element_factory_make_log ("rtpvrawpay", "rtpvrawpay");
	gst_bin_add(GST_BIN(pipeline), rtp);
	gst_element_link(input, rtp);
	input = rtp;

	video_caps = type_detection(GST_BIN(pipeline), input, loop, NULL);
	printf("%s\n", gst_structure_to_string(video_caps));



	GstStructure *video_caps;
	video_caps = type_detection(GST_BIN(pipeline), input, loop, NULL);
	printf("%s\n", gst_structure_to_string(video_caps));

	GstElement *queue= gst_element_factory_make_log("queue", "queue");
	gst_bin_add(GST_BIN(pipeline), queue);
	gst_element_link(input, queue);
	input = queue; 

	GstElement *openjpegdec= gst_element_factory_make_log("openjpegdec", "testdec");
	gst_bin_add(GST_BIN(pipeline), openjpegdec);
	gst_element_link(input, openjpegdec);
	printf("add and link openjpegdec\n");
	
	input = openjpegdec;
	video_caps = type_detection(GST_BIN(pipeline), input , loop, NULL);
	printf("%s\n", gst_structure_to_string(video_caps));

	GstElement *queue2= gst_element_factory_make_log("queue2", "queue2");
	gst_bin_add(GST_BIN(pipeline), queue2);
	gst_element_link(input, queue2);
	input = queue2;

	/* For J2K video */
	GstElement *rtp 	= gst_element_factory_make_log ("rtpvrawpay", "rtpvrawpay");
	gst_bin_add(GST_BIN(pipeline), rtp);
	gst_element_link(input, rtp);
	input = rtp;

	video_caps = type_detection(GST_BIN(pipeline), input , loop, NULL);
	printf("%s\n", gst_structure_to_string(video_caps));

	/* For J2K video */
	GstElement *fakesink 	= gst_element_factory_make_log ("fakesink", "fakesink");
	gst_bin_add(GST_BIN(pipeline), fakesink);
	gst_element_link(input, fakesink);	
	gst_element_set_state (pipeline, GST_STATE_PLAYING);
	g_main_loop_run(main_loop);
#endif //if 0

	/* add sink to pipeline */
	if ( !gst_bin_add(GST_BIN (pipeline), sink )){
		g_printerr("Unable to add %s to pipeline", gst_element_get_name(sink));
		return NULL;
	}

	
	/* we link the elements together */
	if ( !gst_element_link_log (input, sink))
	    return NULL;

	return sink;
}

/**
 * \brief Create the pipeline, add information to MIB at the same time
 * \param pipeline the pipepline of the stream
 * \param bus the bus associated to the pipeline
 * \param bust_watch_id the watch associated to the bus
 * \param loop the GMainLoop
 * \param input the las element of the pipeline, (avenc_mp4 or capsfilter) as we built our own source
 * \param ip the ip to which send the stream on
 * \port the port to use
 * \return GstElement* the last element added to the pipeline (should be udpsink if everything went ok)
 */
GstElement* create_pipeline_serviceUser( gpointer 					stream_datas,
									 	 GMainLoop 					*loop,
										 GstCaps 					*caps,
										 struct channelTable_entry 	*channel_entry,
										 gchar 						*cmdline,
										 redirect_data 				*redirect){

	stream_data 	*data 	=  stream_datas;
	/* create the empty videoFormatTable_entry structure to intiate the MIB */
	struct videoFormatTable_entry *video_stream_info;
	video_stream_info = SNMP_MALLOC_TYPEDEF(struct videoFormatTable_entry);
	if( !video_stream_info){
		g_printerr("Failed to create temporary empty entry for the table\n");
		return NULL;
	}
	
	GstElement 	*pipeline 		= data->pipeline;
	GstBus 		*bus 			= data->bus;
    guint 		bus_watch_id 	= data->bus_watch_id;


	GstElement *first, *last;

	first =  addUDP_SU( pipeline, 		bus,
						bus_watch_id, 	loop,
						caps, 			channel_entry);

	/* check if everything went ok */
	if ( first == NULL )
		return NULL;

	/* one element in pipeline: first is last, and last is first */
	last = first;
	
	/* Add RTP depayloader element */
	last = addRTP_SU( 	pipeline, 		bus,
						bus_watch_id, 	loop,
						first, 			video_stream_info,
						data, 			caps);

	channelTable_fill_entry(channel_entry, video_stream_info);

	last = addSink_SU( pipeline, bus, bus_watch_id, loop, last, channel_entry, cmdline, redirect , caps );

	return last;
}
