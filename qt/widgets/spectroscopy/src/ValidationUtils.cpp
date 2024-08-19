// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/ValidationUtils.h"

#include <boost/algorithm/string.hpp>

namespace {

bool isWithinRange(std::size_t const &spectraNumber, std::size_t const &spectraMin, std::size_t const &spectraMax) {
  return spectraMax != 0 && spectraNumber >= spectraMin && spectraNumber <= spectraMax;
}

} // namespace

namespace MantidQt {
namespace CustomInterfaces {
namespace ValidationUtils {

bool groupingStrInRange(std::string const &customString, std::size_t const &spectraMin, std::size_t const &spectraMax) {
  if (customString.empty()) {
    return false;
  }

  std::vector<std::string> groupingStrings;
  std::vector<std::size_t> groupingNumbers;
  // Split the custom string by its delimiters
  boost::split(groupingStrings, customString, boost::is_any_of(" ,-+:"));
  // Remove empty strings
  groupingStrings.erase(std::remove_if(groupingStrings.begin(), groupingStrings.end(),
                                       [](std::string const &str) { return str.empty(); }),
                        groupingStrings.end());
  // Transform strings to size_t's
  std::transform(groupingStrings.cbegin(), groupingStrings.cend(), std::back_inserter(groupingNumbers),
                 [](std::string const &str) { return std::stoull(str); });
  // Find min and max elements
  auto const range = std::minmax_element(groupingNumbers.cbegin(), groupingNumbers.cend());
  return isWithinRange(*range.first, spectraMin, spectraMax) && isWithinRange(*range.second, spectraMin, spectraMax);
}

std::optional<std::string> validateGroupingProperties(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties,
                                                      std::size_t const &spectraMin, std::size_t const &spectraMax) {
  std::string groupingType = properties->getProperty("GroupingMethod");
  if (groupingType == "File" && !properties->existsProperty("GroupingFile")) {
    return "Please supply a map file for grouping detectors.";
  } else if (groupingType == "Custom") {
    if (!properties->existsProperty("GroupingString")) {
      return "Please supply a custom string for grouping detectors.";
    }
    std::string customString = properties->getProperty("GroupingString");
    return groupingStrInRange(customString, spectraMin, spectraMax)
               ? std::nullopt
               : std::optional<std::string>("Please supply a custom grouping within the correct range.");
  } else if (groupingType == "Groups") {
    auto const numberOfSpectra = spectraMax - spectraMin + 1;
    auto const numberOfGroups = std::stoull(properties->getPropertyValue("NGroups"));
    if (numberOfGroups > numberOfSpectra) {
      return "The number of groups must be less or equal to the number of spectra (" + std::to_string(numberOfSpectra) +
             ").";
    }
  }
  return std::nullopt;
}

} // namespace ValidationUtils
} // namespace CustomInterfaces
} // namespace MantidQt
