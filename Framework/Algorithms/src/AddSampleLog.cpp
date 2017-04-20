#include "MantidAlgorithms/AddSampleLog.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/PropertyWithValue.h"

#include <string>

namespace {

static const std::string intTypeOption = "Int";
static const std::string doubleTypeOption = "Double";
static const std::string autoTypeOption = "AutoDetect";

static const std::string stringLogOption = "String";
static const std::string numberLogOption = "Number";
static const std::string numberSeriesLogOption = "Number Series";
}

namespace Mantid {
namespace Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddSampleLog)

using namespace Kernel;
using namespace API;

void AddSampleLog::init() {
  declareProperty(Kernel::make_unique<WorkspaceProperty<Workspace>>(
                      "Workspace", "", Direction::InOut),
                  "Workspace to add the log entry to");
  declareProperty("LogName", "",
                  boost::make_shared<MandatoryValidator<std::string>>(),
                  "The name that will identify the log entry");

  declareProperty("LogText", "", "The content of the log");

  std::vector<std::string> propOptions;
  propOptions.push_back(stringLogOption);
  propOptions.push_back(numberLogOption);
  propOptions.push_back(numberSeriesLogOption);
  declareProperty("LogType", stringLogOption,
                  boost::make_shared<StringListValidator>(propOptions),
                  "The type that the log data will be.");
  declareProperty("LogUnit", "", "The units of the log");

  std::vector<std::string> typeOptions;
  typeOptions.push_back(intTypeOption);
  typeOptions.push_back(doubleTypeOption);
  typeOptions.push_back(autoTypeOption);
  declareProperty("NumberType", autoTypeOption,
                  boost::make_shared<StringListValidator>(typeOptions),
                  "Force LogText to be interpreted as a number of type 'int' "
                  "or 'double'.");
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
  std::string propUnit = getProperty("LogUnit");
  std::string propType = getPropertyValue("LogType");
  std::string propNumberType = getPropertyValue("NumberType");

  if ((propNumberType != autoTypeOption) &&
      ((propType != numberLogOption) && (propType != numberSeriesLogOption))) {
    throw std::invalid_argument(
        "You may only use NumberType 'Int' or 'Double' options if "
        "LogType is 'Number' or 'Number Series'");
  }

  // Remove any existing log
  if (theRun.hasProperty(propName)) {
    theRun.removeLogData(propName);
  }

  if (propType == stringLogOption) {
    theRun.addLogData(new PropertyWithValue<std::string>(propName, propValue));
    theRun.getProperty(propName)->setUnits(propUnit);
    return;
  }

  int intVal;
  double dblVal;
  bool value_is_int = false;

  if (propNumberType != autoTypeOption) {
    value_is_int = (propNumberType == intTypeOption);
    if (value_is_int) {
      if (!Strings::convert(propValue, intVal)) {
        throw std::invalid_argument("Error interpreting string '" + propValue +
                                    "' as NumberType Int.");
      }
    } else if (!Strings::convert(propValue, dblVal)) {
      throw std::invalid_argument("Error interpreting string '" + propValue +
                                  "' as NumberType Double.");
    }
  } else {
    if (Strings::convert(propValue, intVal)) {
      value_is_int = true;
    } else if (!Strings::convert(propValue, dblVal)) {
      throw std::invalid_argument("Error interpreting string '" + propValue +
                                  "' as a number.");
    }
  }

  if (propType == numberLogOption) {
    if (value_is_int)
      theRun.addLogData(new PropertyWithValue<int>(propName, intVal));
    else
      theRun.addLogData(new PropertyWithValue<double>(propName, dblVal));
  } else if (propType == numberSeriesLogOption) {
    Kernel::DateAndTime startTime;
    try {
      startTime = theRun.startTime();
    } catch (std::runtime_error &) {
      // Swallow the error - startTime will just be 0
    }

    if (value_is_int) {
      auto tsp = new TimeSeriesProperty<int>(propName);
      tsp->addValue(startTime, intVal);
      theRun.addLogData(tsp);
    } else {
      auto tsp = new TimeSeriesProperty<double>(propName);
      tsp->addValue(startTime, dblVal);
      theRun.addLogData(tsp);
    }
  }
  theRun.getProperty(propName)->setUnits(propUnit);
}

} // namespace Algorithms
} // namespace Mantid
