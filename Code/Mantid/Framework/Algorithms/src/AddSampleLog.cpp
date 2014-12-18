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

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddSampleLog)

using namespace Kernel;
using namespace API;

void AddSampleLog::init() {
  declareProperty(
      new WorkspaceProperty<Workspace>("Workspace", "", Direction::InOut),
      "Workspace to add the log entry to");
  declareProperty("LogName", "",
                  boost::make_shared<MandatoryValidator<std::string> >(),
                  "The name that will identify the log entry");

  declareProperty("LogText", "", "The content of the log");

  std::vector<std::string> propOptions;
  propOptions.push_back("String");
  propOptions.push_back("Number");
  propOptions.push_back("Number Series");
  declareProperty("LogType", "String",
                  boost::make_shared<StringListValidator>(propOptions),
                  "The type that the log data will be.");
}

void AddSampleLog::exec() {
  // A pointer to the workspace to add a log to
  Workspace_sptr ws1 = getProperty("Workspace");
  ExperimentInfo_sptr ws = boost::dynamic_pointer_cast<ExperimentInfo>(ws1);
  // we're going to edit the workspaces run details so get a non-const reference
  // to it
  Run &theRun = ws->mutableRun();
  // get the data that the user wants to add
  std::string propName = getProperty("LogName");
  std::string propValue = getProperty("LogText");
  std::string propType = getPropertyValue("LogType");

  // Remove any existing log
  if (theRun.hasProperty(propName)) {
    theRun.removeLogData(propName);
  }

  if (propType == "String") {
    theRun.addLogData(new PropertyWithValue<std::string>(propName, propValue));
    return;
  }

  bool valueIsInt(false);
  int intVal;
  double dblVal;
  if (Strings::convert(propValue, intVal)) {
    valueIsInt = true;
  } else if (!Strings::convert(propValue, dblVal)) {
    throw std::invalid_argument("Error interpreting string '" + propValue +
                                "' as a number.");
  }

  if (propType == "Number") {
    if (valueIsInt)
      theRun.addLogData(new PropertyWithValue<int>(propName, intVal));
    else
      theRun.addLogData(new PropertyWithValue<double>(propName, dblVal));
  } else if (propType == "Number Series") {
    Kernel::DateAndTime startTime;
    try {
      startTime = theRun.startTime();
    }
    catch (std::runtime_error &) {
      // Swallow the error - startTime will just be 0
    }

    if (valueIsInt) {
      auto tsp = new TimeSeriesProperty<int>(propName);
      tsp->addValue(startTime, intVal);
      theRun.addLogData(tsp);
    } else {
      auto tsp = new TimeSeriesProperty<double>(propName);
      tsp->addValue(startTime, dblVal);
      theRun.addLogData(tsp);
    }
  }
}

} // namespace Algorithms
} // namespace Mantid
