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
 
#include "tea.h"
/* encryptBlock
 *   Encrypts byte array data of length len with key key using TEA
 * Arguments:
 *   data - pointer to 8 bit data array to be encrypted - SEE NOTES
 *   len - length of array
 *   key - Pointer to four integer array (16 bytes) holding TEA key
 * Returns:
 *   data - encrypted data held here
 *   len - size of the new data array
 * Side effects:
 *   Modifies data and len
 * NOTES:
 * data size must be equal to or larger than ((len + 7) / 8) * 8 + 8
 * TEA encrypts in 8 byte blocks, so it must include enough space to 
 * hold the entire data to pad out to an 8 byte boundary, plus another
 * 8 bytes at the end to give the length to the decrypt algorithm.
 *
 *  - Shortcut - make sure that data is at least len + 15 bytes in size.
 */

void encrypt (int32_t* v, int32_t* k);
void decrypt (int32_t* v, int32_t* k);

void encryptBlock(uint8_t* data, uint32_t * len, int32_t * key)
{
   uint32_t blocks, i;
   uint32_t * data32;

   // treat the data as 32 bit unsigned integers
   data32 = (uint32_t *) data;

   // Find the number of 8 byte blocks, add one for the length
   blocks = (((*len) + 7) / 8) + 1;

   // Set the last block to the original data length
   data32[(blocks*2) - 1] = *len;

   // Set the encrypted data length
   *len = blocks * 8;

   for(i = 0; i< blocks; i++)
   {
      encrypt((int32_t*)&data32[i*2], key);
   }
}

/* decryptBlock
 *   Decrypts byte array data of length len with key key using TEA
 * Arguments:
 *   data - pointer to 8 bit data array to be decrypted - SEE NOTES
 *   len - length of array
 *   key - Pointer to four integer array (16 bytes) holding TEA key
 * Returns:
 *   data - decrypted data held here
 *   len - size of the new data array
 * Side effects:
 *   Modifies data and len
 * NOTES:
 *   None
 */
void decryptBlock(uint8_t *data, uint32_t *len, int32_t *key)
{
   uint32_t blocks, i;
   int32_t * data32;

   // treat the data as 32 bit unsigned integers
   data32 = (int32_t *) data;

   // Find the number of 8 byte blocks
   blocks = (*len)/8;

   for(i = 0; i< blocks; i++)
   {
      decrypt(&data32[i*2], key);
   }

   // Return the length of the original data
   *len = data32[(blocks*2) - 1];
}

/* encrypt
 *   Encrypt 64 bits with a 128 bit key using TEA
 *   From http://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm
 * Arguments:
 *   v - array of two 32 bit uints to be encoded in place
 *   k - array of four 32 bit uints to act as key
 * Returns:
 *   v - encrypted result
 * Side effects:
 *   None
 */
#pragma O0
void encrypt (int32_t* v, int32_t* k) 
{
    int32_t v0=v[0], v1=v[1], sum=0, i;           /* set up */
    int32_t delta=0x9e3779b9;                     /* a key schedule constant */
    int32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */
    for (i=0; i < 32; i++)                        /* basic cycle start */
    {
        sum += delta;
        v0 += ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
        v1 += ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);  
    }                                              /* end cycle */
    v[0]=v0; v[1]=v1;
}

/* decrypt
 *   Decrypt 64 bits with a 128 bit key using TEA
 *   From http://en.wikipedia.org/wiki/Tiny_Encryption_Algorithm
 * Arguments:
 *   v - array of two 32 bit uints to be decoded in place
 *   k - array of four 32 bit uints to act as key
 * Returns:
 *   v - decrypted result
 * Side effects:
 *   None
 */
#pragma O0
void decrypt (int32_t* v, int32_t* k) 
{
    int32_t v0=v[0], v1=v[1], sum=0xC6EF3720, i;  /* set up */
    int32_t delta=0x9e3779b9;                     /* a key schedule constant */
    int32_t k0=k[0], k1=k[1], k2=k[2], k3=k[3];   /* cache key */
    for (i=0; i<32; i++)                          /* basic cycle start */
    {    v1 -= ((v0<<4) + k2) ^ (v0 + sum) ^ ((v0>>5) + k3);
        v0 -= ((v1<<4) + k0) ^ (v1 + sum) ^ ((v1>>5) + k1);
        sum -= delta;                                   
    }                                              /* end cycle */
    v[0]=v0; v[1]=v1;
}

