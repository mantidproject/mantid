#ifndef ENDIAN_CONVERT
#define ENDIAN_CONVERT

// We aren't going to EVER change this code - so let's ignore the warnings.
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#elif defined(_WIN32)
#pragma warning(disable : 4100)
#endif

typedef int fort_int;

unsigned short local_to_vax_short(const unsigned short *s);
unsigned short vax_to_local_short(const unsigned short *s);
unsigned local_to_vax_int(const fort_int *i);
unsigned vax_to_local_int(const fort_int *i);

void local_to_vax_shorts(unsigned short *sa, const int *n);
void vax_to_local_shorts(unsigned short *sa, const int *n);
void local_to_vax_ints(fort_int *ia, const fort_int *n);
void vax_to_local_ints(fort_int *ia, const fort_int *n);

/* these routines return 0 = success, 1 = failure */

/* convert an IEEE single float to VAX F FLOAT format */
/* int ieee_to_vax_float(float* fp); */

/* convert VAX F FLOAT into an IEEE single float */
/* int vax_to_ieee_float(float* fp); */

/* convert float array val[n] to and from vax float */
void vaxf_to_local(float *val, const int *n, int *errcode);
void local_to_vaxf(float *val, const int *n, int *errcode);
void local_to_ieee_float(float *val, const int *n, int *errcode);
void local_to_ieee_double(double *val, const int *n, int *errcode);
void ieee_float_to_local(float *val, const int *n, int *errcode);
void ieee_double_to_local(double *val, const int *n, int *errcode);

#endif /* ENDIAN_CONVERT */
