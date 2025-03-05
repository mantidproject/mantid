// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidMuon/LoadMuonLog.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <algorithm>
#include <ctime>

namespace {
void toLower(std::string &name) {
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
}
} // namespace

namespace Mantid::Muon {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadMuonLog)

using namespace Kernel;
using API::FileProperty;
using API::MatrixWorkspace;
using API::MatrixWorkspace_sptr;
using API::Progress;
using API::WorkspaceProperty;
using DataObjects::Workspace2D_sptr;

/// Initialisation method.
void LoadMuonLog::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>("Workspace", "Anonymous", Direction::InOut),
                  "The name of the workspace to which the log data will be added.");
  declareProperty(std::make_unique<FileProperty>("Filename", "", FileProperty::Load),
                  "The filename (including its full or relative path) of the "
                  "Muon Nexus file.");
}

/** Executes the algorithm. Reading in Log entries from the Nexus file
 *
 *  @throw Mantid::Kernel::Exception::FileError  Thrown if file is not
 *recognised to be a Nexus datafile
 *  @throw std::runtime_error Thrown with Workspace problems
 */
void LoadMuonLog::exec() {
  // Retrieve the filename from the properties and perform some initial checks
  // on the filename

  m_filename = getPropertyValue("Filename");
  MuonNexusReader nxload;
  nxload.readLogData(m_filename);

  // Get the input workspace and retrieve sample from workspace.
  // the log data will be loaded into the Sample container of the workspace
  // Also set the sample name at this point, as part of the sample related log
  // data.

  MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");
  localWorkspace->mutableSample().setName(nxload.getSampleName());

  std::set<std::string> logNames;
  const auto &logs = localWorkspace->mutableRun().getLogData();
  // need to remove case
  for (const auto &log : logs) {
    std::string logName = log->name();
    toLower(logName);
    logNames.insert(logName);
  }

  // Attempt to load the content of each NXlog section into the Sample object
  // Assumes that MuonNexusReader has read all log data
  // Two cases of double or string data allowed
  Progress prog(this, 0.0, 1.0, nxload.numberOfLogs());
  for (int i = 0; i < nxload.numberOfLogs(); i++) {
    addLogValueFromIndex(nxload, i, localWorkspace, logNames);
    prog.report();
  }
  // operation was a success and ended normally
}

void LoadMuonLog::addLogValueFromIndex(MuonNexusReader &nxload, const int &index,
                                       API::MatrixWorkspace_sptr &localWorkspace, std::set<std::string> &logNames) {
  std::string newLogName = nxload.getLogName(index);
  // want to keep the original case for the logs
  std::string logNameLower = nxload.getLogName(index);
  toLower(logNameLower);
  // check if log name already exists
  if (logNames.find(logNameLower) != logNames.end()) {
    g_log.warning("The log " + newLogName +
                  " is already in the nexus file. The sample log names are "
                  "case insensitive.");
    return;
  }
  logNames.insert(newLogName);
  auto l_PropertyDouble = std::make_unique<TimeSeriesProperty<double>>(newLogName);
  auto l_PropertyString = std::make_unique<TimeSeriesProperty<std::string>>(newLogName);

  // Read log file into Property which is then stored in Sample object
  if (!nxload.logTypeNumeric(index)) {
    std::string logValue;
    std::time_t logTime;
    for (int j = 0; j < nxload.getLogLength(index); j++) {
      nxload.getLogStringValues(index, j, logTime, logValue);
      l_PropertyString->addValue(logTime, logValue);
      l_PropertyString->setUnits(nxload.logUnits(index));
    }
  } else {
    double logValue;
    std::time_t logTime;
    for (int j = 0; j < nxload.getLogLength(index); j++) {
      nxload.getLogValues(index, j, logTime, logValue);
      l_PropertyDouble->addValue(logTime, logValue);
      l_PropertyDouble->setUnits(nxload.logUnits(index));
    }
  }

  // store Property in Sample object and delete unused object
  if (nxload.logTypeNumeric(index)) {
    localWorkspace->mutableRun().addLogData(std::move(l_PropertyDouble));
  } else {
    localWorkspace->mutableRun().addLogData(std::move(l_PropertyString));
  }
}

/** change each element of the string to lower case
 * @param strToConvert :: The input string
 * @returns The string but with all characters in lower case
 */
std::string LoadMuonLog::stringToLower(std::string strToConvert) {
  std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::tolower);
  return strToConvert; // return the converted string
}

/** check if first 19 characters of a string is data-time string according to
 * yyyy-mm-ddThh:mm:ss
 * @param str :: The string to test
 * @returns true if the strings format matched the expected date format
 */
bool LoadMuonLog::isDateTimeString(const std::string &str) {
  if (str.size() >= 19)
    if (str.compare(4, 1, "-") == 0 && str.compare(7, 1, "-") == 0 && str.compare(13, 1, ":") == 0 &&
        str.compare(16, 1, ":") == 0 && str.compare(10, 1, "T") == 0)
      return true;

  return false;
}

} // namespace Mantid::Muon
