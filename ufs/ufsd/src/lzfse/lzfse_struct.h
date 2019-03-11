////////////////////////////////////////////////////////////////
//
// Abstract:
//
//    ICompress interface for lzfse compression
//
//    Based on Apple Inc. implementation
//
// Revision History:
//
//    17-November-2017 - Created
//
////////////////////////////////////////////////////////////////
/*
Copyright (c) 2015-2016, Apple Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1.  Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2.  Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer
    in the documentation and/or other materials provided with the distribution.

3.  Neither the name of the copyright holder(s) nor the names of any contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef __LZFSE_STRUCT
#define __LZFSE_STRUCT

#include <api/types.hpp>

#define LZFSE_NO_BLOCK_MAGIC             0x00000000 // 0    (invalid)
#define LZFSE_ENDOFSTREAM_BLOCK_MAGIC    0x24787662 // bvx$ (end of stream)
#define LZFSE_UNCOMPRESSED_BLOCK_MAGIC   0x2d787662 // bvx- (raw data)
#define LZFSE_COMPRESSEDV1_BLOCK_MAGIC   0x31787662 // bvx1 (lzfse compressed, uncompressed tables)
#define LZFSE_COMPRESSEDV2_BLOCK_MAGIC   0x32787662 // bvx2 (lzfse compressed, compressed tables)
#define LZFSE_COMPRESSEDLZVN_BLOCK_MAGIC 0x6e787662 // bvxn (lzvn compressed)

#ifndef HAVE_LABELS_AS_VALUES
#  if defined __GNUC__ || defined __clang__
#    define HAVE_LABELS_AS_VALUES 1
#  else
#    define HAVE_LABELS_AS_VALUES 0
#  endif
#endif

#define LZFSE_ENCODE_L_STATES 64
#define LZFSE_ENCODE_M_STATES 64
#define LZFSE_ENCODE_D_STATES 256
#define LZFSE_ENCODE_LITERAL_STATES 1024
#define LZFSE_MATCHES_PER_BLOCK 10000
#define LZFSE_LITERALS_PER_BLOCK (4 * LZFSE_MATCHES_PER_BLOCK)

typedef struct
{
  size_t          accum;            // Input bits
  int             accum_nbits;      // Number of valid bits in ACCUM, other bits are 0
} fse_in_stream;

typedef struct
{      // DO NOT REORDER THE FIELDS
  unsigned char total_bits;         // state bits + extra value bits = shift for next decode
  unsigned char value_bits;         // extra value bits
  short         delta;              // state base (delta)
  int           vbase;              // value base
} fse_value_decoder_entry;

typedef struct
{
  unsigned int          n_matches;            //  Number of matches remaining in the block.
  unsigned int          n_lmd_payload_bytes;  //  Number of bytes used to encode L, M, D triplets for the block.
  const unsigned char*  current_literal;      //  Pointer to the next literal to emit.

  //  L, M, D triplet for the match currently being emitted. This is used only
  //  if we need to restart after reaching the end of the destination buffer in
  //  the middle of a literal or match.
  unsigned int          l_value;
  unsigned int          m_value;
  unsigned int          d_value;

  fse_in_stream         lmd_in_stream;        //  FSE stream object.
  //  Offset of L,M,D encoding in the input buffer. Because we read through an
  //  FSE stream *backwards* while decoding, this is decremented as we move
  //  through a block.
  unsigned int          lmd_in_buf;
  //  The current state of the L, M, and D FSE decoders.
  unsigned short        l_state;
  unsigned short        m_state;
  unsigned short        d_state;
  //  Internal FSE decoder tables for the current block. These have
  //  alignment forced to 8 bytes to guarantee that a single state's
  //  entry cannot span two cachelines.
  fse_value_decoder_entry l_decoder[LZFSE_ENCODE_L_STATES] __attribute__((__aligned__(8)));
  fse_value_decoder_entry m_decoder[LZFSE_ENCODE_M_STATES] __attribute__((__aligned__(8)));
  fse_value_decoder_entry d_decoder[LZFSE_ENCODE_D_STATES] __attribute__((__aligned__(8)));
  int                   literal_decoder[LZFSE_ENCODE_LITERAL_STATES];
  //  The literal stream for the block, plus padding to allow for faster copy
  //  operations.
  unsigned char         literals[LZFSE_LITERALS_PER_BLOCK + 64];
} lzfse_compressed_block_decoder_state;

typedef struct
{
  unsigned int          n_raw_bytes;
} uncompressed_block_decoder_state;

typedef struct
{
  unsigned int          n_raw_bytes;
  unsigned int          n_payload_bytes;
  unsigned int          d_prev;
} lzvn_compressed_block_decoder_state;

typedef struct
{
  const unsigned char*  src;
  const unsigned char*  src_begin;
  const unsigned char*  src_end;
  unsigned char*        dst;
  unsigned char*        dst_begin;
  unsigned char*        dst_end;
  bool                  end_of_stream;

  unsigned int          block_magic;

  lzfse_compressed_block_decoder_state  compressed_lzfse_block_state;
  lzvn_compressed_block_decoder_state   compressed_lzvn_block_state;
  uncompressed_block_decoder_state      uncompressed_block_state;
} lzfse_decoder_state;

typedef struct
{
  const unsigned char*  src;
  const unsigned char*  src_end;
  unsigned char*        dst;
  unsigned char*        dst_begin;
  unsigned char*        dst_end;
  unsigned char*        dst_current;

  // Partially expanded match, or 0,0,0.
  // In that case, src points to the next literal to copy, or the next op-code
  // if L==0.
  size_t                L;
  size_t                M;
  size_t                D;
  INT64                 d_prev;           // Distance for last emitted match, or 0
  bool                  end_of_stream;
} lzvn_decoder_state;


typedef struct
{
  unsigned int          magic;            //  Magic number, always LZFSE_UNCOMPRESSED_BLOCK_MAGIC.
  unsigned int          n_raw_bytes;      //  Number of raw bytes in block.
} uncompressed_block_header;

typedef struct
{
  unsigned int          magic;            //  Magic number, always LZFSE_COMPRESSEDLZVN_BLOCK_MAGIC.
  unsigned int          n_raw_bytes;      //  Number of decoded (output) bytes.
  unsigned int          n_payload_bytes;  //  Number of encoded (source) bytes.
} lzvn_compressed_block_header;

#endif
