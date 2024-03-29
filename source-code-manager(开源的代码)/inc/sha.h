/**
 * file sha1.h
 *
 *  Copyright (C) 2006-2010, Brainspark B.V.
 *
 *  This file is part of PolarSSL (http://www.polarssl.org)
 *  Lead Maintainer: Paul Bakker <polarssl_maintainer at polarssl.org>
 *
 *  All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef POLARSSL_SHA1_H
#define POLARSSL_SHA1_H


/*20 bytes for sha so 40 bytes to put it in hex and one byte for NULL
 * A SHA is valid only if it ends with a NULL char*/
#define SHA_HASH_LENGTH				40
typedef unsigned char ShaBuffer[SHA_HASH_LENGTH+1];

bool sha_buffer(const unsigned char *input, int ilen, ShaBuffer sha);
bool sha_file(const char *path, ShaBuffer sha);
bool sha_compare(const ShaBuffer s1, const ShaBuffer s2);
inline void sha_reset(ShaBuffer sha);
#endif /* sha1.h */
