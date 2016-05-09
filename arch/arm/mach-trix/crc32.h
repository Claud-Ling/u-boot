/**
*   @file       crc32.h
*   @author
*	@version
*	@date
*
* @if copyright_display
*		Copyright (c) 2007 Trident Microsystems, Inc.
*		All rights reserved
*
*	 	The content of this file or document is CONFIDENTIAL and PROPRIETARY
*		to Trident Microsystems, Inc.  It is subject to the terms of a
*		License Agreement between Licensee and Trident Microsystems, Inc.
*		restricting among other things, the use, reproduction, distribution
*		and transfer.  Each of the embodiments, including this information and
*		any derivative work shall retain this copyright notice.
*	@endif
*
*	More description ...
*
*	@if modification_History
*
*		<b>Modification History:\n</b>
*		Date				Name			Comment \n\n
*
*
*	@endif
*/
#ifndef __CRC32_LITE_H__
#define __CRC32_LITE_H__

#ifndef __ASSEMBLY__

#if defined __cplusplus || defined __cplusplus__
extern "C"
{
#endif

/**
*	@fn		unsigned long crc32_lite(unsigned long val, const void *ss, int len);
*	@brief		Get a 32-bit CRC of the contents of the buffer
*	@param[in]	val CRC seed value
*	@param[in]	ss Address space
*	@param[in]	len Length
*	@return 	a 32-bit CRC of the contents of the buffer
*
*  Description
*
* @if bug_display
*  	Bug1.
*  	...
* @endif
*
* @if warning_display
*  	Warning1.
*  	...
* @endif
*
*
*/
unsigned long crc32_lite(unsigned long val, const void *ss, int len);

#if defined __cplusplus || defined __cplusplus__
}
#endif

#endif //__ASSEMBLY__

#endif
