/* 
 * PJMEDIA - Multimedia over IP Stack 
 * (C)2003-2005 Benny Prijono <bennylp@bulukucing.org>
 *
 * Author:
 *  Benny Prijono <bennylp@bulukucing.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef PA_TYPES_H
#define PA_TYPES_H

/*
    SIZEOF_SHORT, SIZEOF_INT and SIZEOF_LONG are set by the configure script
    when it is used. Otherwise we default to the common 32 bit values, if your
    platform doesn't use configure, and doesn't use the default values below
    you will need to explicitly define these symbols in your make file.

    A PA_VALIDATE_SIZES macro is provided to assert that the values set in this
    file are correct.
*/

#ifndef SIZEOF_SHORT
#define SIZEOF_SHORT 2
#endif

#ifndef SIZEOF_INT
#define SIZEOF_INT 4
#endif

#ifndef SIZEOF_LONG
#define SIZEOF_LONG 4
#endif


#if SIZEOF_SHORT == 2
typedef signed short PaInt16;
typedef unsigned short PaUint16;
#elif SIZEOF_INT == 2
typedef signed int PaInt16;
typedef unsigned int PaUint16;
#else
#error pa_types.h was unable to determine which type to use for 16bit integers on the target platform
#endif

#if SIZEOF_SHORT == 4
typedef signed short PaInt32;
typedef unsigned short PaUint32;
#elif SIZEOF_INT == 4
typedef signed int PaInt32;
typedef unsigned int PaUint32;
#elif SIZEOF_LONG == 4
typedef signed long PaInt32;
typedef unsigned long PaUint32;
#else
#error pa_types.h was unable to determine which type to use for 32bit integers on the target platform
#endif


/* PA_VALIDATE_TYPE_SIZES compares the size of the integer types at runtime to
 ensure that PortAudio was configured correctly, and raises an assertion if
 they don't match the expected values. <assert.h> must be included in the
 context in which this macro is used.
*/
#define PA_VALIDATE_TYPE_SIZES \
    { \
        assert( "PortAudio: type sizes are not correct in pa_types.h" && sizeof( PaUint16 ) == 2 ); \
        assert( "PortAudio: type sizes are not correct in pa_types.h" && sizeof( PaInt16 ) == 2 ); \
        assert( "PortAudio: type sizes are not correct in pa_types.h" && sizeof( PaUint32 ) == 4 ); \
        assert( "PortAudio: type sizes are not correct in pa_types.h" && sizeof( PaInt32 ) == 4 ); \
    }


#endif /* PA_TYPES_H */
