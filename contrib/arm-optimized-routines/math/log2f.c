/*
 * Single-precision log2 function.
 *
 * Copyright (c) 2017-2024, Arm Limited.
 * SPDX-License-Identifier: MIT OR Apache-2.0 WITH LLVM-exception
 */

#include <math.h>
#include <stdint.h>
#include "math_config.h"
#include "test_defs.h"
#include "test_sig.h"

/*
LOG2F_TABLE_BITS = 4
LOG2F_POLY_ORDER = 4

ULP error: 0.752 (nearest rounding.)
Relative error: 1.9 * 2^-26 (before rounding.)
*/

#define N (1 << LOG2F_TABLE_BITS)
#define T __log2f_data.tab
#define A __log2f_data.poly
#define OFF 0x3f330000

float
log2f (float x)
{
  /* double_t for better performance on targets with FLT_EVAL_METHOD==2.  */
  double_t z, r, r2, p, y, y0, invc, logc;
  uint32_t ix, iz, top, tmp;
  int k, i;

  ix = asuint (x);
#if WANT_ROUNDING
  /* Fix sign of zero with downward rounding when x==1.  */
  if (unlikely (ix == 0x3f800000))
    return 0;
#endif
  if (unlikely (ix - 0x00800000 >= 0x7f800000 - 0x00800000))
    {
      /* x < 0x1p-126 or inf or nan.  */
      if (ix * 2 == 0)
	return __math_divzerof (1);
      if (ix == 0x7f800000) /* log2(inf) == inf.  */
	return x;
      if ((ix & 0x80000000) || ix * 2 >= 0xff000000)
	return __math_invalidf (x);
      /* x is subnormal, normalize it.  */
      ix = asuint (x * 0x1p23f);
      ix -= 23 << 23;
    }

  /* x = 2^k z; where z is in range [OFF,2*OFF] and exact.
     The range is split into N subintervals.
     The ith subinterval contains z and c is near its center.  */
  tmp = ix - OFF;
  i = (tmp >> (23 - LOG2F_TABLE_BITS)) % N;
  top = tmp & 0xff800000;
  iz = ix - top;
  k = (int32_t) tmp >> 23; /* arithmetic shift */
  invc = T[i].invc;
  logc = T[i].logc;
  z = (double_t) asfloat (iz);

  /* log2(x) = log1p(z/c-1)/ln2 + log2(c) + k */
  r = z * invc - 1;
  y0 = logc + (double_t) k;

  /* Pipelined polynomial evaluation to approximate log1p(r)/ln2.  */
  r2 = r * r;
  y = A[1] * r + A[2];
  y = A[0] * r2 + y;
  p = A[3] * r + y0;
  y = y * r2 + p;
  return eval_as_float (y);
}
#if USE_GLIBC_ABI
strong_alias (log2f, __log2f_finite)
hidden_alias (log2f, __ieee754_log2f)
#endif

TEST_SIG (S, F, 1, log2, 0.01, 11.1)
TEST_ULP (log2f, 0.26)
TEST_ULP_NONNEAREST (log2f, 0.5)
TEST_INTERVAL (log2f, 0, 0xffff0000, 10000)
TEST_INTERVAL (log2f, 0x1p-4, 0x1p4, 50000)
TEST_INTERVAL (log2f, 0, inf, 50000)
