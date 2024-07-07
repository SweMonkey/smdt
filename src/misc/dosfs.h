/*
	DOSFS Embedded FAT-Compatible Filesystem
	(C) 2005 Lewin A.R.W. Edwards (sysadm@zws.com)
*/

#ifndef _DOSFS_H
#define _DOSFS_H

#include "Utils.h"

//===================================================================
// User-supplied functions
u32 DFS_ReadSector(u8 unit, u8 *buffer, u32 sector, u32 count);
u32 DFS_WriteSector(u8 unit, u8 *buffer, u32 sector, u32 count);


//===================================================================
// Configurable items
#define MAX_PATH		64		// Maximum path length (increasing this will
								// GREATLY increase stack requirements!)
#define DIR_SEPARATOR	'/'		// character separating directory components

// End of configurable items
//===================================================================

//===================================================================
// 32-bit error codes
#define DFS_OK			0			// no error
#define DFS_EOF			1			// end of file (not an error)
#define DFS_WRITEPROT	2			// volume is write protected
#define DFS_NOTFOUND	3			// path or file not found
#define DFS_PATHLEN		4			// path too long
#define DFS_ALLOCNEW	5			// must allocate new directory cluster
#define DFS_ERRMISC		0xffffffff	// generic error

//===================================================================
// File access modes
#define DFS_READ		1			// read-only
#define DFS_WRITE		2			// write-only

//===================================================================
// Miscellaneous constants
#define SECTOR_SIZE		512		// sector size in bytes

//===================================================================
// Internal subformat identifiers
#define FAT12			0
#define FAT16			1
#define FAT32			2

//===================================================================
// DOS attribute bits
#define ATTR_READ_ONLY	0x01
#define ATTR_HIDDEN		0x02
#define ATTR_SYSTEM		0x04
#define ATTR_VOLUME_ID	0x08
#define ATTR_DIRECTORY	0x10
#define ATTR_ARCHIVE	0x20
#define ATTR_LONG_NAME	(ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)


/*
	Directory entry structure
	note: if name[0] == 0xe5, this is a free dir entry
	      if name[0] == 0x00, this is a free entry and all subsequent entries are free
		  if name[0] == 0x05, the first character of the name is 0xe5 [a kanji nicety]

	Date format: bit 0-4  = day of month (1-31)
	             bit 5-8  = month, 1=Jan..12=Dec
				 bit 9-15 =	count of years since 1980 (0-127)
	Time format: bit 0-4  = 2-second count, (0-29)
	             bit 5-10 = minutes (0-59)
				 bit 11-15= hours (0-23)
*/
typedef struct _tagDIRENT {
	u8 name[11];			// filename
	u8 attr;				// attributes (see ATTR_* constant definitions)
	u8 reserved;			// reserved, must be 0
	u8 crttimetenth;		// create time, 10ths of a second (0-199 are valid)
	u8 crttime_l;			// creation time low byte
	u8 crttime_h;			// creation time high byte
	u8 crtdate_l;			// creation date low byte
	u8 crtdate_h;			// creation date high byte
	u8 lstaccdate_l;		// last access date low byte
	u8 lstaccdate_h;		// last access date high byte
	u8 startclus_h_l;		// high word of first cluster, low byte (FAT32)
	u8 startclus_h_h;		// high word of first cluster, high byte (FAT32)
	u8 wrttime_l;			// last write time low byte
	u8 wrttime_h;			// last write time high byte
	u8 wrtdate_l;			// last write date low byte
	u8 wrtdate_h;			// last write date high byte
	u8 startclus_l_l;		// low word of first cluster, low byte
	u8 startclus_l_h;		// low word of first cluster, high byte
	u8 filesize_0;			// file size, low byte
	u8 filesize_1;			//
	u8 filesize_2;			//
	u8 filesize_3;			// file size, high byte
} DIRENT, *PDIRENT;

/*
	Partition table entry structure
*/
typedef struct _tagPTINFO {
	u8		active;			// 0x80 if partition active
	u8		start_h;		// starting head
	u8		start_cs_l;		// starting cylinder and sector (low byte)
	u8		start_cs_h;		// starting cylinder and sector (high byte)
	u8		type;			// type ID byte
	u8		end_h;			// ending head
	u8		end_cs_l;		// ending cylinder and sector (low byte)
	u8		end_cs_h;		// ending cylinder and sector (high byte)
	u8		start_0;		// starting sector# (low byte)
	u8		start_1;		//
	u8		start_2;		//
	u8		start_3;		// starting sector# (high byte)
	u8		size_0;			// size of partition (low byte)
	u8		size_1;			//
	u8		size_2;			//
	u8		size_3;			// size of partition (high byte)
} PTINFO, *PPTINFO;

/*
	Master Boot Record structure
*/
typedef struct _tagMBR {
	u8 bootcode[0x1be];	// boot sector
	PTINFO ptable[4];			// four partition table structures
	u8 sig_55;				// 0x55 signature byte
	u8 sig_aa;				// 0xaa signature byte
} MBR, *PMBR;

/*
	BIOS Parameter Block structure (FAT12/16)
*/
typedef struct _tagBPB {
	u8 bytepersec_l;		// bytes per sector low byte (0x00)
	u8 bytepersec_h;		// bytes per sector high byte (0x02)
	u8	secperclus;			// sectors per cluster (1,2,4,8,16,32,64,128 are valid)
	u8 reserved_l;			// reserved sectors low byte
	u8 reserved_h;			// reserved sectors high byte
	u8 numfats;			// number of FAT copies (2)
	u8 rootentries_l;		// number of root dir entries low byte (0x00 normally)
	u8 rootentries_h;		// number of root dir entries high byte (0x02 normally)
	u8 sectors_s_l;		// small num sectors low byte
	u8 sectors_s_h;		// small num sectors high byte
	u8 mediatype;			// media descriptor byte
	u8 secperfat_l;		// sectors per FAT low byte
	u8 secperfat_h;		// sectors per FAT high byte
	u8 secpertrk_l;		// sectors per track low byte
	u8 secpertrk_h;		// sectors per track high byte
	u8 heads_l;			// heads low byte
	u8 heads_h;			// heads high byte
	u8 hidden_0;			// hidden sectors low byte
	u8 hidden_1;			// (note - this is the number of MEDIA sectors before
	u8 hidden_2;			// first sector of VOLUME - we rely on the MBR instead)
	u8 hidden_3;			// hidden sectors high byte
	u8 sectors_l_0;		// large num sectors low byte
	u8 sectors_l_1;		//
	u8 sectors_l_2;		//
	u8 sectors_l_3;		// large num sectors high byte
} BPB, *PBPB;

/*
	Extended BIOS Parameter Block structure (FAT12/16)
*/
typedef struct _tagEBPB {
	u8 unit;				// int 13h drive#
	u8 head;				// archaic, used by Windows NT-class OSes for flags
	u8 signature;			// 0x28 or 0x29
	u8 serial_0;			// serial#
	u8 serial_1;			// serial#
	u8 serial_2;			// serial#
	u8 serial_3;			// serial#
	u8 label[11];			// volume label
	u8 system[8];			// filesystem ID
} EBPB, *PEBPB;

/*
	Extended BIOS Parameter Block structure (FAT32)
*/
typedef struct _tagEBPB32 {
	u8 fatsize_0;			// big FAT size in sectors low byte
	u8 fatsize_1;			//
	u8 fatsize_2;			//
	u8 fatsize_3;			// big FAT size in sectors high byte
	u8 extflags_l;			// extended flags low byte
	u8 extflags_h;			// extended flags high byte
	u8 fsver_l;			// filesystem version (0x00) low byte
	u8 fsver_h;			// filesystem version (0x00) high byte
	u8 root_0;				// cluster of root dir, low byte
	u8 root_1;				//
	u8 root_2;				//
	u8 root_3;				// cluster of root dir, high byte
	u8 fsinfo_l;			// sector pointer to FSINFO within reserved area, low byte (2)
	u8 fsinfo_h;			// sector pointer to FSINFO within reserved area, high byte (0)
	u8 bkboot_l;			// sector pointer to backup boot sector within reserved area, low byte (6)
	u8 bkboot_h;			// sector pointer to backup boot sector within reserved area, high byte (0)
	u8 reserved[12];		// reserved, should be 0

	u8 unit;				// int 13h drive#
	u8 head;				// archaic, used by Windows NT-class OSes for flags
	u8 signature;			// 0x28 or 0x29
	u8 serial_0;			// serial#
	u8 serial_1;			// serial#
	u8 serial_2;			// serial#
	u8 serial_3;			// serial#
	u8 label[11];			// volume label
	u8 system[8];			// filesystem ID
} EBPB32, *PEBPB32;

/*
	Logical Boot Record structure (volume boot sector)
*/
typedef struct _tagLBR {
	u8 jump[3];			// JMP instruction
	u8 oemid[8];			// OEM ID, space-padded
	BPB bpb;					// BIOS Parameter Block
	union {
		EBPB ebpb;				// FAT12/16 Extended BIOS Parameter Block
		EBPB32 ebpb32;			// FAT32 Extended BIOS Parameter Block
	} ebpb;
	u8 code[420];			// boot sector code
	u8 sig_55;				// 0x55 signature byte
	u8 sig_aa;				// 0xaa signature byte
} LBR, *PLBR;

/*
	Volume information structure (Internal to DOSFS)
*/
typedef struct _tagVOLINFO {
	u8 unit;				// unit on which this volume resides
	u8 filesystem;			// formatted filesystem

// These two fields aren't very useful, so support for them has been commented out to
// save memory. (Note that the "system" tag is not actually used by DOS to determine
// filesystem type - that decision is made entirely on the basis of how many clusters
// the drive contains. DOSFS works the same way).
// See tag: OEMID in dosfs.c
//	u8 oemid[9];			// OEM ID ASCIIZ
//	u8 system[9];			// system ID ASCIIZ
	u8 label[12];			// volume label ASCIIZ
	u32 startsector;		// starting sector of filesystem
	u8 secperclus;			// sectors per cluster
	u16 reservedsecs;		// reserved sectors
	u32 numsecs;			// number of sectors in volume
	u32 secperfat;			// sectors per FAT
	u16 rootentries;		// number of root dir entries

	u32 numclusters;		// number of clusters on drive

	// The fields below are PHYSICAL SECTOR NUMBERS.
	u32 fat1;				// starting sector# of FAT copy 1
	u32 rootdir;			// starting sector# of root directory (FAT12/FAT16) or cluster (FAT32)
	u32 dataarea;			// starting sector# of data area (cluster #2)
} VOLINFO, *PVOLINFO;

/*
	Flags in DIRINFO.flags
*/
#define DFS_DI_BLANKENT		0x01	// Searching for blank entry

/*
	Directory search structure (Internal to DOSFS)
*/
typedef struct _tagDIRINFO {
	u32 currentcluster;	// current cluster in dir
	u8 currentsector;		// current sector in cluster
	u8 currententry;		// current dir entry in sector
	u8 *scratch;			// ptr to user-supplied scratch buffer (one sector)
	u8 flags;				// internal DOSFS flags
} DIRINFO, *PDIRINFO;

/*
	File handle structure (Internal to DOSFS)
*/
typedef struct _tagFILEINFO {
	PVOLINFO volinfo;			// VOLINFO used to open this file
	u32 dirsector;			// physical sector containing dir entry of this file
	u8 diroffset;			// # of this entry within the dir sector
	u8 mode;				// mode in which this file was opened
	u32 firstcluster;		// first cluster of file
	u32 filelen;			// byte length of file

	u32 cluster;			// current cluster
	u32 pointer;			// current (BYTE) pointer
} FILEINFO, *PFILEINFO;

/*
	Get starting sector# of specified partition on drive #unit
	NOTE: This code ASSUMES an MBR on the disk.
	scratchsector should point to a SECTOR_SIZE scratch area
	Returns 0xffffffff for any error.
	If pactive is non-NULL, this function also returns the partition active flag.
	If pptype is non-NULL, this function also returns the partition type.
	If psize is non-NULL, this function also returns the partition size.
*/
u32 DFS_GetPtnStart(u8 unit, u8 *scratchsector, u8 pnum, u8 *pactive, u8 *pptype, u32 *psize);

/*
	Retrieve volume info from BPB and store it in a VOLINFO structure
	You must provide the unit and starting sector of the filesystem, and
	a pointer to a sector buffer for scratch
	Attempts to read BPB and glean information about the FS from that.
	Returns 0 OK, nonzero for any error.
*/
u32 DFS_GetVolInfo(u8 unit, u8 *scratchsector, u32 startsector, PVOLINFO volinfo);

/*
	Open a directory for enumeration by DFS_GetNextDirEnt
	You must supply a populated VOLINFO (see DFS_GetVolInfo)
	The empty string or a string containing only the directory separator are
	considered to be the root directory.
	Returns 0 OK, nonzero for any error.
*/
u32 DFS_OpenDir(PVOLINFO volinfo, u8 *dirname, PDIRINFO dirinfo);

/*
	Get next entry in opened directory structure. Copies fields into the dirent
	structure, updates dirinfo. Note that it is the _caller's_ responsibility to
	handle the '.' and '..' entries.
	A deleted file will be returned as a NULL entry (first char of filename=0)
	by this code. Filenames beginning with 0x05 will be translated to 0xE5
	automatically. Long file name entries will be returned as NULL.
	returns DFS_EOF if there are no more entries, DFS_OK if this entry is valid,
	or DFS_ERRMISC for a media error
*/
u32 DFS_GetNext(PVOLINFO volinfo, PDIRINFO dirinfo, PDIRENT dirent);

/*
	Open a file for reading or writing. You supply populated VOLINFO, a path to the file,
	mode (DFS_READ or DFS_WRITE) and an empty fileinfo structure. You also need to
	provide a pointer to a sector-sized scratch buffer.
	Returns various DFS_* error states. If the result is DFS_OK, fileinfo can be used
	to access the file from this point on.
*/
u32 DFS_OpenFile(PVOLINFO volinfo, u8 *path, u8 mode, u8 *scratch, PFILEINFO fileinfo);

/*
	Read an open file
	You must supply a prepopulated FILEINFO as provided by DFS_OpenFile, and a
	pointer to a SECTOR_SIZE scratch buffer.
	Note that returning DFS_EOF is not an error condition. This function updates the
	successcount field with the number of bytes actually read.
*/
u32 DFS_ReadFile(PFILEINFO fileinfo, u8 *scratch, u8 *buffer, u32 *successcount, u32 len);

/*
	Write an open file
	You must supply a prepopulated FILEINFO as provided by DFS_OpenFile, and a
	pointer to a SECTOR_SIZE scratch buffer.
	This function updates the successcount field with the number of bytes actually written.
*/
u32 DFS_WriteFile(PFILEINFO fileinfo, u8 *scratch, u8 *buffer, u32 *successcount, u32 len);

/*
	Seek file pointer to a given position
	This function does not return status - refer to the fileinfo->pointer value
	to see where the pointer wound up.
	Requires a SECTOR_SIZE scratch buffer
*/
void DFS_Seek(PFILEINFO fileinfo, u32 offset, u8 *scratch);

/*
	Delete a file
	scratch must point to a sector-sized buffer
*/
u32 DFS_UnlinkFile(PVOLINFO volinfo, u8 *path, u8 *scratch);

#endif // _DOSFS_H
