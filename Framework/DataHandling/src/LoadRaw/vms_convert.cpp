// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
/*
 *      Module:         vms_convert
 *      Author:         Freddie Akeroyd, ISIS
 *      Purpose:        Routines to convert from VAX to local integer
 *representations
 *
 * $Id: vms_convert.cpp 4 2004-09-09 22:03:23Z faa59 $
 */
#include "vms_convert.h"
#include "MantidKernel/WarningSuppressions.h"

/*
 * Byte swaps for int and short
 */

#if 0
inline unsigned swap_int(unsigned a)
{
    union { unsigned u; unsigned char c[4]; } temp;
    unsigned char ctemp;
    temp.u = a;
    ctemp = temp.c[0]; temp.c[0] = temp.c[3]; temp.c[3] = ctemp;
    ctemp = temp.c[1]; temp.c[1] = temp.c[2]; temp.c[2] = ctemp;
    return temp.u;
}
#endif

#define swap_int(a) (((a) << 24) | (((a) << 8) & 0x00ff0000) | (((a) >> 8) & 0x0000ff00) | ((unsigned long)(a) >> 24))

#define swap_short(a) (((a & 0xff) << 8) | ((unsigned short)(a) >> 8))

/* VAXes are little endian */

unsigned short local_to_vax_short(const unsigned short *s) {
#if defined(WORDS_BIGENDIAN)
  return swap_short(*s);
#else
  return *s;
#endif /* WORDS_BIGENDIAN */
}

unsigned short vax_to_local_short(const unsigned short *s) {
#if defined(WORDS_BIGENDIAN)
  return swap_short(*s);
#else
  return *s;
#endif /* WORDS_BIGENDIAN */
}

unsigned local_to_vax_int(const fort_int *i) {
#if defined(WORDS_BIGENDIAN)
  return swap_int(*(unsigned *)i);
#else
  return *i;
#endif /* WORDS_BIGENDIAN */
}

unsigned vax_to_local_int(const fort_int *i) {
#if defined(WORDS_BIGENDIAN)
  return swap_int(*(unsigned *)i);
#else
  return *i;
#endif /* WORDS_BIGENDIAN */
}

void local_to_vax_shorts(const unsigned short *sa, const int *n) {
#if defined(WORDS_BIGENDIAN)
  int i;
  for (i = 0; i < *n; i++) {
    sa[i] = swap_short(sa[i]);
  }
#endif /* WORDS_BIGENDIAN */
  (void)sa;
  (void)n; // Avoid compiler warning
}

void vax_to_local_shorts(const unsigned short *sa, const int *n) {
#if defined(WORDS_BIGENDIAN)
  int i;
  for (i = 0; i < *n; i++) {
    sa[i] = swap_short(sa[i]);
  }
#endif /* WORDS_BIGENDIAN */
  (void)sa;
  (void)n; // Avoid compiler warning
}

void local_to_vax_ints(const fort_int *ia, const fort_int *n) {
#if defined(WORDS_BIGENDIAN)
  int i;
  unsigned *uia = (unsigned *)ia;
  for (i = 0; i < *n; i++) {
    uia[i] = swap_int(uia[i]);
  }
#endif /* WORDS_BIGENDIAN */
  (void)ia;
  (void)n; // Avoid compiler warning
}

void vax_to_local_ints(const fort_int *ia, const fort_int *n) {
#if defined(WORDS_BIGENDIAN)
  int i;
  unsigned *uia = (unsigned *)ia;
  for (i = 0; i < *n; i++) {
    uia[i] = swap_int(uia[i]);
  }
#endif /* WORDS_BIGENDIAN */
  (void)ia;
  (void)n; // Avoid compiler warning
}

/*
 * determine a few things we need to know to write machine independent data
 */

#ifndef __VMS
#define IEEEFP 1
#endif /* __VMS */

#ifdef __VMS
/* set up codes for use of CVT$CONVERT_FLOAT */
#if __IEEE_FLOAT
#define IEEEFP 1
#define VMS_FLOAT_NATIVE CVT$K_IEEE_S
#define VMS_DOUBLE_NATIVE CVT$K_IEEE_T
#elif __D_FLOAT
#define VAXFP 1
#define VMS_FLOAT_NATIVE CVT$K_VAX_F
#define VMS_DOUBLE_NATIVE CVT$K_VAX_D
#elif __G_FLOAT
#define VAXFP 1
#define VMS_FLOAT_NATIVE CVT$K_VAX_F
#define VMS_DOUBLE_NATIVE CVT$K_VAX_G
#else
#error Cannot determine VMS floating point format
#endif
#endif /* __VMS */

#if WORDS_BIGENDIAN

/* What IEEE single precision floating point looks like on local machine */
struct ieee_single {
  unsigned int sign : 1;
  unsigned int exp : 8;
  unsigned int mantissa : 23;
};

/* Vax single precision floating point */
struct vax_single {
  unsigned int mantissa2 : 16;
  unsigned int sign : 1;
  unsigned int exp : 8;
  unsigned int mantissa1 : 7;
};

#else

/** What IEEE single precision floating point looks like on local machine */
struct ieee_single {
  unsigned int mantissa : 23; ///< mantissa
  unsigned int exp : 8;       ///< Exponential
  unsigned int sign : 1;      ///< sign
};

/** Vax single precision floating point */
struct vax_single {
  unsigned int mantissa1 : 7;  ///< mantissa 1
  unsigned int exp : 8;        ///< Exponential
  unsigned int sign : 1;       ///< sign
  unsigned int mantissa2 : 16; ///< mantissa 2
};

#endif /* WORDS_BIGENDIAN */

#define VAX_SNG_BIAS 0x81
#define IEEE_SNG_BIAS 0x7f

/// Structure holding the limits of s single
static const struct sgl_limits_struct {
  struct vax_single s;     ///< vax single struct
  struct ieee_single ieee; ///< ieee single struct
} sgl_limits[2] = {
    {{0x7f, 0xff, 0x0, 0xffff}, /* Max Vax */
     {0x0, 0xff, 0x0}},         /* Max IEEE */
    {{0x0, 0x0, 0x0, 0x0},      /* Min Vax */
     {0x0, 0x0, 0x0}}           /* Min IEEE */
};

#define mmax sgl_limits[0]
#define mmin sgl_limits[1]

#if WORDS_BIGENDIAN

/* What IEEE double precision floating point looks like */
struct ieee_double {
  unsigned int mantissa2 : 32;
  unsigned int sign : 1;
  unsigned int exp : 11;
  unsigned int mantissa1 : 20;
};

/* Vax double precision floating point */
struct vax_double {
  //	unsigned int	mantissa4 : 16;
  //	unsigned int	mantissa3 : 16;
  unsigned int mantissa2 : 16;
  unsigned int sign : 1;
  unsigned int exp : 8;
  unsigned int mantissa1 : 7;
};

#else

/** What IEEE double precision floating point looks like */
struct ieee_double {
  // cppcheck-suppress-begin unusedStructMember
  unsigned int mantissa1 : 20; ///< mantissa 1
  unsigned int exp : 11;       ///< exponential
  unsigned int sign : 1;       ///< sign
  unsigned int mantissa2 : 32; ///< mantissa 2
  // cppcheck-suppress-end unusedStructMember
};

/** Vax double precision floating point */
struct vax_double {
  unsigned int mantissa1 : 7;  ///< mantissa 1
  unsigned int exp : 8;        ///< exponential
  unsigned int sign : 1;       ///< sign
  unsigned int mantissa2 : 16; ///< mantissa 2
                               //	unsigned int	mantissa3 : 16;  ///<mantissa 3
                               //	unsigned int	mantissa4 : 16;  ///<mantissa 4
};

#endif /* WORDS_BIGENDIAN */

#define VAX_DBL_BIAS 0x81
#define IEEE_DBL_BIAS 0x3ff
#define MASK(nbits) ((1 << nbits) - 1)

// static struct dbl_limits {
//	struct	vax_double d;
//	struct	ieee_double ieee;
//} dbl_limits[2] = {
//	{{ 0x7f, 0xff, 0x0, 0xffff, 0xffff, 0xffff },	/* Max Vax */
//	{ 0x0, 0x7ff, 0x0, 0x0 }},			/* Max IEEE */
//	{{ 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},		/* Min Vax */
//	{ 0x0, 0x0, 0x0, 0x0 }}				/* Min IEEE */
//};

/* VAX is little endian, so we may need to flip */
#if WORDS_BIGENDIAN
static int maybe_flip_bytes(void *p, size_t n) {
  unsigned i;
  unsigned char c_tmp, *c_p = (unsigned char *)p;
  for (i = 0; i < n / 2; i++) {
    c_tmp = c_p[i];
    c_p[i] = c_p[n - i - 1];
    c_p[n - i - 1] = c_tmp;
  }
  return 0;
}
#else
#define maybe_flip_bytes(__p, __n)
#endif /* WORDS_BIGENDIAN */

GNU_DIAG_OFF("uninitialized")

/* convert VAX F FLOAT into a local IEEE single float */
static int vax_to_ieee_float(float *fp) {
  struct ieee_single is;
  struct vax_single vs;
  struct sgl_limits_struct;
  maybe_flip_bytes(fp, sizeof(float));
  vs = *(reinterpret_cast<struct vax_single *>(fp));
  switch (vs.exp) {
  case 0:
    /* all vax float with zero exponent map to zero */
    is = mmin.ieee;
    break;
  case 2:
  case 1:
    /* These will map to subnormals */
    is.exp = 0;
    is.mantissa = (vs.mantissa1 << 16) | vs.mantissa2;
    /* lose some precision */
    is.mantissa >>= 3 - vs.exp;
    is.mantissa += (1 << (20 + vs.exp));
    break;
  case 0xff: /* mmax.s.exp */
    if (vs.mantissa2 == mmax.s.mantissa2 && vs.mantissa1 == mmax.s.mantissa1) {
      /* map largest vax float to ieee infinity */
      is = mmax.ieee;
      break;
    } /* else, fall thru */
  default:
    is.exp = vs.exp - VAX_SNG_BIAS + IEEE_SNG_BIAS;
    is.mantissa = (vs.mantissa1 << 16) | vs.mantissa2;
  }

  is.sign = vs.sign;
  *fp = *(reinterpret_cast<float *>(&is));
  return 0;
}

/* convert a local IEEE single float to little endian VAX F FLOAT format */
static int ieee_to_vax_float(float *fp) {
  struct ieee_single is;
  struct vax_single vs;
  struct sgl_limits_struct;
  is = *(reinterpret_cast<struct ieee_single *>(fp));
  switch (is.exp) {
  case 0:
    if (is.mantissa == mmin.ieee.mantissa) {
      vs = mmin.s;
    } else {
      unsigned tmp = is.mantissa >> 20;
      if (tmp >= 4) {
        vs.exp = 2;
      } else if (tmp >= 2) {
        vs.exp = 1;
      } else {
        vs = mmin.s;
        break;
      } /* else */
      tmp = is.mantissa - (1 << (20 + vs.exp));
      tmp <<= 3 - vs.exp;
      vs.mantissa2 = tmp;
      vs.mantissa1 = (tmp >> 16);
    }
    break;
  case 0xfe:
  case 0xff:
    vs = mmax.s;
    break;
  default:
    vs.exp = is.exp - IEEE_SNG_BIAS + VAX_SNG_BIAS;
    vs.mantissa2 = is.mantissa;
    vs.mantissa1 = (is.mantissa >> 16);
  }

  vs.sign = is.sign;
  *fp = *(reinterpret_cast<float *>(&vs));
  maybe_flip_bytes(fp, sizeof(float)); /* Make little endian */
  return 0;
}

GNU_DIAG_ON("uninitialized")

void vaxf_to_local(float *val, const int *n, int *errcode) {
#if defined(VAXFP)
#include <cvt$routines>
#include <cvtdef>
#endif
  int i;
  *errcode = 0;
#if defined(VAXFP)
  /* Do a null conversion to replace invalid values (prevents floating
   * exceptions later on) */
  for (i = 0; i < *n; i++) {
    if (cvt$ftof(val + i, CVT$K_VAX_F, val + i, CVT$K_VAX_F, CVT$M_REPORT_ALL) != CVT$K_NORMAL) {
      val[i] = 0.0; /* Fix with a safe value when status shows an invalid real
                       is being converted */
    }
  }
#elif defined(IEEEFP)
  for (i = 0; i < *n; i++) {
    if (vax_to_ieee_float(i + val) != 0) {
      *errcode = 1;
    }
  }
#else
#error Unknown floating point format
#endif
}

void local_to_vaxf(float *val, const int *n, int *errcode) {
  *errcode = 0;
#if defined(VAXFP)
/* nothing required */
#elif defined(IEEEFP)
  for (int i = 0; i < *n; i++) {
    if (ieee_to_vax_float(i + val) != 0) {
      *errcode = 1;
    }
  }
#else
#error Unknown floating point format
#endif
}

void ieee_float_to_local(const float *val, const int *n, int *errcode) {
#if defined(IEEEFP)
  (void)val;
  (void)n; // Avoid compiler warning
  *errcode = 0;
#elif defined(VAXFP)
  int i;
  *errcode = 0;
  for (i = 0; i < *n; i++) {
    //                if (ieee_to_vax_float(i+val) != 0)
    if (cvt$ftof(val + i, CVT$K_IEEE_S, val + i, VMS_FLOAT_NATIVE, 0) != CVT$K_NORMAL) {
      *errcode = 1;
      val[i] = 0.0;
    }
  }
#else
#error Unknown floating point format
#endif
}

void ieee_double_to_local(const double *val, const int *n, int *errcode) {
#if defined(IEEEFP)
  (void)val;
  (void)n; // Avoid compiler warning
  *errcode = 0;
#elif defined(VAXFP)
#include <cvt$routines>
#include <cvtdef>
  int i;
  *errcode = 0;
  for (i = 0; i < *n; i++) {
    if (cvt$ftof(val + i, CVT$K_IEEE_T, val + i, VMS_DOUBLE_NATIVE, CVT$M_REPORT_ALL) != CVT$K_NORMAL) {
      val[i] = 0.0;
      *errcode = 1;
    }
  }
#else
#error Unknown floating point format
#endif
}

void local_to_ieee_float(const float *val, const int *n, int *errcode) {
#if defined(IEEEFP)
  (void)val;
  (void)n; // Avoid compiler warning
  *errcode = 0;
#elif defined(VAXFP)
#include <cvt$routines>
#include <cvtdef>
  int i;
  *errcode = 0;
  for (i = 0; i < *n; i++) {
    if (cvt$ftof(val + i, VMS_FLOAT_NATIVE, val + i, CVT$K_IEEE_S, CVT$M_REPORT_ALL) != CVT$K_NORMAL) {
      val[i] = 0.0;
      *errcode = 1;
    }
  }
#else
#error Unknown floating point format
#endif
}

void local_to_ieee_double(const double *val, const int *n, int *errcode) {
#if defined(IEEEFP)
  (void)val;
  (void)n; // Avoid compiler warning
  *errcode = 0;
#elif defined(VAXFP)
  int i;
  *errcode = 0;
  for (i = 0; i < *n; i++) {
    if (cvt$ftof(val + i, VMS_DOUBLE_NATIVE, val + i, CVT$K_IEEE_T, CVT$M_REPORT_ALL) != CVT$K_NORMAL) {
      val[i] = 0.0;
      *errcode = 1;
    }
  }
#else
#error Unknown floating point format
#endif
}

#if 0
static unsigned char flip_bits(unsigned char cc)
{
    static int init = 0;
    static unsigned char ct[256];
    if (!init)
    {
        unsigned _i,_j;
        for(_i=0; _i<256; _i++)
        {
	    ct[_i] = 0;
 	    for(_j=0; _j<8; _j++)
	    {
    	        if (_i & (1<<_j)) { ct[_i] |= (128 >> _j); }
 	    }
        }
	init = 1;
    }
    return ct[cc];
}
#endif
