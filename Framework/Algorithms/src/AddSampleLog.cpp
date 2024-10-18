// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/AddSampleLog.h"
#include "MantidAPI/ExperimentInfo.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/MultipleExperimentInfos.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/Workspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/EnumeratedString.h"
#include "MantidKernel/EnumeratedStringProperty.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"

#include <string>

namespace {

const std::string NumberType("NumberType");
const std::vector<std::string> typeOptions{"AutoDetect", "Int", "Double"};
enum class TypeMode { AUTO_DETECT, INT, DOUBLE, enum_count };
typedef Mantid::Kernel::EnumeratedString<TypeMode, &typeOptions> TYPEMODE;

const std::string LogType("LogType");
const std::vector<std::string> propOptions{"String", "Number", "Number Series"};
enum class LogMode { STRING_LOG, NUMBER_LOG, NUMBER_SERIES_LOG, enum_count };
typedef Mantid::Kernel::EnumeratedString<LogMode, &propOptions> LOGMODE;
} // namespace

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(AddSampleLog)

using namespace Kernel;
using namespace API;
using Types::Core::DateAndTime;

void AddSampleLog::init() {
  declareProperty(std::make_unique<WorkspaceProperty<Workspace>>("Workspace", "", Direction::InOut),
                  "Workspace to add the log entry to");
  declareProperty("LogName", "", std::make_shared<MandatoryValidator<std::string>>(),
                  "The name that will identify the log entry");

  declareProperty("LogText", "", "The content of the log");

  declareProperty(std::make_unique<EnumeratedStringProperty<LogMode, &propOptions>>(LogType),
                  "The type that the log data will be.");
  declareProperty("LogUnit", "", "The units of the log");

  declareProperty(std::make_unique<EnumeratedStringProperty<TypeMode, &typeOptions>>(NumberType),
                  "Force LogText to be interpreted as a number of type 'int' "
                  "or 'double'.");

  // add optional workspace which contains the data of the TimeSeriesProperty
  declareProperty(std::make_unique<WorkspaceProperty<API::MatrixWorkspace>>("TimeSeriesWorkspace", "", Direction::Input,
                                                                            PropertyMode::Optional),
                  "Optional workspace contain the data");
  declareProperty("WorkspaceIndex", 0, "The workspace index of the TimeSeriesWorkspace to be imported.");
  declareProperty("AutoMetaData", false,
                  "If it is specified as true, then all the meta data "
                  "information will be retrieved from the input workspace. It "
                  "will be used with algorithm ExportTimeSeriesProperty.");

  std::vector<std::string> time_units{"Second", "Nanosecond"};
  declareProperty("TimeUnit", "Second", std::make_shared<Kernel::StringListValidator>(time_units),
                  "The unit of the time of the input workspace");
  declareProperty("RelativeTime", true,
                  "If specified as True, then then the "
                  "time stamps are relative to the run "
                  "start time of the target workspace.");
  declareProperty("UpdateInstrumentParameters", false,
                  "Update instrument parameters to account for new log. "
                  "This will update detector positions that depend on motors, for example.");
}

/**
 * @brief AddSampleLog::exec
 */
void AddSampleLog::exec() {
  // A pointer to the workspace to add a log to
  Workspace_sptr target_workspace = getProperty("Workspace");
  auto expinfo_ws = std::dynamic_pointer_cast<ExperimentInfo>(target_workspace);
  if (!expinfo_ws) {
    // We're dealing with an MD workspace which has multiple experiment infos
    auto infos = std::dynamic_pointer_cast<MultipleExperimentInfos>(target_workspace);
    if (!infos) {
      throw std::invalid_argument("Input workspace does not support sample logs");
    }
    if (infos->getNumExperimentInfo() < 1) {
      ExperimentInfo_sptr info(new ExperimentInfo());
      infos->addExperimentInfo(info);
    }
    expinfo_ws = infos->getExperimentInfo(0);
  }
  // we're going to edit the workspaces run details so get a non-const reference
  // to it
  Run &theRun = expinfo_ws->mutableRun();
  // get the data that the user wants to add
  std::string propName = getProperty("LogName");
  std::string propValue = getProperty("LogText");
  std::string propUnit = getProperty("LogUnit");
  LOGMODE propType = getPropertyValue(LogType);
  TYPEMODE propNumberType = getPropertyValue("NumberType");

  // check inputs
  if ((propNumberType != TypeMode::AUTO_DETECT) && (propType == LogMode::STRING_LOG)) {
    throw std::invalid_argument("You may only use NumberType 'Int' or 'Double' options if "
                                "LogType is 'Number' or 'Number Series'");
  }

  // Remove any existing log
  if (theRun.hasProperty(propName)) {
    theRun.removeLogData(propName);
  }

  // add sample log!
  if (propType == LogMode::STRING_LOG) {
    // add string log value and return
    addStringLog(theRun, propName, propValue, propUnit);
  } else if (propType == LogMode::NUMBER_SERIES_LOG) {
    // add a TimeSeriesProperty
    // TODO: Need to re-define the behavior on the default propNumberType for
    // Series.
    //       If propValue is given, then use this value to determine whether the
    //       type is
    //       integer or double; Otherwise, the series should be double
    addTimeSeriesProperty(theRun, propName, propValue, propUnit, propNumberType);
  } else {
    // add a single value property
    addSingleValueProperty(theRun, propName, propValue, propUnit, propNumberType);
  }

  // update parameters based on the new log information if requested
  const bool updateInstrumentParams = getProperty("UpdateInstrumentParameters");
  if (updateInstrumentParams && expinfo_ws->getInstrument()->isParametrized())
    expinfo_ws->populateInstrumentParameters();
}

/** Add a single value property
 * @brief AddSampleLog::addSingleValueProperty
 * @param theRun
 * @param propName
 * @param propValue
 * @param propUnit
 * @param propNumberType
 */
void AddSampleLog::addSingleValueProperty(Run &theRun, const std::string &propName, const std::string &propValue,
                                          const std::string &propUnit, const std::string &propNumberType) {
  // add a single value property, integer or double
  bool value_is_int(false);
  TYPEMODE propType = propNumberType;
  if (propType != TypeMode::AUTO_DETECT) {
    value_is_int = (propType == TypeMode::INT);
  } else {
    int intVal;
    if (Strings::convert(propValue, intVal)) {
      value_is_int = true;
    }
  }

  // set value
  if (value_is_int) {
    // convert to integer
    int intVal;
    int convert_to_int = Strings::convert(propValue, intVal);
    if (convert_to_int == 0) {
      // spit out error message and set to default value
      g_log.error() << "Error interpreting string '" << propValue << "' as NumberType Int.";
      throw std::runtime_error("Invalie integer input");
    }
    theRun.addLogData(new PropertyWithValue<int>(propName, intVal));
  } else {
    // convert to double
    double dblVal;
    int convert_to_dbl = Strings::convert(propValue, dblVal);
    if (convert_to_dbl == 0) {
      g_log.error() << "Error interpreting string '" << propValue << "' as NumberType Double.";
      throw std::runtime_error("Invalid double input.");
    }
    theRun.addLogData(new PropertyWithValue<double>(propName, dblVal));
    g_log.information() << "added property " << propName << " with value " << dblVal << "\n";
  }

  // add unit
  theRun.getProperty(propName)->setUnits(propUnit);

  return;
}

/** Add a sample log (property) with value as string
 * @brief AddSampleLog::addStringLog
 * @param theRun
 * @param propName
 * @param propValue
 * @param propUnit
 */
void AddSampleLog::addStringLog(Run &theRun, const std::string &propName, const std::string &propValue,
                                const std::string &propUnit) {
  theRun.addLogData(new PropertyWithValue<std::string>(propName, propValue));
  theRun.getProperty(propName)->setUnits(propUnit);
  return;
}

/** Add a sample log as a TimeSeriesProperty
 * @brief AddSampleLog::addTimeSeriesProperty
 * @param run_obj
 * @param prop_name
 * @param prop_value
 * @param prop_unit
 * @param prop_number_type
 */
void AddSampleLog::addTimeSeriesProperty(Run &run_obj, const std::string &prop_name, const std::string &prop_value,
                                         const std::string &prop_unit, const std::string &prop_number_type) {
  // set up the number type right
  bool is_int_series(false);
  TYPEMODE propType = prop_number_type;
  if (propType == TypeMode::INT) {
    // integer type
    is_int_series = true;
  } else if (propType == TypeMode::AUTO_DETECT) {
    // auto type. by default
    if (prop_value.empty())
      g_log.warning("For sample log in TimeSeriesProperty and values are given "
                    "by MarixWorkspace, the default data type "
                    "is double.");
    else {
      // find out the time series data type by prop_value. integer or double
      int intVal;
      if (Strings::convert(prop_value, intVal)) {
        is_int_series = true;
      }
    }
  } else if (propType != TypeMode::DOUBLE) {
    // unsupported type: anything but double, integer or auto
    g_log.error() << "TimeSeriesProperty with data type " << prop_number_type << " is not supported.\n";
    throw std::runtime_error("Unsupported TimeSeriesProperty type.");
  }

  // check using workspace or some specified start value
  std::string tsp_ws_name = getPropertyValue("TimeSeriesWorkspace");
  bool use_ws = !tsp_ws_name.empty();
  bool use_single_value = !prop_value.empty();
  if (use_ws && use_single_value) {
    throw std::runtime_error("Both TimeSeries workspace and sing value are "
                             "specified.  It is not allowed.");
  } else if (!use_ws && !use_single_value) {
    throw std::runtime_error("Neither TimeSeries workspace or sing value are "
                             "specified.  It is not allowed.");
  }

  // create workspace
  // get run start
  Types::Core::DateAndTime startTime = getRunStart(run_obj);

  // initialze the TimeSeriesProperty and add unit
  if (is_int_series) {
    auto tsp = std::make_unique<TimeSeriesProperty<int>>(prop_name);
    if (use_single_value) {
      int intVal;
      if (Strings::convert(prop_value, intVal)) {
        tsp->addValue(startTime, intVal);
      } else {
        throw std::invalid_argument("Input value cannot be converted to an integer value.");
      }
    }
    run_obj.addLogData(std::move(tsp));
  } else {
    auto tsp = std::make_unique<TimeSeriesProperty<double>>(prop_name);
    if (use_single_value) {
      double dblVal;
      if (Strings::convert(prop_value, dblVal)) {
        tsp->addValue(startTime, dblVal);
      } else {
        throw std::invalid_argument("Input value cannot be converted to a double number.");
      }
    }
    run_obj.addLogData(std::move(tsp));
  }
  // add unit
  run_obj.getProperty(prop_name)->setUnits(prop_unit);

  if (use_ws)
    setTimeSeriesData(run_obj, prop_name, is_int_series);
}

/** Set value of a TimeSeriesProperty from input workspace
 * @brief AddSampleLog::setTimeSeriesData
 * @param run_obj
 * @param property_name
 * @param value_is_int
 */
void AddSampleLog::setTimeSeriesData(Run &run_obj, const std::string &property_name, bool value_is_int) {
  // get input and
  MatrixWorkspace_sptr data_ws = getProperty("TimeSeriesWorkspace");
  int ws_index = getProperty("WorkspaceIndex");
  if (ws_index < 0 || ws_index > static_cast<int>(data_ws->getNumberHistograms()))
    throw std::runtime_error("Input workspace index is out of range");

  // get meta data
  bool epochtime(false);
  std::string timeunit;
  getMetaData(data_ws, epochtime, timeunit);
  bool is_second = timeunit == "Second";

  // convert the data in workspace to time series property value
  std::vector<DateAndTime> time_vec = getTimes(data_ws, ws_index, epochtime, is_second, run_obj);
  if (value_is_int) {
    // integer property
    auto *int_prop = dynamic_cast<TimeSeriesProperty<int> *>(run_obj.getProperty(property_name));
    std::vector<int> value_vec = getIntValues(data_ws, ws_index);
    int_prop->addValues(time_vec, value_vec);
  } else {
    // double property
    auto *int_prop = dynamic_cast<TimeSeriesProperty<double> *>(run_obj.getProperty(property_name));
    std::vector<double> value_vec = getDblValues(data_ws, ws_index);
    int_prop->addValues(time_vec, value_vec);
  }

  return;
}

/** Get the time vector for the TimeSeriesProperty to which the entries is to
 * set
 * @brief AddSampleLog::getTimes
 * @param dataws
 * @param workspace_index
 * @param is_epoch
 * @param is_second
 * @param run_obj
 * @return
 */
std::vector<Types::Core::DateAndTime> AddSampleLog::getTimes(const API::MatrixWorkspace_const_sptr &dataws,
                                                             int workspace_index, bool is_epoch, bool is_second,
                                                             API::Run &run_obj) {
  // get run start time
  int64_t timeshift(0);
  if (!is_epoch) {
    // get the run start time
    Types::Core::DateAndTime run_start_time = getRunStart(run_obj);
    timeshift = run_start_time.totalNanoseconds();
  }

  // set up the time vector
  std::vector<Types::Core::DateAndTime> timevec;
  size_t vecsize = dataws->readX(workspace_index).size();
  for (size_t i = 0; i < vecsize; ++i) {
    double timedbl = dataws->readX(workspace_index)[i];
    if (is_second)
      timedbl *= 1.E9;
    auto entry_i64 = static_cast<int64_t>(timedbl);
    Types::Core::DateAndTime entry(timeshift + entry_i64);
    timevec.emplace_back(entry);
  }

  return timevec;
}

/** Get run start time from the target workspace to add the property
 * @brief AddSampleLog::getRunStart
 * @param run_obj
 * @return
 */
Types::Core::DateAndTime AddSampleLog::getRunStart(const API::Run &run_obj) {
  // TODO/ISSUE/NOW - data ws should be the target workspace with run_start or
  // proton_charge property!
  Types::Core::DateAndTime runstart(0);
  try {
    runstart = run_obj.startTime();
  } catch (const std::runtime_error &) {
    // Swallow the error - startTime will just be 0
  }

  return runstart;
}

/** Get the values (in double) of the TimeSeriesProperty's entries from input
 * workspace
 * @brief AddSampleLog::getDblValues
 * @param dataws
 * @param workspace_index
 * @return
 */
std::vector<double> AddSampleLog::getDblValues(const API::MatrixWorkspace_const_sptr &dataws, int workspace_index) {
  std::vector<double> valuevec;
  size_t vecsize = dataws->readY(workspace_index).size();
  for (size_t i = 0; i < vecsize; ++i)
    valuevec.emplace_back(dataws->readY(workspace_index)[i]);

  return valuevec;
}

/** Get the values (in integer) of the TimeSeriesProperty's entries from input
 * workspace
 * @brief AddSampleLog::getIntValues
 * @param dataws
 * @param workspace_index
 * @return
 */
std::vector<int> AddSampleLog::getIntValues(const API::MatrixWorkspace_const_sptr &dataws, int workspace_index) {
  std::vector<int> valuevec;
  size_t vecsize = dataws->readY(workspace_index).size();
  for (size_t i = 0; i < vecsize; ++i)
    valuevec.emplace_back(static_cast<int>(dataws->readY(workspace_index)[i]));

  return valuevec;
}

/** Get Meta data from input data workspace's properties
 * @brief AddSampleLog::getMetaData
 * @param dataws
 * @param epochtime
 * @param timeunit
 */
void AddSampleLog::getMetaData(const API::MatrixWorkspace_const_sptr &dataws, bool &epochtime, std::string &timeunit) {
  bool auto_meta = getProperty("AutoMetaData");
  if (auto_meta) {
    // get the meta data from the input workspace
    std::string epochtimestr = dataws->run().getProperty("IsEpochTime")->value();
    epochtime = epochtimestr == "true";
    timeunit = dataws->run().getProperty("TimeUnit")->value();
  } else {
    // get the meta data from input
    epochtime = !getProperty("RelativeTime");
    timeunit = getPropertyValue("TimeUnit");
  }

  return;
}

} // namespace Mantid::Algorithms
