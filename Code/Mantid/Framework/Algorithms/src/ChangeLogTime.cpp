#include "MantidAlgorithms/ChangeLogTime.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <sstream>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(ChangeLogTime);

using std::string;
using std::stringstream;
using std::vector;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;

ChangeLogTime::ChangeLogTime()
{
}

ChangeLogTime::~ChangeLogTime()
{
}

/// Algorithm's name for identification
const string ChangeLogTime::name() const
{
  return "ChangeLogTime";
}

/// Algorithm's version for identification
int ChangeLogTime::version() const
{
  return 1;
}

/// Algorithm's category for identification
const std::string ChangeLogTime::category() const
{
  return "General";
}

void ChangeLogTime::initDocs()
{

}

void ChangeLogTime::init()
{
  declareProperty( new WorkspaceProperty<API::MatrixWorkspace>("InputWorkspace","",Direction::Input),
    "A workspace with units of TOF" );
  declareProperty( new WorkspaceProperty<API::MatrixWorkspace>("OutputWorkspace","",Direction::Output),
    "The name to use for the output workspace" );
  this->declareProperty("LogName", "", "Name of the log to add the offset to");
  this->declareProperty(new PropertyWithValue<double>("TimeOffset",Direction::Input),
        "Number of seconds (a float) to add to the time of each log value. Required.");
}

void ChangeLogTime::exec()
{
  // check that a log was specified
  string logname = this->getProperty("LogName");
  if (logname.empty()) {
    throw std::runtime_error("Failed to supply a LogName");
  }
  // everything will need an offset
  double offset = this->getProperty("TimeOffset");

  // make sure the log is in the input workspace
  MatrixWorkspace_sptr inputWS = getProperty("InputWorkspace");
  Kernel::TimeSeriesProperty<double> * oldlog = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( inputWS->run().getLogData(logname) );
  if (!oldlog) {
    stringstream msg;
    msg << "InputWorkspace \'" << this->getPropertyValue("InputWorkspace")
        << "\' does not have LogName \'" << logname << "\'";
    throw std::runtime_error(msg.str());
  }

  // Create the new log
  TimeSeriesProperty<double>* newlog = new TimeSeriesProperty<double>(logname);
  newlog->setUnits(oldlog->units());
  int size = oldlog->realSize();
  vector<double> values = oldlog->valuesAsVector();
  vector<DateAndTime> times = oldlog->timesAsVector();
  for (int i = 0; i < size; i++) {
    newlog->addValue(times[i] + offset, values[i]);
  }

  // Just overwrite if the change is in place
  API::MatrixWorkspace_sptr outputWS = getProperty("OutputWorkspace");
  if (outputWS == inputWS) {
    inputWS->mutableRun().addProperty(newlog, true);
    return;
  }

  // see if input workspace is event
  EventWorkspace_const_sptr eventWS = boost::dynamic_pointer_cast<const EventWorkspace>(inputWS);
  if (eventWS != NULL)
  {
    // TODO
  }
  else
  {
    // TODO
  }
}

} // namespace Mantid
} // namespace Algorithms
