// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

// We aren't going to EVER change this code - so let's ignore the warnings.
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#elif defined(_WIN32)
#pragma warning(disable : 4100)
#endif

using fort_int = int;

unsigned short local_to_vax_short(const unsigned short *s);
unsigned short vax_to_local_short(const unsigned short *s);
unsigned local_to_vax_int(const fort_int *i);
unsigned vax_to_local_int(const fort_int *i);

void local_to_vax_shorts(const unsigned short *sa, const int *n);
void vax_to_local_shorts(const unsigned short *sa, const int *n);
void local_to_vax_ints(const fort_int *ia, const fort_int *n);
void vax_to_local_ints(const fort_int *ia, const fort_int *n);

/* these routines return 0 = success, 1 = failure */

/* convert an IEEE single float to VAX F FLOAT format */
/* int ieee_to_vax_float(float* fp); */

/* convert VAX F FLOAT into an IEEE single float */
/* int vax_to_ieee_float(float* fp); */

/* convert float array val[n] to and from vax float */
void vaxf_to_local(float *val, const int *n, int *errcode);
void local_to_vaxf(float *val, const int *n, int *errcode);
void local_to_ieee_float(const float *val, const int *n, int *errcode);
void local_to_ieee_double(const double *val, const int *n, int *errcode);
void ieee_float_to_local(const float *val, const int *n, int *errcode);
void ieee_double_to_local(const double *val, const int *n, int *errcode);
