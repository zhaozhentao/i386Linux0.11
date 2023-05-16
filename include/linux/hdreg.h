#ifndef _HDREG_H
#define _HDREG_H

#define HD_DATA		0x1f0	/* _CTL when writing */

#define HD_STATUS	0x1f7	/* see status-bits */

#define HD_CMD		0x3f6

#define WIN_READ		0x20

struct partition {
	unsigned char boot_ind;		/* 0x80 - active (unused) */
	unsigned char head;		/* ? */
	unsigned char sector;		/* ? */
	unsigned char cyl;		/* ? */
	unsigned char sys_ind;		/* ? */
	unsigned char end_head;		/* ? */
	unsigned char end_sector;	/* ? */
	unsigned char end_cyl;		/* ? */
	unsigned int start_sect;	/* starting sector counting from 0 */
	unsigned int nr_sects;		/* nr of sectors in partition */
};

#endif

