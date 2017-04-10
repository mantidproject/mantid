#include "MantidAlgorithms/AddSampleLog.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/MatrixWorkspace.h"
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


  // add optional workspace which contains the data of the TimeSeriesProperty
  declareProperty(Kernel::make_unique<WorkspaceProperty<Workspace>>(
                    "TimeSeriesWorkspace" "", Direction::Input, PropertyMode::Optional),
                  "Optional workspace contain the ");
  declareProperty("WorkspaceIndex", 0, "The workspace index of the TimeSeriesWorkspace to be imported.");
  declareProperty("AutoMetaData", false,
                  "If it is specified as true, then all the meta data information will be retrieved from the input workspace. It will be used with algorithm ExportTimeSeriesProperty.");

  std::vector<std::string> time_units{"Second", "Nanosecond"};
  declareProperty("TimeUnit", "Second",  boost::make_shared<Kernel::StringListValidator>(time_units),
                  "The unit of the time of the input workspace");
  declareProperty("RelativeTime", true, "If specified as True, then then the time stamps are relative to the run start time of the target workspace.");
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

  // add the values to the added new TimeSeriesProperty
  std::string data_ws_name = getPropertyValue("TimeSeriesWorkspace");
  if (data_ws_name.size() > 0)
  {
    setTimeSeriesData(ws1, propName, value_is_int);
  }

  return;
}

void AddSampleLog::setTimeSeriesData(API::MatrixWorkspace_const_sptr outws, const std::string &property_name, bool value_is_int)
{
  // get input and
  MatrixWorkspace_const_sptr data_ws = getProperty("TimeSeriesWorkspace");
  int ws_index = getProperty("WorkspaceIndex");
  if (ws_index < 0 || ws_index > data_ws->getNumberHistograms())
    throw std::runtime_error("Input workspace index is out of range");

  // get meta data
  bool epochtime(false);
  std::string timeunit;
  getMetaData(epochtime, timeunit);
  bool is_second = timeunit.compare("Second") == 0;

  // convert the data in workspace to time series property value
  std::vector<DateAndTime> time_vec = getTimes(data_ws, ws_index, epochtime, is_second);
  if (value_is_int)
  {
    // integer property
    TimeSeriesProperty<int> * int_prop = dynamic_cast<TimeSeriesProperty<int> *>(outws->run().getProperty(property_name));
    std::vector<int> value_vec = getIntValues(data_ws, ws_index);
    int_prop->addValues(time_vec, value_vec);
  }
  else
  {
    // double property
    TimeSeriesProperty<double> * int_prop = dynamic_cast<TimeSeriesProperty<double> *>(outws->run().getProperty(property_name));
    std::vector<double> value_vec = getDblValues(data_ws, ws_index);
    int_prop->addValues(time_vec, value_vec);
  }

  return;
}

std::vector<Kernel::DateAndTime> AddSampleLog::getTimes(API::MatrixWorkspace_const_sptr dataws, int workspace_index, bool is_epoch, bool is_second)
{
  // get run start time
  int64_t timeshift(0);
  if (!is_epoch)
  {
    // get the run start time
    Kernel::DateAndTime run_start_time = getRunStart(dataws);
    timeshift = run_start_time.totalNanoseconds();
  }

  // set up the time vector
  std::vector<Kernel::DateAndTime> timevec;
  size_t vecsize = dataws->readX(workspace_index).size();
  for (size_t i = 0; i < vecsize; ++i)
  {
    double timedbl = dataws->readX(workspace_index)[i];
    if (is_second)
      timedbl *= 1.E9;
    int64_t entry_i64 = static_cast<int64_t>(timedbl);
    Kernel::DateAndTime entry(timeshift + entry_i64);
    timevec.push_back(entry);
  }

  return timevec;
}

Kernel::DateAndTime AddSampleLog::getRunStart(API::MatrixWorkspace_const_sptr dataws)
{
  Kernel::DateAndTime runstart(0);

  return runstart;
}

std::vector<double> AddSampleLog::getDblValues(API::MatrixWorkspace_const_sptr dataws, int workspace_index)
{
  std::vector<double> valuevec;
  size_t vecsize = dataws->readY(workspace_index).size();
  for (size_t i = 0; i < vecsize; ++i)
    valuevec.push_back(dataws->readY(workspace_index)[i]);

  return valuevec;
}

std::vector<int> AddSampleLog::getIntValues(API::MatrixWorkspace_const_sptr dataws, int workspace_index)
{
  std::vector<int> valuevec;
  size_t vecsize = dataws->readY(workspace_index).size();
  for (size_t i = 0; i < vecsize; ++i)
    valuevec.push_back(static_cast<int>(dataws->readY(workspace_index)[i]));

  return valuevec;
}

void AddSampleLog::getMetaData(API::MatrixWorkspace_const_sptr dataws, bool &epochtime, std::string &timeunit)
{
  bool auto_meta = getProperty("AutoMetaData");
  if (auto_meta)
  {
    // get the meta data from the input workspace
    std::string epochtimestr = dataws->run().getProperty("IsEpochTime")->value();
    epochtime = epochtimestr.compare("true") == 0;
    timeunit = dataws->run().getProperty("TimeUnit")->value();
  }
  else
  {
    // get the meta data from input
    epochtime = !getProperty("RelativeTime");
    timeunit = getProperty("TimeUnit");
  }

  return;
}

} // namespace Algorithms
} // namespace Mantid
