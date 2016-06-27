/*
 * Note: this file originally auto-generated by mib2c using
 *  $
 */
#ifndef ETHERNETIFTABLE_H
#define ETHERNETIFTABLE_H

/* function declarations */
struct ethernetIfTableEntry
	*ethernetIfTableEntry_create( long ethernetIfSpeed,
		u_char ethernetIfMacAddress[6],
		size_t ethernetIfMacAddress_len,
		in_addr_t ethernetIfIpAddress,
		in_addr_t ethernetIfSubnetMask,
		in_addr_t ethernetIfIpAddressConflict);

//gboolean select_interfaces();
void init_ethernetIfTable(void);
struct ethernetIfTableEntry *init_ethernet(const char* iface);
void MAC_to_byte_array(u_char dest[6], u_char* source);
void init_ethernetIfTable_content(int entryNumber);
void initialize_ethernetIfTableEntry();
Netsnmp_Node_Handler 		ethernetIfTable_handler;
Netsnmp_First_Data_Point 	ethernetIfTable_get_first_data_point;
Netsnmp_Next_Data_Point 	ethernetIfTable_get_next_data_point;


/* column number definitions for table ethernetIfTable */
#define COLUMN_ETHERNETIFINDEX                  1
#define COLUMN_ETHERNETIFSPEED		            2
#define COLUMN_ETHERNETIFMACADDRESS	        	3
#define COLUMN_ETHERNETIFIPADDRESS	           	4
#define COLUMN_ETHERNETIFSUBNETMASK	        	5
#define COLUMN_ETHERNETIFIPADDRESSCONFLICT		6

/* define the size in bytes, that should be a MAC address */
#define MAC_ADDRESS_SIZE						6

/* Typical data structure for a row entry */
struct ethernetIfTableEntry {
    /* Index values this is the index of the EthernetIfEntry in the table */
    long ethernetIfIndex;

    /* Column values */
    long ethernetIfSpeed;
    char ethernetIfMacAddress[6];
    size_t ethernetIfMacAddress_len;
    in_addr_t ethernetIfIpAddress;
    in_addr_t old_ethernetIfIpAddress;
    in_addr_t ethernetIfSubnetMask;
    in_addr_t old_ethernetIfSubnetMask;
    in_addr_t ethernetIfIpAddressConflict;
    in_addr_t old_ethernetIfIpAddressConflict;

    /* Illustrate using a simple linked list */
    int   valid;
    struct ethernetIfTableEntry *next;

};

struct ethernetIfTableEntry  *ethernetIfTable_head;
void ethernetIfTableEntry_delete();
gchar *get_primary_interface_name();
gchar *get_interface_name(struct ethernetIfTableEntry *entry);
#endif /* ETHERNETIFTABLE_H */
