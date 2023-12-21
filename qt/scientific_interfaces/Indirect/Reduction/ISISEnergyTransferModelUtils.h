// Mantid Repository : https://github.ervicecom/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <boost/algorithm/string.hpp>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidQtWidgets/Common/ConfiguredAlgorithm.h"

#include <filesystem>

using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}

MatrixWorkspace_sptr getADSMatrixWorkspace(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(workspaceName);
}

WorkspaceGroup_sptr getADSWorkspaceGroup(std::string const &workspaceName) {
  return AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(workspaceName);
}

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

std::string createRangeString(std::size_t const &from, std::size_t const &to) {
  return std::to_string(from) + "-" + std::to_string(to);
}

std::string createGroupString(std::size_t const &start, std::size_t const &size) {
  return createRangeString(start, start + size - 1);
}

std::string createGroupingString(std::size_t const &groupSize, std::size_t const &numberOfGroups,
                                 std::size_t const &spectraMin) {
  auto groupingString = createRangeString(spectraMin, spectraMin + groupSize - 1);
  for (auto i = spectraMin + groupSize; i < spectraMin + groupSize * numberOfGroups; i += groupSize)
    groupingString += "," + createGroupString(i, groupSize);
  return groupingString;
}

std::string createDetectorGroupingString(std::size_t const &groupSize, std::size_t const &numberOfGroups,
                                         std::size_t const &numberOfDetectors, std::size_t const &spectraMin) {
  const auto groupingString = createGroupingString(groupSize, numberOfGroups, spectraMin);
  const auto remainder = numberOfDetectors % numberOfGroups;
  if (remainder == 0)
    return groupingString;
  return groupingString + "," +
         createRangeString(spectraMin + numberOfDetectors - remainder, spectraMin + numberOfDetectors - 1);
}

std::string createDetectorGroupingString(std::size_t const &numberOfDetectors, std::size_t const &numberOfGroups,
                                         std::size_t const &spectraMin) {
  const auto groupSize = numberOfDetectors / numberOfGroups;
  if (groupSize == 0)
    return createRangeString(spectraMin, spectraMin + numberOfDetectors - 1);
  return createDetectorGroupingString(groupSize, numberOfGroups, numberOfDetectors, spectraMin);
}

double getSampleLog(const MatrixWorkspace_const_sptr &workspace, std::vector<std::string> const &logNames,
                    double const &defaultValue) {
  for (auto const &logName : logNames) {
    if (workspace->run().hasProperty(logName))
      return workspace->getLogAsSingleValue(logName);
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
    auto workspace = getADSMatrixWorkspace(temporaryWorkspace);
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