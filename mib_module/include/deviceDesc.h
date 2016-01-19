/*
 * Licence: GPL
 * Created: Thu, 18 Jan 2016 12:15:31 +0100
 * Main authors:
 *     - hoel <hvasseur@openwide.fr>
 */
#ifndef DEVICEDESC_H
#define DEVICEDESC_H

/*function for internal behaviour*/
void process_value(char * string);

/* function declarations */
void init_deviceDesc(void);
Netsnmp_Node_Handler handle_deviceDesc;

#endif /* DEVICEDESC_H */
