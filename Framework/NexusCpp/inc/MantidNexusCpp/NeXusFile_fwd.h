/** This class defines data types which are used as part of the NeXus API.
 * They should more properly be moved into NeXusFile, when the nexus layer has been cleaned up.
 * OR all type and enum definitions in NeXusFile all moved here.
 */

#pragma once

#include "MantidNexusCpp/DllConfig.h"
#include <iosfwd>

typedef const char CONSTCHAR;

typedef void *NXhandle; /* really a pointer to a NexusFile structure */
typedef char NXname[128];

/** \enum NXaccess_mode
 * NeXus file access codes.
 * \li NXACC_READ read-only
 * \li NXACC_RDWR open an existing file for reading and writing.
 * \li NXACC_CREATE create a NeXus HDF-4 file
 * \li NXACC_CREATE4 create a NeXus HDF-4 file -- this is discouraged, and will no loger be supported
 * \li NXACC_CREATE5 create a NeXus HDF-5 file.
 * \li NXACC_CREATEXML create a NeXus XML file -- this is no longer be supported, exists for legacy reasons
 * \li NXACC_CHECKNAMESYNTAX Check names conform to NeXus allowed characters.
 */
typedef enum {
  NXACC_READ = 1,
  NXACC_RDWR = 2,
  NXACC_CREATE = 3,
  NXACC_CREATE4 = 4,
  NXACC_CREATE5 = 5,
  NXACC_CREATEXML = 6,
  NXACC_TABLE = 8,
  NXACC_NOSTRIP = 128,
  NXACC_CHECKNAMESYNTAX = 256
} NXaccess_mode;

/**
 * A combination of options from #NXaccess_mode
 */
typedef int NXaccess;

typedef struct {
  char *iname;
  int type;
} info_type, *pinfo;

typedef struct {
  long iTag;             /* HDF4 variable */
  long iRef;             /* HDF4 variable */
  char targetPath[1024]; /* path to item to link */
  int linkType;          /* HDF5: 0 for group link, 1 for SDS link */
} NXlink;

/**
 * Special codes for NeXus file status.
 * \li OKAY success +1.
 * \li ERROR error 0
 * \li EOD end of file -1
 * \ingroup cpp_types
 */
enum class NXstatus : const int { NX_OK = 1, NX_ERROR = 0, NX_EOD = -1 };

/**
 * \ingroup c_types
 * \def NX_FLOAT32
 * 32 bit float
 * \def NX_FLOAT64
 * 64 bit float == double
 * \def NX_INT8
 * 8 bit integer == byte
 * \def NX_UINT8
 * 8 bit unsigned integer
 * \def NX_INT16
 * 16 bit integer
 * \def NX_UINT16
 * 16 bit unsigned integer
 * \def NX_INT32
 * 32 bit integer
 * \def NX_UINT32
 * 32 bit unsigned integer
 * \def NX_CHAR
 * 8 bit character
 * \def NX_BINARY
 * lump of binary data == NX_UINT8
 */
/*--------------------------------------------------------------------------*/

/* Map NeXus to HDF types */
constexpr int NX_FLOAT32{5};
constexpr int NX_FLOAT64{6};
constexpr int NX_INT8{20};
constexpr int NX_UINT8{21};
constexpr int NX_BOOLEAN{NX_UINT8};
constexpr int NX_INT16{22};
constexpr int NX_UINT16{23};
constexpr int NX_INT32{24};
constexpr int NX_UINT32{25};
constexpr int NX_INT64{26};
constexpr int NX_UINT64{27};
constexpr int NX_CHAR{4};
constexpr int NX_BINARY{21};

/**
 * The primitive types published by this API.
 * \li FLOAT32 float.
 * \li FLOAT64 double
 * \li INT8 int8_t
 * \li UINT8 uint8_t
 * \li INT16 int16_t
 * \li UINT16 uint16_t
 * \li INT32 int32_t
 * \li UINT32 uint32_t
 * \li INT64 int8_t if available on the machine
 * \li UINT64 uint8_t if available on the machine
 * \ingroup cpp_types
 */
enum class NXnumtype : const int {
  FLOAT32 = NX_FLOAT32,
  FLOAT64 = NX_FLOAT64,
  INT8 = NX_INT8,
  UINT8 = NX_UINT8,
  BOOLEAN = NX_BOOLEAN,
  INT16 = NX_INT16,
  UINT16 = NX_UINT16,
  INT32 = NX_INT32,
  UINT32 = NX_UINT32,
  INT64 = NX_INT64,
  UINT64 = NX_UINT64,
  CHAR = NX_CHAR,
  BINARY = NX_BINARY,
  BAD = -1
};

MANTID_NEXUSCPP_DLL std::ostream &operator<<(std::ostream &stm, const NXstatus status);
MANTID_NEXUSCPP_DLL std::ostream &operator<<(std::ostream &stm, const NXnumtype type);
