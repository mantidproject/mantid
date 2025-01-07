#ifndef NAPICONFIG_H
#define NAPICONFIG_H

#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

#define HAVE_HDF4
#define WITH_HDF4

#define HAVE_HDF5
#define WITH_HDF5

// ----- contents of nxconfig.h - START
// not enabling HAVE_FTIME - not used?

// not enabling HAVE_TZSET - not used?

// not enabling HAVE_STRDUP - not used?

// not enabling HAVE_LONG_LONG_INT - only in tests

// not enabling HAVE_UNSIGNED_LONG_LONG_INT - not used?

// should be standard everywhere
#define HAVE_STDINT_H 1

// not enabling HAVE_INTTYPES_H - stdint.h should be everywhere
// ----- contents of nxconfig.h - END

/*
 * Integer type definitions
 *
 * int32_t etc will be defined by configure in nxconfig.h
 * if they exist; otherwise include an appropriate header
 */
#include <stdint.h>

#endif /* NAPICONFIG_H */
