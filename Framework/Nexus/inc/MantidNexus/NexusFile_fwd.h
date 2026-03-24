/** This class defines data types which are used as part of the Nexus API.
 */

#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusAddress.h"
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

// forward declare typedefs from hdf5
typedef int64_t hid_t;
typedef uint64_t hsize_t;
typedef int herr_t;

/** \enum NXaccess
 * Nexus file access codes.
 * these codes are taken directly from values used in hdf5 package
 * https://github.com/HDFGroup/hdf5/blob/develop/src/H5Fpublic.h
 * \li READ read-only. Same as H5F_ACC_RDONLY
 * \li RDWR open an existing file for reading and writing. Same as H5F_ACC_RDWR.
 * \li CREATE5 create a Nexus HDF-5 file. Same as H5F_ACC_TRUNC.
 */
enum class NXaccess : unsigned int { READ = 0x0000u, RDWR = 0x0001u, CREATE5 = 0x0002u };

MANTID_NEXUS_DLL std::ostream &operator<<(std::ostream &os, const NXaccess &value);

/** \enum NXentrytype
 * Describes the type of entry in a Nexus file, either group or dataset
 * \li group the entry is a group
 * \li sds the entry is a dataset (class SDS)
 */
enum class NXentrytype : int { group = 0, sds = 1 };

/**
 * \struct NXlink
 * Represents a link between entries in a Nexus file
 * \li targetAddress address to item to link
 * \li linkType 0 for group link, 1 for SDS link
 */
typedef struct {
  std::string targetAddress; /* address to item to link */
  NXentrytype linkType;      /* HDF5: 0 for group link, 1 for SDS link */
} NXlink;

/*--------------------------------------------------------------------------*/

/** \class NXnumtype
 * The primitive types published by this API.
 * \li FLOAT32 32-bit float.
 * \li FLOAT64 64-bit double
 * \li INT8 byte-width integer -- int8_t
 * \li UINT8 byte-width unsigned integer -- uint8_t
 * \li INT16 double-byte-width integer -- int16_t
 * \li UINT16 double-byte-width unsigned integer -- uint16_t
 * \li INT32 quad-byte-width integer -- int32_t
 * \li UINT32 quad-byte-width unsigned integer -- uint32_t
 * \li INT64 eight-byte-width integer -- int8_t if available on the machine
 * \li UINT64 eight-byte-width integer -- uint8_t if available on the machine
 * \li BINARY lump of binary data, same as UINT8
 * \ingroup cpp_types
 */
class MANTID_NEXUS_DLL NXnumtype {
public:
  // first hexadigit: 2 = float, 1 = signed int, 0 = unsigned int, F = special
  // second hexadigit: width in bytes
  static unsigned short constexpr FLOAT32 = 0x24u; // 10 0100 = 0x24
  static unsigned short constexpr FLOAT64 = 0x28u; // 10 1000 = 0x28
  static unsigned short constexpr INT8 = 0x11u;    // 01 0001 = 0x11
  static unsigned short constexpr INT16 = 0x12u;   // 01 0010 = 0x12
  static unsigned short constexpr INT32 = 0x14u;   // 01 0100 = 0x14
  static unsigned short constexpr INT64 = 0x18u;   // 01 1000 = 0x18
  static unsigned short constexpr UINT8 = 0x01u;   // 00 0001 = 0x01
  static unsigned short constexpr UINT16 = 0x02u;  // 00 0010 = 0x02
  static unsigned short constexpr UINT32 = 0x04u;  // 00 0100 = 0x04
  static unsigned short constexpr UINT64 = 0x08u;  // 00 1000 = 0x08
  static unsigned short constexpr CHAR = 0xF0u;    // 11 0000 = 0xF0
  static unsigned short constexpr BINARY = 0xF1u;  // 11 0001 = 0xF1
  static unsigned short constexpr BAD = 0xFFu;     // 11 1111 = 0xFF

  // for &'ing with a type to check what it is:
  // & will be true (nonzero) if it is of type indicated; false (zero) else
  static unsigned short constexpr FLOAT_TYPE = 0x20u;
  static unsigned short constexpr SPECIAL_TYPE = 0x80u;

private:
  int m_val;
  static int validate_val(int const x);

public:
  NXnumtype();
  NXnumtype(int const val);
  NXnumtype &operator=(int const);
  operator int() const;
  operator std::string() const;

  // Will return true if the type is a float
  bool isFloat() { return m_val & FLOAT_TYPE; }
  // Will return true if the type is a special (char, binary, or bad)
  bool isSpecial() { return m_val & SPECIAL_TYPE; }
};

MANTID_NEXUS_DLL std::ostream &operator<<(std::ostream &os, const NXnumtype &value);

/**
 * The available compression types:
 * \li CHUNK chunk encoding
 * \li NONE no compression
 * \li LZW Lossless Lempel Ziv Welch compression (recommended)
 * \li RLE Run length encoding (only HDF-4)
 * \li HUF Huffmann encoding (only HDF-4)
 * \ingroup cpp_types
 */
enum class NXcompression { CHUNK, NONE, LZW, RLE, HUF };

MANTID_NEXUS_DLL std::ostream &operator<<(std::ostream &os, const NXcompression &value);

// forward declare
namespace Mantid::Nexus {

inline std::string const GROUP_CLASS_SPEC("NX_class");
inline std::string const UNKNOWN_GROUP_SPEC("NX_UNKNOWN_GROUP");
inline std::string const SCIENTIFIC_DATA_SET("SDS");

typedef hsize_t dimsize_t;
typedef std::vector<dimsize_t> DimVector;

typedef std::pair<std::string, std::string> Entry;
typedef std::map<std::string, std::string> Entries;

/**
 * This structure holds the type and dimensions of a primative field/array.
 * \li type: NXnumtype for data type
 * \li dims: DimVector, size is rank
 */
struct Info {
  /** The primative type for the field. */
  NXnumtype type;
  /** The dimensions of the file. */
  DimVector dims;
};

/** Information about an attribute.
 * \li type: NXnumtype for data type
 * \li length: length of attribute, if string (otherwise 1)
 * \li name: the name of thr attribute
 */
struct AttrInfo {
  /** The primitive type for the attribute. */
  NXnumtype type;
  /** The length of the attribute */
  std::size_t length;
  /** The name of the attribute. */
  std::string name;
};

/** Forward declare of Nexus::File */
class File;
} // namespace Mantid::Nexus

constexpr std::size_t NX_MAXRANK(32);
constexpr Mantid::Nexus::dimsize_t NX_UNLIMITED(-1); // AKA max of unsigned long, equivalent to H5S_UNLIMITED
