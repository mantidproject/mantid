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
using spec2index_map = std::unordered_map<specnum_t, size_t>;
/// Map with key = detector ID, value = workspace index
using detid2index_map = std::unordered_map<detid_t, size_t>;
/// Map single det ID of group to its members
using det2group_map = std::unordered_map<detid_t, std::set<detid_t>>;
}

#endif // MANTID_API_SPECTRADETECTORMAP_TYPES
