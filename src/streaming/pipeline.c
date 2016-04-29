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
#include "../../include/streaming/roi.h"
#include "../../include/streaming/pipeline.h"
#include "../../include/streaming/stream.h"

/**
 * \brief the J2K's media type we could have after a J2K encoder (onlu x-jpc can be linked to rtp payloader however)
 */
#define JPEG_FORMAT_NUMBER_SUPPORTED 	3
static const gchar* J2K_STR_NAMES[JPEG_FORMAT_NUMBER_SUPPORTED] = {"image/x-j2c", "image/x-jpc", "image/jp2"};
	
/**
 * \brief This function add the RTP element to the pipeline for service provider * \param pipeline the pipeline associated to this SP
 * \param bus the bus the channel
 * \param bus_watch_id an id watch on the bus
 * \param input last element added in pipeline to which we should link elements added
 * \param video_info a "fake" entry to VFT into which we will save all element we can retrieve from the video caps of the stream (via fill_entry() )
 * \param stream_data the data associated to this SP's pipeline
 * \param caps the caps of the input stream if this is a redirection, NULL otherwise
 * \param channel_entry_index the channel's index of this SP: to build the multicast address
 * \return the last element added in pipeline (rtp payloader if everything goes ok)
 */
static GstElement* addRTP( 	GstElement 						*pipeline, 		GstBus *bus,
							guint 							bus_watch_id, 	GstElement* input,
							struct videoFormatTable_entry 	*video_info,	gpointer stream_datas, 	
							GstCaps 						*caps){

	/*Create element that will be add to the pipeline */
	GstElement *rtp = NULL;
	GstElement *parser;
	GstStructure *video_caps;

	if (caps == NULL){
		/* Media stream Type detection */
		video_caps = type_detection(GST_BIN(pipeline), input, NULL);

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

	input = handle_roi ( pipeline ,  input , video_info ,  video_caps ) ;
	if ( !input)
		return NULL; 

	video_caps = type_detection(GST_BIN(pipeline), input, NULL);

	if ( !input )
		return NULL;

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

		/*
		 * For J2K video our RTP payloader can only accept image/x-jpc input video media type. However, not all encoders have this caos on their src pads.
		 * So we first link the output of the encoder to a capsfilter with image/x-jpc media type. If the encoder and the capfsilter cannot negociate caps, then the encoder
		 * cannot be link to the RTP payloader, so we stop there. As an example: avenc_jpeg2000 only has image/x-j2c as a media type. But openjpegenc has image/x-j2c, image/-xjpc
		 * and image-j2p
		 */

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
		if (!filter_VIVOE(type_detection(GST_BIN(pipeline), input,NULL),input, rtp))
			return NULL;

		/* Now that wa have added the RTP payloader to the pipeline, we can get the new caps of the video stream*/
		/* Media stream Type detection */
		video_caps = type_detection(GST_BIN(pipeline), rtp, NULL);

		if ( video_caps == NULL) 
			return NULL; 

		/*Fill the MIB a second time after creating payload*/
		fill_entry(video_caps, video_info, stream_datas);
	}else{
		
		/* link input to rtp payloader */
		if ( !gst_element_link_log(input, rtp))
		   return NULL;	

		input = rtp ;
		
		video_caps = type_detection(GST_BIN(pipeline), input ,NULL);

		/*Fill the MIB a second time after creating payload, this is needed to get rtp_data needed to build SDP files */
		fill_entry(video_caps, video_info, stream_datas);

	}

	/* Finally return*/
	return rtp;
}

/**
 * \brief set the paramemetrs to the element udpsink added in SP's pipeline
 * \param udpsink the udpsink element of which we need to modify the parameters
 * \param channel_entry_index the channel's index of this SP
 */
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

/**
 * \brief This function add the UDP element (udpsink) to the Service Provider's pipeline * \param pipeline the associated pipeline of the channel
 * \param pipeline the pipeline associated to this SP
 * \param bus the bus the channel
 * \param bus_watch_id an id watch on the bus
 * \param input last element added in pipeline to which we should link elements added
 * \param channel_entry_index the channel's index of this SP: to build the multicast address
 * \return last element added in pipeline, so udpsink if everything goes well
 */
static GstElement* addUDP( 	GstElement *pipeline, 	GstBus *bus,
							guint bus_watch_id, 	GstElement *input, 		
							long channel_entry_index){

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
 * \brief Create the pipeline for an SP (fill the MIB at the same time)
 * \param stream_data the stream_data associated to the pipeline of this SP
 * \param input last element added in pipeline to which we should link elements added
 * \param videoChannelIndex the index of the channel associated to this SP
 * \return GstElement* the last element added to the pipeline (should be udpsink if everything went ok)
 */
GstElement* create_pipeline_videoChannel( 	gpointer stream_datas,
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
 	last = addRTP( 	pipeline, 	  		bus,
					bus_watch_id, 		input,
		  			video_stream_info, 	stream_datas, 
					NULL);
	
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
					bus_watch_id, 		last, 
					channel_entry_index
				);

	if (last == NULL){
		g_printerr("Failed to create pipeline\n");
		return NULL;
	}

	data->udp_elem = last;
	/* Before returning, free the entry created at the begging*/
	free(video_stream_info);
	return last;

}

/**
 * \brief add elements to the redirection source's pipeline, now that we know the rtp payloader to add
 * \param caps tha video caps of the stream received by the serviceUser to redirect, ( the caps detected before the appsink)
 * \param videoFormatIndex the VF of the redirection's SP
 * \return last element added in pipeline, so udpsink if everything goes well
 */
GstElement *append_SP_pipeline_for_redirection(GstCaps *caps, long videoFormatIndex ){

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
	 * data->udp_elem, so the input element to pass to the addRTP function is the last element added in pipeline ( the bin made from the 
	 * gst_source commmand line given by the user in vivoe-mib.conf configuration file */

	last = addRTP(data->pipeline , data->bus , data->bus_watch_id , data->udp_elem  , videoFormat_entry , data , caps);
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
					data->bus_watch_id, last, 	 		
					channel_entry->channelIndex
				);

	if (last == NULL){
		g_printerr("Failed to create pipeline\n");
		return NULL;
	}

	/* foe convenience sinl contained the source element "intervideosrc" but it is not it purposes */
	data->udp_elem = last;
	channel_entry->stream_datas = data;
	return last;
}

/**
 * \brief set the paramemetrs to the element udpsink added in SP's pipeline
 * \param udpsrc the udpsrc element of which we need to modify the parameters
 * \param channel_entry_index the channel's index of this SP
 */
void set_udpsrc_param( GstElement *udpsrc, struct channelTable_entry * channel_entry, GstCaps* caps ){

	/* get the multicast IP */
	struct in_addr multicast_group;
	multicast_group.s_addr = channel_entry->channelReceiveIpAddress;

	/*Set UDP sink properties */
    g_object_set(   G_OBJECT(udpsrc),
                	"multicast-group", 	inet_ntoa( multicast_group ),
                    "port", 			DEFAULT_MULTICAST_PORT,
					"caps", 			caps, 
                    NULL);

}

/**
 * \brief This function add the UDP element (udpsrc) to the pipeline / fort a ServiceUser channel
 * \param pipeline the associated pipeline of the channel
 * \param bus the bus the channel
 * \param bus_watch_id an id watch on the bus
 * \param caps the input video caps built from SDP file, and to give to upsrc
 * \param channel_entry the channel of the SU in the device's channel Table
 * \return last element added in pipeline (should be udpsrc normally)
 */
static GstElement* addUDP_SU( 	GstElement *pipeline, 	GstBus *bus,
								guint bus_watch_id, 	GstCaps* caps,
								struct channelTable_entry * channel_entry){

	/*Create element that will be add to the pipeline */
	GstElement *udpsrc;
		
	/* Create the UDP sink */
    udpsrc = gst_element_factory_make_log ("udpsrc", "udpsrc");

	if ( !udpsrc )
		return NULL;

	/* set the parameter of the udpsrc element */
	set_udpsrc_param( udpsrc, channel_entry, caps ) ;

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
 * \param input the gstelement to link in input
 * \param video_info a videoFormatTable_entry to store detected caps 
 * \param stream_data the stream_data associated to the pipeline
 * \param caps the input video caps
 * \return GstElement the last element added in pipeline 
 */
static GstElement* addRTP_SU( 	GstElement *pipeline, 						GstBus *bus,
								guint bus_watch_id,							GstElement* input,
								struct videoFormatTable_entry *video_info, 	gpointer stream_datas,
								GstCaps *caps){

	/*Create element that will be add to the pipeline */
	GstElement 		*rtp = NULL;
	GstElement 		*last = NULL;
	GstStructure 	*video_caps 	= gst_caps_get_structure(caps, 0);
	char 			*encoding  		= (char*) gst_structure_get_string(video_caps, "encoding-name");

	/* Fill the MIB a first Time */
	fill_entry(video_caps, video_info, stream_datas);

	if ( gst_structure_has_field( video_caps, "encoding-name")){
		/* For Raw video */		
		if ( strcmp( RAW_NAME,encoding) == 0 ){
			rtp 	= gst_element_factory_make_log ("rtpvrawdepay", "rtpvrawdepay");
			/* Check if everything went ok */
			if( rtp == NULL)
				return NULL;

		}else if  ( strcmp( MPEG4_NAME , encoding) == 0 ){
			/* For MPEG-4 video */
			rtp 	= gst_element_factory_make_log ("rtpmp4vdepay", "rtpmp4vdepay");
			/* Checek if everything went ok */
			if( rtp == NULL)
				return NULL;

		} 	
		/* For J2K video */		
		else if ( strcmp( J2K_NAME , encoding) == 0 ){
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

/*
 * \brief Build the sink of SU's pipeline, if this is a reidrection, modify the caps to pass it to the SP pipeline
 * \param pipeline the pipepline of the stream
 * \param input the las element of the pipeline, (avenc_mp4 or capsfilter) as we built our own source
 * \param caps the caps built from the SDP to replace if you have MPEG-4 or J2K video
 * \return GstElement* the last element added to the pipeline (RAW: return input, MPEG: mpeg4videoparse, J2k:capsfilter)
 */
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
	video_caps = type_detection(GST_BIN(pipeline), input, NULL);
	gst_caps_append_structure( caps , gst_structure_copy ( video_caps ) );
	gst_caps_remove_structure ( caps, 0 ) ;
	
	return input;

}
/**
 * \brief For redirection: build a gstreamer's bin from the pipeline in SU's gst_sink cmd line and SP's gst_source cmd line
 * \param pipeline the pipeline associated to this redirection
 * \param redirect the redirection data associated to this redirection
 * \param input the last element of the pipeline, (avenc_mp4 or capsfilter) as we built our own source
 * \return the last element added in pipeline, should be the created bin or input
 */
static GstElement *parse_conf_for_redirection( GstElement *pipeline, GstElement *input, redirect_data *redirect){
	
	gchar *full_pipeline = NULL;
	gchar *sink_suffix = g_strrstr(redirect->gst_sink, "!");
	gchar *sink_remaning_pipeline = NULL;

	if ( sink_suffix ) 
		sink_remaning_pipeline 	= g_strndup (redirect->gst_sink , strlen(redirect->gst_sink) - strlen(g_strrstr(redirect->gst_sink, "!")));

	gchar *source_remaining_pipeline 	= strchr(redirect->gst_source, '!');
	GstElement *bin;
	GError *error = NULL;

	/* 
	 * Concatenate gst_sink to gst_source to simulate a gstreamer pipeline (add + 1 to source_remaining_pipeline to delete the remaining '!'
	 */
	/* if there is no remaining pipeline neither for source and sink */
	if ( !sink_remaning_pipeline && !source_remaining_pipeline)
		return input;
	/*  if there is no remaining pipeline for sink but there is one for source*/
	else if ( !sink_remaning_pipeline )
		full_pipeline = source_remaining_pipeline + 1; /* to delete the ! remaining at the begining of the string */
	/* if there is no remaining pipeline for source but there is one for sink */
	else if ( !source_remaining_pipeline ){
		full_pipeline = sink_remaning_pipeline;
	}
	/* if there are remaining pipelines for both */	
	else
		full_pipeline =  g_strconcat(sink_remaning_pipeline, source_remaining_pipeline , NULL) ; 

	if ( full_pipeline != NULL  ) {
		bin = gst_parse_bin_from_description ( full_pipeline,
				TRUE,
				&error);

		if ( error != NULL){
			g_printerr("Failed to parse: %s\n",error->message);
			return NULL;	
		}

	}else
		return input;

	if ( !bin )
		return input;
	else{
		/* add rtp to pipeline */
		if ( !gst_bin_add(GST_BIN (pipeline), bin )){
			g_printerr("Unable to add %s to pipeline", gst_element_get_name(bin));
			return NULL;
		}
		/* we link the elements together */
		if ( !gst_element_link_log (input, bin)){
			gst_bin_remove( GST_BIN (pipeline), bin);
		    return NULL;
		}

		return bin;

	}

}

/**
 * \brief Build the sink of SU's pipeline, if this is a reidrection, modify the caps to pass it to the SP pipeline
 * \param pipeline the pipepline of the stream
 * \param bus the bus associated to the pipeline
 * \param bust_watch_id the watch associated to the bus
 * \param loop the GMainLoop
 * \param input the last element of the pipeline, (avenc_mp4 or capsfilter) as we built our own source
 * \param channel_entry the channel of the SU in the device's channel Table
 * \param cmdline teh gst_sink from the configuration vivoe-mib.conf
 * \param redirect the redirection data (mapping SU's channel Index --> SP's videoFormatIndex)
 * \param caps the caps built from the SDP, may be replace if this is a redirection
 * \return GstElement* the last element added to the pipeline (appsink or a displayer like x(v)imagesink)
 */
static GstElement* addSink_SU( 	GstElement 					*pipeline, 		GstBus 		*bus,
								guint 						bus_watch_id, 	GstElement 	*input, 	
								struct channelTable_entry 	*channel_entry, gchar 		*cmdline, 
								redirect_data 				*redirect, 		GstCaps 	*caps
								){

	GError 		*error 				= NULL; /* an Object to save errors when they occurs */
	GstElement 	*sink 				= NULL; /* to return last element of pipeline */

	/*
	 * Classic case
	 */
	if ( !redirect ){

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

		input = parse_conf_for_redirection( pipeline, input, redirect);

		if ( !input )
			return NULL;

	}

	/* detect the caps of the video after the gstreamer pipeline given by the user in gst_sink command line in vivoe-mib.conf */
	GstStructure *video_caps;
	video_caps = type_detection(GST_BIN(pipeline), input, NULL);
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
 * \param input the las element of the pipeline, (avenc_mp4 or capsfilter) as we built our own source
 * \param ip the ip to which send the stream on
 * \port the port to use
 * \return GstElement* the last element added to the pipeline
 */
GstElement* create_pipeline_serviceUser( gpointer 					stream_datas,
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
						bus_watch_id, 	caps, 
						channel_entry);

	data->udp_elem = first;

	/* check if everything went ok */
	if ( first == NULL )
		return NULL;

	/* one element in pipeline: first is last, and last is first */
	last = first;
	
	/* Add RTP depayloader element */
	last = addRTP_SU( 	pipeline, 			bus,
						bus_watch_id,  		first,
						video_stream_info,	data, 	
						caps);

	channelTable_fill_entry(channel_entry, video_stream_info);

	last = addSink_SU( pipeline, bus, bus_watch_id, last, channel_entry, cmdline, redirect , caps );

	return last;

}
