// <copyright file="apfs_struct.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
/////////////////////////////////////////////////////////////////////////////
//
// Revision History:
//
//     04-October-2017  - created
//
/////////////////////////////////////////////////////////////////////////////


#ifndef __UFSD_APFS_STRUCT_H
#define __UFSD_APFS_STRUCT_H

#include "../h/assert.h"

#ifndef NO_PRAGMA_PACK
#pragma pack( push, 1 )
#endif


#define APFS_CSB_MAGIC                  0x4253584E  /* 'NXSB' */
#define APFS_VSB_MAGIC                  0x42535041  /* 'APSB' */
#define APFS_EFI_JUMPSTART_MAGIC        0x5244534a  /* 'JSDR' */
#define APFS_ENCRYPTION_BLOCK_MAGIC     0x464c4142  /* 'BALF' */

#define APFS_MSB_OFFSET                 0x0
#define APFS_MSB_SIZE                   0x1000
#define APFS_MAX_SUBVOLUMES             100

#define APFS_VSB_CASE_NSENS             0x01        /* Volume no case-sensitive */
//#define APFS_VSB_ENCRYPTED              0x04        /* Volume encrypted */

#define APFS_ROOT_PARENT_INO            0x01
#define APFS_ROOT_INO                   0x02        /* 'root' inode */
#define APFS_PRIVATE_DIR_INO            0x03        /* 'private-dir' inode */
#define APFS_SNAPSHOT_DIR_INO           0x06        /* The inode number for the directory where snapshot metadata is stored */
#define APFS_FIRST_FREE_INO             0x10        /* First non-reserved inode */

//APFS block types
#define APFS_TYPE_EMPTY                 0x00        /*empty block*/
#define APFS_TYPE_SUPERBLOCK            0x01        /*apfs_sb*/
#define APFS_TYPE_ROOT_NODE_BLOCK       0x02        /*tree root node block*/
#define APFS_TYPE_NODE_BLOCK            0x03        /*tree node block*/

#define APFS_TYPE_BITMAP_DESCRIPTOR     0x05        /*BMD*/
#define APFS_TYPE_BITMAP_INDEX_BLOCK    0x06        /*BMIB*/
#define APFS_TYPE_BITMAP_RECORD_BLOCK   0x07        /*BMRB*/

#define APFS_TYPE_BTREE                 0x0B        /*apfs_btreed*/
#define APFS_TYPE_SUPERBLOCK_MAP        0x0C        /*apfs_sb_map*/
#define APFS_TYPE_VOLUME_SUPERBLOCK     0x0D        /*apfs_vsb*/

#define APFS_TYPE_VOLUME_REAP           0x11        /*apfs_volreap*/
#define APFS_TYPE_REAP_LIST             0x12        /*apfs_reaplist*/
#define APFS_TYPE_EFI_JUMPSTART         0x14

#define APFS_TYPE_FUSION                0x16
#define APFS_TYPE_FUSION_LIST           0x17

#define APFS_TYPE_ENCRYPTION_DESCRIPTOR 0x18        /*apfs_encryption_descriptor*/
#define APFS_TYPE_ENCRYPTION_ROOT       0x19        /*apfs_encryption_root*/
#define APFS_TYPE_ENCRYPTION            0x1B
#define APFS_TYPE_MAX_VALID             0x1B

//block flags
#define APFS_BLOCK_FLAG_COMMON          0           /*common APFS block*/
#define APFS_BLOCK_FLAG_POSITION        0x4000      /*block id same as block position*/
#define APFS_BLOCK_FLAG_CONTAINER       0x8000      /*container metadata*/

//table block content
#define APFS_CONTENT_EMPTY              0x00
#define APFS_CONTENT_HISTORY            0x09
#define APFS_CONTENT_LOCATION           0x0B
#define APFS_CONTENT_FILES              0x0E
#define APFS_CONTENT_EXTENTS            0x0F
#define APFS_CONTENT_SNAPSHOTS          0x10
#define APFS_CONTENT_SNAPSHOTS_MAP      0x13
#define APFS_CONTENT_FUSION             0x15
#define APFS_CONTENT_ENCRYPTION         0x1a
#define APFS_CONTENT_MAX_VALID          0x1a

#define APFS_ENCRYPTED_BLOCK            PU64(0x8000000000000000)
#define APFS_MAPPED_BLOCK               PU64(0x0004000000000000)

//Time in nanoseconds since 1970-01-01.
typedef UINT64 apfs_time;

//Block header structure
//The majority of meta - data blocks in APFS have a 32 - byte header. The exceptions include the Superblocks and the bitmap blocks.
struct apfs_block_header
{
  UINT64            checksum;                   //0x00 //Fletchers Checksum Algorithm
  UINT64            id;                         //0x08 //Object id or block number
  UINT64            checkpoint_id;              //0x10 //id of checkpoint this block belongs to
  unsigned short    block_type;                 //0x18 //APFS_TYPE_ XXX
  unsigned short    flags;                      //0x1A //APFS_BLOCK_FLAG_ XXX
  unsigned short    content_type;               //0x1C //APFS_CONTENT_ XXX
  unsigned short    padding;                    //0x1E //0
}__attribute__((packed));


//main/checkpoint superblock
struct apfs_sb
{
  apfs_block_header header;                     //0x00 //Standart block header

  unsigned int      sb_magic;                   //0x20 //'NXSB'
  unsigned int      sb_block_size;              //0x24 //Size of allocationt unit for fs
  UINT64            sb_total_blocks;            //0x28 //Number of blocks in the container

  UINT64            sb_features;
  UINT64            sb_ro_compat_features;
  UINT64            sb_incompat_features;
  unsigned char     sb_uuid[0x10];              //0x48 //Uuid of container
  UINT64            sb_next_block_id;           //0x58
  UINT64            sb_next_checkpoint_id;      //0x60 //Next checkpoint id. Used for searching last CSB
  unsigned int      sb_number_of_sb;            //0x68 //number of blocks from sb_base_sb for SB
  unsigned int      sb_number_of_meta;          //0x6c //number of blocks from sb_first_meta for meta

  UINT64            sb_first_sb;                //0x70 //start place of container superblocks buffer
  UINT64            sb_first_meta;              //0x78 //start place of container metadata buffer
  unsigned int      sb_next_sb;                 //0x80 //Next free superblock from sb_first_sb
  unsigned int      sb_next_meta;               //0x84 //Next free meta from sb_first_meta
  unsigned int      sb_current_sb;              //0x88 //The current state SB_MAP is located in block sb_base_block + sb_curent_sb
  unsigned int      sb_current_sb_len;          //0x8C //number of blocks used in superblock buffer with current superblock (2 = sb_map + sb)
  unsigned int      sb_current_meta;            //0x90 //First meta block for this checkpoint
  unsigned int      sb_blocks_sb_map_entries;   //0x98

  UINT64            sb_spaceman_id;             //0x98 //id of bitmap
  UINT64            sb_volume_root_block;       //0xA0 //Contains address of Volume Block
  UINT64            sb_reap_volume;             //0xA8 //id of reap volume block
  unsigned int      sb_unknown_0xB0;            //0xB0 //always zero
  unsigned int      sb_max_volumes;             //0xB4 //Max number of volumes (Actual number is less or equal). Highest observed number is 0x64. Minimal value is 1.
  UINT64            sb_volumes_id[100];         //0xB8 //Array of volume id's. Contains sb_volumes_array items

  UINT64            sb_counters[0x20];          //0x3D8 // some debug counters (CKSUM_SET, CKSUM_FAIL)
  UINT64            sb_unknown_0x4d8[2];
  UINT64            sb_evict_mapping_tree_id;   //0x4E8 // tree id used to keep track of objects that must be moved out of blocked-out storage
  UINT64            sb_flags;                   //0x4F0 // container flags APFS_CFLAG_XXX

  UINT64            sb_efi_jumpstart_block;     //0x4f8 // EFI jump start block
  unsigned char     sb_fusion_uuid[0x10];       //0x500 // fusion Uuid
  UINT64            sb_keybag_block;            //0x510 // block number with encryption key
  UINT64            sb_keybag_count;            //0x518 // number of encryption key blocks

  unsigned short    sb_unknown_0x520;           //0x520 // 1
  unsigned short    sb_unknown_0x522;           //0x522 // 4
  unsigned short    sb_unknown_0x524;           //0x524 // 8 (1 in one case) (6)
  unsigned short    sb_unknown_0x526;           //0x526 // 0
  unsigned char     sb_unknown_0x528[0x20];     //0x528 // 0

  //fusion params
  UINT64            sb_fusion_map_root;         //0x548 // root block number of fusion map tree
  UINT64            sb_fusion_block_id;         //0x550 // id of fusion block type
  UINT64            sb_fusion_list_block;       //0x558 // first block of fusion list
  UINT64            sb_size_of_fusion_list;     //0x560 // number of block in fusion list
}__attribute__((packed));
C_ASSERT(sizeof(apfs_sb) == 0x568);


//entries in superblock map
struct apfs_sb_map_entry
{
  unsigned short    block_type;                 //0x00 //APFS_TYPE_ XXX
  unsigned short    flags;                      //0x02 //APFS_BLOCK_FLAG_ XXX
  unsigned short    content_type;               //0x04 //APFS_CONTENT_ XXX
  unsigned short    padding;                    //0x06

  unsigned int      size;                       //0x08 //Block size for this structure
  unsigned int      padding2;                   //0x0C
  UINT64            unknown_0x10;               //0x10
  UINT64            object_id;                  //0x18
  UINT64            block;                      //0x20
}__attribute__((packed));


//superblock map used for convert superblock id to block number
#define APFS_SBMAP_LAST_MAP             1
#define MAX_SBMAP_ENTRIES               0x65

struct apfs_sb_map
{
  apfs_block_header header;                     //0x00 //Standart block header
  unsigned int      flags;                      //0x20 //APFS_SBMAP_LAST_MAP
  unsigned int      entries_count;              //0x24 //Number of map entries
  apfs_sb_map_entry entries[1];                 //0x28 //Array of map entries
}__attribute__((packed));


struct apfs_shapshot_link
{
  UINT64            block;
  UINT64            checkpoint;
}__attribute__((packed));

//BTREE descriptor
struct apfs_btreed
{
  apfs_block_header header;                     //0x00 //Standart block header
  unsigned int      type;                       //0x20 //0-inode tree, 1-volume tree
  unsigned int      number_of_snapshots;        //0x24 //
  unsigned int      tree_type;                  //0x28 //0x40000002
  unsigned int      tree_type2;                 //0x2c //0x40000002
  UINT64            btree_root;                 //0x30
  apfs_shapshot_link snapshot[1];               //0x38 // TODO: possible wrong data structure
}__attribute__((packed));


//Bitmap descriptor or spacemanager
struct apfs_bmd
{
  apfs_block_header header;                     //0x00 //Standart block header
  unsigned int      block_size;                 //0x20 //Size of allocationt unit
  unsigned int      number_of_blocks_in_bmb;    //0x24 //blocks per chunk
  unsigned int      max_records_in_bmrb;        //0x28 //chunks per cib
  unsigned int      max_records_in_bmib;        //0x2c //cibs per cab

  //main disk
  UINT64            bmd_total_blocks;           //0x30 //main disk total blocks
  UINT64            number_of_bb;               //0x38 //max number of bitmap blocks
  unsigned int      number_of_bmrb;             //0x40
  unsigned int      number_of_bmib;             //0x44 //0-if bmib not used
  UINT64            free_blocks;                //0x48
  unsigned int      offset_to_entries;          //0x50 //array of bmib or bmrb
  unsigned char     reserved[0xc];              //0x54 //0

  //fusion disk
  UINT64            d2_total_blocks;            //0x60 //second disk total block
  UINT64            d2_number_of_bb;            //0x68 //max number of bitmap blocks
  unsigned int      d2_number_of_bmrb;          //0x70
  unsigned int      d2_number_of_bmib;          //0x74 //0-if bmib not used
  UINT64            d2_free_blocks;             //0x78 //second disk free entries
  unsigned int      d2_offset_to_entries;       //0x80 //array of bmib or bmrb
  unsigned char     d2_reserved[0xc];           //0x84 //0

  //from this to offset_to_entries there is some specific data area
  unsigned int      flags;                      //0x90 //0,1
  unsigned int      ratio;                      //0x94 //0x10

  UINT64            number_of_bitmap_blocks;    //0x98 //area size for bitmap blocks //next is volume area

  unsigned int      metabitmap_len;             //0xa0 //number of block in meta bitmap
  unsigned int      metabitmap_buff_size;       //0xa4 //len of buffer for meta bitmap
  UINT64            metabitmap_buff_block;      //0xa8 //first block of buffer for meta bitmap

  UINT64            bitmap_block;               //0xb0 //begin of bitmap blocks
  UINT64            fs_reserve_block_count;     //0xb8 //0
  UINT64            fs_reserve_alloc_count;     //0xc0 //0

  // spaceman_free_queue (0)
  //bitmap history
  UINT64            bmh_number_of_blocks;       //0xc8 //number of blocks in history
  UINT64            bmh_id;                     //0xd0 //id of history
  UINT64            bmh_last_checkpoint;        //0xd8 //last chekpoint in history
  UINT64            bmh_max_size;               //0xe0 //max number of blocks in history tree
  UINT64            bmh_reserved;               //0xe8 //0

  //volume history
  UINT64            vh_number_of_blocks;        //0xf0 //number of blocks in history
  UINT64            vh_id;                      //0xf8 //id of history
  UINT64            vh_last_checkpoint;         //0x100 //last chekpoint in history
  UINT64            vh_max_size;                //0x108 //max number of blocks in history tree
  UINT64            vh_reserved;                //0x110 //0

  //fusion history
  UINT64            fh_number_of_blocks;        //0x118 //number of blocks in history
  UINT64            fh_id;                      //0x120 //id of history
  UINT64            fh_last_checkpoint;         //0x128 //last chekpoint in history
  UINT64            fh_max_size;                //0x130 //max number of blocks in history tree
  UINT64            fh_reserved;                //0x138 //0

  //some meta bitmap area control zone
  unsigned short    next_marker_pos;            //0x140 //next 0xff marker pos in cyclic buffer
  unsigned short    prev_marker_pos;            //0x142 //first 0xff marker pos in cyclic buffer
  unsigned int      offset_mod_checkpoint;      //0x144 //offset to array of checkpoint number where 0xff marker was moved [UINT64]
  unsigned int      offset_metabitmap_pos;      //0x148 //offset to array of 0xff markers pos [unsigned short]
  unsigned int      offset_cyclic_buff;         //0x14c //metabitmap_buff_block [unsigned short]

  //[] ???

  //typical initial layout for small volumes
  //UINT64            mod_checkpoint[1];        //0x150 //
  //unsigned short    metabitmap_pos[1];        //0x158 //blocks offset in metabitmap buffer
  //unsigned short    cyclic_buff[0x16];        //0x160 //0xffff-used pos else free entry pos

  //control zone initial state
  //0x0140: 01 00 0f 00 50 01 00 00 58 01 00 00 60 01 00 00
  //0x0150: 01 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  //cyclic buffer initial state
  //0x0160: ff ff 02 00 03 00 04 00 05 00 06 00 07 00 08 00
  //0x0170: 09 00 0a 00 0b 00 0c 00 0d 00 0e 00 0f 00 ff ff
  //on every change [ff ff] marker from prev_marker_pos moves to next_marker_pos

  //UINT64          entries[]                   //0x180 //array of bmib for big volume or bmrb
}__attribute__((packed));


//bitmap index block
struct apfs_bmib
{
  apfs_block_header header;                     //0x00 //Standart block header
  unsigned int      block_index;                //0x20 //number of this index block
  unsigned int      number_of_bmrb;             //0x24 //numbers of entry
  UINT64            bmrb[1];                    //0x28 //array of bmrb blocks
}__attribute__((packed));


//bitmap record entry
struct apfs_bm_entry
{
  UINT64            checkpoint;                 //0x00 //checkpoint when this entry was changed
  UINT64            first_block;                //0x08 //first block covered by this record
  unsigned int      total_blocks;               //0x10 //total blocks in this record
  unsigned int      free_blocks;                //0x14 //free blocks in this record
  UINT64            bmb_block;                  //0x18 //bitmap block 0-meance not allocated
}__attribute__((packed));


//bitmap record block
struct apfs_bmrb
{
  apfs_block_header header;                     //0x00 //Standart block header
  unsigned int      block_index;                //0x20 //number of this record block
  unsigned int      number_of_records;          //0x24 //numbers of entry
  apfs_bm_entry     records[1];                 //0x28 //array of entries
}__attribute__((packed));


struct apfs_reap_entry
{
  unsigned int      next_entry;                 //0xffffffff - last entry
  unsigned int      flags;                      //flags
  unsigned short    block_type;                 //APFS_TYPE_ XXX
  unsigned short    block_flags;                //APFS_BLOCK_FLAG_ XXX
  unsigned int      block_size;
  UINT64            volume_id;
  UINT64            id;
  UINT64            check_point;
}__attribute__((packed));


struct apfs_volreap
{
  apfs_block_header header;                     //0x00 //Standart block header
  UINT64            checkpoint1;                //0x20
  UINT64            checkpoint2;                //0x28
  UINT64            reaplist_id1;               //0x30 //head
  UINT64            reaplist_id2;               //0x38 //tail
  apfs_reap_entry   entry;                      //0x40
  unsigned int      flags;                      //0x48
  unsigned int      entry_space;                //0x4c
  //some struct array like apfs_reap_entry
}__attribute__((packed));


struct apfs_reaplist
{
  apfs_block_header header;                     //0x00 //Standart block header
  unsigned char     reserved[12];               //0x20 //0
  unsigned int      max_record_count;           //0x2c //100
  unsigned int      entry_count;
  unsigned int      first_entry;
  unsigned int      last_entry;
  unsigned int      free_entry;
  apfs_reap_entry   entry[0x64];                //0x40
}__attribute__((packed));


struct apfs_efi_extent
{
  UINT64            extent_block;
  UINT64            extent_len;
}__attribute__((packed));


struct apfs_efi_jumpstart
{
  apfs_block_header header;                     //0x00 //Standart block header
  unsigned int      magic;                      //0x20 //APFS_EFI_JUMPSTART_MAGIC
  unsigned int      version;                    //0x24
  unsigned int      file_length;                //0x28
  unsigned int      extent_count;               //0x2c
  unsigned char     reserved[0x80];             //0x30
  apfs_efi_extent   extents[1];                 //0xc0
}__attribute__((packed));


//volume checkpoint
struct apfs_volume_update
{
  char              name[0x20];                 //0x00 //updater name
  apfs_time         time;                       //0x20 //update time
  UINT64            checkpoint_id;              //0x28 //update Checkpoint ID
}__attribute__((packed));

#define VOLNAME_LEN 0x100

//volume superblock
struct apfs_vsb
{
  apfs_block_header header;                     //0x00 //Standart block header
  unsigned int      vsb_magic;                  //0x20 //"APSB"
  unsigned int      vsb_volume_index;           //0x24 //Volume index. First volume has index 0
  UINT64            vsb_features;               //0x28
  UINT64            vsb_ro_compat_features;     //0x30
  UINT64            vsb_incompat_features;      //0x38
  UINT64            vsb_unmount_time;           //0x40 //???
  UINT64            vsb_blocks_reserved;        //0x48
  UINT64            vsb_blocks_quota;           //0x50
  UINT64            vsb_blocks_used;            //0x58

  UINT64            vsb_unknown_0x60;           //0x60 //5
  unsigned int      vsb_unknown_0x68;           //0x68 //6
  unsigned int      vsb_unknown_0x6c;           //0x6c //95 01 41 11 or same
  unsigned int      vsb_unknown_0x70;           //0x70 //1
  unsigned int      vsb_unknown_0x74;           //0x74 //2
  unsigned int      vsb_unknown_0x78;           //0x78 //0x40000002
  unsigned int      vsb_unknown_0x7c;           //0x7c //0x40000002

  UINT64            vsb_btom_root;              //0x80 //Block number to initial block of catalog B-Tree Object Map(BTOM)
  UINT64            vsb_root_node_id;           //0x88 //Node ID of root node
  UINT64            vsb_extent_root;            //0x90 //Block number to Extents B-Tree
  UINT64            vsb_snapshot_list;          //0x98 //Block number to list of Snapshots

  UINT64            vsb_unknown_0xa0[2];        //0xA0

  UINT64            vsb_next_cnid;              //0xB0 //Next availible catalog node id (CNID)
  UINT64            vsb_files_count;            //0xB8 //Number of files on the volume
  UINT64            vsb_dirs_count;             //0xC0 //Number of folders on the volume
  UINT64            vsb_symlinks_count;         //0xc8 //symlink counter
  UINT64            vsb_spec_files_count;       //0xd0 //fifo, socket, device counter
  UINT64            vsb_snapshots_count;        //0xd8 //number of snapshots
  UINT64            vsb_allocate_count;         //0xe0 //calc extents only
  UINT64            vsb_free_count;             //0xe8 //calc extents only
  unsigned char     vsb_uuid[0x10];             //0xF0 //UUID of volume. Same result as the API provide with System/Library/Filesystems/apfs.fs/Contents/Resources/apfs.util -k /dev/disk?s?s?
  apfs_time         vsb_time_modified;          //0x100 //Time Volume last written / modified
  UINT64            vsb_volume_flags;           //0x108 //1,8 volume flags
  apfs_volume_update vsb_created;               //0x110 //volume creation
  apfs_volume_update vsb_updates[8];            //0x140 //volume updates history

  char              vsb_volname[VOLNAME_LEN];   //0x2C0 //Volume name
  unsigned int      vsb_next_doc_id;            //0x3C0
  unsigned int      vsb_unknown_0x3c4;          //0x3c4 //??? some flags
  UINT64            vsb_unknown_0x3c8;          //0x3c8 // 0
  UINT64            vsb_encryption_block;       //0x3d0 //apfs_encryption_descriptor

  UINT64            vsb_cnid_on_update;         //0x3d8 cnid on last volume update ???
  UINT64            vsb_checkpoint_on_update;   //0x3e0 checkpoint on last volume update
  UINT64            vsb_unknown_0x3e8;          //0x3e8
}__attribute__((aligned(2), packed));
C_ASSERT(sizeof(apfs_vsb) == 0x3F0);


struct apfs_encryption_descriptor
{
  apfs_block_header header;                     //0x00 //Standart block header
  unsigned int      magic;                      //0x20 //APFS_ENCRYPTION_BLOCK_MAGIC
  unsigned int      unknown_0x24;               //0x24 //1
  UINT64            unknown_0x28;               //0x28 //maybe some flags 1;1001;1021;2021;3021

  UINT64            unknown_0x30;               //0x30 //0
  UINT64            num1;                       //0x38
  UINT64            flags;                      //0x40
  UINT64            block1;                     //0x48
  UINT64            block2;                     //0x50
  UINT64            num2;                       //0x58
  UINT64            last_num;                   //0x60

  UINT64            encryption_root_block;      //0x68 //0x19 block
  UINT64            flags2;                     //0x70
  UINT64            block3;                     //0x78

  UINT64            array[1];                   //0x80 //???some encryption pattern
}__attribute__((packed));


struct apfs_encryption_root
{
  apfs_block_header header;                     //0x00 //Standart block header
  UINT64            encryption_tree;            //0x20 //encryption tree root block
  UINT64            total_blocks;               //0x28
}__attribute__((packed));


struct apfs_encryption
{
  apfs_block_header header;                     //0x00 //Standart block header
  unsigned char     unknown_0x20[0x1d0];        //0x20 //0
  UINT64            flags;                      //0x1f0 //???
  UINT64            unknown_0x1f8;              //0x1f8 //0
  //unsigned char     bitmap[1];                  //0x200 //looks like bitmap array
}__attribute__((packed));


struct apfs_fusion
{
  apfs_block_header header;                     //0x00 //Standart block header
  UINT64            unknown_0x20;               //0x20 //0x70
  UINT64            first_not_flushed;          //0x28 //first not flushed block
  UINT64            last_not_flushed;           //0x30 //last not flushed block
  UINT64            prev_block;                 //0x38 //previously flushed block
  UINT64            last_block;                 //0x40 //last block
  UINT64            list_blocks;                //0x48 //number of list blocks to flush
  UINT64            total_blocks;               //0x50 //total blocks in extents
  UINT64            to_flush_blocks;            //0x58 //not flushed blocks in extents
}__attribute__((packed));

struct apfs_fusion_list_entry
{
  UINT64            block;                      //0x00
  UINT64            block_id;
  unsigned int      length;
  unsigned int      flags;
}__attribute__((packed));


struct apfs_fusion_list
{
  apfs_block_header         header;             //0x00 //Standart block header
  UINT64                    flags;              //0x20 //0x40
  UINT64                    block_id;           //0x28
  unsigned int              unknown_0x30;       //0x30 //0 ...
  unsigned int              number_of_entries;  //0x34
  UINT64                    id;                 //0xx38 //0xa8
  apfs_fusion_list_entry    entry[1];           //0x40
}__attribute__((packed));


#define APFS_TABLE_HAS_FOOTER                   1
#define APFS_TABLE_LEAF_NODE                    2
#define APFS_TABLE_FIXED_ENTRY_SIZE             4
#define APFS_TABLE_MASK                         7

#define APFS_INDEX_AREA_MULTIPLIER              0x40

#define EMPTY_OFFSET                            0xffff

//empty list in key or data area
struct apfs_empty_entry
{
  unsigned short next_entry_offset;             //EMPTY_OFFSET - EOL
  unsigned short current_entry_size;
}__attribute__((packed));

#define MIN_EMPTY_ENTRY_SIZE                    sizeof(apfs_empty_entry)

//Common table header
struct apfs_table_header
{
  unsigned short    flags;                      //0x00 // Table flags
  unsigned short    level;                      //0x02 // Level of B-tree
  unsigned int      count;                      //0x04 // Items count
  unsigned short    unknown_0x08;               //0x08 // 0 ? may be size of some reserved area
  unsigned short    index_area_size;            //0x0A // Size of table index area 0x40 and multiplicity 0x40
  unsigned short    key_area_size;              //0x0C // Size of table key area
  unsigned short    free_area_size;             //0x0E // Size of table free area. Data area ends at offset sizeof(apfs_block_header) + sizeof (apfs_table_header) + sum of area sizes

  unsigned short    empty_key_offset;           //0x10 //offset to first free entry in list
  unsigned short    empty_key_size;             //0x12 //size of all free entries
  unsigned short    empty_data_offset;          //0x14 //offset to first free entry in list
  unsigned short    empty_data_size;            //0x16 //size of all free entries
}__attribute__((packed));


//Table index with various size
struct apfs_table_var_item
{
  unsigned short    key_offset;                 //0x00 //Offset of key from start of key area
  unsigned short    key_size;                   //0x02 //Size of key item
  unsigned short    data_offset;                //0x04 //Offset of data from end of the table or from footer
  unsigned short    data_size;                  //0x06 //Size of data item
}__attribute__((packed));


//Table index with fixed size
struct apfs_table_fixed_item
{
  unsigned short    key_offset;                 //0x00 //Offset of key from start of key area
  unsigned short    data_offset;                //0x02 //Offset of data from and of the table or from footer
}__attribute__((packed));


//apfs table footer structure
struct apfs_table_footer
{
  unsigned int  tf_flags;                       //0x00 //some or flags that depended of block content_type ? may be rebalansing treshold
  unsigned int  tf_block_size;                  //0x04 //0x1000
  unsigned int  tf_key_size;                    //0x08 //0-if variable size
  unsigned int  tf_data_size;                   //0x0c //0-if variable size
  unsigned int  tf_max_key_size;                //0x10 //max key size
  unsigned int  tf_max_data_size;               //0x14 //max data size
  UINT64        tf_count;                       //0x18 //records number in the table and underlaying tables
  UINT64        tf_number_of_blocks;            //0x20 //total number of blocks in all levels of Btree
}__attribute__((packed));


//apfs table structure
struct apfs_table
{
  apfs_block_header header;                     //0x00 //Common header of blocks
  apfs_table_header table_header;               //0x20 //Table header
  //indexes array. apfs_table_var_item struct or apfs_table_fixed_item for fVarEntry = (pTab->type & 4) == 0;
  //keys array
  //... free space
  //data array
  //table footer
}__attribute__((packed));


//table fixed size entry {{{
struct apfs_location_table_key
{
  UINT64            ltk_id;
  UINT64            ltk_checkpoint;
}__attribute__((packed));

struct apfs_location_table_data
{
  unsigned int      ltd_flags;                  //1- checkpoint in block differ from key; 4- encrypted
  unsigned int      ltd_length;                 //block size
  UINT64            ltd_block;
}__attribute__((packed));


struct apfs_history_table_key
{
  UINT64            htk_checkpoint;
  UINT64            htk_block;
}__attribute__((packed));

struct apfs_history_table_data
{
  unsigned int      htd_number_of_blocks;
  unsigned int      htd_unknown_0x04;
}__attribute__((packed));


struct apfs_snapshot_map_key
{
  UINT64            checkpoint;
}__attribute__((packed));

struct apfs_snapshot_map_data
{
  UINT64            unknown_0x00;               //0
  UINT64            unknown_0x08;               //0
}__attribute__((packed));

struct apfs_fusion_key
{
  UINT64            block_id;
}__attribute__((packed));

struct apfs_fusion_data
{
  UINT64            block;                      //mapped extent block
  unsigned int      length;                     //extent length
  unsigned int      flags;                      //0-extent correct/ 1-see extent in fusion map list
}__attribute__((packed));
//table fixed size entry }}}

//table variable size entry {{{

//record type for variable entry
enum NodeRecordType
{
  ApfsSnapshotCheckpoint = 0x1,
  ApfsVolumeExtent       = 0x2,
  ApfsInode              = 0x3,                 // File/Folder record
  ApfsAttribute          = 0x4,
  ApfsHardlink           = 0x5,
  ApfsExtentStatus       = 0x6,                 // Extent status. If this record exists, the file object has record(s) int the Extent B-Tree
  ApfsEncryption         = 0x7,                 //
  ApfsExtent             = 0x8,
  ApfsDirEntry           = 0x9,
  ApfsDirStats           = 0xA,                 // dir size ???
  ApfsSnapshotName       = 0xB,
  ApfsHardLinkId         = 0xC,                 // Hard link Id with back reference to inode
  ApfsTypeMaxValid       = 0xC
};

#define TYPE_SHIFT      60

struct apfs_volume_extent_key
{
  UINT64            id;                         //first block
}__attribute__((packed));

struct apfs_volume_extent_data
{
#ifndef BASE_BIGENDIAN
  UINT64            ved_number_of_blocks : 56;  //
  UINT64            ved_flags            :  8;  //0x10,0x20
#else
  UINT64            ved_flags            :  8;  //0x10,0x20
  UINT64            ved_number_of_blocks : 56;  //
#endif
  UINT64            ved_inode;                  //extent owner inode
  unsigned int      ved_extent_links;           //number of links to this extent
}__attribute__((packed));


struct apfs_snapshot_checkpoint_key
{
  UINT64            id;                         //checkpoint
}__attribute__((packed));

struct apfs_snapshot_checkpoint_data
{
  UINT64            extent_block;
  UINT64            vsb_block;
  apfs_time         creation_time;
  apfs_time         modification_time;
  UINT64            next_cnid;
  UINT64            id_type;
  unsigned short    name_length;
  char              snapshot_name[1];
}__attribute__((packed));


struct apfs_snapshot_name_key
{
  UINT64            id;
  unsigned short    name_length;
  char              snapshot_name[1];
}__attribute__((packed));

struct apfs_snapshot_name_data
{
  UINT64            checkpoint;
}__attribute__((packed));


struct apfs_direntry_key
{
  UINT64            id;
#ifndef BASE_BIGENDIAN
  unsigned int      name_len  : 10;
  unsigned int      name_hash : 22;
#else
  unsigned int      name_hash : 22;
  unsigned int      name_len  : 10;
#endif
  unsigned char     name[MAX_FILENAME];
}__attribute__((packed));

enum ApfsEntryType                              //bits from unix file mode
{
  ApfsFifo          = 001,
  ApfsBlockDev      = 002,
  ApfsSubdir        = 004,
  ApfsCharDev       = 006,
  ApfsFile          = 010,
  ApfsSymlink       = 012,
  ApfsSocket        = 014
};

struct apfs_direntry_data
{
  UINT64            id;
  apfs_time         timestamp;
  unsigned short    type;                       // ApfsEntryType
}__attribute__((packed));


struct apfs_hlink_key
{
  UINT64            id;
  UINT64            hlink_id;
}__attribute__((packed));

struct apfs_hlink_data
{
  UINT64            id;                         // Item ID or Parent ID
  unsigned short    name_len;
  unsigned char     name[1];
}__attribute__((packed));


struct apfs_extent_key
{
  UINT64            id;                         //0x00 - extent Id
  UINT64            file_offset;                //0x08 - extent offset from begin of file
}__attribute__((packed));

#define EXTENT_FLAG_ENCRYPTED   1

struct apfs_extent_data
{
#ifndef BASE_BIGENDIAN
  UINT64            ed_size  : 56;              //
  UINT64            ed_flags :  8;              //EXTENT_FLAG_ XXX
#else
  UINT64            ed_flags :  8;              //EXTENT_FLAG_ XXX
  UINT64            ed_size  : 56;              //
#endif
  UINT64            ed_block;
  UINT64            ed_crypto_id;               //crypto_id = ed_crypto_id + read offset from begin of extent
}__attribute__((packed));


//#define APFS_DREC_EXT_SIBLING_ID        0x1
//#define APFS_INO_EXT_TYPE_UNKNOWN_x2    0x2
//#define APFS_INO_EXT_TYPE_DOCUMENT_ID   0x3 +
//#define APFS_INO_EXT_TYPE_NAME          0x4 +
//#define APFS_INO_EXT_TYPE_UNKNOWN_x5    0x5
//#define APFS_INO_EXT_TYPE_UNKNOWN_x6    0x6
//#define APFS_INO_EXT_TYPE_UNKNOWN_x7    0x7
//#define APFS_INO_EXT_TYPE_DSTREAM       0x8 +
//#define APFS_INO_EXT_TYPE_UNKNOWN_x9    0x9
//#define APFS_INO_EXT_TYPE_DIR_STATS_KEY 0xA
//#define APFS_INO_EXT_TYPE_FS_UUID       0xB
//#define APFS_INO_EXT_TYPE_UNKNOWN_xC    0xC
//#define APFS_INO_EXT_TYPE_SPARSE_BYTES  0xD +
//#define APFS_INO_EXT_TYPE_DEVICE        0xE +

#define INODE_FTYPE_DOC_ID       0x03
#define INODE_FTYPE_NAME         0x04
#define INODE_FTYPE_05           0x05
#define INODE_FTYPE_SIZE         0x08
#define INODE_FTYPE_SPARSE_BYTES 0x0d
#define INODE_FTYPE_DEVICE       0x0e

#define INODE_FFLAG_00           0x00
#define INODE_FFLAG_02           0x02            //field real used size is aligned to 8
#define INODE_FFLAG_08           0x08
#define INODE_FFLAG_20           0x20            //

#define INODE_FIELD_NAME            ((INODE_FFLAG_02<<8)                  |INODE_FTYPE_NAME)           //0x0204 asciiz
#define INODE_FIELD_SIZE            ((INODE_FFLAG_20<<8)                  |INODE_FTYPE_SIZE)           //0x2008 apfs_data_size
#define INODE_FIELD_SPARSE_BYTES    (((INODE_FFLAG_20|INODE_FFLAG_08)<<8) |INODE_FTYPE_SPARSE_BYTES)   //0x280d UINT64
#define INODE_FIELD_DOC_ID          ((INODE_FFLAG_02<<8)                  |INODE_FTYPE_DOC_ID)         //0x0203|0x2203 unsigned int
#define INODE_FIELD_DEVICE          (((INODE_FFLAG_20|INODE_FFLAG_02)<<8) |INODE_FTYPE_DEVICE)         //0x220e unsigned int
#define INODE_FIELD_0005            ((INODE_FFLAG_00<<8)                  |INODE_FTYPE_05)             //0x0005 UINT64

#define APFS_DEVICE_SPEC_MAJOR(x) (((x) >> 24) & 0xff)
#define APFS_DEVICE_SPEC_MINOR(x) ((x) & 0xffffff)

struct apfs_inode_field
{
  union {
    unsigned short    if_field;                 //INODE_FIELD_ XXX
    unsigned char     if_ftype;                 //INODE_FTYPE_ XXX
    //unsigned char     if_fflags;                //INODE_FFLAG_ XXX
  };
  unsigned short    if_size;                    //size of inode field
}__attribute__((packed));


//BSD File Flags
//UF_NODUMP     0x1
//UF_IMMUTABLE  0x2
//UF_APPEND     0x4
//UF_OPAQUE     0x8
//UF_NOUNLINK   0x10
//UF_COMPRESSED 0x20
//UF_TRACKED    0x40            /* send an event to a tracked file handler in user mode on any change to the file's dentry */
//UF_DATAVAULT  0x80            /* entitlement required for reading and writing */
//UF_HIDDEN     0x8000

//SF_ARCHIVED   0x10000
//SF_IMMUTABLE  0x20000
//SF_APPEND     0x40000
//SF_RESTRICTED 0x80000         /* entitlement required for writing */
//SF_NOUNLINK   0x100000
//SF_SNAPSHOT   0x200000

#define FS_UFLAG_IMMUTABLE      0x000002
#define FS_UFLAG_COMPRESSED     0x000020
#define FS_UFLAG_TRACKED        0x000040
#define FS_UFLAG_DATAVAULT      0x000080
#define FS_UFLAG_HIDDEN         0x008000
#define FS_SFLAG_RESTRICTED     0x080000
#define FS_SFLAG_UNLINK         0x100000
#define FS_FLAG_MASK            0x1880e2

//inode flags
#define FS_IFLAG_0008           0x0008          //??? 0x8008
#define FS_IFLAG_FULL_CLONED    0x0010          //data or EA extents is cloned (full cloned)
#define FS_IFLAG_SECURITY       0x0040          //EA system.Security is present
#define FS_IFLAG_0080           0x0080          //???
#define FS_IFLAG_FINDER_INFO    0x0100          //EA FinderInfo is present
#define FS_IFLAG_SPARSE         0x0200          //file has sparse data
#define FS_IFLAG_CLONED         0x0400          //??? 0x84xx some part of data cloned
#define FS_IFLAG_FORK           0x4000          //EA ResourceFork is present
#define FS_IFLAG_DEFAULT        0x8000          //default flag
#define FS_IFLAG_FILESIZE       0x40000         //flag for file size in inode ai_file_size

#define FS_IFLAG_MASK           0x4c7d8

//system extended attributes
#define XATTR_SYMLINK           "com.apple.fs.symlink"
#define XATTR_FORK              "com.apple.ResourceFork"
#define XATTR_COMPRESSED        "com.apple.decmpfs"
#define XATTR_SECURITY          "com.apple.system.Security"
#define XATTR_FINDER_INFO       "com.apple.FinderInfo"

#define XATTR_QUARANTINE        "com.apple.quarantine"
#define XATTR_ROOTLESS          "com.apple.rootless"

#define XATTR_SYMLINK_LEN       sizeof(XATTR_SYMLINK) - 1
#define XATTR_FORK_LEN          sizeof(XATTR_FORK) - 1
#define XATTR_COMPRESSED_LEN    sizeof(XATTR_COMPRESSED) - 1
#define XATTR_SECURITY_LEN      sizeof(XATTR_SECURITY) - 1
#define XATTR_FINDER_LEN        sizeof(XATTR_FINDER_INFO) - 1

struct apfs_inode
{
  UINT64            ai_parent_id;               //0x00
  UINT64            ai_extent_id;               //0x08 - extent id (may be differ from own id)
  apfs_time         ai_crtime;                  //0x10 - create time
  apfs_time         ai_mtime;                   //0x18 - write time
  apfs_time         ai_ctime;                   //0x20 - inode modify time
  apfs_time         ai_atime;                   //0x28 - last access time
  unsigned int      ai_inode_flags;             //0x30 - FS_IFLAG_ XXX
  unsigned int      ai_reserved;                //0x34 - 0
  union                                         //0x38 - for file number of hard links, for subdirectory number of childs
  {
    unsigned int    ai_nchildren;               //     - number of directory entries
    unsigned int    ai_nlinks;                  //     - number of hard links whose target is this inode
  };
  unsigned int      ai_unknown_0x3c;            //0x3c - (0-6) some flags
  unsigned int      ai_unknown_0x40;            //0x40 - some random value
  unsigned int      ai_fs_flags;                //0x44 - FS_FLAG_ XXX FileInfo->FSAttrib
  unsigned int      ai_uid;                     //0x48 - uid
  unsigned int      ai_gid;                     //0x4C - gid
  unsigned int      ai_mode;                    //0x50 - mode
  UINT64            ai_file_size;               //0x54 - 0 or FileSize for compressed file if flag FS_IFLAG_FILESIZE is set

  unsigned short    ai_number_of_fields;        //0x5C - number of data fields (field array size)
  unsigned short    ai_fields_data_size;        //0x5e - size of fields data

  apfs_inode_field  field[1];                   //0x60 - array that describe the inode data fields
  //idode data fields
}__attribute__((packed));
C_ASSERT(sizeof(apfs_inode) == 0x64);

struct apfs_data_size
{
  UINT64            ds_bytes;                   //0x00 - file size in bytes
  UINT64            ds_blocks;                  //0x08 - file size in blocks
  UINT64            ds_flags;                   //0x10 - 4-encrypted
  UINT64            ds_written;                 //0x18 - bytes written
  UINT64            ds_read;                    //0x20 - bytes read
}__attribute__((packed));


#define XATTR_DATA_TYPE_EXTENT  1
#define XATTR_DATA_TYPE_EXTENT1 0x11
#define XATTR_DATA_TYPE_BINARY  2
#define XATTR_DATA_TYPE_STRING  6

#define APFS_MAX_XATTR_NAME_LEN 127
#define APFS_MAX_XATTR_INLINE_SIZE 1023


typedef apfs_hlink_data apfs_xattr_key;

struct apfs_xattr_data
{
  unsigned short    xd_type;                    // XATTR_DATA_TYPE_ XXX
  unsigned short    xd_len;
  char              xd_data[1];
}__attribute__((packed));

struct apfs_xattr_data_extent                   // for fork & compressed data
{
  UINT64            extent_id;
  apfs_data_size    ds;
}__attribute__((packed));

//table variable size entry }}}


union apfs_key
{
  UINT64                        id;
  apfs_direntry_key             direntry;
  apfs_hlink_key                hlink;
  apfs_xattr_key                xattr;
  apfs_extent_key               extent;
  apfs_volume_extent_key        vextent;

  apfs_snapshot_checkpoint_key  snapshot_cp;
  apfs_snapshot_name_key        snapshot_name;

  apfs_location_table_key       location;
  apfs_history_table_key        history;
};

union apfs_data
{
  UINT64                        tree_link;      //id of next level tree node
  apfs_inode                    inode;
  apfs_direntry_data            direntry;
  apfs_hlink_data               hlink;
  apfs_xattr_data               xattr;
  apfs_extent_data              extent;
  apfs_volume_extent_data       vextent;

  apfs_snapshot_checkpoint_data snapshot_cp;
  apfs_snapshot_name_data       snapshot_name;

  apfs_location_table_data      location;
  apfs_history_table_data       history;
};

//disk structure of XATTR_COMPRESSED attribute value
#define APFS_COMPRESSED_ATTR_MAGIC                  0x636d7066  /* 'fpmc' */

// storage and compression options
enum StorageCompressionOptions
{
  DecmpfsInlineZlibData = 0x3,
  ResourceForkZlibData  = 0x4,
  DecmpfsVersion        = 0x5, //psevdo compressed file
  DecmpfsInlineLZData   = 0x7,
  ResourceForkLZData    = 0x8
};

struct apfs_compressed_attr
{
  unsigned int magic;        //magic number, always "fpmc"
  unsigned int type;         //attribute type (StorageCompressionOptions)
  UINT64 data_size;          //size of this fs object data (uncompressed)
}__attribute__((packed));

#define APFS_LZFSE_ENDOFSTREAM_BLOCK_MAGIC      0x24787662   /* bvx$ (end of stream) */
#define APFS_LZFSE_COMPRESSEDLZVN_BLOCK_MAGIC   0x6e787662   /* bvxn (lzvn compressed) */
#define APFS_LZFSE_UNCOMPRESSED_BLOCK_MAGIC     0x2d787662   /* bvx- (raw data) */
#define APFS_LZFSE_UNCOMPRESSED_DATA            6            /* in start of block in xattr */
#define APFS_UNCOMPRESS_BUFFER_SIZE             0x10000      /* max size of uncompressed buffer (64 kb) */

struct apfs_lzvn_compressed_block_header
{
  unsigned int magic;           // APFS_LZFSE_COMPRESSEDLZVN_BLOCK_MAGIC (bvxn)
  unsigned int n_raw_bytes;     // Number of decoded (output) bytes.
  unsigned int n_payload_bytes; // Number of encoded (source) bytes.
}__attribute__((packed));

struct apfs_lzvn_uncompressed_block_header
{
  unsigned int magic;       // APFS_LZFSE_UNCOMPRESSED_BLOCK_MAGIC (bvx$)
  unsigned int n_raw_bytes; // Number of bytes in block
}__attribute__((packed));

struct apfs_compressed_resource_header
{
  unsigned int header_size;         // always 0x100
  unsigned int footer_offset;
  unsigned int data_size;           // = footer_offset - header_size
  unsigned int footer_size;         // always 0x32
  unsigned char reserved[0xf0];     // zero-filled
}__attribute__((packed));

struct apfs_compressed_block
{
  unsigned int offset;
  unsigned int size;
}__attribute__((packed));

// this structure is followed by array of apfs_compressed_block
// that contains data_table_num_entries elements
struct apfs_zlib_compressed_resource_fork
{
  apfs_compressed_resource_header header;
  unsigned int data_table_size;
  unsigned int data_table_num_entries;
}__attribute__((packed));


///////////////////////////////////////////////////////////////////////////////
//encrypted blocks structures
struct apfs_block_header_enc
{
  UINT64            checksum;                   //0x00 //Fletchers Checksum Algorithm
  UINT64            id;                         //0x08 //0
  UINT64            checkpoint_id;              //0x10 //id of checkpoint this block belongs to
  unsigned int      block_type;                 //0x18 //encrypted block type
  unsigned int      block_subtype;              //0x1C //0
}__attribute__((packed));


#define APFS_TYPE_KEYBAG    0x6b657973 //'syek'
#define APFS_TYPE_KEYRECS   0x72656373 //'scer'
#define KEYBAG_VERSION      2

struct apfs_keybag_hdr
{
   unsigned short version;                      //KEYBAG_VERSION
   unsigned short number_of_keys;
   unsigned int   number_of_bytes;              //all key length including this header
   UINT64         padding;
}__attribute__((packed));

struct apfs_key_hdr
{
  unsigned char  uuid[0x10];
  unsigned short type;
  unsigned short length;                        //key length
  unsigned int   padding;
  //key data depend of type rounded to 0x10
}__attribute__((packed));

struct apfs_blob                                //keybag type 2 VEK 0x7c or keyrec type 3 KEK 0x94
{
  apfs_key_hdr      hdr;
  unsigned char     blob[1];
}__attribute__((packed));

struct apfs_key_extent                          //keybag type 3 keybag extent
{
  apfs_key_hdr   hdr;
  UINT64         block;
  UINT64         count;
}__attribute__((packed));

struct apfs_key_hint                            //keyrec type 4 password hint
{
  apfs_key_hdr   hdr;
  char           name[1];                       //name chars without zero end
}__attribute__((packed));

union apfs_keys
{
  apfs_key_hdr          hdr;

  apfs_blob             blob;
  apfs_key_extent       key_extent;
  apfs_key_hint         key_hint;
};

struct apfs_keybag
{
  apfs_block_header_enc header;                 //0x00 //Encrypted block header
  apfs_keybag_hdr       keybag_hdr;             //0x20 //keybag header
  apfs_keys             keys;                   //0x30 //list of keys
}__attribute__((packed));

//Disk structures for blob

//definitions for encryption keys in keybags
#define APFS_ENCRKEY_TYPE_VEK_BLOB          2
#define APFS_ENCRKEY_TYPE_RECS_BAG_EXTENT   3
#define APFS_ENCRKEY_TYPE_KEK_BLOB          3
#define APFS_ENCRKEY_TYPE_HINT              4
#define APFS_ENCRKEY_LEN_MASK               0xf

//encryption tags (record types)
#define APFS_BLOB_HEADER_TAG                0x30
#define APFS_BLOB_TAG_80                    0x80
#define APFS_BLOB_HEADER_HMAC_TAG           0x81
#define APFS_BLOB_HEADER_SALT_TAG           0x82
#define APFS_BLOB_DATA_TAG                  0xA3
#define APFS_BLOB_UUID_TAG                  0x81
#define APFS_BLOB_TAG_82                    0x82
#define APFS_BLOB_WRAPPED_TAG               0x83
#define APFS_BLOB_ITERATIONS_TAG            0x84
#define APFS_BLOB_SALT_TAG                  0x85

//definitions for vek calculations
#define APFS_ENCRYPTION_RECORD_LEN_MASK     0x7F
#define APFS_ENCRYPT_KEY_SIZE               0x20
#define RFC_3394_DEFAULT_IV                 0xA6A6A6A6A6A6A6A6ULL

#define APFS_BLOB_AES256                     0x0
#define APFS_BLOB_AES128                     0x2
#define APFS_BLOB_UNK82_10                  0x10

//definitions for encryption/decryption
#define APFS_ENCRYPT_PORTION                0x200
#define APFS_ENCRYPT_PORTION_LOG            9

//encryption entry (entries stored in kek blob and vek blob)
struct apfs_blob_entry
{
  unsigned char tag;
  unsigned char len;
  //if len stored in several bytes than len bytes
  //value (array of unsigned char)
}__attribute__((packed));

//blob key parsed structures
struct apfs_blob_header_t                       //type 0x30
{
  UINT64                tag80;                  //type 0x80
  unsigned char         hmac[0x20];             //type 0x81
  unsigned char         salt[0x08];             //type 0x82
};

struct apfs_type_0x82                           //type 0x82
{
  unsigned int      unk82_00;
  unsigned short    unk82_04;
  unsigned char     unk82_06;
  unsigned char     unk82_07;
}__attribute__((packed));

struct apfs_vek                                 //type 0xa3
{
  UINT64            tag80;                      //type 0x80
  unsigned char     uuid[0x10];                 //type 0x81
  apfs_type_0x82    tag82;                      //type 0x82
  unsigned char     wrapped_vek[0x28];          //type ox83
}__attribute__((packed));

struct apfs_kek                                 //type 0xa3
{
  UINT64            tag80;                      //type 0x80
  unsigned char     uuid[0x10];                 //type 0x81
  apfs_type_0x82    tag82;                      //type 0x82
  unsigned char     wrapped_kek[0x28];          //type 0x83
  UINT64            iterations;                 //type 0x84
  unsigned char     salt[0x10];                 //type 0x85
}__attribute__((packed));


#ifndef NO_PRAGMA_PACK
#pragma pack(pop)
#endif

#endif   // __UFSD_APFS_STRUCT_H
