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

#include "../h/versions.h"

#ifdef UFSD_LZFSE

#ifdef UFSD_TRACE_ERROR
static const char s_pFileName[] = __FILE__ ",$Revision: 331868 $";
#endif

#include <ufsd.h>

#include "../h/uerrors.h"
#include "../h/assert.h"
#include "../h/utrace.h"
#if defined BASE_BIGENDIAN || !defined UFSD_APFS_RO
#include "../h/uswap.h"
#endif

#include "ulzfse.h"

namespace UFSD {


/////////////////////////////////////////////////////////////////////////////
int CLzfseCompression::Decompress(
  IN  const void* InBuf,
  IN  size_t      InBufLen,
  OUT void*       OutBuf,
  IN  size_t      OutBufLen,
  OUT size_t*     OutSize
  )
{
  void* pBuffer = Malloc2(sizeof(lzfse_decoder_state) + 1);
  CHECK_PTR(pBuffer);

  lzfse_decoder_state* s = (lzfse_decoder_state *)pBuffer;
  Memzero2(s, sizeof(lzfse_decoder_state));

  s->src = s->src_begin = (unsigned char*)InBuf;
  s->src_end = s->src + InBufLen;
  s->dst = s->dst_begin = (unsigned char*)OutBuf;
  s->dst_end = s->dst + OutBufLen;

  size_t Bytes = 0;
  int Status = Decode(s);

  if (Status == ERR_MORE_DATA)
    Bytes = OutBufLen;
  else if (UFSD_SUCCESS(Status))
    Bytes = PtrOffset(OutBuf, s->dst);

  Free2(pBuffer);

  if (OutSize)
    *OutSize = Bytes;

  return Status;
}


/////////////////////////////////////////////////////////////////////////////
int CLzfseCompression::Decode(IN lzfse_decoder_state* State) const
{
  for (;;)
  {
    switch (State->block_magic)
    {
    case LZFSE_NO_BLOCK_MAGIC:
    {
      if (State->src + 4 > State->src_end)
        return ERR_BADPARAMS;

      unsigned int magic = Value4(State->src);

      if (magic == LZFSE_ENDOFSTREAM_BLOCK_MAGIC)
      {
        State->src += 4;
        State->end_of_stream = true;
        return ERR_NOERROR;
      }

      if (magic == LZFSE_UNCOMPRESSED_BLOCK_MAGIC)
      {
        if (State->src + sizeof(uncompressed_block_header) > State->src_end)
          return ERR_BADPARAMS;

        uncompressed_block_decoder_state *bs = &(State->uncompressed_block_state);
        bs->n_raw_bytes = Value4(State->src + offsetof(uncompressed_block_header, n_raw_bytes));
        State->src += sizeof(uncompressed_block_header);
        State->block_magic = magic;
        break;
      }

      if (magic == LZFSE_COMPRESSEDLZVN_BLOCK_MAGIC)
      {
        if (State->src + sizeof(lzvn_compressed_block_header) > State->src_end)
          return ERR_BADPARAMS;

        lzvn_compressed_block_decoder_state *bs = &(State->compressed_lzvn_block_state);
        bs->n_raw_bytes = Value4(State->src + offsetof(lzvn_compressed_block_header, n_raw_bytes));
        bs->n_payload_bytes = Value4(State->src + offsetof(lzvn_compressed_block_header, n_payload_bytes));
        bs->d_prev = 0;
        State->src += sizeof(lzvn_compressed_block_header);
        State->block_magic = magic;
        break;
      }

      assert(!"Invalid magic");
      return ERR_NOT_SUCCESS;
    }

    case LZFSE_UNCOMPRESSED_BLOCK_MAGIC:
    {
      uncompressed_block_decoder_state *bs = &(State->uncompressed_block_state);
      unsigned int copy_size = bs->n_raw_bytes;

      if (copy_size == 0)
      {
        State->block_magic = 0;
        break;
      } // end of block

      if (State->src_end <= State->src)
        return ERR_MORE_DATA; // need more SRC data

      const size_t src_space = State->src_end - State->src;

      if (copy_size > src_space)
        copy_size = static_cast<unsigned int>(src_space);

      if (State->dst_end <= State->dst)
        return ERR_MORE_DATA; // need more DST capacity

      const size_t dst_space = State->dst_end - State->dst;

      if (copy_size > dst_space)
        copy_size = static_cast<unsigned int>(dst_space);

      Memcpy2(State->dst, State->src, copy_size);
      State->src += copy_size;
      State->dst += copy_size;
      bs->n_raw_bytes -= copy_size;

      break;
    }

    case LZFSE_COMPRESSEDLZVN_BLOCK_MAGIC:
    {
      lzvn_compressed_block_decoder_state *bs = &(State->compressed_lzvn_block_state);

      if (bs->n_payload_bytes > 0 && State->src_end <= State->src)
        return ERR_MORE_DATA; // need more SRC data

      lzvn_decoder_state dstate;

      Memzero2(&dstate, sizeof(dstate));
      dstate.src = State->src;
      dstate.src_end = State->src_end;

      if (dstate.src_end > State->src + bs->n_payload_bytes)
        dstate.src_end = State->src + bs->n_payload_bytes;

      dstate.dst_begin = State->dst_begin;
      dstate.dst = State->dst;
      dstate.dst_end = State->dst_end;

      if (dstate.dst_end > State->dst + bs->n_raw_bytes)
        dstate.dst_end = State->dst + bs->n_raw_bytes;

      dstate.d_prev = bs->d_prev;
      dstate.end_of_stream = false;

      DecodeLZVN(&dstate);

      size_t src_used = dstate.src - State->src;
      size_t dst_used = dstate.dst - State->dst;

      if (src_used > bs->n_payload_bytes || dst_used > bs->n_raw_bytes)
        return ERR_BADPARAMS;

      State->src = dstate.src;
      State->dst = dstate.dst;
      bs->n_payload_bytes -= static_cast<unsigned int>(src_used);
      bs->n_raw_bytes -= static_cast<unsigned int>(dst_used);
      bs->d_prev = static_cast<unsigned int>(dstate.d_prev);

      if (bs->n_payload_bytes == 0 && bs->n_raw_bytes == 0 && dstate.end_of_stream)
      {
        State->block_magic = 0;
        break;
      }

      if (bs->n_payload_bytes == 0 || bs->n_raw_bytes == 0 || dstate.end_of_stream)
        return ERR_BADPARAMS;

      // block is not done and State is valid, so we need more space in dst.
      return ERR_INSUFFICIENT_BUFFER;
    }

    default:
      assert(!"Invalid magic");
      return ERR_BADPARAMS;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////
static size_t extract(size_t x, unsigned int lsb, unsigned int width)
{
  static const size_t container_width = sizeof(x) << 3;
  assert(lsb < container_width);
  assert(width > 0 && width <= container_width);
  assert(lsb + width <= container_width);
  if (width == container_width)
    return x;
  return (x >> lsb) & (((size_t)1 << width) - 1);
}

void CLzfseCompression::DecodeLZVN(IN lzvn_decoder_state* State) const
{
#if HAVE_LABELS_AS_VALUES
  static const void *opc_tbl[256] = {
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&eos,   &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&nop,   &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&nop,   &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&udef,  &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&udef,  &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&udef,  &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&udef,  &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&udef,  &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&pre_d, &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&pre_d, &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&pre_d, &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&pre_d, &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&pre_d, &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&pre_d, &&lrg_d,
    &&udef,  &&udef,  &&udef,  &&udef,  &&udef,  &&udef,  &&udef,  &&udef,
    &&udef,  &&udef,  &&udef,  &&udef,  &&udef,  &&udef,  &&udef,  &&udef,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&pre_d, &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&pre_d, &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&pre_d, &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&pre_d, &&lrg_d,
    &&med_d, &&med_d, &&med_d, &&med_d, &&med_d, &&med_d, &&med_d, &&med_d,
    &&med_d, &&med_d, &&med_d, &&med_d, &&med_d, &&med_d, &&med_d, &&med_d,
    &&med_d, &&med_d, &&med_d, &&med_d, &&med_d, &&med_d, &&med_d, &&med_d,
    &&med_d, &&med_d, &&med_d, &&med_d, &&med_d, &&med_d, &&med_d, &&med_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&pre_d, &&lrg_d,
    &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&sml_d, &&pre_d, &&lrg_d,
    &&udef,  &&udef,  &&udef,  &&udef,  &&udef,  &&udef,  &&udef,  &&udef,
    &&udef,  &&udef,  &&udef,  &&udef,  &&udef,  &&udef,  &&udef,  &&udef,
    &&lrg_l, &&sml_l, &&sml_l, &&sml_l, &&sml_l, &&sml_l, &&sml_l, &&sml_l,
    &&sml_l, &&sml_l, &&sml_l, &&sml_l, &&sml_l, &&sml_l, &&sml_l, &&sml_l,
    &&lrg_m, &&sml_m, &&sml_m, &&sml_m, &&sml_m, &&sml_m, &&sml_m, &&sml_m,
    &&sml_m, &&sml_m, &&sml_m, &&sml_m, &&sml_m, &&sml_m, &&sml_m, &&sml_m };
#endif
  size_t SrcLen = State->src_end - State->src;
  size_t DstLen = State->dst_end - State->dst;

  if (SrcLen == 0 || DstLen == 0)
    return;

  const unsigned char* pSrc = State->src;
  unsigned char* pDst = State->dst;
  size_t D = State->d_prev;
  size_t M;
  size_t L;
  size_t OpcSize;
  unsigned char opc;

  if (State->L != 0 || State->M != 0)
  {
    L = State->L;
    M = State->M;
    D = State->D;
    OpcSize = 0;
    State->L = State->M = State->D = 0;

    if (M == 0)
      goto copy_literal;
    if (L == 0)
      goto copy_match;
    goto copy_literal_and_match;
  }

  opc = pSrc[0];

#if HAVE_LABELS_AS_VALUES
  goto *opc_tbl[opc];
#else
  for (;;)
  {
    switch (opc)
    {
#endif

#if HAVE_LABELS_AS_VALUES
      sml_d :
#else
    case 0: case 1: case 2: case 3: case 4: case 5:
    case 8: case 9: case 10: case 11: case 12: case 13:
    case 16: case 17: case 18: case 19: case 20: case 21:
    case 24: case 25: case 26: case 27: case 28: case 29:
    case 32: case 33: case 34: case 35: case 36: case 37:
    case 40: case 41: case 42: case 43: case 44: case 45:
    case 48: case 49: case 50: case 51: case 52: case 53:
    case 56: case 57: case 58: case 59: case 60: case 61:
    case 64: case 65: case 66: case 67: case 68: case 69:
    case 72: case 73: case 74: case 75: case 76: case 77:
    case 80: case 81: case 82: case 83: case 84: case 85:
    case 88: case 89: case 90: case 91: case 92: case 93:
    case 96: case 97: case 98: case 99: case 100: case 101:
    case 104: case 105: case 106: case 107: case 108: case 109:
    case 128: case 129: case 130: case 131: case 132: case 133:
    case 136: case 137: case 138: case 139: case 140: case 141:
    case 144: case 145: case 146: case 147: case 148: case 149:
    case 152: case 153: case 154: case 155: case 156: case 157:
    case 192: case 193: case 194: case 195: case 196: case 197:
    case 200: case 201: case 202: case 203: case 204: case 205:
#endif
      State->src = pSrc, State->dst = pDst, State->d_prev = D;

      // small distance
      OpcSize = 2;
      L = static_cast<size_t>(extract(opc, 6, 2));
      M = static_cast<size_t>(extract(opc, 3, 3)) + 3;

      if (SrcLen <= OpcSize + L)
        return; // source truncated

      D = static_cast<size_t>(extract(opc, 0, 3)) << 8 | pSrc[1];
      goto copy_literal_and_match;

#if HAVE_LABELS_AS_VALUES
      med_d :
#else
    case 160: case 161: case 162: case 163: case 164: case 165: case 166: case 167: case 168: case 169:
    case 170: case 171: case 172: case 173: case 174: case 175: case 176: case 177: case 178: case 179:
    case 180: case 181: case 182: case 183: case 184: case 185: case 186: case 187: case 188: case 189:
    case 190: case 191:
#endif
    {
      State->src = pSrc, State->dst = pDst, State->d_prev = D;

      // medium distance
      OpcSize = 3;
      L = static_cast<size_t>(extract(opc, 3, 2));

      if (SrcLen <= OpcSize + L)
        return; // source truncated

      unsigned short opc23 = CPU2LE(Value2(&pSrc[1]));

      M = static_cast<size_t>((extract(opc, 0, 3) << 2 | extract(opc23, 0, 2)) + 3);
      D = static_cast<size_t>(extract(opc23, 2, 14));
      goto copy_literal_and_match;
    }

#if HAVE_LABELS_AS_VALUES
    lrg_d :
#else
    case 7: case 15: case 23: case 31: case 39: case 47: case 55: case 63: case 71: case 79: case 87:
    case 95: case 103: case 111: case 135: case 143: case 151: case 159: case 199: case 207:
#endif
      State->src = pSrc, State->dst = pDst, State->d_prev = D;

      // large distance
      OpcSize = 3;
      L = static_cast<size_t>(extract(opc, 6, 2));
      M = static_cast<size_t>(extract(opc, 3, 3)) + 3;

      if (SrcLen <= OpcSize + L)
        return; // source truncated

      D = CPU2LE(Value2(&pSrc[1]));
      goto copy_literal_and_match;

#if HAVE_LABELS_AS_VALUES
      pre_d :
#else
    case 70: case 78: case 86: case 94: case 102: case 110: case 134:
    case 142: case 150: case 158: case 198: case 206:
#endif
      State->src = pSrc, State->dst = pDst, State->d_prev = D;

      // previous distance
      OpcSize = 1;
      L = static_cast<size_t>(extract(opc, 6, 2));
      M = static_cast<size_t>(extract(opc, 3, 3)) + 3;

      if (SrcLen <= OpcSize + L)
        return; // source truncated

copy_literal_and_match:
      pSrc += OpcSize, SrcLen -= OpcSize;

      if (DstLen >= 4 && SrcLen >= 4)
        Store(pDst, Value4(pSrc));
      else if (L <= DstLen)
      {
        for (size_t i = 0; i < L; ++i)
          pDst[i] = pSrc[i];
      }
      else
      {
        for (size_t i = 0; i < DstLen; ++i)
          pDst[i] = pSrc[i];

        State->src = pSrc + DstLen;
        State->dst = pDst + DstLen;
        State->L = L - DstLen;
        State->M = M;
        State->D = D;
        return;
      }

      pDst += L, DstLen -= L;
      pSrc += L, SrcLen -= L;

      if (D > static_cast<size_t>(pDst - State->dst_begin) || D == 0)
        return;

copy_match:
      if (DstLen >= M + 7 && D >= 8)
      {
        for (size_t i = 0; i < M; i += 8)
          Store(&pDst[i], Value8(&pDst[i - D]));
      }
      else if (M <= DstLen)
      {
        for (size_t i = 0; i < M; ++i)
          pDst[i] = pDst[i - D];
      }
      else
      {
        // Destination truncated
        for (size_t i = 0; i < DstLen; ++i)
          pDst[i] = pDst[i - D];

        State->src = pSrc;
        State->dst = pDst + DstLen;
        State->L = 0;
        State->M = M - DstLen;
        State->D = D;
        return;
      }

      pDst += M, DstLen -= M;
      opc = pSrc[0];
#if HAVE_LABELS_AS_VALUES
      goto *opc_tbl[opc];
#else
      break;
#endif

#if HAVE_LABELS_AS_VALUES
      sml_m :
#else
    case 241: case 242: case 243: case 244: case 245: case 246: case 247: case 248:
    case 249: case 250: case 251: case 252: case 253: case 254: case 255:
#endif
      State->src = pSrc, State->dst = pDst, State->d_prev = D;

      //  small match
      OpcSize = 1;
      if (SrcLen <= OpcSize)
        return; // source truncated

      M = static_cast<size_t>(extract(opc, 0, 4));
      pSrc += OpcSize, SrcLen -= OpcSize;
      goto copy_match;

#if HAVE_LABELS_AS_VALUES
      lrg_m :
#else
    case 240:
#endif
      State->src = pSrc, State->dst = pDst, State->d_prev = D;

      //  large match
      OpcSize = 2;
      if (SrcLen <= OpcSize)
        return; // source truncated

      M = pSrc[1] + 16;
      pSrc += OpcSize, SrcLen -= OpcSize;
      goto copy_match;

#if HAVE_LABELS_AS_VALUES
      sml_l :
#else
    case 225: case 226: case 227: case 228: case 229: case 230: case 231:
    case 232: case 233: case 234: case 235: case 236: case 237: case 238: case 239:
#endif
      State->src = pSrc, State->dst = pDst, State->d_prev = D;

      //  small literal
      OpcSize = 1;
      L = static_cast<size_t>(extract(opc, 0, 4));
      goto copy_literal;

#if HAVE_LABELS_AS_VALUES
      lrg_l :
#else
    case 224:
#endif
      State->src = pSrc, State->dst = pDst, State->d_prev = D;

      //  large literal
      OpcSize = 2;
      if (SrcLen <= 2)
        return; // source truncated

      L = pSrc[1] + 16;

copy_literal:
      if (SrcLen <= OpcSize + L)
        return; // source truncated

      pSrc += OpcSize, SrcLen -= OpcSize;

      if (DstLen >= L + 7 && SrcLen >= L + 7)
      {
        for (size_t i = 0; i < L; i += 8)
          Store(&pDst[i], Value8(&pSrc[i]));
      }
      else if (L <= DstLen)
      {
        for (size_t i = 0; i < L; ++i)
          pDst[i] = pSrc[i];
      }
      else
      {
        for (size_t i = 0; i < DstLen; ++i)
          pDst[i] = pSrc[i];

        State->src = pSrc + DstLen;
        State->dst = pDst + DstLen;
        State->L = L - DstLen;
        State->M = 0;
        State->D = D;
        return;
      }

      pDst += L, DstLen -= L;
      pSrc += L, SrcLen -= L;
      opc = pSrc[0];
#if HAVE_LABELS_AS_VALUES
      goto *opc_tbl[opc];
#else
      break;
#endif

#if HAVE_LABELS_AS_VALUES
      nop :
#else
    case 14:
    case 22:
#endif
      State->src = pSrc, State->dst = pDst, State->d_prev = D;
      OpcSize = 1;
      if (SrcLen <= OpcSize)
        return; // source truncated

      pSrc += OpcSize, SrcLen -= OpcSize;
      opc = pSrc[0];
#if HAVE_LABELS_AS_VALUES
      goto *opc_tbl[opc];
#else
      break;
#endif


#if HAVE_LABELS_AS_VALUES
      eos :
#else
    case 6:
#endif
      OpcSize = 8;
      if (SrcLen < OpcSize)
        return; // source truncated (here we don't need an extra byte for next opcode)

      pSrc += OpcSize, SrcLen -= OpcSize;
      State->end_of_stream = true;
      State->src = pSrc, State->dst = pDst, State->d_prev = D;
      return; // end-of-stream

#if HAVE_LABELS_AS_VALUES
      udef :
#else
    case 30: case 38: case 46: case 54: case 62: case 112: case 113: case 114:
    case 115: case 116: case 117: case 118: case 119: case 120: case 121: case 122:
    case 123: case 124: case 125: case 126: case 127: case 208: case 209: case 210:
    case 211: case 212: case 213: case 214: case 215: case 216: case 217: case 218:
    case 219: case 220: case 221: case 222: case 223:
#endif
      return;

#if !HAVE_LABELS_AS_VALUES
    default:
      assert(!"Oops! Unexpectable case...");
      break;
    }
  }
#endif
}

} // namespace UFSD

#endif      // UFSD_LZFSE
