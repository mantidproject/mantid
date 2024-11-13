// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "ALCDataLoadingModel.h"
#include "ALCLatestFileFinder.h"
#include "IALCDataLoadingView.h"
#include "MuonAnalysisHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Run.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/Strings.h"

#include <Poco/ActiveResult.h>
#include <algorithm>

using namespace Mantid::API;

namespace MantidQt::CustomInterfaces {

ALCDataLoadingModel::ALCDataLoadingModel()
    : m_numDetectors(0), m_loadingData(false), m_directoryChanged(false), m_lastRunLoadedAuto(-2),
      m_wasLastAutoRange(false), m_minTime(0) {}

// Getters
bool ALCDataLoadingModel::getLoadingData() const { return m_loadingData; }

MatrixWorkspace_sptr ALCDataLoadingModel::getLoadedData() const { return m_loadedData; }

Mantid::API::MatrixWorkspace_sptr ALCDataLoadingModel::getWsForMuonInfo() const { return m_wsForInfo; }

double ALCDataLoadingModel::getMinTime() const { return m_minTime; }

std::vector<std::string> &ALCDataLoadingModel::getLogs() { return m_logs; }

std::string &ALCDataLoadingModel::getRunsText() { return m_runsText; }

std::vector<std::string> &ALCDataLoadingModel::getPeriods() { return m_periods; }

void ALCDataLoadingModel::cancelLoading() const { this->m_LoadingAlg->cancel(); }

// Setters
void ALCDataLoadingModel::setLoadingData(bool isLoading) { m_loadingData = isLoading; }

void ALCDataLoadingModel::setLoadedData(const MatrixWorkspace_sptr &data) { m_loadedData = data; }

void ALCDataLoadingModel::setDirectoryChanged(bool hasDirectoryChanged) { m_directoryChanged = hasDirectoryChanged; }

void ALCDataLoadingModel::setFilesToLoad(const std::vector<std::string> &files) { m_filesToLoad = files; }

void ALCDataLoadingModel::setLogs(const MatrixWorkspace_sptr &ws) {

  std::vector<std::string> logs;

  const auto &properties = ws->run().getProperties();
  std::transform(properties.cbegin(), properties.cend(), std::back_inserter(logs),
                 [](const auto &property) { return property->name(); });

  // sort alphabetically
  // cannot use standard sort alone as some logs are capitalised and some are
  // not
  std::sort(logs.begin(), logs.end(), [](const auto &log1, const auto &log2) {
    // compare logs char by char and return pair of non-equal elements
    const auto result =
        std::mismatch(log1.cbegin(), log1.cend(), log2.cbegin(), log2.cend(),
                      [](const auto &lhs, const auto &rhs) { return std::tolower(lhs) == std::tolower(rhs); });
    // compare the two elements to decide which log should go first
    return result.second != log2.cend() &&
           (result.first == log1.cend() || std::tolower(*result.first) < std::tolower(*result.second));
  });
  m_logs = logs;
}

void ALCDataLoadingModel::setPeriods(const Workspace_sptr &loadedWs) {

  size_t numPeriods = MuonAnalysisHelper::numPeriods(loadedWs);
  std::vector<std::string> periods;
  for (size_t i = 0; i < numPeriods; i++) {
    std::stringstream buffer;
    buffer << i + 1;
    periods.emplace_back(buffer.str());
  }
  m_periods = periods;
}
MatrixWorkspace_sptr ALCDataLoadingModel::exportWorkspace() {
  if (m_loadedData)
    return std::const_pointer_cast<MatrixWorkspace>(m_loadedData);
  return MatrixWorkspace_sptr{};
}

void ALCDataLoadingModel::setWsForMuonInfo(const std::string &filename) {

  IAlgorithm_sptr loadAlg = AlgorithmManager::Instance().create("Load");
  loadAlg->setChild(true); // Don't want workspaces in the ADS

  // We need logs only but we have to use Load
  // (can't use LoadMuonLogs as not all the logs would be
  // loaded), so we load the minimum amount of data, i.e., one spectrum
  loadAlg->setProperty("Filename", filename);
  loadAlg->setPropertyValue("SpectrumMin", "1");
  loadAlg->setPropertyValue("SpectrumMax", "1");
  loadAlg->setPropertyValue("OutputWorkspace", "__NotUsed");
  loadAlg->execute();

  Workspace_sptr loadedWs = loadAlg->getProperty("OutputWorkspace");
  setPeriods(loadedWs);

  double firstGoodData = loadAlg->getProperty("FirstGoodData");
  double timeZero = loadAlg->getProperty("TimeZero");
  m_minTime = firstGoodData - timeZero;

  m_wsForInfo = MuonAnalysisHelper::firstPeriod(loadedWs);

  setLogs(m_wsForInfo);
  // Update number of detectors for this new first run
  m_numDetectors = m_wsForInfo->getInstrument()->getNumberDetectors();
}

/**
 * Load new data into model
 */
void ALCDataLoadingModel::load(const std::string &log, const std::string &function, const std::string &calculationType,
                               const std::string &deadTimeType, const std::string &deadTimeFile,
                               const std::string &redPeriod, const std::optional<std::pair<double, double>> &timeRange,
                               const std::string &detectorGroupingType, const std::string &forwardGrouping,
                               const std::string &backwardGrouping, const std::string &alphaValue,
                               const bool &subtractIsChecked, const std::string &greenPeriod) {

  IAlgorithm_sptr alg = AlgorithmManager::Instance().create("PlotAsymmetryByLogValue");
  alg->setAlwaysStoreInADS(false); // Don't want workspaces in the ADS

  // Change first last run to WorkspaceNames
  alg->setProperty("WorkspaceNames", m_filesToLoad);
  alg->setProperty("LogValue", log);
  alg->setProperty("Function", function);
  alg->setProperty("Type", calculationType);
  alg->setProperty("DeadTimeCorrType", deadTimeType);
  alg->setProperty("Red", redPeriod);

  // If time limiting requested, set min/max times
  if (auto tRange = timeRange) {
    double timeMin = (*tRange).first;
    double timeMax = (*tRange).second;
    if (timeMin >= timeMax) {
      throw std::invalid_argument("Invalid time limits");
    }
    alg->setProperty("TimeMin", timeMin);
    alg->setProperty("TimeMax", timeMax);
  }

  // If corrections from custom file requested, set file property
  if (deadTimeType == "FromSpecifiedFile") {
    alg->setProperty("DeadTimeCorrFile", deadTimeFile);
  }

  // If custom grouping requested, set forward/backward groupings
  if (detectorGroupingType == "Custom") {
    alg->setProperty("ForwardSpectra", forwardGrouping);
    alg->setProperty("BackwardSpectra", backwardGrouping);
  }

  // Set alpha for balance parameter
  alg->setProperty("Alpha", alphaValue);

  // If Subtract checkbox is selected, set green period
  if (subtractIsChecked) {
    alg->setProperty("Green", greenPeriod);
  }

  alg->setPropertyValue("OutputWorkspace", "__NotUsed");

  // Execute async so we can show progress bar
  Poco::ActiveResult<bool> result(alg->executeAsync());
  while (!result.available()) {
    QCoreApplication::processEvents();
  }
  if (!result.error().empty()) {
    throw std::runtime_error(result.error());
  }

  // Set loading alg equal to alg
  this->m_LoadingAlg = alg;

  MatrixWorkspace_sptr tmp = alg->getProperty("OutputWorkspace");
  IAlgorithm_sptr sortAlg = AlgorithmManager::Instance().create("SortXAxis");
  sortAlg->setAlwaysStoreInADS(false); // Don't want workspaces in the ADS
  sortAlg->setProperty("InputWorkspace", tmp);
  sortAlg->setProperty("Ordering", "Ascending");
  sortAlg->setProperty("OutputWorkspace", "__NotUsed__");
  sortAlg->execute();

  m_loadedData = sortAlg->getProperty("OutputWorkspace");

  // If errors are properly caught, shouldn't happen
  assert(m_loadedData);
  // If subtract is not checked, only one spectrum,
  // else four spectra
  if (!subtractIsChecked) {
    assert(m_loadedData->getNumberHistograms() == 1);
  } else {
    assert(m_loadedData->getNumberHistograms() == 4);
  }
}

/**
 * Remove the run number from a full file path
 * @param file :: [input] full path which contains a run number
 * @return An integer representation of the run number
 */
int ALCDataLoadingModel::extractRunNumber(const std::string &file) {
  if (file.empty())
    return -1;

  auto returnVal = file;
  // Strip beginning of path to just the run (e.g. MUSR00015189.nxs)
  std::size_t found = returnVal.find_last_of("/\\");
  returnVal = returnVal.substr(found + 1);

  // Remove all non-digits
  returnVal.erase(std::remove_if(returnVal.begin(), returnVal.end(), [](unsigned char c) { return !std::isdigit(c); }),
                  returnVal.end());

  // Return run number as int (removes leading 0's)
  return std::stoi(returnVal);
}

std::string ALCDataLoadingModel::getPathFromFiles(const std::vector<std::string> &files) const {
  if (files.empty())
    return "";
  const auto firstDirectory = files[0u].substr(0u, files[0u].find_last_of("/\\"));
  // Lambda to compare directories from a path
  const auto hasSameDirectory = [&firstDirectory](const auto &path) {
    return path.substr(0u, path.find_last_of("/\\")) == firstDirectory;
  };
  const auto sameDirectory = std::all_of(files.cbegin(), files.cend(), hasSameDirectory);
  return sameDirectory ? firstDirectory : "Multiple Directories";
}

/**
 * If custom grouping is supplied, check all detector numbers are valid
 * @returns :: True if grouping OK, false if bad
 */
bool ALCDataLoadingModel::checkCustomGrouping(const std::string &detGroupingType, const std::string &forwardGrouping,
                                              const std::string &backwardGrouping) {
  if (detGroupingType != "Custom") {
    return true;
  }

  bool groupingOK = true;

  auto detectors = Mantid::Kernel::Strings::parseRange(isCustomGroupingValid(forwardGrouping, groupingOK));
  if (!groupingOK) {
    return false;
  }
  const auto backward = Mantid::Kernel::Strings::parseRange(isCustomGroupingValid(backwardGrouping, groupingOK));
  if (!groupingOK) {
    return false;
  }

  detectors.insert(detectors.end(), backward.begin(), backward.end());
  auto isOutsideDetRange = [this](const auto det) { return det < 0 || det > static_cast<int>(m_numDetectors); };

  return !std::any_of(detectors.cbegin(), detectors.cend(), isOutsideDetRange);
}

/**
 * Check basic group string is valid
 * i.e. does not contain letters or start with , or -
 * @param group :: the string of the grouping
 * @param isValid :: bool to say if the string is valid
 * @returns :: True if grouping OK, false if bad
 */
std::string ALCDataLoadingModel::isCustomGroupingValid(const std::string &group, bool &isValid) {

  auto isDecimal = [](const char c) { return c == '.'; };

  if (!std::isdigit(static_cast<unsigned char>(group[0])) ||
      std::any_of(std::begin(group), std::end(group), ::isalpha) ||
      std::any_of(std::begin(group), std::end(group), isDecimal)) {
    isValid = false;
    return "";
  }
  return group;
}

void ALCDataLoadingModel::updateAutoLoadCancelled() {
  m_lastRunLoadedAuto = -2; // Ensures negative if +1 to not be valid
  m_wasLastAutoRange = false;
}

/**
 * This timer runs every second when we are watching a directory.
 * If any changes have occurred in the meantime, reload.
 */
bool ALCDataLoadingModel::loadFilesFromWatchingDirectory(const std::string &firstFile,
                                                         const std::vector<std::string> &files,
                                                         const std::string &runsText) {

  // Check if there are changes to watched directory
  if (!m_directoryChanged.load()) {
    return false;
  }

  // If currently loading load new files
  if (m_loadingData) {
    return false;
  }

  // Need to add most recent file to add to list
  ALCLatestFileFinder finder(firstFile);
  const auto latestFile = finder.getMostRecentFile();

  // Check if file found
  if (latestFile.empty()) {
    // Could not find file this time, but don't reset directory changed flag
    m_directoryChanged = false;
    return false;
  }

  if (std::find(files.begin(), files.end(), latestFile) != files.end()) {
    m_directoryChanged = false;
    return false;
  }
  // File added is valid

  // Get old text
  auto newText = runsText;

  // Extract run number from latest file
  auto runNumber = extractRunNumber(latestFile);

  // If new run number is less then error
  if (runNumber <= m_lastRunLoadedAuto) {
    // Is error but continue to watch
    m_directoryChanged = false;
    return false;
  }

  if (runNumber == m_lastRunLoadedAuto + 1) {
    // Add as range
    // Check if last added was a range
    if (newText.find('-') != std::string::npos) {
      // Remove last run number from text
      newText = newText.substr(0, newText.find_last_of('-'));
    }
    newText += "-" + std::to_string(runNumber);

  } else {
    // Add as comma
    newText += "," + std::to_string(runNumber);
  }

  m_filesToLoad.push_back(latestFile);
  m_lastRunLoadedAuto = runNumber;
  m_runsText = newText;
  m_directoryChanged = false;
  return true;
}

} // namespace MantidQt::CustomInterfaces
