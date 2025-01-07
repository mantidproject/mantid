// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/ShiftLogTime.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"

#include "MantidKernel/TimeSeriesProperty.h"

#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;
using std::string;
using std::stringstream;
using std::vector;

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ShiftLogTime)

/// Algorithm's name for identification
const string ShiftLogTime::name() const { return "ShiftLogTime"; }

/// Algorithm's version for identification
int ShiftLogTime::version() const { return 1; }

/// Algorithm's category for identification
const string ShiftLogTime::category() const { return "DataHandling\\Logs"; }

/** Initialize the algorithm's properties.
 */
void ShiftLogTime::init() {
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
                  "A workspace with units of TOF");
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");
  this->declareProperty("LogName", "", "Name of the log to add the offset to");
  this->declareProperty(std::make_unique<PropertyWithValue<int>>("IndexShift", Direction::Input),
                        "Number of (integer) values to move the log values. Required.");
}

/** Execute the algorithm.
 */
void ShiftLogTime::exec() {
  // check that a log was specified
  string logname = this->getProperty("LogName");
  if (logname.empty()) {
    throw std::runtime_error("Failed to supply a LogName");
  }
  // everything will need an offset
  int indexshift = this->getProperty("IndexShift");

  // make sure the log is in the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  auto *oldlog = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(inputWS->run().getLogData(logname));
  if (!oldlog) {
    stringstream msg;
    msg << "InputWorkspace \'" << this->getPropertyValue("InputWorkspace") << "\' does not have LogName \'" << logname
        << "\'";
    throw std::runtime_error(msg.str());
  }

  // get the data out of the old log
  int size = oldlog->realSize();
  if (abs(indexshift) > size) {
    stringstream msg;
    msg << "IndexShift (=" << indexshift << ") is larger than the log length (=" << size << ")";
    throw std::runtime_error(msg.str());
  }
  vector<double> values = oldlog->valuesAsVector();
  vector<DateAndTime> times = oldlog->timesAsVector();

  // 'fix' the indices
  if (indexshift == 0) {
    // nothing to do
  } else if (indexshift > 0) {
    values.erase(values.end() - indexshift, values.end());
    times.erase(times.begin(), times.begin() + indexshift);
  } else // indexshift < 0
  {
    // indexshift<0, so -indexshift>0
    values.erase(values.begin(), values.begin() - indexshift);
    times.erase(times.end() + indexshift, times.end());
  }

  // Create the new log
  auto newlog = new TimeSeriesProperty<double>(logname);
  newlog->setUnits(oldlog->units());
  newlog->create(times, values);

  // Just overwrite if the change is in place
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    auto duplicate = createChildAlgorithm("CloneWorkspace");
    duplicate->initialize();
    duplicate->setProperty<Workspace_sptr>("InputWorkspace", std::dynamic_pointer_cast<Workspace>(inputWS));
    duplicate->execute();
    Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
    outputWS = std::dynamic_pointer_cast<MatrixWorkspace>(temp);

    setProperty("OutputWorkspace", outputWS);
  }

  outputWS->mutableRun().addProperty(newlog, true);
}

} // namespace Mantid::Algorithms
