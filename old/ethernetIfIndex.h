#ifndef ETHERNETIFINDEX_H
#define ETHERNETIFINDEX_H

/*this is the ethernetIfIndex parameter of the MIB*/
/*It should be a dynamically initialized array of 32-bits integer*/

int* ethernetIfIndex;
void initi_ethernetIfIndex(void);

#endif /* ETHERNETIFINDEX_H */
