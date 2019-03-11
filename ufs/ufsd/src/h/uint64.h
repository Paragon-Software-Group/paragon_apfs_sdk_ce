// <copyright file="uint64.h" company="Paragon Software Group">
//
// Copyright (c) 2002-2019 Paragon Software Group, All rights reserved.
//
// The license for this file is defined in a separate document "LICENSE.txt"
// located at the root of the project.
//
// </copyright>
//
//  This file contains usefull definition for 64 bit value
//

namespace UFSD{


#if __WORDSIZE >= 64

  #define div_u64( a, b ) ( (a) / (b) )

#elif defined USE_BUILTINT_DIV64

  // Use inline implementation of int64 div and mod support located in <asm/div64.h> header

  // This function divides 64 bit value to 32 bit value
  // and returns the quotient: Val / Div
  static inline UINT64 div_u64( UINT64 Val, unsigned int Div )
  {
    // WARNING: do_div changes its first argument(!)
    (void)do_div(Val, Div);
    return Val;
  }

#elif defined UFSD_USE_EMUL_DIV64

  // This function divides 64 bit value to 32 bit value
  // and returns the quotient: Val / Div
  static inline UINT64 div_u64( UINT64 Val, unsigned int Div )
  {
	  UINT64 rem    = Val;
	  UINT64 b      = Div;
	  UINT64 res, d = 1;
	  unsigned int high = (unsigned int)(rem >> 32);

	  // Reduce the thing a bit first
	  res = 0;
	  if ( high >= Div )
    {
		  high /= Div;
		  res  = (UINT64) high << 32;
		  rem -= (UINT64) (high*Div) << 32;
	  }

	  while ( (INT64)b > 0 && b < rem )
    {
		  b = b + b;
		  d = d + d;
	  }

	  do
    {
		  if ( rem >= b )
      {
			  rem -= b;
			  res += d;
		  }
		  b >>= 1;
		  d >>= 1;
	  } while (d);

	  return res;
  }
#elif defined UFSD_EFI
	#define div_u64 DivU64x32
#else

  #define div_u64( a, b ) ( (a) / (b) )

#endif

#if __WORDSIZE >= 64
  #define mod_u64( a, b ) ( (a) % (b) )
#elif defined UFSD_EFI
  #define mod_u64 ModU64x32
#elif defined UFSD_USE_EMUL_DIV64
  static inline UINT64 mod_u64( UINT64 Val, unsigned int Div )
  {
  	UINT64 c_mod_div = div_u64(Val, Div);
  	return Val - Div * c_mod_div;
  }
#else
  #define mod_u64( a, b ) ( (a) % (b) )
#endif


} // namespace UFSD{
