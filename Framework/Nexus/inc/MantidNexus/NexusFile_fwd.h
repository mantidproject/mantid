/** This class defines data types which are used as part of the NeXus API.
 * They should more properly be moved into NexusFile, when the nexus layer has been cleaned up.
 * OR all type and enum definitions in NexusFile all moved here.
 */

#pragma once

#include "MantidNexus/DllConfig.h"
#include "MantidNexus/NexusAddress.h"
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

constexpr int NX_MAXADDRESSLEN = 1024;

typedef int64_t hid_t;
typedef uint64_t hsize_t;

/** \enum NXaccess
 * NeXus file access codes.
 * these codes are taken directly from values used in hdf5 package
 * https://github.com/HDFGroup/hdf5/blob/develop/src/H5Fpublic.h
 * \li READ read-only. Same as H5F_ACC_RDONLY
 * \li RDWR open an existing file for reading and writing. Same as H5F_ACC_RDWR.
 * \li CREATE5 create a NeXus HDF-5 file. Same as H5F_ACC_TRUNC.
 */
enum class NXaccess : unsigned int { READ = 0x0000u, RDWR = 0x0001u, CREATE5 = 0x0002u };

MANTID_NEXUS_DLL std::ostream &operator<<(std::ostream &os, const NXaccess &value);

/** NexusFile5
 * NOTE this needs to either be moved to a separate file,
 * or entirely absorbed inside Nexus::File
 */
struct MANTID_NEXUS_DLL NexusFile5 {
  std::vector<hid_t> iStack5;
  hid_t iFID;
  hid_t iCurrentG;
  hid_t iCurrentD;
  hid_t iCurrentS;
  hid_t iCurrentT;
  Mantid::Nexus::NexusAddress groupaddr;

  // constructors
  NexusFile5() = delete;
  NexusFile5(std::string const &, NXaccess const);
  NexusFile5(NexusFile5 const &);
  // assignment
  NexusFile5 &operator=(NexusFile5 const &);
  // destructor
  ~NexusFile5();
};

typedef NexusFile5 *pNexusFile5;
typedef NexusFile5 *NXhandle;

typedef char NXname[128];

/** \enum NXentrytype
 * Describes the type of entry in a NeXus file, either group or dataset
 * \li group the entry is a group
 * \li sds the entry is a dataset (class SDS)
 */
enum class NXentrytype : int { group = 0, sds = 1 };

/**
 * \struct NXlink
 * Represents a link between entries in a NeXus file
 * \li targetAddress address to item to link
 * \li linkType 0 for group link, 1 for SDS link
 */
typedef struct {
  std::string targetAddress; /* address to item to link */
  NXentrytype linkType;      /* HDF5: 0 for group link, 1 for SDS link */
} NXlink;

/**
 * Special codes for NeXus file status.
 * \li OKAY success +1.
 * \li ERROR error 0
 * \li EOD end of file -1
 * \ingroup cpp_types
 */
enum class NXstatus : const int { NX_OK = 1, NX_ERROR = 0 };

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

/** Forward declare of NeXus::File */
class File;
} // namespace Mantid::Nexus

constexpr std::size_t NX_MAXRANK(32);
constexpr Mantid::Nexus::dimsize_t NX_UNLIMITED(-1); // 0xffffffffffffffffUL; // AKA max of unsigned long
