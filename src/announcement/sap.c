/*
 * Licence: GPL
 * Created: Wed, 02 Mar 2016 09:12:58 +0100
 * Main authors:
 *     - hoel <hoel.vasseur@openwide.fr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <glib-2.0/glib.h>
#include <gstreamer-1.0/gst/gst.h>
#include <gstreamer-1.0/gst/sdp/gstsdpmessage.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>

/* Header file */
#include "../../include/mibParameters.h"
#include "../../include/deviceInfo/ethernetIfTable.h"
#include "../../include/channelControl/channelTable.h"
#include "../../include/announcement/sap.h"
#include "../../include/announcement/sdp.h"
#include "../../include/streaming/stream_registration.h"
#include "../../include/streaming/stream.h"

/*
 * BYTE 0:
 * 	Version Number (V): 3 bits
		Specifies the SAP version. This field shall be set to 1.
	Addsap_multicast_addrs Type (A): 1 bit
		This bit shall be set to 0 to indicate that 32-bit IPv4 addsap_multicast_addrses are used.
	Reserved (R): 1 bit
		This bit shall be set to 0.
	Message Type (T): 1 bit
		This bit shall be set to 0 for session announcements and 1 for session deletions.
	Encryption (E): 1 bit
		SDP payload encryption shall not be used. This bit shall therefore be set to 0 for an unencrypted payload.
	Compsap_multicast_addrsed (C): 1 bit
		SDP payload compsap_multicast_addrsion shall not be used. This bit shall therefore be set to 0 for an uncompsap_multicast_addrsed payload
	So te binary version is:
	_ 00100000 thus 0x20 for announcement
	_ 00100000 thus 0x24 for announcement

*/
#define SAP_header_announcement 	0x20
#define SAP_header_deletion 		0x24
/*
 * BYTE 1:
 * Authentication Length: 8 bits
 *  This shall be set to 0 in VIVOE as no authentication data is psap_multicast_addrent
 */
#define SAP_header_AL 				0x00

/*
 * BYTE 9 and more:
 * Optional Payload Type:
 * This is a ASCII text string followed by null character.
 * this shall be set to "application/sdp"
 */
#define SAP_header_OPT 				"application/sdp"

/*
 * After the really hard and scholar computation I have deduced that the total
 * size of the header of the SAP is 24 bytes:
 * byte 0 as described bedfore 	--> 	1 byte
 * Authentication Length: 		--> + 	1 byte
 * Message Identifier Hash: 	--> + 	2 bytes
 * Operating Source: 			--> + 	4 bytes
 * Optional Payload Type: 		--> + 	16 bytes
 * 									_____________
 * Total: 								24 bytes
 */
#define SAP_header_size 			24


/**
 * \brief a global struct that represent the socket address on which to sent SAP/SDP announcement messages
 */
struct {
	struct sockaddr_in 	multicast_addr;
	int 				udp_socket_fd;
	int 				udp_socket_fd_rec;	
}sap_socket;

/**
 * \brief inits the mulcast_addr parameter
 */
void init_sap_multicast(){
	int status = -1;
	memset(&sap_socket.multicast_addr, 0, sizeof(struct sockaddr_in));
	sap_socket.multicast_addr.sin_family 	= AF_INET;
	sap_socket.multicast_addr.sin_port 		= htons(9875);
	inet_aton("224.2.127.254", &sap_socket.multicast_addr.sin_addr);
	
	/* open UDP socket */
	int udp_socket_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if ( udp_socket_fd == -1 ) {
		g_printerr(" Failed to create UDP socket to send SAP/SDP announcement: %s\n",strerror(errno));
	}
	/* save value of socket into global structure */
	sap_socket.udp_socket_fd = udp_socket_fd;
	
	/* open UDP socket */
	udp_socket_fd = socket(AF_INET,SOCK_DGRAM  , IPPROTO_UDP);
	fcntl(udp_socket_fd, F_SETFL, O_NONBLOCK);
	if ( udp_socket_fd == -1 ) {
		g_printerr(" Failed to create UDP socket to send SAP/SDP announcement: %s\n",strerror(errno));
	}
	/* save value of socket into global structure */
	sap_socket.udp_socket_fd_rec = udp_socket_fd;

	/* define a TTL of one, this will restrict the Multicast datagram to be restricted to the same subnet, which we want for VIVOE */
	uint8_t ttl = 1;
	setsockopt(sap_socket.udp_socket_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

	status = bind( 	sap_socket.udp_socket_fd_rec,
		   			(struct sockaddr *)&(sap_socket.multicast_addr),
					//sizeof(sap_socket.multicast_addr) );
					sizeof( struct sockaddr_in));
	
	/* check for binding errors */
	if ( status < 0 )
		g_printerr("Failed to bind socket: %s\n", strerror(errno));
	struct ip_mreq 		imreq;
	/* set content of struct saddr and imreq to zero */
	memset(&imreq, 0, sizeof(struct ip_mreq));
	imreq.imr_multiaddr.s_addr = inet_addr(sap_multi_addr); /* multicast group to join*/
	imreq.imr_interface.s_addr = ethernetIfTable_head->ethernetIfIpAddress; /* use DEFAULT interface */
	/* JOIN multicast group on default interface - pass the ip_multicast_request to kernel */
	status = setsockopt(sap_socket.udp_socket_fd_rec,
						IPPROTO_IP,
						IP_ADD_MEMBERSHIP,
						(const void *)&imreq,
						sizeof(struct ip_mreq) );
}


/**
 * \brief 	Build the first byte corsap_multicast_addrponding to the Message Identifier Hash (MAI) according to the norm
 * \return 	the 16 bits corsap_multicast_addrponding to the Message Identifier Hash field of SAP header
 */
static const uint16_t build_MAI(int session_version){
	/*
	 * the Message Identifier Hash should be derived from the session version number of the SDP message
	 * As the Session Version is a 32-bits integer and the MAI is only a 16-bits field, we have to take only
	 * a part of the Session Version, but has the session version shall be increased by 1 each time a new SDP
	 * message is created, we should reverberate this on the MAI. So we will take only the 16 LSB of the session version
	 */
	return (char) (session_version & 0x00FF);
}

/**
 * \brief 	Build the 4 bytes of SAP header corsap_multicast_addrponding to Operating Source field
 * 			This should contained the IP source of the Service Provider
 * \return 	in_addr_t (uint32_t) the 32 bits corsap_multicast_addrponding to the IP of the Service Provider
 */
static const in_addr_t build_OS(void){
	/*
	 * VIVOE norm specifies that the field Operating Source of the SAP header should contain the IP of the Service Provider
	 * This IP is the IP of the Primary interface that we retrieve from the ethernetIfTable
	 */

	/* Primary interface is the head of the list of entries of ethernetIfTable */
	return ethernetIfTable_head->ethernetIfIpAddress;
}

/**
 * \brief 	build the SAP header of datagram, as defined by the VIVOE norme
 * \param  	session_version the number of session_version to use to create the Message Identifier Hash
 * \return 	char* an array of byte corsap_multicast_addrponding to the built header
 */
static gboolean build_SAP_header(char *header, int session_version, gboolean deletion){
	char returnv[SAP_header_size];
	int offset 						= 0; /* an offset use to concatenate the arrays */
	
	if(deletion)
		returnv[0] 	= SAP_header_deletion;
	else
		returnv[0] 	= SAP_header_announcement;
	offset++;
	returnv[1] 		= SAP_header_AL;
	offset++;
	uint16_t mhi 	= build_MAI(session_version);
	memcpy(returnv+offset, &mhi, sizeof(uint16_t));
	offset += sizeof(uint16_t);
	in_addr_t  operatig_source  = build_OS();
	memcpy(returnv+offset, &operatig_source, sizeof(uint32_t));
	offset += sizeof(uint32_t);
	memcpy(returnv+offset, SAP_header_OPT , sizeof(SAP_header_OPT)/sizeof(char) );
	offset += sizeof(SAP_header_OPT)/sizeof(char);
	memcpy( header,returnv,SAP_header_size );
	return TRUE;
}

/**
 * \brief creates SAP message: header + payload returned in a buffer
 * \param channelTable_entry * entry the entry in the channelTable associated to the socket
 * \param sap_msg_length a location to save the lenght of the SAP message
 * \param gboolean deletion specify if we should build the SAP deletion message
 * \return char* buffer that contains the SAP message (the UDP payload)
 */
static char*  build_SAP_msg(struct channelTable_entry * entry, int *sap_msg_length, gboolean deletion){

	GstSDPMessage *msg;
	/* create a new SDP message */
	if (gst_sdp_message_new(&msg)){
		g_printerr("Failed to create SDP message\n");
		return FALSE;
	}
	create_SDP(msg, entry);

	/* extract the randomly generated session_version integer */
	const GstSDPOrigin 	*origin 			= gst_sdp_message_get_origin (msg);
	int 				session_version 	= htonl(atoi(origin->sess_version));

	/* Build the  header */
	char header[SAP_header_size];
   	build_SAP_header(header, session_version, deletion);

	/* Build the *payload */
	char *payload = g_strdup (gst_sdp_message_as_text(msg));

	/* concat header and payload */
	int 	sap_msg_size 	= SAP_header_size+strlen(payload);
	char 	*sap_msg 		= (char*) malloc(sap_msg_size*sizeof(char));
	memcpy(sap_msg, header, SAP_header_size);
	memcpy(sap_msg+SAP_header_size, payload, strlen(payload));
	*sap_msg_length = sap_msg_size; // save the length if the message into sap_msg_length
	free(payload);
	return sap_msg;
}

/**
 * \brief get SDP message from payload
 * \param udp_payload the datagram received 
 * \return  char* the SDP message as a byte string
 */
static char* SAP_depay(char* udp_payload){
	/* set a pointer on the beginning of udp payload */
	char *head = udp_payload;
	head += SAP_header_size;
	return head;
}

/**
 * \brief This function will send the SAP message, encapsulate into the payload of an UDP datagram, on the UDP socket with IP: 224.2.127.254 port 9875.
 * \param channelTable_entry * entry the entry in the channelTable associated to the socket
 * \return gboolean TRUE
 */
gboolean prepare_socket(struct channelTable_entry * entry ){
	/* create UDP payload and save the payload into sap_datas structure */
	entry->sap_datas->udp_payload_length = 0;
	if( entry->channelType == videoChannel ){
		entry->sap_datas->udp_payload =  build_SAP_msg(entry, &(entry->sap_datas->udp_payload_length), FALSE); /* build SAP announcement message (not deletion message) */
		/* for(int i = 0; i<entry->sap_datas->udp_payload_length ; i++)
			printf("%02X\t%d\n", entry->sap_datas->udp_payload[i], i); */
		return TRUE;		
	}else if(  entry->channelType == serviceUser ) {
		entry->sap_datas->udp_payload_length 	= 400; /* set the Max size of received datagram */
		entry->sap_datas->udp_payload 			= (char*) malloc(entry->sap_datas->udp_payload_length * sizeof(char));
		return TRUE;
	}
	return FALSE;
}


/**
 * \brief Sends UDP datagram on the UDP socket with IP: 224.2.127.254 port 9875, payload contains SAP/SDP message (announcement or deletion)
 * \param gpointer entry the entry in the channelTable associated to the socket
 * \return gboolean TRUE on succed, FALSE on failure
 */
gboolean send_announcement(gpointer entry){

	/* check channel status, this is check as a stop condition
	 * if the status in stop, we return false, which means that
	 * we will stop to call repeteadly create_SDP
	 */
	int nb_bytes = -1;
	struct channelTable_entry * channel_entry = entry;
	if( channel_entry->channelStatus == stop ){
		/* Build deletion packet */
		channel_entry->sap_datas->udp_payload = build_SAP_msg(channel_entry, &(channel_entry->sap_datas->udp_payload_length), TRUE);
		nb_bytes = sendto( 	sap_socket.udp_socket_fd,
							channel_entry->sap_datas->udp_payload,
							channel_entry->sap_datas->udp_payload_length,
							0,
							(struct sockaddr *) &(sap_socket.multicast_addr),
							sizeof(sap_socket.multicast_addr));
		return FALSE;
	}else{
		/* Send Annoucement message */
		nb_bytes = sendto( 	sap_socket.udp_socket_fd,
							channel_entry->sap_datas->udp_payload,
							channel_entry->sap_datas->udp_payload_length,
							0,
							(struct sockaddr *) &(sap_socket.multicast_addr),
							sizeof(sap_socket.multicast_addr));
	}
	/* check if the number of bytes send is -1, if so an error as occured */
	if ( nb_bytes	== -1 )
	{
    	g_printerr("Failed to send SAP/SDP announcement: %s",strerror(errno));
		return FALSE;
	}
	return TRUE;
}

/**
 * \brief Joins multicast group and receives UDP datagram on the UDP socket 224.2.127.254:9875, payload contains SAP/SDP message (announcement or deletion)
 * \param gpointer entry the entry in the channelTable associated to the socket
 * \return gboolean TRUE on succed, FALSE on failure
 */
gboolean receive_announcement(gpointer entry){
	struct channelTable_entry * channel_entry = entry;

	int 				status;
	char 				temp[400];
/*	unsigned int 		socklen;
	
	struct timeval timeout;
    timeout.tv_sec 	= 3;
    timeout.tv_usec = 0;
	status = setsockopt (sap_socket.udp_socket_fd_rec,
						 SOL_SOCKET,
						 SO_RCVTIMEO,
						 (char *)&timeout,
		 				 sizeof(timeout) );
    if ( status < 0)
		g_printerr("Failed to set socket timeout: %s\n", strerror(errno)); 
	*/
	/* receive packet from socket 
	socklen = sizeof(sap_socket.multicast_addr);
	status = recvfrom( 	sap_socket.udp_socket_fd_rec,
		   				channel_entry->sap_datas->udp_payload,
						channel_entry->sap_datas->udp_payload_length,
			   			MSG_DONTWAIT, flags 
						&(sap_socket.multicast_addr),
						&socklen );*/
	status = read( 	sap_socket.udp_socket_fd_rec,
		   			temp,
					channel_entry->sap_datas->udp_payload_length );
	if (status == -1) {
		g_printerr("Failed to receive: %s\n",strerror(errno));
		return TRUE;
	} else if ( status == sizeof(channel_entry->sap_datas->udp_payload )) {
	    g_printerr("WARNING: datagram too large for buffer: truncated");
		return FALSE;
	} else {
		/* compare teh payloed read on the socket with the payload read before 
		 * if there different, save the new payload into the sap_dat of the channel
		 */
		if ( strcmp(channel_entry->sap_datas->udp_payload, temp) != 0 ){
			memcpy(channel_entry->sap_datas->udp_payload, temp, channel_entry->sap_datas->udp_payload_length );
			unsigned char *sdp_msg = (unsigned char*) SAP_depay(channel_entry->sap_datas->udp_payload);
			return get_SDP(sdp_msg, status - SAP_header_size);
		}
		/* otherwise, do not do anything */
		return TRUE;
	}
}
