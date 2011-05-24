#ifndef MANTID_API_SPECTRADETECTORMAP_TYPES
#define MANTID_API_SPECTRADETECTORMAP_TYPES
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/IDTypes.h"

#ifndef HAS_UNORDERED_MAP_H
#include <map>
#else
#include <tr1/unordered_map>
#endif

namespace Mantid
{

#ifndef HAS_UNORDERED_MAP_H
  /// Map with key = spectrum number, value = workspace index
  typedef std::map<specid_t,size_t> spec2index_map;
  /// Map with key = workspace index, value = spectrum number
  typedef std::map<size_t, specid_t> index2spec_map;
  /// Map with key = detector ID, value = workspace index
  typedef std::map<detid_t, size_t> detid2index_map;
  /// Map with key = workspace index, value = detector ID
  typedef std::map<size_t, detid_t> index2detid_map;
#else
  /// Map with key = spectrum number, value = workspace index
  typedef std::tr1::unordered_map<specid_t,size_t> spec2index_map;
  /// Map with key = workspace index, value = spectrum number
  typedef std::tr1::unordered_map<size_t,specid_t> spec2index_map;
  /// Map with key = detector ID, value = workspace index
  typedef std::tr1::unordered_map<detid_t, size_t> detid2index_map;
  /// Map with key = workspace index, value = detector ID
  typedef std::tr1::unordered_map<size_t, detid_t> index2detid_map;
#endif

}

#endif //MANTID_API_SPECTRADETECTORMAP_TYPES
