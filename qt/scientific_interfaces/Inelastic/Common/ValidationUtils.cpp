// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ValidationUtils.h"
#include "InterfaceUtils.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ValidationUtils {

std::optional<std::string> validateGroupingProperties(std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> properties,
                                                      std::size_t const &spectraMin, std::size_t const &spectraMax) {
  std::string groupingType = properties->getProperty("GroupingMethod");
  if (groupingType == "File" && !properties->existsProperty("MapFile")) {
    return "Please supply a map file for grouping detectors.";
  } else if (groupingType == "Custom") {
    if (!properties->existsProperty("GroupingString")) {
      return "Please supply a custom string for grouping detectors.";
    }
    std::string customString = properties->getProperty("GroupingString");
    return InterfaceUtils::groupingStrInRange(customString, spectraMin, spectraMax)
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
