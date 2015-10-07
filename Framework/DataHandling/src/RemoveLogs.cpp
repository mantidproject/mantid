//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAPI/FileProperty.h"
#include "MantidDataHandling/RemoveLogs.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Glob.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <boost/algorithm/string.hpp>
#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/DateTimeParser.h>
#include <Poco/DateTimeFormat.h>

#include <fstream> // used to get ifstream
#include <sstream>
#include <algorithm>

namespace Mantid {
namespace DataHandling {

// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(RemoveLogs)

using namespace Kernel;
using namespace API;
using DataObjects::Workspace2D;
using DataObjects::Workspace2D_sptr;

/// Empty default constructor
RemoveLogs::RemoveLogs() {}

/// Initialisation method.
void RemoveLogs::init() {
  // When used as a Child Algorithm the workspace name is not used - hence the
  // "Anonymous" to satisfy the validator
  declareProperty(
      new WorkspaceProperty<MatrixWorkspace>("Workspace", "Anonymous",
                                             Direction::InOut),
      "The name of the workspace to which the log data will be removed");
  declareProperty(new ArrayProperty<std::string>("KeepLogs", Direction::Input),
                  "List(comma separated) of logs to be kept");
}

/** Executes the algorithm. Reading in log file(s)
 *
 *  @throw Mantid::Kernel::Exception::FileError  Thrown if file is not
 *recognised to be a raw datafile or log file
 *  @throw std::runtime_error Thrown with Workspace problems
 */
void RemoveLogs::exec() {
  // Get the input workspace and retrieve run from workspace.
  // the log file(s) will be loaded into the run object of the workspace
  const MatrixWorkspace_sptr localWorkspace = getProperty("Workspace");
  const std::vector<Mantid::Kernel::Property *> &logData =
      localWorkspace->run().getLogData();
  std::vector<std::string> keepLogs = getProperty("KeepLogs");
  std::vector<std::string> logNames;
  auto pEnd = logData.end();
  for (auto pItr = logData.begin(); pItr != pEnd; ++pItr) {
    logNames.push_back((*pItr)->name().c_str());
  }
  for (std::vector<std::string>::const_iterator it = logNames.begin();
       it != logNames.end(); ++it) {
    auto location = std::find(keepLogs.begin(), keepLogs.end(), (*it));
    if (location == keepLogs.end()) {
      localWorkspace->mutableRun().removeLogData(*it);
    }
  }

  // operation was a success and ended normally
  return;
}

} // namespace DataHandling
} // namespace Mantid
