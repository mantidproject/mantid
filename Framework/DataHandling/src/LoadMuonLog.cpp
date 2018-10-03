// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/LoadMuonLog.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidNexus/MuonNexusReader.h"

#include <ctime>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(LoadMuonLog)

using namespace Kernel;
using API::FileProperty;
using API::MatrixWorkspace;
using API::MatrixWorkspace_sptr;
using API::Progress;
using API::WorkspaceProperty;
using DataObjects::Workspace2D_sptr;

/// Empty default constructor
LoadMuonLog::LoadMuonLog() {}

/// Initialisation method.
void LoadMuonLog::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(
      make_unique<WorkspaceProperty<MatrixWorkspace>>("Workspace", "Anonymous",
                                                      Direction::InOut),
      "The name of the workspace to which the log data will be added.");
  declareProperty(make_unique<FileProperty>("Filename", "", FileProperty::Load),
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

  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");
  localWorkspace->mutableSample().setName(nxload.getSampleName());

  // Attempt to load the content of each NXlog section into the Sample object
  // Assumes that MuonNexusReader has read all log data
  // Two cases of double or string data allowed
  Progress prog(this, 0.0, 1.0, nxload.numberOfLogs());
  for (int i = 0; i < nxload.numberOfLogs(); i++) {
    std::string logName = nxload.getLogName(i);
    auto l_PropertyDouble =
        Kernel::make_unique<TimeSeriesProperty<double>>(logName);
    auto l_PropertyString =
        Kernel::make_unique<TimeSeriesProperty<std::string>>(logName);

    // Read log file into Property which is then stored in Sample object
    if (!nxload.logTypeNumeric(i)) {
      std::string logValue;
      std::time_t logTime;
      for (int j = 0; j < nxload.getLogLength(i); j++) {
        nxload.getLogStringValues(i, j, logTime, logValue);
        l_PropertyString->addValue(logTime, logValue);
      }
    } else {
      double logValue;
      std::time_t logTime;
      for (int j = 0; j < nxload.getLogLength(i); j++) {
        nxload.getLogValues(i, j, logTime, logValue);
        l_PropertyDouble->addValue(logTime, logValue);
      }
    }

    // store Property in Sample object and delete unused object
    if (nxload.logTypeNumeric(i)) {
      localWorkspace->mutableRun().addLogData(std::move(l_PropertyDouble));
    } else {
      localWorkspace->mutableRun().addLogData(std::move(l_PropertyString));
    }
    prog.report();
  } // end for

  // operation was a success and ended normally
}

/** change each element of the string to lower case
 * @param strToConvert :: The input string
 * @returns The string but with all characters in lower case
 */
std::string LoadMuonLog::stringToLower(std::string strToConvert) {
  std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(),
                 ::tolower);
  return strToConvert; // return the converted string
}

/** check if first 19 characters of a string is data-time string according to
 * yyyy-mm-ddThh:mm:ss
 * @param str :: The string to test
 * @returns true if the strings format matched the expected date format
 */
bool LoadMuonLog::isDateTimeString(const std::string &str) {
  if (str.size() >= 19)
    if (str.compare(4, 1, "-") == 0 && str.compare(7, 1, "-") == 0 &&
        str.compare(13, 1, ":") == 0 && str.compare(16, 1, ":") == 0 &&
        str.compare(10, 1, "T") == 0)
      return true;

  return false;
}

} // namespace DataHandling
} // namespace Mantid
