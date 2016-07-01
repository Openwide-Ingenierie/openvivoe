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

#include "mibParameters.h"
#include "log.h"
#include "deviceInfo/ethernetIfTable.h"
#include "videoFormatInfo/videoFormatTable.h"
#include "channelControl/channelTable.h"
#include "streaming/stream_registration.h"
#include "multicast.h"
#include "mibParameters.h"
#include "streaming/name.h"
#include "streaming/filter.h"
#include "streaming/detect.h"
#include "streaming/roi.h"
#include "streaming/pipeline.h"
#include "streaming/stream.h"

/**
 * \brief the J2K's media type we could have after a J2K encoder (onlu x-jpc can be linked to rtp payloader however)
 */
#define JPEG_FORMAT_NUMBER_SUPPORTED 	3
static const gchar* J2K_STR_NAMES[JPEG_FORMAT_NUMBER_SUPPORTED] = {"image/x-j2c", "image/x-jpc", "image/jp2"};


/**
 * \brief get the allowed kinds of rtpj2kpay on source pad
 * \return the caps allowed on pad of rtpj2kpay
 */
static GstCaps *get_rtpj2kpay_allowed_caps(){
	GstElement 	*rtpj2kpay 	= gst_element_factory_make_log ( "rtpj2kpay", NULL);
	GstPad 		*pad 		= gst_element_get_static_pad( rtpj2kpay , "sink" );
	GstCaps *allowed_caps 	= gst_pad_get_pad_template_caps ( pad );
	gst_object_unref (rtpj2kpay);
	gst_object_unref (pad);
	return allowed_caps;

}


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

	g_debug("addRTP: add RTP payloader to Service Provider's pipeline");

	if (caps == NULL){

		/* Media stream Type detection */
		video_caps = type_detection(GST_BIN(pipeline), input, NULL);
		if ( video_caps == NULL )
			return NULL;

		/* Fill the MIB a first Time */
		fill_entry(video_caps, video_info, stream_datas);

 	}else{

		GstElement *appsrc = gst_bin_get_by_name( GST_BIN ( pipeline ) , APPSRC_NAME ) ;
		g_object_set ( appsrc , "caps" ,  caps , NULL) ;
		video_caps = gst_caps_get_structure( caps, 0 );
		/* This is a redirection, fill the MIB once for all */
		fill_entry(video_caps, video_info, stream_datas);

	}

	/*
	 * Handle the ROI
	 */
	handle_roi ( pipeline ,  video_info , NULL ) ;

 	/* in case RAW video type has been detected */
	if ( gst_structure_has_name( video_caps, "video/x-raw") ){

		g_debug("%s video detected: add %s to SP pipeline", RAW_NAME , RTPRAWPAY_NAME);

		/* For Raw video */
		rtp 	= gst_element_factory_make_log ("rtpvrawpay", RTPRAWPAY_NAME);
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

			parser 	= gst_element_factory_make_log ("mpeg4videoparse", MPEG4PARSER_NAME );
			if ( !parser )
				return NULL;

			g_debug("%s video detected: add %s to pipeline", MPEG4_NAME , MPEG4PARSER_NAME);

			gst_bin_add(GST_BIN(pipeline),parser);

			if ( !gst_element_link_log(input, parser))
				return NULL;

			input = parser;

		}

		g_debug("%s video detected: add %s to SP pipeline", MPEG4_NAME , RTPMP4PAY_NAME );

		rtp 	= gst_element_factory_make_log ("rtpmp4vpay", RTPMP4PAY_NAME );
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

		GstElement *capsfilter = gst_element_factory_make_log("capsfilter", CAPSFITER_J2K_NAME ) ;
		GstCaps *caps_jpeg2000 = get_rtpj2kpay_allowed_caps();

		/* Put the source in the pipeline */
		g_object_set (capsfilter, "caps",caps_jpeg2000 , NULL);

		g_debug("%s video detected: add %s to SP pipeline", J2K_NAME , CAPSFITER_J2K_NAME );

		gst_bin_add(GST_BIN(pipeline),capsfilter);

		if ( !gst_element_link_log(input,capsfilter )){
			g_critical("JPEG2000 format can only be x-jpc");
			return NULL;
		}

		input = capsfilter;

		g_debug("%s video detected: add %s to SP pipeline", J2K_NAME , RTPJ2KPAY_NAME  );

		/* For J2K video */
		rtp 	= gst_element_factory_make_log ("rtpj2kpay", RTPJ2KPAY_NAME );
		if ( !rtp )
			return NULL;
	}
 	/* in case the video type detected is unknown */
	else
	{
		g_critical("unknow type of video stream");
		return NULL;
	}

	/* add rtp to pipeline */
	gst_bin_add(GST_BIN (pipeline), rtp);

	if (caps == NULL ){

		/* Filters out non VIVOE videos, and link input to RTP if video has a valid format*/
		video_caps = type_detection(GST_BIN(pipeline), input,NULL);
		if (!filter_VIVOE(video_caps,input, rtp))
			return NULL;

		/* Now that we have added the RTP payloader to the pipeline, we can get the new caps of the video stream*/
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
		if ( !video_caps )
			return NULL;

		/*Fill the MIB a second time after creating payload, this is needed to get rtp_data needed to build SDP files */
		fill_entry(video_caps, video_info, stream_datas);

	}

	/* Finally return*/
	return rtp;
}

/**
 * \brief Create a branch from input, to branch1 and branch2. In this case it is used to add a typefind element for ROI management and the udpsrc
 * \param pipeline the pipeline associated to this SP
 * \param input last element added in pipeline from which we want to branch
 * \param branch1 the next element in branch 1
 * \param branch2 the next element in branch 2
 * \return TRUE on succes, FALSE on FAILURE
 */
static gboolean create_branch_in_pipeline( GstElement *pipeline , GstElement *input , GstElement *branch1 , GstElement *branch2){

	GstPadTemplate *tee_src_pad_template;
	GstPad *tee_branch1_sink_pad, *tee_branch2_sink_pad;
	GstPad *branch1_src_pad, *branch2_src_pad;

	g_debug("create a branch in pipeline, using %s",  TEE_NAME );

	/*
	 * Create the tee element, use to create the branchs in pipeline
	 */
	GstElement *tee = gst_element_factory_make_log( "tee", TEE_NAME );

	/* add it in pipeline */
	if ( !gst_bin_add ( GST_BIN(pipeline), tee) ){
		g_critical("Failed to add %s element in pipeline", TEE_NAME );
		return FALSE;
	}
	if ( ! gst_element_link_log ( input , tee))
		return FALSE;

	/* retrieve tee source pad template (after linking it to input element*/
	tee_src_pad_template = gst_element_class_get_pad_template(GST_ELEMENT_GET_CLASS(tee), "src_%u");
	tee_branch1_sink_pad = gst_element_request_pad(tee, tee_src_pad_template, NULL, NULL );

	/* get sink pad from branch 1 */
	branch1_src_pad = gst_element_get_static_pad( branch1 , "sink");

	tee_branch2_sink_pad = gst_element_request_pad(tee, tee_src_pad_template, NULL, NULL );

	/* get sink pad from branch 2 */
	branch2_src_pad = gst_element_get_static_pad(branch2, "sink");

	if(gst_pad_link(tee_branch1_sink_pad, branch1_src_pad) != GST_PAD_LINK_OK || gst_pad_link(tee_branch2_sink_pad, branch2_src_pad) != GST_PAD_LINK_OK) {
		g_critical("%s could not be linked to %s and %s", TEE_NAME , GST_ELEMENT_NAME(branch1) ,  GST_ELEMENT_NAME(branch2) );
		return FALSE;
	}

	gst_object_unref(branch1_src_pad);
	gst_object_unref(branch2_src_pad);

	return TRUE;

}


/**
 * \brief set the paramemetrs to the element udpsink added in SP's pipeline
 * \param udpsink the udpsink element of which we need to modify the parameters
 * \param channel_entry_index the channel's index of this SP
 */
void set_udpsink_param( GstElement *udpsink, long channel_entry_index){

	/* compute IP */
	long ip 			= define_vivoe_multicast( ethernetIfTable_head , channel_entry_index);

	/* transform IP from long to char * */
	struct in_addr ip_addr;
	ip_addr.s_addr = ip;

	g_debug("set new parameters to %s ", UDPSINK_NAME );

	/* set the default multicast-interface */
	struct in_addr multicast_iface;
	multicast_iface.s_addr = ethernetIfTable_head->ethernetIfIpAddress;

	/*Set UDP sink properties */
    g_object_set(   G_OBJECT(udpsink),
					"multicast-iface", 	inet_ntoa( multicast_iface ),
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

	g_debug("add %s in Service Provider's pipeline", UDPSINK_NAME);

	/* Create the UDP sink */
    udpsink = gst_element_factory_make_log ("udpsink", UDPSINK_NAME );

	if ( !udpsink )
		return NULL;


	set_udpsink_param(udpsink, channel_entry_index);

	/* add udpsink to pipeline */
	if ( !gst_bin_add(GST_BIN (pipeline), udpsink )){
		g_critical("Unable to add %s to pipeline", gst_element_get_name(udpsink));
		return NULL;
	}

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
	GstElement 		*last, *udpsink;
	stream_data 	*data 	=  stream_datas;
	/* create the empty videoFormatTable_entry structure to intiate the MIB */
	struct videoFormatTable_entry * video_stream_info;
	video_stream_info = SNMP_MALLOC_TYPEDEF(struct videoFormatTable_entry);
	video_stream_info->videoFormatIndex = videoChannelIndex;

	g_debug("create Service Provider's pipeline");

	if( !video_stream_info){
		g_critical("Failed to create temporary empty entry for the table");
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
		g_critical("Failed to create pipeline");
		return NULL;
	}

	/*
	 * Before returning the starting the stream
	 * Add the entry to the Table, if necessary.
	 */
	long channel_entry_index = 0;
	if( initialize_videoFormat(video_stream_info, stream_datas,  &channel_entry_index)){
		g_critical("Failed to add entry in the videoFormatTable or in channelTable");
		return NULL;
	}

/*
 * This is where we add the last element of pipeline. But in order to handle properly ROI for MPEG-4 video,
 * we must give the possibility to perforpm typefind after the RTP element. A solution, is to unlink RTP and UDP element, add and perform
 * the typefing, and lik RTP and UDP again. This is not very efficient. Then, another solution is have beeen imagined: we will place a tee element
 * before the udp element. A tee has the power to split date to multiple pads. So after the type there will be two branches in our pipeline:
 * _ one with the udp element as usuall.
 * _ one with a typefind element. But this typefind, on contrary to others used should never been removed from the pipeline.
 */

	udpsink = addUDP( 	pipeline, 	bus,
			bus_watch_id, 		last,
			channel_entry_index
			);

	if (udpsink == NULL){
		g_critical("Failed to create pipeline");
		return NULL;
	}

	GstElement *typefind_roi = type_detection_element_for_roi(GST_BIN( pipeline) );

	if ( !typefind_roi ){
		g_critical("Failed to create pipeline");
		return NULL;
	}

	/*
	 * If the videoFormat is a ROI, create the branch from RTP elment, branch 1 is UDP element, branch 2 is typefind_roi element
	 * rtp and updsink will be link there.
	 * Otherwise juist link udp source to payloader.
	 *
	 */

	if ( video_stream_info->videoFormatType == roi ){

		if ( !create_branch_in_pipeline( pipeline , last , udpsink , typefind_roi ) ){
			g_critical("Failed to create pipeline");
			return NULL;
		}

	}
	else
	{
		/* we link the elements together */
		if ( !gst_element_link_log (last , udpsink))
			return NULL;
	}


	last = udpsink;

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
GstElement *append_SP_pipeline_for_redirection(GstCaps *caps, long videoFormatIndex, redirect_data *redirection ){

	GstElement 		*last;

	g_debug("append_SP_pipeline_for_redirection: append Service Provider's pipeline for source_%ld", videoFormatIndex );

	/* get the entry in videoFormatTable that corresponds to the mapping */
	struct videoFormatTable_entry* videoFormat_entry = videoFormatTable_getEntry(videoFormatIndex);

	stream_data 	*data 	=  videoFormat_entry->stream_datas;

	if ( data == NULL ){
		g_critical("Failed to append pipeline for redirection");
		return NULL;
	}

	/* Now build the corresponding pipeline according to the caps*/
	/*
	 * As it was convenient, the last element added in a redirection's service provider's pipeline has been stored in
	 * data->udp_elem, so the input element to pass to the addRTP function is the last element added in pipeline ( the bin made from the
	 * gst_source commmand line given by the user in vivoe-mib.conf configuration file */

	last = addRTP(data->pipeline , data->bus , data->bus_watch_id , data->udp_elem  , videoFormat_entry , data , caps);

	if(last == NULL){
		g_critical("Failed to append pipeline for redirection");
		return NULL;
	}

	/* get the index of the channel corresponding to the videoFormatIndex */
	struct channelTable_entry *channel_entry = channelTable_get_from_VF_index(videoFormatIndex);

	/* fill the channel Table */
	channelTable_fill_entry ( channel_entry , videoFormat_entry );

	/* Add UDP element */
	GstElement *udpsink = addUDP(  data->pipeline, 	data->bus,
			data->bus_watch_id, last,
			channel_entry->channelIndex
			);

	if (udpsink == NULL){
		g_critical("Failed to create pipeline");
		return NULL;
	}

	if ( redirection->roi_presence ){
		videoFormat_entry->videoFormatType = roi;
		GstElement *typefind_roi = type_detection_element_for_roi(GST_BIN(data->pipeline) );

		if ( !typefind_roi ){
			g_critical("Failed to create pipeline");
			return NULL;
		}

		if ( !create_branch_in_pipeline( data->pipeline , last , udpsink , typefind_roi ) ){
			g_critical("Failed to create pipeline");
			return NULL;
		}

	}
	else
	{
		if (last == NULL){
			g_critical("Failed to create pipeline");
			return NULL;
		}
		/* we link the elements together */
		if ( !gst_element_link_log (last , udpsink))
			return NULL;
	}

	last = udpsink;
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

	/* set the default multicast-interface */
	struct in_addr multicast_iface;
	multicast_iface.s_addr = ethernetIfTable_head->ethernetIfIpAddress;


	g_debug("set new parameters to %s ", UDPSRC_NAME );

	/*Set UDP sink properties */
    g_object_set(   G_OBJECT(udpsrc),
					"multicast-iface", 	inet_ntoa( multicast_iface ),
                	"address", 			inet_ntoa( multicast_group ),
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

	g_debug("add %s to Service Users's pipeline", UDPSRC_NAME );

	/* Create the UDP sink */
    udpsrc = gst_element_factory_make_log ("udpsrc", UDPSRC_NAME );

	if ( !udpsrc )
		return NULL;

	/* set the parameter of the udpsrc element */
	set_udpsrc_param( udpsrc, channel_entry, caps ) ;

	/* add rtp to pipeline */
	if ( !gst_bin_add(GST_BIN (pipeline), udpsrc )){
		g_critical("Unable to add %s to pipeline", gst_element_get_name(udpsrc));
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

	g_debug("addRTP_SU: add RTP payloader to Service Users's pipeline");

	/* Fill the MIB a first Time */
	fill_entry(video_caps, video_info, stream_datas);

	if ( gst_structure_has_field( video_caps, "encoding-name")){
		/* For Raw video */
		if ( strcmp( RAW_NAME,encoding) == 0 ){
			rtp 	= gst_element_factory_make_log ("rtpvrawdepay" , RTPRAWDEPAY_NAME );
			/* Check if everything went ok */
			g_debug("%s video dectect, add %s to SU pipeline", RAW_NAME ,  RTPRAWDEPAY_NAME );
			if( rtp == NULL)
				return NULL;

		}else if  ( strcmp( MPEG4_NAME , encoding) == 0 ){
			/* For MPEG-4 video */
			rtp 	= gst_element_factory_make_log ("rtpmp4vdepay" , RTPMP4DEPAY_NAME );
			g_debug("%s video dectect, add %s to SU pipeline", MPEG4_NAME , RTPMP4DEPAY_NAME  );
			/* Check if everything went ok */
			if( rtp == NULL)
				return NULL;

		}
		/* For J2K video */
		else if ( strcmp( J2K_NAME , encoding) == 0 ){
			rtp 	= gst_element_factory_make_log ("rtpj2kdepay" , RTPJ2KDEPAY_NAME );
			g_debug("%s video dectect, add %s to SU pipeline", J2K_NAME , RTPJ2KDEPAY_NAME  );
			/* Check if everything went ok */
			if( rtp == NULL)
				return NULL;

		}
		else {
			g_critical("unknow type of video stream");
			return NULL;
		}
	}else{
		g_critical("encoding format not found");
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
	appsource = gst_bin_get_by_name (GST_BIN (pipeline_SP), APPSRC_NAME );
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

	g_debug("handle_redirection_SU_pipeline: checks and handles Service User's pipeline for redirection");

	/* in case MPEG4 video type has been detected */
	if  (gst_structure_has_name( video_caps, "video/mpeg")){

		/* Add the MPEG-4 parser in SU pipeline */
		GstElement *parser 	= gst_element_factory_make_log ("mpeg4videoparse", MPEG4PARSER_NAME);
		if ( !parser )
			return NULL;

		g_debug("add %s to pipeline", MPEG4PARSER_NAME);

		gst_bin_add(GST_BIN(pipeline),parser);

		if ( !gst_element_link_log(input, parser))
			return NULL;

		input = parser;

	}
	/* in case J2K video type has been detected */
	else if  ( g_strv_contains ( J2K_STR_NAMES, gst_structure_get_name(video_caps))){

		GstElement *capsfilter = gst_element_factory_make_log("capsfilter", CAPSFITER_J2K_NAME );

		GstCaps *caps_jpeg2000 = get_rtpj2kpay_allowed_caps();

		/* Put the source in the pipeline */
		g_object_set (capsfilter, "caps", caps_jpeg2000 , NULL);

		g_debug("add %s to pipeline", CAPSFITER_J2K_NAME );

		gst_bin_add(GST_BIN(pipeline),capsfilter);

		if ( !gst_element_link_log(input,capsfilter )){
			g_critical("JPEG2000 format can only be %s", gst_caps_to_string( caps_jpeg2000 ) );
			return NULL;
		}

		input = capsfilter;
	}

	/*
	 * Run a new typefind to update caps in order to succeed to run a last typefind in the pipeline of the SP of the
	 * redirection. Update caps in consequence
	 */
	video_caps = type_detection(GST_BIN(pipeline), input, NULL);
	if( !video_caps )
		return NULL;

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

	g_debug("parse_conf_for_redirection: Parsing configuration file to get redirection data");

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
			g_critical("Failed to parse: %s",error->message);
			return NULL;
		}

	}else
		return input;

	if ( !bin )
		return input;
	else{
		/* add rtp to pipeline */
		if ( !gst_bin_add(GST_BIN (pipeline), bin )){
			g_critical("Unable to add %s to pipeline", gst_element_get_name(bin));
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
								redirect_data 				*redirect, 		GstCaps 	*caps,
								struct videoFormatTable_entry *video_stream_info
								){

	GError 		*error 				= NULL; /* an Object to save errors when they occurs */
	GstElement 	*sink 				= NULL; /* to return last element of pipeline */

	g_debug("addSink_SU: add sink element to Service User's pipeline");

	/*
	 * Classic case
	 */
	if ( !redirect ){

		sink  = gst_parse_bin_from_description (cmdline,
												TRUE,
												&error);
		if ( !sink )
			return NULL;

		g_object_set(sink, "name", "gst_sink", NULL);

	}
	else /* redirection case */
	{

		/* add app sink */
		sink = gst_element_factory_make_log( "appsink" , APPSINK_NAME );

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
	if ( !video_caps )
		return NULL;

	gst_caps_append_structure( caps , gst_structure_copy ( video_caps ) );
	gst_caps_remove_structure ( caps, 0 ) ;
	GstElement *last = handle_redirection_SU_pipeline(pipeline, caps, input);
	/* update input element to link to appsink */
	input = last ;

	/* Create the sink */
    if(sink == NULL){
       g_critical ( "error cannot create element for: %s","sink");
	   return NULL;
    }

	g_debug("add %s to SU pipeline", GST_ELEMENT_NAME(sink));

	/* add sink to pipeline */
	if ( !gst_bin_add(GST_BIN (pipeline), sink )){
		g_critical("Unable to add %s to pipeline", gst_element_get_name(sink));
		return NULL;
	}

	/*
	 * Then, after sink has been added, handle the ROI
	 * To do so, we need to copy to the videoFormat the value of channelRoiOrigin and channelRoiExtent parameters
	 */
	if (handle_roi ( pipeline , video_stream_info, channel_entry ) && redirect ){

			redirect->roi_presence = TRUE;

			GstElement *typefind_roi = type_detection_element_for_roi(GST_BIN( pipeline) );

			if ( !typefind_roi ){
				g_critical("Failed to create pipeline");
				return NULL;
			}

			/*
			 * If the videoFormat is a ROI, create the branch from RTP elment, branch 1 is UDP element, branch 2 is typefind_roi element
			 * rtp and updsink will be link there.
			 * Otherwise juist link udp source to payloader.
			 *
			 */

			if ( video_stream_info->videoFormatType == roi ){
				if ( !create_branch_in_pipeline( pipeline , input , sink , typefind_roi ) ){
					g_critical("Failed to create pipeline");
					return NULL;
				}

			}

	}else{

		/* we link the elements together */
		if ( !gst_element_link_log (input, sink))
			return NULL;
	}

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
		g_critical("Failed to create temporary empty entry for the table");
		return NULL;
	}

	g_debug("Create Service User's pipeline");

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

	/*
	 * Gather a maximum of information from caps into the MIB
	 */
	fill_entry ( gst_caps_get_structure ( caps , 0 ), video_stream_info, data);
	last = addSink_SU( pipeline, bus, bus_watch_id, last, channel_entry, cmdline, redirect , caps , video_stream_info);
	fill_entry ( gst_caps_get_structure ( caps , 0 ), video_stream_info, data);
	/*
	 * Fill the channel Table with parameters from the video_format_table , copy them
	 */
	channelTable_fill_entry(channel_entry, video_stream_info);

	/*
	 * As we are a Service User, we are registering the resolution of the VIDEO contained in SDP
	 * This will help us later, when the user will change the ROI or Resolution parameters of the channel
	 * to prevent from Gstreamer's errors
	 */
	g_debug("save SDP height and SDP width in channelTable_entry for ROI management");
	channel_entry->sdp_height 	= video_stream_info->videoFormatMaxVertRes;
	channel_entry->sdp_width 	= video_stream_info->videoFormatMaxHorzRes;

	return last;

}
