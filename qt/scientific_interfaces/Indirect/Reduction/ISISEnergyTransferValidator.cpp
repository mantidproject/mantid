// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ISISEnergyTransferValidator.h"
#include "Common/WorkspaceUtils.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"

#include <filesystem>

using namespace Mantid::API;

namespace {
IAlgorithm_sptr loadAlgorithm(std::string const &filename, std::string const &outputName) {
  auto loader = AlgorithmManager::Instance().create("Load");
  loader->initialize();
  loader->setProperty("Filename", filename);
  loader->setProperty("OutputWorkspace", outputName);
  return loader;
}
} // namespace

namespace MantidQt {
namespace CustomInterfaces {

std::string IETDataValidator::validateConversionData(IETConversionData conversionData) {
  const int specMin = conversionData.getSpectraMin();
  const int specMax = conversionData.getSpectraMax();

  if (specMin > specMax) {
    return "Minimum spectra must be less than maximum spectra.";
  }

  return "";
}

std::vector<std::string> IETDataValidator::validateBackgroundData(IETBackgroundData backgroundData,
                                                                  IETConversionData conversionData,
                                                                  std::string firstFileName, bool isRunFileValid) {
  std::vector<std::string> errors;

  if (isRunFileValid) {
    std::filesystem::path rawFileInfo(firstFileName);
    std::string name = rawFileInfo.filename().string();

    int specMin = conversionData.getSpectraMin();
    int specMax = conversionData.getSpectraMax();

    auto loadAlg = loadAlgorithm(firstFileName, name);
    if (loadAlg->existsProperty("LoadLogFiles")) {
      loadAlg->setProperty("LoadLogFiles", false);
    }
    loadAlg->setPropertyValue("SpectrumMin", std::to_string(specMin));
    loadAlg->setPropertyValue("SpectrumMax", std::to_string(specMax));
    loadAlg->execute();

    if (backgroundData.getRemoveBackground()) {
      const int backgroundStart = backgroundData.getBackgroundStart();
      const int backgroundEnd = backgroundData.getBackgroundEnd();

      if (backgroundStart > backgroundEnd) {
        errors.push_back("Background Start must be less than Background End");
      }

      auto tempWs = WorkspaceUtils::getADSWorkspace(name);

      const double minBack = tempWs->x(0).front();
      const double maxBack = tempWs->x(0).back();

      if (backgroundStart < minBack) {
        errors.push_back("The Start of Background Removal is less than the minimum of the data range");
      }

      if (backgroundEnd > maxBack) {
        errors.push_back("The End of Background Removal is more than the maximum of the data range");
      }
    }
  }

  return errors;
}

std::string IETDataValidator::validateAnalysisData(IETAnalysisData analysisData) {
  if (analysisData.getUseDetailedBalance()) {
    if (analysisData.getDetailedBalance() == 0.0) {
      return "Detailed Balance must be more than 0 K";
    }
  }
  return "";
}

std::string IETDataValidator::validateDetectorGrouping(Mantid::API::AlgorithmRuntimeProps *groupingProperties,
                                                       std::size_t const &defaultSpectraMin,
                                                       std::size_t const &defaultSpectraMax) {
  if (!groupingProperties->existsProperty("GroupingMethod"))
    return "Please provide a grouping method.";

  std::string groupingType = groupingProperties->getProperty("GroupingMethod");
  if (groupingType == "File") {
    if (!groupingProperties->existsProperty("MapFile"))
      return "Mapping file is invalid.";
  } else if (groupingType == "Custom") {
    if (!groupingProperties->existsProperty("GroupingString"))
      return "Please supply a custom grouping for detectors.";
    else {
      std::string customString = groupingProperties->getProperty("GroupingString");
      return checkCustomGroupingNumbersInRange(getCustomGroupingNumbers(customString), defaultSpectraMin,
                                               defaultSpectraMax);
    }
  } else if (groupingType == "Groups") {
    int nGroups = groupingProperties->getProperty("NGroups");
    if (nGroups < 1)
      return "The number of groups must be a positive number.";
  }
  return "";
}

std::vector<std::size_t> IETDataValidator::getCustomGroupingNumbers(std::string const &customString) {
  std::vector<std::string> customGroupingStrings;
  std::vector<std::size_t> customGroupingNumbers;
  // Get the numbers from customString and store them in customGroupingStrings
  boost::split(customGroupingStrings, customString, boost::is_any_of(" ,-+:"));
  for (const auto &string : customGroupingStrings)
    if (!string.empty())
      customGroupingNumbers.emplace_back(std::stoull(string));
  return customGroupingNumbers;
}

std::string IETDataValidator::checkCustomGroupingNumbersInRange(std::vector<std::size_t> const &customGroupingNumbers,
                                                                std::size_t const &spectraMin,
                                                                std::size_t const &spectraMax) const {
  if (std::any_of(customGroupingNumbers.cbegin(), customGroupingNumbers.cend(),
                  [this, spectraMin, spectraMax](auto number) {
                    return !this->numberInCorrectRange(number, spectraMin, spectraMax);
                  })) {
    return "Please supply a custom grouping within the correct range";
  } else {
    return "";
  }
}

bool IETDataValidator::numberInCorrectRange(std::size_t const &spectraNumber, std::size_t const &spectraMin,
                                            std::size_t const &spectraMax) const {
  if (spectraMin != 0 && spectraMax != 0) {
    return spectraNumber >= spectraMin && spectraNumber <= spectraMax;
  }
  return false;
}

} // namespace CustomInterfaces
} // namespace MantidQt