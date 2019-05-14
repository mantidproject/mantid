// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/DeltaEMode.h"

#include <boost/algorithm/string/predicate.hpp>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace Mantid {
namespace Kernel {
namespace // unnamed
{
struct ModeIndex {
  std::map<DeltaEMode::Type, std::string> index{
      {DeltaEMode::Elastic, "Elastic"},
      {DeltaEMode::Direct, "Direct"},
      {DeltaEMode::Indirect, "Indirect"},
      {DeltaEMode::Undefined, "Undefined"}};
};
/// Returns the map storing the mode->string lookup
ModeIndex &typeStringLookup() {
  static ModeIndex typeLookup;
  return typeLookup;
}
} // namespace

/**
 * Returns the string list of available modes
 * @return
 */
const std::vector<std::string> DeltaEMode::availableTypes() {
  const ModeIndex &lookup = typeStringLookup();
  std::vector<std::string> modes;
  modes.reserve(lookup.index.size());
  for (const auto &iter : lookup.index) {
    if (iter.first == DeltaEMode::Undefined)
      continue;
    modes.push_back(iter.second);
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

  auto emode = std::find_if(
      lookup.index.cbegin(), lookup.index.cend(),
      [&modeStr](auto it) { return boost::iequals(it.second, modeStr); });

  if (emode != lookup.index.cend()) {
    return emode->first;
  }

  // Unknown mode
  throw std::invalid_argument(
      "DeltaEMode::fromString - Unknown energy transfer mode: " + modeStr);
}
} // namespace Kernel
} // namespace Mantid
