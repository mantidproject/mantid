#ifndef MANTID_API_SPECTRADETECTORMAP_TYPES
#define MANTID_API_SPECTRADETECTORMAP_TYPES
//------------------------------------------------------------------------------
// Includes
//------------------------------------------------------------------------------
#include "MantidGeometry/IDTypes.h"
#include <unordered_map>
#include <set>

namespace Mantid {

/// Map with key = spectrum number, value = workspace index
typedef std::unordered_map<specnum_t, size_t> spec2index_map;
/// Map with key = detector ID, value = workspace index
typedef std::unordered_map<detid_t, size_t> detid2index_map;
/// Map single det ID of group to its members
typedef std::unordered_map<detid_t, std::set<detid_t>> det2group_map;
}

#endif // MANTID_API_SPECTRADETECTORMAP_TYPES
