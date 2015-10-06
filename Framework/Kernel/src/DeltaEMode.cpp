//
#include "MantidKernel/DeltaEMode.h"
#include "MantidKernel/Exception.h"

#include <boost/algorithm/string.hpp>
#include <map>
#include <sstream>

namespace Mantid {
namespace Kernel {
namespace // unnamed
    {
struct ModeIndex {
  ModeIndex() {
    index.insert(std::make_pair(DeltaEMode::Elastic, "Elastic"));
    index.insert(std::make_pair(DeltaEMode::Direct, "Direct"));
    index.insert(std::make_pair(DeltaEMode::Indirect, "Indirect"));
    index.insert(std::make_pair(DeltaEMode::Undefined, "Undefined"));
  }
  std::map<DeltaEMode::Type, std::string> index;
};
/// Returns the map storing the mode->string lookup
ModeIndex &typeStringLookup() {
  static ModeIndex typeLookup;
  return typeLookup;
}
}

/**
 * Returns the string list of available modes
 * @return
 */
const std::vector<std::string> DeltaEMode::availableTypes() {
  const ModeIndex &lookup = typeStringLookup();
  std::vector<std::string> modes;
  modes.reserve(lookup.index.size());
  size_t index(0);
  for (auto iter = lookup.index.begin(); iter != lookup.index.end(); ++iter) {
    if (iter->first == DeltaEMode::Undefined)
      continue;
    modes.push_back(iter->second);
    ++index;
  }
  return modes;
}

/**
 * Return a string representation of the given mode
 * @param mode An enumeration of an energy transfer mode
 * @return The string representation
 */
std::string DeltaEMode::asString(const Type mode) {
  const ModeIndex &lookup = typeStringLookup();
  auto iter = lookup.index.find(mode);
  if (iter != lookup.index.end()) {
    return iter->second;
  } else {
    std::ostringstream os;
    os << "DeltaEMode::asString - Unknown energy transfer mode: " << mode;
    throw std::invalid_argument(os.str());
  }
}

/**
 * Returns the emode from the given string. Throws if the mode is not known
 * @param modeStr A string containing an energy transfer mode
 * @return The corresponding enumeration
 */
DeltaEMode::Type DeltaEMode::fromString(const std::string &modeStr) {
  const ModeIndex &lookup = typeStringLookup();
  for (auto iter = lookup.index.begin(); iter != lookup.index.end(); ++iter) {
    if (boost::iequals(modeStr, iter->second)) // case-insensitive
    {
      return iter->first;
    }
  }
  // Unknown mode
  throw std::invalid_argument(
      "DeltaEMode::fromString - Unknown energy transfer mode: " + modeStr);
}
}
}
