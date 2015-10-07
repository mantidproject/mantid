#include "MantidAlgorithms/ShiftLogTime.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <sstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using std::string;
using std::stringstream;
using std::vector;

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ShiftLogTime)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ShiftLogTime::ShiftLogTime() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
ShiftLogTime::~ShiftLogTime() {}

/// Algorithm's name for identification
const string ShiftLogTime::name() const { return "ShiftLogTime"; }

/// Algorithm's version for identification
int ShiftLogTime::version() const { return 1; }

/// Algorithm's category for identification
const string ShiftLogTime::category() const { return "DataHandling\\Logs"; }

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void ShiftLogTime::init() {
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "A workspace with units of TOF");
  declareProperty(new WorkspaceProperty<API::MatrixWorkspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to use for the output workspace");
  this->declareProperty("LogName", "", "Name of the log to add the offset to");
  this->declareProperty(
      new PropertyWithValue<int>("IndexShift", Direction::Input),
      "Number of (integer) values to move the log values. Required.");
}

//----------------------------------------------------------------------------------------------
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
  Kernel::TimeSeriesProperty<double> *oldlog =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
          inputWS->run().getLogData(logname));
  if (!oldlog) {
    stringstream msg;
    msg << "InputWorkspace \'" << this->getPropertyValue("InputWorkspace")
        << "\' does not have LogName \'" << logname << "\'";
    throw std::runtime_error(msg.str());
  }

  // get the data out of the old log
  int size = oldlog->realSize();
  if (abs(indexshift) > size) {
    stringstream msg;
    msg << "IndexShift (=" << indexshift
        << ") is larger than the log length (=" << size << ")";
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
    values.erase(values.begin(), values.begin() + indexshift);
    times.erase(times.end() - indexshift, times.end());
  }

  // Create the new log
  TimeSeriesProperty<double> *newlog = new TimeSeriesProperty<double>(logname);
  newlog->setUnits(oldlog->units());
  newlog->create(times, values);

  // Just overwrite if the change is in place
  MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS != inputWS) {
    IAlgorithm_sptr duplicate = createChildAlgorithm("CloneWorkspace");
    duplicate->initialize();
    duplicate->setProperty<Workspace_sptr>(
        "InputWorkspace", boost::dynamic_pointer_cast<Workspace>(inputWS));
    duplicate->execute();
    Workspace_sptr temp = duplicate->getProperty("OutputWorkspace");
    outputWS = boost::dynamic_pointer_cast<MatrixWorkspace>(temp);

    setProperty("OutputWorkspace", outputWS);
  }

  outputWS->mutableRun().addProperty(newlog, true);
}

} // namespace Mantid
} // namespace Algorithms
