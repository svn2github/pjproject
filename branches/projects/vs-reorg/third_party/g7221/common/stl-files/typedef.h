/*
  ===========================================================================
   File: TYPEDEF.H                                       v.1.0 - 26.Jan.2000
  ===========================================================================

		      ITU-T STL  BASIC OPERATORS

 		      TYPE DEFINITION PROTOTYPES

   History:
   26.Jan.00	v1.0	Incorporated to the STL from updated G.723.1/G.729 
                        basic operator library (based on basic_op.h)
  ============================================================================
*/

#ifndef TYPEDEF_H
#define TYPEDEF_H "$Id $"

#include <limits.h>

#if defined(__BORLANDC__) || defined(__WATCOMC__) || defined(_MSC_VER) || defined(__ZTC__) || defined(__CYGWIN__)
typedef signed char Word8;
typedef short Word16;
typedef long Word32;
typedef int Flag;

#elif defined(__sun)
typedef signed char Word8;
typedef short Word16;
typedef long Word32;
typedef int Flag;

#elif defined(__unix__) || defined(__unix)
typedef signed char Word8;
typedef short Word16;
typedef int Word32;
typedef int Flag;

#endif

/* define 16 bit unsigned types for G.722.1 */
#if INT_MAX == 32767
typedef unsigned int UWord16;
#elif SHRT_MAX == 32767
typedef unsigned short UWord16;
#endif

/* define 32 bit unsigned types for G.722.1 */
#if INT_MAX == 2147483647L
typedef unsigned int UWord32;
#elif LONG_MAX == 2147483647L
typedef unsigned long UWord32;
#endif

#endif /* TYPEDEF_H */
