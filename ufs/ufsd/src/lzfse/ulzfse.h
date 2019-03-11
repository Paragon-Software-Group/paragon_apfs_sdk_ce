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


#ifndef __ULZFSE_H__
#define __ULZFSE_H__

#include <api/compress.hpp>
#include "lzfse_struct.h"

#define Value2 Value<unsigned short>
#define Value4 Value<unsigned int>
#define Value8 Value<UINT64>

namespace UFSD
{

class CLzfseCompression : public api::ICompress, public UMemBased<CLzfseCompression>
{
  api::IBaseLog*        m_Log;
public:
  CLzfseCompression(api::IBaseMemoryManager* Mm, api::IBaseLog* Log)
    : UMemBased<CLzfseCompression>(Mm)
    , m_Log(Log)
  {}

  virtual ~CLzfseCompression() {}

  api::IBaseLog* GetLog() const { return m_Log; }

  virtual int SetCompressLevel(unsigned int /*CompressLevel*/)
  {
    return ERR_NOTIMPLEMENTED;
  }

  virtual int Compress(
    IN  const void* /*InBuf*/,
    IN  size_t      /*InBufLen*/,
    OUT void*       /*OutBuf*/,
    IN  size_t      /*OutBufSize*/,
    OUT size_t*     /*OutLen*/
    )
  {
    return ERR_NOTIMPLEMENTED;
  }

  virtual int Decompress(
    IN  const void* InBuf,
    IN  size_t      InBufLen,
    OUT void*       OutBuf,
    IN  size_t      OutBufLen,
    OUT size_t*     OutSize
    );

  virtual void Destroy() { delete this; }
private:
  int Decode(IN lzfse_decoder_state* State) const;
  void DecodeLZVN(IN lzvn_decoder_state* State) const;

  template<typename T>
  T Value(const void* p) const
  {
    T data;
    Memcpy2(&data, p, sizeof(T));
    return data;
  }

  template<typename T>
  void Store(void* p, T data) const
  {
    Memcpy2(p, &data, sizeof(T));
  }
};

}; // namespace UFSD

#endif // __ULZFSE_H__
