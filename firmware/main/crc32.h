/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Ken W.T. Kan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
 
/**
 *	\file		crc32.h
 *	\brief		CRC32 calculation module
 *	\author		Gary S. Brown
 *	\date		1986
 *	\copyright	Public Domain
 */

#ifndef _CRC32_H_
#define _CRC32_H_

#include <stdint.h>

/**
 *	\brief				Calculate CRC32.
 *	\param[in]	buf		Point to data.
 *	\param[in]	size	Data size in bytes.
 *	\return 			Calculated CRC32 value.
 */
uint32_t crc32(const void *buf, uint16_t size);
int ValidateCrc32Blk(void *src, uint16_t size);
void CalcCrc32Blk(void *src, uint16_t size);
#endif /* _CRC32_H_ */
