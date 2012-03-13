/*WIKI* 


Workspaces contain information in logs. Often these detail what happened to the sample during the experiment. This algorithm allows one named log to be entered. 

The log can be either a String, a Number, or a Number Series. If you select Number Series, the current time will be used as the time of the log entry, and the number in the text used as the (only) value.


*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidAlgorithms/AddSampleLog.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/PropertyWithValue.h"
#include <string>

namespace Mantid
{
namespace Algorithms
{

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddSampleLog)

/// Sets documentation strings for this algorithm
void AddSampleLog::initDocs()
{
  this->setWikiSummary("Used to insert a value into the sample logs in a workspace.");
  this->setOptionalMessage("Used to insert a value into the sample logs in a workspace.");
}

using namespace Kernel;
using namespace API;

void AddSampleLog::init()
{
  declareProperty(new WorkspaceProperty<>("Workspace","",Direction::InOut),
    "Workspace to add the log entry to");
  declareProperty("LogName", "", boost::make_shared<MandatoryValidator<std::string> >(),
    "The name that will identify the log entry");

  declareProperty("LogText", "",
    "The content of the log");

  std::vector<std::string> propOptions;
  propOptions.push_back("String");
  propOptions.push_back("Number");
  propOptions.push_back("Number Series");
  declareProperty("LogType", "String",boost::make_shared<StringListValidator>(propOptions),
    "The type that the log data will be."
     );
}

void AddSampleLog::exec()
{
  // A pointer to the workspace to add a log to
  MatrixWorkspace_sptr wSpace = getProperty("Workspace");
  // we're going to edit the workspaces run details so get a non-const reference to it
  Run &theRun = wSpace->mutableRun();

  // get the data that the user wants to add
  std::string propName = getProperty("LogName");
  std::string propValue = getProperty("LogText");
  std::string propType = getPropertyValue("LogType");

  // Remove any existing log
  if (theRun.hasProperty(propName))
    theRun.removeLogData(propName);

  if (propType == "String")
  {
    theRun.addLogData(new PropertyWithValue<std::string>(propName, propValue));
  }
  else if (propType == "Number")
  {
    double val;
    if (!Strings::convert(propValue, val))
      throw std::invalid_argument("Error interpreting string '" + propValue + "' as a number.");
    theRun.addLogData(new PropertyWithValue<double>(propName, val));
  }
  else if (propType == "Number Series")
  {
    double val;
    if (!Strings::convert(propValue, val))
      throw std::invalid_argument("Error interpreting string '" + propValue + "' as a number.");
    Kernel::DateAndTime now = Kernel::DateAndTime::getCurrentTime();
    TimeSeriesProperty<double> * tsp = new TimeSeriesProperty<double>(propName);
    tsp->addValue(now, val);
    theRun.addLogData(tsp);
  }

  setProperty("Workspace", wSpace);
}

} // namespace Algorithms
} // namespace Mantid
