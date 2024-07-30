// Mantid Repository : https://github.ervicecom/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Run.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include <boost/algorithm/string.hpp>

#include <filesystem>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;

namespace MantidQt::CustomInterfaces {

IAlgorithm_sptr loadAlgorithm(std::string const &filename, std::string const &outputName) {
  auto loader = AlgorithmManager::Instance().create("Load");
  loader->initialize();
  loader->setProperty("Filename", filename);
  loader->setProperty("OutputWorkspace", outputName);
  return loader;
}

void deleteWorkspace(std::string const &name) {
  auto deleter = AlgorithmManager::Instance().create("DeleteWorkspace");
  deleter->initialize();
  deleter->setProperty("Workspace", name);
  deleter->execute();
}

std::tuple<std::string, std::string> parseInputFiles(std::string const &inputFiles) {
  std::string rawFile = inputFiles.substr(0, inputFiles.find(',')); // getting the name of the first file
  std::filesystem::path rawFileInfo(rawFile);
  return {rawFile, rawFileInfo.filename().string()};
}

std::vector<int> createDetectorList(int const spectraMin, int const spectraMax) {
  std::vector<int> detectorList(spectraMax - spectraMin + 1);
  std::iota(detectorList.begin(), detectorList.end(), spectraMin);
  return detectorList;
}

double getSampleLog(const MatrixWorkspace_const_sptr &workspace, std::vector<std::string> const &logNames,
                    double const &defaultValue) {
  const auto it = std::find_if(logNames.cbegin(), logNames.cend(),
                               [&workspace](const auto &logName) { return workspace->run().hasProperty(logName); });
  if (it != logNames.cend()) {
    return workspace->getLogAsSingleValue(*it);
  }
  return defaultValue;
}

double loadSampleLog(std::string const &filename, std::vector<std::string> const &logNames,
                     double const &defaultValue) {
  auto const temporaryWorkspace("__sample_log_subject");
  auto loader = loadAlgorithm(filename, temporaryWorkspace);
  loader->execute();

  double value(defaultValue);

  if (doesExistInADS(temporaryWorkspace)) {
    auto workspace = getADSWorkspace(temporaryWorkspace);
    value = getSampleLog(workspace, logNames, defaultValue);
    deleteWorkspace(workspace->getName());
  }

  return value;
}

std::vector<std::size_t> getCustomGroupingNumbers(std::string const &customString) {
  std::vector<std::string> customGroupingStrings;
  std::vector<std::size_t> customGroupingNumbers;
  // Get the numbers from customString and store them in customGroupingStrings
  boost::split(customGroupingStrings, customString, boost::is_any_of(" ,-+:"));
  for (const auto &string : customGroupingStrings)
    if (!string.empty())
      customGroupingNumbers.emplace_back(std::stoull(string));
  return customGroupingNumbers;
}
} // namespace MantidQt::CustomInterfaces