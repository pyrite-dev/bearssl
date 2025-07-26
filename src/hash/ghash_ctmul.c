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

/*
 * We compute "carryless multiplications" through normal integer
 * multiplications, masking out enough bits to create "holes" in which
 * carries may expand without altering our bits; we really use 8 data
 * bits per 32-bit word, spaced every fourth bit. Accumulated carries
 * may not exceed 8 in total, which fits in 4 bits.
 *
 * It would be possible to use a 3-bit spacing, allowing two operands,
 * one with 7 non-zero data bits, the other one with 10 or 11 non-zero
 * data bits; this asymmetric splitting makes the overall code more
 * complex with thresholds and exceptions, and does not appear to be
 * worth the effort.
 */

/*
 * We cannot really autodetect whether multiplications are "slow" or
 * not. A typical example is the ARM Cortex M0+, which exists in two
 * versions: one with a 1-cycle multiplication opcode, the other with
 * a 32-cycle multiplication opcode. They both use exactly the same
 * architecture and ABI, and cannot be distinguished from each other
 * at compile-time.
 *
 * Since most modern CPU (even embedded CPU) still have fast
 * multiplications, we use the "fast mul" code by default.
 */

#if BR_SLOW_MUL

/*
 * This implementation uses Karatsuba-like reduction to make fewer
 * integer multiplications (9 instead of 16), at the expense of extra
 * logical operations (XOR, shifts...). On modern x86 CPU that offer
 * fast, pipelined multiplications, this code is about twice slower than
 * the simpler code with 16 multiplications. This tendency may be
 * reversed on low-end platforms with expensive multiplications.
 */

#define MUL32(h, l, x, y)   do { \
		br_ssl_u64 mul32tmp = MUL(x, y); \
		(h) = (br_ssl_u32)(mul32tmp >> 32); \
		(l) = (br_ssl_u32)mul32tmp; \
	} while (0)

static inline void
bmul(br_ssl_u32 *hi, br_ssl_u32 *lo, br_ssl_u32 x, br_ssl_u32 y)
{
	br_ssl_u32 x0, x1, x2, x3;
	br_ssl_u32 y0, y1, y2, y3;
	br_ssl_u32 a0, a1, a2, a3, a4, a5, a6, a7, a8;
	br_ssl_u32 b0, b1, b2, b3, b4, b5, b6, b7, b8;

	x0 = x & (br_ssl_u32)0x11111111;
	x1 = x & (br_ssl_u32)0x22222222;
	x2 = x & (br_ssl_u32)0x44444444;
	x3 = x & (br_ssl_u32)0x88888888;
	y0 = y & (br_ssl_u32)0x11111111;
	y1 = y & (br_ssl_u32)0x22222222;
	y2 = y & (br_ssl_u32)0x44444444;
	y3 = y & (br_ssl_u32)0x88888888;

	/*
	 * (x0+W*x1)*(y0+W*y1) -> a0:b0
	 * (x2+W*x3)*(y2+W*y3) -> a3:b3
	 * ((x0+x2)+W*(x1+x3))*((y0+y2)+W*(y1+y3)) -> a6:b6
	 */
	a0 = x0;
	b0 = y0;
	a1 = x1 >> 1;
	b1 = y1 >> 1;
	a2 = a0 ^ a1;
	b2 = b0 ^ b1;
	a3 = x2 >> 2;
	b3 = y2 >> 2;
	a4 = x3 >> 3;
	b4 = y3 >> 3;
	a5 = a3 ^ a4;
	b5 = b3 ^ b4;
	a6 = a0 ^ a3;
	b6 = b0 ^ b3;
	a7 = a1 ^ a4;
	b7 = b1 ^ b4;
	a8 = a6 ^ a7;
	b8 = b6 ^ b7;

	MUL32(b0, a0, b0, a0);
	MUL32(b1, a1, b1, a1);
	MUL32(b2, a2, b2, a2);
	MUL32(b3, a3, b3, a3);
	MUL32(b4, a4, b4, a4);
	MUL32(b5, a5, b5, a5);
	MUL32(b6, a6, b6, a6);
	MUL32(b7, a7, b7, a7);
	MUL32(b8, a8, b8, a8);

	a0 &= (br_ssl_u32)0x11111111;
	a1 &= (br_ssl_u32)0x11111111;
	a2 &= (br_ssl_u32)0x11111111;
	a3 &= (br_ssl_u32)0x11111111;
	a4 &= (br_ssl_u32)0x11111111;
	a5 &= (br_ssl_u32)0x11111111;
	a6 &= (br_ssl_u32)0x11111111;
	a7 &= (br_ssl_u32)0x11111111;
	a8 &= (br_ssl_u32)0x11111111;
	b0 &= (br_ssl_u32)0x11111111;
	b1 &= (br_ssl_u32)0x11111111;
	b2 &= (br_ssl_u32)0x11111111;
	b3 &= (br_ssl_u32)0x11111111;
	b4 &= (br_ssl_u32)0x11111111;
	b5 &= (br_ssl_u32)0x11111111;
	b6 &= (br_ssl_u32)0x11111111;
	b7 &= (br_ssl_u32)0x11111111;
	b8 &= (br_ssl_u32)0x11111111;

	a2 ^= a0 ^ a1;
	b2 ^= b0 ^ b1;
	a0 ^= (a2 << 1) ^ (a1 << 2);
	b0 ^= (b2 << 1) ^ (b1 << 2);
	a5 ^= a3 ^ a4;
	b5 ^= b3 ^ b4;
	a3 ^= (a5 << 1) ^ (a4 << 2);
	b3 ^= (b5 << 1) ^ (b4 << 2);
	a8 ^= a6 ^ a7;
	b8 ^= b6 ^ b7;
	a6 ^= (a8 << 1) ^ (a7 << 2);
	b6 ^= (b8 << 1) ^ (b7 << 2);
	a6 ^= a0 ^ a3;
	b6 ^= b0 ^ b3;
	*lo = a0 ^ (a6 << 2) ^ (a3 << 4);
	*hi = b0 ^ (b6 << 2) ^ (b3 << 4) ^ (a6 >> 30) ^ (a3 >> 28);
}

#else

/*
 * Simple multiplication in GF(2)[X], using 16 integer multiplications.
 */

static inline void
bmul(br_ssl_u32 *hi, br_ssl_u32 *lo, br_ssl_u32 x, br_ssl_u32 y)
{
	br_ssl_u32 x0, x1, x2, x3;
	br_ssl_u32 y0, y1, y2, y3;
	br_ssl_u64 z0, z1, z2, z3;
	br_ssl_u64 z;

	x0 = x & (br_ssl_u32)0x11111111;
	x1 = x & (br_ssl_u32)0x22222222;
	x2 = x & (br_ssl_u32)0x44444444;
	x3 = x & (br_ssl_u32)0x88888888;
	y0 = y & (br_ssl_u32)0x11111111;
	y1 = y & (br_ssl_u32)0x22222222;
	y2 = y & (br_ssl_u32)0x44444444;
	y3 = y & (br_ssl_u32)0x88888888;
	z0 = MUL(x0, y0) ^ MUL(x1, y3) ^ MUL(x2, y2) ^ MUL(x3, y1);
	z1 = MUL(x0, y1) ^ MUL(x1, y0) ^ MUL(x2, y3) ^ MUL(x3, y2);
	z2 = MUL(x0, y2) ^ MUL(x1, y1) ^ MUL(x2, y0) ^ MUL(x3, y3);
	z3 = MUL(x0, y3) ^ MUL(x1, y2) ^ MUL(x2, y1) ^ MUL(x3, y0);
	z0 &= (br_ssl_u64)0x1111111111111111;
	z1 &= (br_ssl_u64)0x2222222222222222;
	z2 &= (br_ssl_u64)0x4444444444444444;
	z3 &= (br_ssl_u64)0x8888888888888888;
	z = z0 | z1 | z2 | z3;
	*lo = (br_ssl_u32)z;
	*hi = (br_ssl_u32)(z >> 32);
}

#endif

/* see bearssl_hash.h */
void
br_ghash_ctmul(void *y, const void *h, const void *data, size_t len)
{
	const unsigned char *buf, *hb;
	unsigned char *yb;
	br_ssl_u32 yw[4];
	br_ssl_u32 hw[4];

	/*
	 * Throughout the loop we handle the y and h values as arrays
	 * of 32-bit words.
	 */
	buf = data;
	yb = y;
	hb = h;
	yw[3] = br_dec32be(yb);
	yw[2] = br_dec32be(yb + 4);
	yw[1] = br_dec32be(yb + 8);
	yw[0] = br_dec32be(yb + 12);
	hw[3] = br_dec32be(hb);
	hw[2] = br_dec32be(hb + 4);
	hw[1] = br_dec32be(hb + 8);
	hw[0] = br_dec32be(hb + 12);
	while (len > 0) {
		const unsigned char *src;
		unsigned char tmp[16];
		int i;
		br_ssl_u32 a[9], b[9], zw[8];
		br_ssl_u32 c0, c1, c2, c3, d0, d1, d2, d3, e0, e1, e2, e3;

		/*
		 * Get the next 16-byte block (using zero-padding if
		 * necessary).
		 */
		if (len >= 16) {
			src = buf;
			buf += 16;
			len -= 16;
		} else {
			memcpy(tmp, buf, len);
			memset(tmp + len, 0, (sizeof tmp) - len);
			src = tmp;
			len = 0;
		}

		/*
		 * Decode the block. The GHASH standard mandates
		 * big-endian encoding.
		 */
		yw[3] ^= br_dec32be(src);
		yw[2] ^= br_dec32be(src + 4);
		yw[1] ^= br_dec32be(src + 8);
		yw[0] ^= br_dec32be(src + 12);

		/*
		 * We multiply two 128-bit field elements. We use
		 * Karatsuba to turn that into three 64-bit
		 * multiplications, which are themselves done with a
		 * total of nine 32-bit multiplications.
		 */

		/*
		 * y[0,1]*h[0,1] -> 0..2
		 * y[2,3]*h[2,3] -> 3..5
		 * (y[0,1]+y[2,3])*(h[0,1]+h[2,3]) -> 6..8
		 */
		a[0] = yw[0];
		b[0] = hw[0];
		a[1] = yw[1];
		b[1] = hw[1];
		a[2] = a[0] ^ a[1];
		b[2] = b[0] ^ b[1];

		a[3] = yw[2];
		b[3] = hw[2];
		a[4] = yw[3];
		b[4] = hw[3];
		a[5] = a[3] ^ a[4];
		b[5] = b[3] ^ b[4];

		a[6] = a[0] ^ a[3];
		b[6] = b[0] ^ b[3];
		a[7] = a[1] ^ a[4];
		b[7] = b[1] ^ b[4];
		a[8] = a[6] ^ a[7];
		b[8] = b[6] ^ b[7];

		for (i = 0; i < 9; i ++) {
			bmul(&b[i], &a[i], b[i], a[i]);
		}

		c0 = a[0];
		c1 = b[0] ^ a[2] ^ a[0] ^ a[1];
		c2 = a[1] ^ b[2] ^ b[0] ^ b[1];
		c3 = b[1];
		d0 = a[3];
		d1 = b[3] ^ a[5] ^ a[3] ^ a[4];
		d2 = a[4] ^ b[5] ^ b[3] ^ b[4];
		d3 = b[4];
		e0 = a[6];
		e1 = b[6] ^ a[8] ^ a[6] ^ a[7];
		e2 = a[7] ^ b[8] ^ b[6] ^ b[7];
		e3 = b[7];

		e0 ^= c0 ^ d0;
		e1 ^= c1 ^ d1;
		e2 ^= c2 ^ d2;
		e3 ^= c3 ^ d3;
		c2 ^= e0;
		c3 ^= e1;
		d0 ^= e2;
		d1 ^= e3;

		/*
		 * GHASH specification has the bits "reversed" (most
		 * significant is in fact least significant), which does
		 * not matter for a carryless multiplication, except that
		 * the 255-bit result must be shifted by 1 bit.
		 */
		zw[0] = c0 << 1;
		zw[1] = (c1 << 1) | (c0 >> 31);
		zw[2] = (c2 << 1) | (c1 >> 31);
		zw[3] = (c3 << 1) | (c2 >> 31);
		zw[4] = (d0 << 1) | (c3 >> 31);
		zw[5] = (d1 << 1) | (d0 >> 31);
		zw[6] = (d2 << 1) | (d1 >> 31);
		zw[7] = (d3 << 1) | (d2 >> 31);

		/*
		 * We now do the reduction modulo the field polynomial
		 * to get back to 128 bits.
		 */
		for (i = 0; i < 4; i ++) {
			br_ssl_u32 lw;

			lw = zw[i];
			zw[i + 4] ^= lw ^ (lw >> 1) ^ (lw >> 2) ^ (lw >> 7);
			zw[i + 3] ^= (lw << 31) ^ (lw << 30) ^ (lw << 25);
		}
		memcpy(yw, zw + 4, sizeof yw);
	}

	/*
	 * Encode back the result.
	 */
	br_enc32be(yb, yw[3]);
	br_enc32be(yb + 4, yw[2]);
	br_enc32be(yb + 8, yw[1]);
	br_enc32be(yb + 12, yw[0]);
}
