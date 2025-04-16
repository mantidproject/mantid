/** This class defines data types which are used as part of the NeXus API.
 * They should more properly be moved into NeXusFile, when the nexus layer has been cleaned up.
 * OR all type and enum definitions in NeXusFile all moved here.
 */

#pragma once

#include "MantidNexus/DllConfig.h"
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

/** \enum NXaccess
 * NeXus file access codes.
 * \li NXACC_READ read-only
 * \li NXACC_RDWR open an existing file for reading and writing.
 * \li NXACC_CREATE5 create a NeXus HDF-5 file.
 */
enum NXaccess : unsigned int { NXACC_READ = 0x0000u, NXACC_RDWR = 0x0001u, NXACC_CREATE5 = 0x0002u };

/** \enum NXentrytype
 * Describes the type of entry in a NeXus file, either group or dataset
 * \li group the entry is a group
 * \li sds the entry is a dataset (class SDS)
 */
enum class NXentrytype { group = 0, sds = 1 };

/**
 * \struct NXlink
 * Represents a link between entries in a NeXus file
 * \li targetPath path to item to link
 * \li linkType 0 for group link, 1 for SDS link
 */
typedef struct {
  std::string targetPath; /* path to item to link */
  NXentrytype linkType;   /* HDF5: 0 for group link, 1 for SDS link */
} NXlink;

/** \enum NXstatus
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

/** \class NXnumtype
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
class MANTID_NEXUS_DLL NXnumtype {
public:
  static int const FLOAT32 = NX_FLOAT32;
  static int const FLOAT64 = NX_FLOAT64;
  static int const INT8 = NX_INT8;
  static int const UINT8 = NX_UINT8;
  static int const BOOLEAN = NX_BOOLEAN;
  static int const INT16 = NX_INT16;
  static int const UINT16 = NX_UINT16;
  static int const INT32 = NX_INT32;
  static int const UINT32 = NX_UINT32;
  static int const INT64 = NX_INT64;
  static int const UINT64 = NX_UINT64;
  static int const CHAR = NX_CHAR;
  static int const BINARY = NX_BINARY;
  static int const BAD = -1;

private:
  int m_val;
  static int validate_val(int const x);

public:
  NXnumtype();
  NXnumtype(int const val);
  NXnumtype &operator=(int const);
  operator int() const;
  operator std::string() const;
};

MANTID_NEXUS_DLL std::ostream &operator<<(std::ostream &os, const NXnumtype &value);

/**
 * The available compression types. These are all ignored in xml files.
 * \li NONE no compression
 * \li LZW Lossless Lempel Ziv Welch compression (recommended)
 * \li RLE Run length encoding (only HDF-4)
 * \li HUF Huffmann encoding (only HDF-4)
 * \ingroup cpp_types
 */
enum class NXcompression { CHUNK = 0, NONE = 100, LZW = 200, RLE = 300, HUF = 400 };

// forward declare
namespace NeXus {

// TODO change to std::size_t
typedef std::int64_t dimsize_t;
// TODO replace all instances with DimArray
typedef std::vector<dimsize_t> DimVector; ///< use specifically for the dims array
//  TODO this is probably the same as DimVector
typedef std::vector<dimsize_t> DimSizeVector; ///< used for start, size, chunk, buffsize, etc.

typedef std::pair<std::string, std::string> Entry;
typedef std::map<std::string, std::string> Entries;

/**
 * This structure holds the type and dimensions of a primative field/array.
 */
struct Info {
  /** The primative type for the field. */
  NXnumtype type;
  /** The dimensions of the file. */
  DimVector dims;
};

/** Information about an attribute. */
struct AttrInfo {
  /** The primitive type for the attribute. */
  NXnumtype type;
  /** The length of the attribute */
  std::size_t length;
  /** The name of the attribute. */
  std::string name;
};

/** Forward declare of NeXus::File */
class File;
} // namespace NeXus

constexpr unsigned NX_MAXRANK(32);
constexpr NeXus::dimsize_t NX_UNLIMITED(-1); // 0xffffffffffffffffUL; // AKA max of unsigned long
