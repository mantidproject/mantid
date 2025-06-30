/** This class defines data types which are used as part of the NeXus API.
 * They should more properly be moved into NexusFile, when the nexus layer has been cleaned up.
 * OR all type and enum definitions in NexusFile all moved here.
 */

#pragma once

#include "MantidNexus/DllConfig.h"
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

typedef const char CONSTCHAR;

#define NX5SIGNATURE 959695

/*
 * Any new NXaccess options should be numbered in 2^n format
 * (8, 16, 32, etc) so that they can be bit masked and tested easily.
 */

constexpr int NX_UNLIMITED = -1;

constexpr int NX_MAXRANK = 32;
constexpr int NX_MAXNAMELEN = 64;
constexpr int NX_MAXADDRESSLEN = 1024;

constexpr int NXMAXSTACK = 50;

typedef int64_t hid_t;
typedef uint64_t hsize_t;

struct NexusFile5 {
  struct iStack5 {
    char irefn[1024];
    hid_t iVref;
    hsize_t iCurrentIDX;
  } iStack5[NXMAXSTACK];
  hid_t iFID;
  hid_t iCurrentG;
  hid_t iCurrentD;
  hid_t iCurrentS;
  hid_t iCurrentT;
  hid_t iCurrentA;
  hsize_t iCurrentIDX;
  int iNX;
  int iStackPtr;
  char name_ref[1024];
  char name_tmp[1024];
};

typedef NexusFile5 *pNexusFile5;
typedef NexusFile5 *NXhandle;

typedef char NXname[128];

/** \enum NXaccess
 * NeXus file access codes.
 * \li READ read-only
 * \li RDWR open an existing file for reading and writing.
 * \li CREATE5 create a NeXus HDF-5 file.
 */
enum class NXaccess : unsigned int {
  READ = 1,   //< 0x0001u = 0001
  RDWR = 2,   //< 0x0002u = 0010
  CREATE5 = 5 //< 0x0005u = 0101
};

MANTID_NEXUS_DLL std::ostream &operator<<(std::ostream &os, const NXaccess &value);

typedef struct {
  char *iname;
  int type;
} info_type, *pinfo;

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
enum class NXstatus : const int { NX_OK = 1, NX_ERROR = 0, NX_EOD = -1 };

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
  static int const FLOAT32 = 5;
  static int const FLOAT64 = 6;
  static int const INT8 = 20;
  static int const UINT8 = 21;
  static int const BOOLEAN = 21;
  static int const INT16 = 22;
  static int const UINT16 = 23;
  static int const INT32 = 24;
  static int const UINT32 = 25;
  static int const INT64 = 26;
  static int const UINT64 = 27;
  static int const CHAR = 4;
  static int const BINARY = 21;
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
} // namespace Mantid::Nexus
