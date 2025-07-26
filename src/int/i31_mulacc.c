/*
 * Copyright (c) 2016 Thomas Pornin <pornin@bolet.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining 
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "inner.h"

/* see inner.h */
void
br_i31_mulacc(br_ssl_u32 *d, const br_ssl_u32 *a, const br_ssl_u32 *b)
{
	size_t alen, blen, u;
	br_ssl_u32 dl, dh;

	alen = (a[0] + 31) >> 5;
	blen = (b[0] + 31) >> 5;

	/*
	 * We want to add the two bit lengths, but these are encoded,
	 * which requires some extra care.
	 */
	dl = (a[0] & 31) + (b[0] & 31);
	dh = (a[0] >> 5) + (b[0] >> 5);
	d[0] = (dh << 5) + dl + (~(br_ssl_u32)(dl - 31) >> 31);

	for (u = 0; u < blen; u ++) {
		br_ssl_u32 f;
		size_t v;

		/*
		 * Carry always fits on 31 bits; we want to keep it in a
		 * 32-bit register on 32-bit architectures (on a 64-bit
		 * architecture, cast down from 64 to 32 bits means
		 * clearing the high bits, which is not free; on a 32-bit
		 * architecture, the same operation really means ignoring
		 * the top register, which has negative or zero cost).
		 */
#if BR_64
		br_ssl_u64 cc;
#else
		br_ssl_u32 cc;
#endif

		f = b[1 + u];
		cc = 0;
		for (v = 0; v < alen; v ++) {
			br_ssl_u64 z;

			z = (br_ssl_u64)d[1 + u + v] + MUL31(f, a[1 + v]) + cc;
			cc = z >> 31;
			d[1 + u + v] = (br_ssl_u32)z & 0x7FFFFFFF;
		}
		d[1 + u + alen] = (br_ssl_u32)cc;
	}
}
