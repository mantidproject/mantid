//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------

#include "MantidAlgorithms/GenerateEventsFilter.h"
#include "MantidKernel/System.h"
#include "MantidKernel/ListValidator.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/Column.h"
#include "MantidKernel/VisibleWhenProperty.h"
#include "MantidKernel/ArrayProperty.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;

using namespace std;

namespace Mantid {
namespace Algorithms {
DECLARE_ALGORITHM(GenerateEventsFilter)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
GenerateEventsFilter::GenerateEventsFilter() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
GenerateEventsFilter::~GenerateEventsFilter() {}

//----------------------------------------------------------------------------------------------
/** Declare input
 */
void GenerateEventsFilter::init() {
  // Input/Output Workspaces
  declareProperty(new API::WorkspaceProperty<DataObjects::EventWorkspace>(
                      "InputWorkspace", "", Direction::Input),
                  "An input event workspace");

  declareProperty(new API::WorkspaceProperty<API::Workspace>(
                      "OutputWorkspace", "", Direction::Output),
                  "The name to use for the output SplittersWorkspace object, "
                  "i.e., the filter.");

  declareProperty(
      new API::WorkspaceProperty<API::ITableWorkspace>("InformationWorkspace",
                                                       "", Direction::Output),
      "Optional output for the information of each splitter workspace index");

  declareProperty(
      "FastLog", false,
      "Fast log will make output workspace to be a maxtrix workspace. ");

  // Time (general) range
  declareProperty(
      "StartTime", "",
      "The start time, such that all event before this time are filtered out. "
      "It could be (1) relative time to run start time "
      "in unit as specified property 'UnitOfTime' or "
      "(2) absolute time. "
      "Absolute time takes a string in format as 1990-01-01T00:00:00, "
      "while the relative time takes integer or float. ");

  declareProperty(
      "StopTime", "",
      "The stop time, such that all events after this time are filtered out. "
      "It could be (1) relative time to run start time "
      "in unit as specified property 'UnitOfTime' or "
      "(2) absolute time. "
      "Absolute time takes a string in format as 1990-01-01T00:00:00, "
      "while the relative time takes integer or float. ");

  // Split by time (only) in steps
  // TODO - clean this part
  // auto timeintervalproperty = boost::make_shared<ArrayProperty<double> >("TimeInterval");
  declareProperty(new ArrayProperty<double>("TimeInterval"),
                  "Length...");
  // declareProperty("TimeInterval", EMPTY_DBL(),
     //              "Length of the time splices if filtered in time only.");
  setPropertySettings("TimeInterval",
                      new VisibleWhenProperty("LogName", IS_EQUAL_TO, ""));

  std::vector<std::string> timeoptions;
  timeoptions.push_back("Seconds");
  timeoptions.push_back("Nanoseconds");
  timeoptions.push_back("Percent");
  declareProperty(
      "UnitOfTime", "Seconds",
      boost::make_shared<Kernel::StringListValidator>(timeoptions),
      "StartTime, StopTime and DeltaTime can be given in various unit."
      "The unit can be 'Seconds' or 'Nanoseconds' from run start time."
      "They can also be defined as 'Percentage' of total run time.");

  // Split by log value (only) in steps
  declareProperty(
      "LogName", "",
      "Name of the sample log to use to filter.\n"
      "For example, the pulse charge is recorded in 'ProtonCharge'.");

  declareProperty("MinimumLogValue", EMPTY_DBL(),
                  "Minimum log value for which to keep events.");
  setPropertySettings("MinimumLogValue",
                      new VisibleWhenProperty("LogName", IS_NOT_EQUAL_TO, ""));

  declareProperty("MaximumLogValue", EMPTY_DBL(),
                  "Maximum log value for which to keep events.");
  setPropertySettings("MaximumLogValue",
                      new VisibleWhenProperty("LogName", IS_NOT_EQUAL_TO, ""));

  declareProperty("LogValueInterval", EMPTY_DBL(),
                  "Delta of log value to be sliced into from min log value and "
                  "max log value.\n"
                  "If not given, then only value ");
  setPropertySettings("LogValueInterval",
                      new VisibleWhenProperty("LogName", IS_NOT_EQUAL_TO, ""));

  /*
    Documentation doesn't include property options in property descriptions or
  anywhere else.
   For example, FilterLogValueByChangingDirection doesn't let me know that
  Increasing and Decreasing
  are valid options without using the MantidPlotGui to open the algorithm
  dialog.

    */

  std::vector<std::string> filteroptions;
  filteroptions.push_back("Both");
  filteroptions.push_back("Increase");
  filteroptions.push_back("Decrease");
  declareProperty(
      "FilterLogValueByChangingDirection", "Both",
      boost::make_shared<Kernel::StringListValidator>(filteroptions),
      "d(log value)/dt can be positive and negative.  They can be put to "
      "different splitters."
      "There are 3 options, 'Both', 'Increase' and 'Decrease' corresponding to "
      "d(log value)/dt can be any value, positive only and negative only "
      "respectively.");
  setPropertySettings("FilterLogValueByChangingDirection",
                      new VisibleWhenProperty("LogName", IS_NOT_EQUAL_TO, ""));

  declareProperty("TimeTolerance", 0.0,
                  "Tolerance in time for the event times to keep. "
                  "It is used in the case to filter by single value.");
  setPropertySettings("TimeTolerance",
                      new VisibleWhenProperty("LogName", IS_NOT_EQUAL_TO, ""));

  vector<string> logboundoptions;
  logboundoptions.push_back("Centre");
  logboundoptions.push_back("Left");
  logboundoptions.push_back("Other");
  auto logvalidator = boost::make_shared<StringListValidator>(logboundoptions);
  declareProperty(
      "LogBoundary", "Centre", logvalidator,
      "How to treat log values as being measured in the centre of time. "
      "There are three options, 'Centre', 'Left' and 'Other'. ");
  setPropertySettings("LogBoundary",
                      new VisibleWhenProperty("LogName", IS_NOT_EQUAL_TO, ""));

  declareProperty("LogValueTolerance", EMPTY_DBL(),
                  "Tolerance of the log value to be included in filter.  It is "
                  "used in the case to filter by multiple values.");
  setPropertySettings("LogValueTolerance",
                      new VisibleWhenProperty("LogName", IS_NOT_EQUAL_TO, ""));

  // Output workspaces' title and name
  declareProperty(
      "TitleOfSplitters", "",
      "Title of output splitters workspace and information workspace.");

  // Linear or parallel
  vector<string> processoptions;
  processoptions.push_back("Serial");
  processoptions.push_back("Parallel");
  auto procvalidator = boost::make_shared<StringListValidator>(processoptions);
  declareProperty(
      "UseParallelProcessing", "Serial", procvalidator,
      "Use multiple cores to generate events filter by log values. \n"
      "Serial: Use a single core. Good for slow log. \n"
      "Parallel: Use multiple cores. Appropriate for fast log. ");

  declareProperty("NumberOfThreads", EMPTY_INT(),
                  "Number of threads forced to use in the parallel mode. ");

  return;
}

//----------------------------------------------------------------------------------------------
/** Main execute body
 */
void GenerateEventsFilter::exec() {
  // Process input properties
  processInOutWorkspaces();

  // Get Time
  processInputTime();

  double prog = 0.1;
  progress(prog);

  // Get Log
  std::string logname = this->getProperty("LogName");
  if (logname.empty()) {
    // Set up filters by time only
    setFilterByTimeOnly();
  } else {
    // Set up filters by log value in time range
    setFilterByLogValue(logname);
  }

  // Set output workspaces
  if (m_forFastLog) {
    if (m_useParallel) {
      generateSplittersInMatrixWorkspaceParallel();
    } else {
      generateSplittersInMatrixWorkspace();
    }
    setProperty("OutputWorkspace", m_filterWS);
  } else {
    generateSplittersInSplitterWS();
    setProperty("OutputWorkspace", m_splitWS);
  }
  setProperty("InformationWorkspace", m_filterInfoWS);

  return;
}

//----------------------------------------------------------------------------------------------
/** Process user properties
  */
void GenerateEventsFilter::processInOutWorkspaces() {
  // Input data workspace
  m_dataWS = this->getProperty("InputWorkspace");

  // Output splitter information workspace
  std::string title = getProperty("TitleOfSplitters");
  if (title.size() == 0) {
    // Using default
    title = "Splitters";
  }
  m_filterInfoWS =
      API::WorkspaceFactory::Instance().createTable("TableWorkspace");
  m_filterInfoWS->setTitle(title);
  m_filterInfoWS->addColumn("int", "workspacegroup");
  m_filterInfoWS->addColumn("str", "title");

  // Output Splitters workspace: MatrixWorkspace (optioned) will be generated in
  // last step
  m_forFastLog = getProperty("FastLog");
  if (!m_forFastLog) {
    m_splitWS = boost::shared_ptr<DataObjects::SplittersWorkspace>(
        new DataObjects::SplittersWorkspace());
    m_splitWS->setTitle(title);
  }

  string algtype = getPropertyValue("UseParallelProcessing");
  if (algtype == "Serial")
    m_useParallel = false;
  else if (algtype == "Parallel")
    m_useParallel = true;
  else
    throw std::runtime_error(
        "Impossible to have 3rd type other than Serial and Parallel. ");

  // Conflict
  if (m_useParallel && !m_forFastLog) {
    g_log.warning("Parallelization is for fast log only.  Automatically turn "
                  "FastLog on. ");
    m_forFastLog = true;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Process the input for time.  A smart but complicated default rule
  * (1) Input time can be in multiple format: absolute time (ISO), relative time
 * (double)
 */
void GenerateEventsFilter::processInputTime() {
  // Get input
  std::string s_inpt0 = this->getProperty("StartTime");
  std::string s_inptf = this->getProperty("StopTime");

  // Default
  bool defaultstart = (s_inpt0.size() == 0);
  bool defaultstop = (s_inptf.size() == 0);

  // Determine format
  bool instringformat = true;
  if (!defaultstart && s_inpt0.find(':') == std::string::npos) {
    // StartTime is not empty and does not contain ":": not ISO
    instringformat = false;
  } else if (!defaultstop && s_inptf.find(':') == std::string::npos) {
    // StopTime is not empty and does not contain ":": not ISO format
    instringformat = false;
  }

  // Obtain run time range
  DateAndTime runstarttime = m_dataWS->run().startTime();

  m_runEndTime = findRunEnd();

  // Obtain time unit converter
  std::string timeunit = this->getProperty("UnitOfTime");
  m_timeUnitConvertFactorToNS = -1.0;
  if (timeunit.compare("Seconds") == 0) {
    // (second)
    m_timeUnitConvertFactorToNS = 1.0E9;
  } else if (timeunit.compare("Nanoseconds") == 0) {
    // (nano-seconds)
    m_timeUnitConvertFactorToNS = 1.0;
  } else if (timeunit.compare("Percent") == 0) {
    // (percent of total run time)
    int64_t runtime_ns =
        m_runEndTime.totalNanoseconds() - runstarttime.totalNanoseconds();
    double runtimed_ns = static_cast<double>(runtime_ns);
    m_timeUnitConvertFactorToNS = 0.01 * runtimed_ns;
  } else {
    // (Not defined/supported)
    stringstream errss;
    errss << "TimeType " << timeunit << " is not supported.";
    throw std::runtime_error(errss.str());
  }

  // Set up start time
  if (defaultstart) {
    // Default
    m_startTime = runstarttime;
  } else if (instringformat) {
    // Time is absolute time in ISO format
    m_startTime = DateAndTime(s_inpt0);
  } else {
    // Relative time in double.
    double inpt0 = atof(s_inpt0.c_str());
    if (inpt0 < 0) {
      stringstream errss;
      errss << "Input relative StartTime " << inpt0 << " cannot be negative. ";
      throw std::invalid_argument(errss.str());
    }
    int64_t t0_ns = runstarttime.totalNanoseconds() +
                    static_cast<int64_t>(inpt0 * m_timeUnitConvertFactorToNS);
    m_startTime = Kernel::DateAndTime(t0_ns);
  }

  // Set up run stop time
  if (defaultstop) {
    // Default
    m_stopTime = m_runEndTime;
  } else if (instringformat) {
    // Absolute time in ISO format
    m_stopTime = DateAndTime(s_inptf);
  } else {
    // Relative time in double
    double inptf = atof(s_inptf.c_str());
    int64_t tf_ns = runstarttime.totalNanoseconds() +
                    static_cast<int64_t>(inptf * m_timeUnitConvertFactorToNS);
    m_stopTime = Kernel::DateAndTime(tf_ns);
  }

  // Check start/stop time
  if (m_startTime.totalNanoseconds() >= m_stopTime.totalNanoseconds()) {
    stringstream errss;
    errss << "Input StartTime " << m_startTime.toISO8601String()
          << " is equal or later than "
          << "input StopTime " << m_stopTime.toFormattedString();
    throw runtime_error(errss.str());
  }

  g_log.information() << "Filter: StartTime = " << m_startTime
                      << ", StopTime = " << m_stopTime
                      << "; Run start = " << runstarttime.toISO8601String()
                      << ", Run stop = " << m_runEndTime.toISO8601String()
                      << "\n";

  return;
}

//----------------------------------------------------------------------------------------------
/** Set splitters by time value / interval only
 */
void GenerateEventsFilter::setFilterByTimeOnly() {
  vector<double> vec_timeintervals = this->getProperty("TimeInterval");

  bool singleslot = false;
  if (vec_timeintervals.size() == 0)
    singleslot = true;

  // Progress
  int64_t totaltime =
      m_stopTime.totalNanoseconds() - m_startTime.totalNanoseconds();

  g_log.warning() << "Filter by time: start @ "
                  << m_startTime.totalNanoseconds() << "; "
                  << "stop @ " << m_stopTime.totalNanoseconds() << "\n";

  if (singleslot) {
    int wsindex = 0;

    // Default and thus just one interval
    std::stringstream ss;
    ss << "Time Interval From " << m_startTime << " to " << m_stopTime;

    addNewTimeFilterSplitter(m_startTime, m_stopTime, wsindex, ss.str());
  } else if (vec_timeintervals.size() == 1) {
    double timeinterval = vec_timeintervals[0];
    int64_t timeslot = 0;

    // Explicitly N time intervals
    int64_t deltatime_ns =
        static_cast<int64_t>(timeinterval * m_timeUnitConvertFactorToNS);

    int64_t curtime_ns = m_startTime.totalNanoseconds();
    int wsindex = 0;
    while (curtime_ns < m_stopTime.totalNanoseconds()) {
      // Calculate next.time
      int64_t nexttime_ns = curtime_ns + deltatime_ns;
      if (nexttime_ns > m_stopTime.totalNanoseconds())
        nexttime_ns = m_stopTime.totalNanoseconds();

      // Create splitter and information
      Kernel::DateAndTime t0(curtime_ns);
      Kernel::DateAndTime tf(nexttime_ns);
      std::stringstream ss;
      ss << "Time Interval From " << t0 << " to " << tf;

      addNewTimeFilterSplitter(t0, tf, wsindex, ss.str());

      // Update loop variable
      curtime_ns = nexttime_ns;
      wsindex++;

      // Update progress
      int64_t newtimeslot =
          (curtime_ns - m_startTime.totalNanoseconds()) * 90 / totaltime;
      if (newtimeslot > timeslot) {
        // There is change and update progress
        timeslot = newtimeslot;
        double prog = 0.1 + double(timeslot) / 100.0;
        progress(prog);
      }
    } // END-WHILE
  }   // END-IF-ELSE
  else
  {
    throw std::runtime_error("Implement multiple time interval ASAP.");
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Generate filters by log values.
  * @param logname :: name of the log to filter with
  */
void GenerateEventsFilter::setFilterByLogValue(std::string logname) {
  // Obtain reference of sample log to filter with
  m_dblLog = dynamic_cast<TimeSeriesProperty<double> *>(
      m_dataWS->run().getProperty(logname));
  m_intLog = dynamic_cast<TimeSeriesProperty<int> *>(
      m_dataWS->run().getProperty(logname));
  if (!m_dblLog && !m_intLog) {
    stringstream errmsg;
    errmsg
        << "Log " << logname
        << " does not exist or is not TimeSeriesProperty in double or integer.";
    throw runtime_error(errmsg.str());
  }

  //  Clear duplicate value and extend to run end
  if (m_dblLog) {
    g_log.debug("Attempting to remove duplicates in double series log.");
    if (m_runEndTime > m_dblLog->lastTime())
      m_dblLog->addValue(m_runEndTime, 0.);
    m_dblLog->eliminateDuplicates();
  } else {
    g_log.debug("Attempting to remove duplicates in integer series log.");
    m_intLog->addValue(m_runEndTime, 0);
    m_intLog->eliminateDuplicates();
  }

  // Process input properties related to filter with log value
  double minvalue = this->getProperty("MinimumLogValue");
  double maxvalue = this->getProperty("MaximumLogValue");
  double deltaValue = this->getProperty("LogValueInterval");

  // Log value change direction
  std::string filterdirection =
      getProperty("FilterLogValueByChangingDirection");
  bool filterIncrease;
  bool filterDecrease;
  if (filterdirection.compare("Both") == 0) {
    filterIncrease = true;
    filterDecrease = true;
  } else if (filterdirection.compare("Increase") == 0) {
    filterIncrease = true;
    filterDecrease = false;
  } else {
    filterIncrease = false;
    filterDecrease = true;
  }

  bool toProcessSingleValueFilter = false;
  if (isEmpty(deltaValue)) {
    toProcessSingleValueFilter = true;
  } else if (deltaValue < 0) {
    throw runtime_error("Delta value cannot be negative.");
  }

  // Log boundary
  string logboundary = getProperty("LogBoundary");
  if (logboundary.compare("Centre"))
    m_logAtCentre = true;
  else
    m_logAtCentre = false;

  m_logTimeTolerance = getProperty("TimeTolerance");

  // Generate filters
  if (m_dblLog) {
    // Double TimeSeriesProperty log
    // Process min/max
    if (minvalue == EMPTY_DBL()) {
      minvalue = m_dblLog->minValue();
    }
    if (maxvalue == EMPTY_DBL()) {
      maxvalue = m_dblLog->maxValue();
    }

    if (minvalue > maxvalue) {
      stringstream errmsg;
      errmsg << "Fatal Error: Input minimum log value " << minvalue
             << " is larger than maximum log value " << maxvalue;
      throw runtime_error(errmsg.str());
    } else {
      g_log.debug() << "Filter by log value: min = " << minvalue
                    << ", max = " << maxvalue << ", process single value? = "
                    << toProcessSingleValueFilter
                    << ", delta value = " << deltaValue << "\n";
    }

    // Filter double value log
    if (toProcessSingleValueFilter) {
      // Generate a filter for a single log value
      processSingleValueFilter(minvalue, maxvalue, filterIncrease,
                               filterDecrease);
    } else {
      // Generate filters for a series of log value
      processMultipleValueFilters(minvalue, deltaValue, maxvalue,
                                  filterIncrease, filterDecrease);
    }
  } else {
    // Integer TimeSeriesProperty log
    // Process min/max allowed value
    int minvaluei, maxvaluei;
    if (minvalue == EMPTY_DBL()) {
      minvaluei = m_intLog->minValue();
      minvalue = static_cast<double>(minvaluei);
    } else
      minvaluei = static_cast<int>(minvalue + 0.5);

    if (maxvalue == EMPTY_DBL()) {
      maxvaluei = m_intLog->maxValue();
      maxvalue = static_cast<double>(maxvaluei);
    } else
      maxvaluei = static_cast<int>(maxvalue + 0.5);

    if (minvalue > maxvalue) {
      stringstream errmsg;
      errmsg << "Fatal Error: Input minimum log value " << minvalue
             << " is larger than maximum log value " << maxvalue;
      throw runtime_error(errmsg.str());
    } else {
      g_log.information() << "Generate event-filter for integer log: min = "
                          << minvaluei << ", "
                          << "max = " << maxvaluei << "\n";
    }

    // Split along log
    DateAndTime runendtime = m_dataWS->run().endTime();
    processIntegerValueFilter(minvaluei, maxvaluei, filterIncrease,
                              filterDecrease, runendtime);

  } // ENDIFELSE: Double/Integer Log

  g_log.information() << "Minimum value = " << minvalue << ", "
                      << "maximum value = " << maxvalue << ".\n";

  return;
}

//----------------------------------------------------------------------------------------------
/** Generate filters by single log value
  * @param minvalue :: minimum value of the allowed log value;
  * @param maxvalue :: maximum value of the allowed log value;
  * @param filterincrease :: if true, log value in the increasing curve should
 * be included;
  * @param filterdecrease :: if true, log value in the decreasing curve should
 * be included;
  */
void GenerateEventsFilter::processSingleValueFilter(double minvalue,
                                                    double maxvalue,
                                                    bool filterincrease,
                                                    bool filterdecrease) {
  // Get parameters time-tolerance and log-boundary
  double timetolerance = this->getProperty("TimeTolerance");
  int64_t timetolerance_ns =
      static_cast<int64_t>(timetolerance * m_timeUnitConvertFactorToNS);

  std::string logboundary = this->getProperty("LogBoundary");
  transform(logboundary.begin(), logboundary.end(), logboundary.begin(),
            ::tolower);

  // Generate filter
  // std::vector<Kernel::SplittingInterval> splitters;
  int wsindex = 0;
  makeFilterBySingleValue(minvalue, maxvalue,
                          static_cast<double>(timetolerance_ns) * 1.0E-9,
                          logboundary.compare("centre") == 0, filterincrease,
                          filterdecrease, m_startTime, m_stopTime, wsindex);

  // Create information table workspace
  if (!m_filterInfoWS)
    throw runtime_error("m_filterInfoWS has not been initialized.");

  API::TableRow row = m_filterInfoWS->appendRow();
  std::stringstream ss;
  ss << "Log " << m_dblLog->name() << " From " << minvalue << " To " << maxvalue
     << "  Value-change-direction ";
  if (filterincrease && filterdecrease) {
    ss << " both ";
  } else if (filterincrease) {
    ss << " increase";
  } else {
    ss << " decrease";
  }
  row << 0 << ss.str();

  return;
}

//----------------------------------------------------------------------------------------------
/** Generate filters from multiple values
  * @param minvalue :: minimum value of the allowed log value;
  * @param valueinterval :: step of the log value for a series of filter
  * @param maxvalue :: maximum value of the allowed log value;
  * @param filterincrease :: if true, log value in the increasing curve should
 * be included;
  * @param filterdecrease :: if true, log value in the decreasing curve should
 * be included;
 */
void GenerateEventsFilter::processMultipleValueFilters(double minvalue,
                                                       double valueinterval,
                                                       double maxvalue,
                                                       bool filterincrease,
                                                       bool filterdecrease) {
  // Read more input
  if (valueinterval <= 0)
    throw std::invalid_argument(
        "Multiple values filter must have LogValueInterval larger than ZERO.");
  double valuetolerance = this->getProperty("LogValueTolerance");

  if (valuetolerance == EMPTY_DBL())
    valuetolerance = 0.5 * valueinterval;
  else if (valuetolerance < 0.0)
    throw std::runtime_error("LogValueTolerance cannot be less than zero.");

  // Create log value interval (low/up boundary) list and split information
  // workspace
  std::map<size_t, int> indexwsindexmap;
  std::vector<double> logvalueranges;
  int wsindex = 0;
  size_t index = 0;

  double curvalue = minvalue;
  while (curvalue - valuetolerance < maxvalue) {
    indexwsindexmap.insert(std::make_pair(index, wsindex));

    // Log interval/value boundary
    double lowbound = curvalue - valuetolerance;
    double upbound = curvalue + valueinterval - valuetolerance;
    logvalueranges.push_back(lowbound);
    logvalueranges.push_back(upbound);

    // Workgroup information
    std::stringstream ss;
    ss << "Log " << m_dblLog->name() << " From " << lowbound << " To "
       << upbound << "  Value-change-direction ";
    if (filterincrease && filterdecrease) {
      ss << " both ";
    } else if (filterincrease) {
      ss << " increase";
    } else {
      ss << " decrease";
    };
    API::TableRow newrow = m_filterInfoWS->appendRow();
    newrow << wsindex << ss.str();

    curvalue += valueinterval;
    wsindex++;
    ++index;
  } // ENDWHILE

  // Debug print
  stringstream dbsplitss;
  dbsplitss << "Index map size = " << indexwsindexmap.size() << "\n";
  for (map<size_t, int>::iterator mit = indexwsindexmap.begin();
       mit != indexwsindexmap.end(); ++mit) {
    dbsplitss << "Index " << mit->first << ":  WS-group = " << mit->second
              << ". Log value range: [" << logvalueranges[mit->first * 2]
              << ", " << logvalueranges[mit->first * 2 + 1] << ").\n";
  }
  g_log.information(dbsplitss.str());

  // Check split interval obtained wehther is with valid size
  if (logvalueranges.size() < 2) {
    g_log.warning("There is no log value interval existing.");
    return;
  }

  {
    // Warning information
    double upperboundinterval0 = logvalueranges[1];
    double lowerboundlastinterval = logvalueranges[logvalueranges.size() - 2];
    double minlogvalue = m_dblLog->minValue();
    double maxlogvalue = m_dblLog->maxValue();
    if (minlogvalue > upperboundinterval0 ||
        maxlogvalue < lowerboundlastinterval) {
      g_log.warning()
          << "User specifies log interval from " << minvalue - valuetolerance
          << " to " << maxvalue - valuetolerance
          << " with interval size = " << valueinterval << "; Log "
          << m_dblLog->name() << " has range " << minlogvalue << " to "
          << maxlogvalue
          << ".  Therefore some workgroup index may not have any splitter."
          << std::endl;
    }
  }

  // Generate event filters by log value
  std::string logboundary = this->getProperty("LogBoundary");
  transform(logboundary.begin(), logboundary.end(), logboundary.begin(),
            ::tolower);

  if (m_useParallel) {
    // Make filters in parallel
    makeMultipleFiltersByValuesParallel(
        indexwsindexmap, logvalueranges, logboundary.compare("centre") == 0,
        filterincrease, filterdecrease, m_startTime, m_stopTime);
  } else {
    // Make filters in serial
    makeMultipleFiltersByValues(
        indexwsindexmap, logvalueranges, logboundary.compare("centre") == 0,
        filterincrease, filterdecrease, m_startTime, m_stopTime);
  }

  return;
}

//-----------------------------------------------------------------------------------------------
/**
 * Fill a TimeSplitterType that will filter the events by matching
 * SINGLE log values >= min and < max. Creates SplittingInterval's where
 * times match the log values, and going to index==0.
 *
 *(removed) split :: Splitter that will be filled.
 * @param min :: Min value.
 * @param max :: Max value.
 * @param TimeTolerance :: Offset added to times in seconds.
 * @param centre :: Whether the log value time is considered centred or at the
 *beginning.
 * @param filterIncrease :: As log value increase, and within (min, max),
 *include this range in the filter.
 * @param filterDecrease :: As log value increase, and within (min, max),
 *include this range in the filter.
 * @param startTime :: Start time.
 * @param stopTime :: Stop time.
 * @param wsindex :: Workspace index.
 */
void GenerateEventsFilter::makeFilterBySingleValue(
    double min, double max, double TimeTolerance, bool centre,
    bool filterIncrease, bool filterDecrease, DateAndTime startTime,
    Kernel::DateAndTime stopTime, int wsindex) {
  // Do nothing if the log is empty.
  if (m_dblLog->size() == 0) {
    g_log.warning() << "There is no entry in this property " << this->name()
                    << "\n";
    return;
  }

  // Clear splitters in vector format
  m_vecSplitterGroup.clear();
  m_vecSplitterTime.clear();

  // Initialize control parameters
  bool lastGood = false;
  bool isGood = false;
  ;
  time_duration tol = DateAndTime::durationFromSeconds(TimeTolerance);
  int numgood = 0;
  DateAndTime lastTime, currT;
  DateAndTime start, stop;

  size_t progslot = 0;
  string info("");

  for (int i = 0; i < m_dblLog->size(); i++) {
    lastTime = currT;
    // The new entry
    currT = m_dblLog->nthTime(i);

    // A good value?
    isGood = identifyLogEntry(i, currT, lastGood, min, max, startTime, stopTime,
                              filterIncrease, filterDecrease);
    if (isGood)
      numgood++;

    // Log status (time/value/value changing direciton) is changed
    if (isGood != lastGood) {
      // We switched from bad to good or good to bad
      if (isGood) {
        // Start of a good section
        if (centre)
          start = currT - tol;
        else
          start = currT;
      } else {
        // End of the good section
        if (centre) {
          stop = currT - tol;
        } else {
          stop = currT;
        }

        addNewTimeFilterSplitter(start, stop, wsindex, info);

        // Reset the number of good ones, for next time
        numgood = 0;
      }
      lastGood = isGood;
    }

    // Progress bar..
    size_t tmpslot = i * 90 / m_dblLog->size();
    if (tmpslot > progslot) {
      progslot = tmpslot;
      double prog = double(progslot) / 100.0 + 0.1;
      progress(prog);
    }

  } // ENDFOR

  if (numgood > 0) {
    // The log ended on "good" so we need to close it using the last time we
    // found
    if (centre)
      stop = currT - tol;
    else
      stop = currT;
    addNewTimeFilterSplitter(start, stop, wsindex, info);
    numgood = 0;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Identify a log entry whether it can be included in the splitter for
 * filtering by single log value
  * - Direction: direction of changing value will be determined by the current
 * log entry and next entry
  *              because the boundary should be set at the first value with new
 * direction (as well as last value
  *              with the old direction)
  */
bool GenerateEventsFilter::identifyLogEntry(
    const int &index, const Kernel::DateAndTime &currT, const bool &lastgood,
    const double &minvalue, const double &maxvalue,
    const Kernel::DateAndTime &startT, const Kernel::DateAndTime &stopT,
    const bool &filterIncrease, const bool &filterDecrease) {
  double val = m_dblLog->nthValue(index);

  // Identify by time and value
  bool isgood =
      (val >= minvalue && val < maxvalue) && (currT >= startT && currT < stopT);

  // Consider direction: not both (i.e., not increase or not decrease)
  if (isgood && (!filterIncrease || !filterDecrease)) {
    int numlogentries = m_dblLog->size();
    double diff;
    if (index < numlogentries - 1) {
      // For a non-last log entry
      diff = m_dblLog->nthValue(index + 1) - val;
    } else {
      // Last log entry: follow the last direction
      diff = val - m_dblLog->nthValue(index - 1);
    }

    if (diff > 0 && filterIncrease)
      isgood = true;
    else if (diff < 0 && filterDecrease)
      isgood = true;
    else if (diff == 0)
      isgood = lastgood;
    else
      isgood = false;
  }

  return isgood;
}

//-----------------------------------------------------------------------------------------------
/** Fill a TimeSplitterType that will filter the events by matching
 * SINGLE log values >= min and < max. Creates SplittingInterval's where
 * times match the log values, and going to index==0.
 * @param indexwsindexmap :: Index.
 * @param logvalueranges ::  A vector of double. Each 2i and 2i+1 pair is one
 * individual log value range.
 * @param centre :: Whether the log value time is considered centred or at the
 * beginning.
 * @param filterIncrease :: As log value increase, and within (min, max),
 * include this range in the filter.
 * @param filterDecrease :: As log value increase, and within (min, max),
 * include this range in the filter.
 * @param startTime :: Start time.
 * @param stopTime :: Stop time.
 */
void GenerateEventsFilter::makeMultipleFiltersByValues(
    map<size_t, int> indexwsindexmap, vector<double> logvalueranges,
    bool centre, bool filterIncrease, bool filterDecrease,
    DateAndTime startTime, DateAndTime stopTime) {
  g_log.notice("Starting method 'makeMultipleFiltersByValues'. ");

  // Return if the log is empty.
  int logsize = m_dblLog->size();
  if (logsize == 0) {
    g_log.warning() << "There is no entry in this property " << m_dblLog->name()
                    << std::endl;
    return;
  }

  // Set up
  double timetolerance = 0.0;
  if (centre) {
    timetolerance = this->getProperty("TimeTolerance");
  }
  time_duration tol = DateAndTime::durationFromSeconds(timetolerance);

  // Determine the number of threads to use
  // int numThreads = 1;
  vector<DateAndTime> tempvectimes;
  // tempvectimes.reserve(m_dblLog->size());
  vector<int> tempvecgroup;
  // tempvecgroup.reserve(m_dblLog->size());
  vecSplitterTimeSet.push_back(tempvectimes);
  vecGroupIndexSet.push_back(tempvecgroup);
  int istart = 0;
  int iend = static_cast<int>(logsize - 1);

  makeMultipleFiltersByValuesPartialLog(
      istart, iend, m_vecSplitterTime, m_vecSplitterGroup, indexwsindexmap,
      logvalueranges, tol, filterIncrease, filterDecrease, startTime, stopTime);

  progress(1.0);

  return;
}

//-----------------------------------------------------------------------------------------------
/** Fill a TimeSplitterType that will filter the events by matching
 * SINGLE log values >= min and < max. Creates SplittingInterval's where
 * times match the log values, and going to index==0.
 *
 * @param indexwsindexmap :: Index.
 * @param logvalueranges ::  A vector of double. Each 2i and 2i+1 pair is one
 *individual log value range.
 * @param centre :: Whether the log value time is considered centred or at the
 *beginning.
 * @param filterIncrease :: As log value increase, and within (min, max),
 *include this range in the filter.
 * @param filterDecrease :: As log value increase, and within (min, max),
 *include this range in the filter.
 * @param startTime :: Start time.
 * @param stopTime :: Stop time.
 */
void GenerateEventsFilter::makeMultipleFiltersByValuesParallel(
    map<size_t, int> indexwsindexmap, vector<double> logvalueranges,
    bool centre, bool filterIncrease, bool filterDecrease,
    DateAndTime startTime, DateAndTime stopTime) {
  // Return if the log is empty.
  int logsize = m_dblLog->size();
  if (logsize == 0) {
    g_log.warning() << "There is no entry in this property " << m_dblLog->name()
                    << std::endl;
    return;
  }

  // Set up
  double timetolerance = 0.0;
  if (centre) {
    timetolerance = this->getProperty("TimeTolerance");
  }
  time_duration tol = DateAndTime::durationFromSeconds(timetolerance);

  // Determine the number of threads to use
  int numThreads = getProperty("NumberOfThreads");
  if (isEmpty(numThreads))
    numThreads = static_cast<int>(PARALLEL_GET_MAX_THREADS);

  // Limit the number of threads.
  numThreads = std::min(numThreads, logsize / 8);

  // Determine the istart and iend
  // For split, log should be [0, n], [n, 2n], [2n, 3n], ... as to look into n
  // log values
  std::vector<int> vecStart, vecEnd;
  int partloglength = (logsize - 1) / numThreads;
  int extra = (logsize - 1) % numThreads;
  for (int i = 0; i < numThreads; ++i) {
    int istart = i * partloglength;
    if (i < extra)
      istart += i;
    else
      istart += extra;

    int iend = istart + partloglength;
    if (i < extra)
      ++iend;

    vecStart.push_back(istart);
    vecEnd.push_back(iend);
  }

  {
    stringstream dbss;
    dbss << "Number of thread = " << numThreads << ", Log Size = " << logsize
         << "\n";
    for (size_t i = 0; i < vecStart.size(); ++i) {
      dbss << "Thread " << i << ": Log range: [" << vecStart[i] << ", "
           << vecEnd[i] << ") "
           << "Size = " << vecEnd[i] - vecStart[i] << "\n";
    }
    g_log.information(dbss.str());
  }

  // Create partial vectors
  vecSplitterTimeSet.clear();
  vecGroupIndexSet.clear();
  for (int i = 0; i < numThreads; ++i) {
    vector<DateAndTime> tempvectimes;
    tempvectimes.reserve(m_dblLog->size());
    vector<int> tempvecgroup;
    tempvecgroup.reserve(m_dblLog->size());
    vecSplitterTimeSet.push_back(tempvectimes);
    vecGroupIndexSet.push_back(tempvecgroup);
  }

  // Create event filters/splitters in parallel
  // cppcheck-suppress syntaxError
    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int i = 0; i < numThreads; ++i) {
      PARALLEL_START_INTERUPT_REGION

      int istart = vecStart[i];
      int iend = vecEnd[i];

      makeMultipleFiltersByValuesPartialLog(
          istart, iend, vecSplitterTimeSet[i], vecGroupIndexSet[i],
          indexwsindexmap, logvalueranges, tol, filterIncrease, filterDecrease,
          startTime, stopTime);
      PARALLEL_END_INTERUPT_REGION
    }
    PARALLEL_CHECK_INTERUPT_REGION

    // Concatenate splitters on different threads together
    for (int i = 1; i < numThreads; ++i) {
      if (vecSplitterTimeSet[i - 1].back() == vecSplitterTimeSet[i].front()) {
        // T_(i).back() = T_(i+1).front()
        if (vecGroupIndexSet[i - 1].back() == vecGroupIndexSet[i].front()) {
          // G_(i).back() = G_(i+1).front(), combine these adjacent 2 splitters
          // Rule out impossible situation
          if (vecGroupIndexSet[i - 1].back() == -1) {
            // Throw with detailed error message
            stringstream errss;
            errss << "Previous vector of group index set (" << (i - 1)
                  << ") is equal to -1. "
                  << "It is not likely to happen!  Size of previous vector of "
                     "group is " << vecGroupIndexSet[i - 1].size()
                  << ". \nSuggest to use sequential mode. ";
            throw runtime_error(errss.str());
          }

          // Pop back last element
          vecGroupIndexSet[i - 1].pop_back();
          DateAndTime newt0 = vecSplitterTimeSet[i - 1].front();
          vecSplitterTimeSet[i - 1].pop_back();
          DateAndTime origtime = vecSplitterTimeSet[i][0];
          vecSplitterTimeSet[i][0] = newt0;
          g_log.debug() << "Splitter at the end of thread " << i
                        << " is extended from " << origtime << " to " << newt0
                        << "\n";
        } else {
          // 2 different and continous spliiter: ideal case without any
          // operation
          ;
        }
      } else {
        // T_(i).back() != T_(i+1).front(): need to fill the gap in time
        int lastindex = vecGroupIndexSet[i - 1].back();
        int firstindex = vecGroupIndexSet[i].front();

        if (lastindex != -1 && firstindex != -1) {
          // T_stop < T_start, I_stop != -1, I_start != 1. : Insert a minus-one
          // entry to make it complete
          vecGroupIndexSet[i - 1].push_back(-1);
          vecSplitterTimeSet[i - 1].push_back(vecSplitterTimeSet[i].front());
        } else if (lastindex == -1 && vecGroupIndexSet[i - 1].size() == 1) {
          // Empty splitter of the thread. Extend this to next
          vecSplitterTimeSet[i - 1].back() = vecSplitterTimeSet[i].front();
          g_log.debug() << "Thread = " << i << ", change ending time of "
                        << i - 1 << " to " << vecSplitterTimeSet[i].front()
                        << "\n";
        } else if (firstindex == -1 && vecGroupIndexSet[i].size() == 1) {
          // Empty splitter of the thread. Extend last one to this
          vecSplitterTimeSet[i].front() = vecSplitterTimeSet[i - 1].back();
          g_log.debug() << "Thread = " << i << ", change starting time to "
                        << vecSplitterTimeSet[i].front() << "\n";
        } else {
          throw runtime_error("It is not possible to have start or end of "
                              "filter to be minus-one index. ");
        }
      }
    }

    progress(1.0);

    return;
}

//----------------------------------------------------------------------------------------------
/** Make filters by multiple log values of partial log
  */
void GenerateEventsFilter::makeMultipleFiltersByValuesPartialLog(
    int istart, int iend, std::vector<Kernel::DateAndTime> &vecSplitTime,
    std::vector<int> &vecSplitGroup, map<size_t, int> indexwsindexmap,
    const vector<double> &logvalueranges, time_duration tol,
    bool filterIncrease, bool filterDecrease, DateAndTime startTime,
    DateAndTime stopTime) {
  // Check
  int logsize = m_dblLog->size();
  if (istart < 0 || iend >= logsize)
    throw runtime_error("Input index of makeMultipleFiltersByValuesPartialLog "
                        "is out of boundary. ");

  int64_t tol_ns = tol.total_nanoseconds();

  // Define loop control parameters
  const Kernel::DateAndTime ZeroTime(0);
  int lastindex = -1;
  int currindex = -1;
  DateAndTime lastTime;
  DateAndTime currTime = ZeroTime;
  DateAndTime start, stop;
  // size_t progslot = 0;

  g_log.information() << "Log time coverage (index: " << istart << ", " << iend
                      << ") from " << m_dblLog->nthTime(istart) << ", "
                      << m_dblLog->nthTime(iend) << "\n";

  DateAndTime laststoptime(0);
  int lastlogindex = m_dblLog->size() - 1;

  int prevDirection = determineChangingDirection(istart);

  for (int i = istart; i <= iend; i++) {
    // Initialize status flags and new entry
    bool breakloop = false;
    bool createsplitter = false;

    lastTime = currTime;
    currTime = m_dblLog->nthTime(i);
    double currValue = m_dblLog->nthValue(i);

    // Filter out by time and direction (optional)
    bool intime = true;
    if (currTime < startTime) {
      // case i.  Too early, do nothing
      createsplitter = false;
    } else if (currTime > stopTime) {
      // case ii. Too late.  Put to splitter if half of splitter is done.  But
      // still within range
      breakloop = true;
      stop = currTime;
      if (start.totalNanoseconds() > 0) {
        createsplitter = true;
      }
    }

    // Check log within given time range
    bool newsplitter = false; // Flag to start a new split in this loop

    // Determine direction
    int direction = 0;
    if (i < lastlogindex) {
      // Not the last log entry
      double diff = m_dblLog->nthValue(i + 1) - m_dblLog->nthValue(i);
      if (diff > 0)
        direction = 1;
      else if (diff < 0)
        direction = -1;
      else
        direction = prevDirection;
    } else {
      // Last log entry: use the previous direction
      direction = prevDirection;
    }
    prevDirection = direction;

    // Examine log value for filter
    if (intime) {
      // Determine whether direction is fine
      bool correctdir = true;
      if (filterIncrease && filterDecrease) {
        // Both direction is fine
        correctdir = true;
      } else {
        // Filter out one direction
        if (filterIncrease && direction > 0)
          correctdir = true;
        else if (filterDecrease && direction < 0)
          correctdir = true;
        else if (direction == 0)
          throw runtime_error("Direction is not determined.");
        else
          correctdir = false;
      } // END-IF-ELSE: Direction

      // Treat the log entry based on: changing direction (+ time range)
      if (correctdir) {
        // Check this value whether it falls into any range
        size_t index = searchValue(logvalueranges, currValue);

        if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
          stringstream dbss;
          dbss << "[DBx257] Examine Log Index " << i
               << ", Value = " << currValue << ", Data Range Index = " << index
               << "; "
               << "Group Index = " << indexwsindexmap[index / 2]
               << " (log value range vector size = " << logvalueranges.size()
               << "): ";
          if (index == 0)
            dbss << logvalueranges[index] << ", " << logvalueranges[index + 1];
          else if (index == logvalueranges.size())
            dbss << logvalueranges[index - 1] << ", " << logvalueranges[index];
          else
            dbss << logvalueranges[index - 1] << ", " << logvalueranges[index]
                 << ", " << logvalueranges[index + 1];
          g_log.debug(dbss.str());
        }

        bool valueWithinMinMax = true;
        if (index > logvalueranges.size()) {
          // Out of range
          valueWithinMinMax = false;
        }

        if (valueWithinMinMax) {
          if (index % 2 == 0) {
            // [Situation] Falls in the interval
            currindex = indexwsindexmap[index / 2];

            if (currindex != lastindex && start.totalNanoseconds() == 0) {
              // Group index is different from last and start is not set up: new
              // a region!
              newsplitter = true;
            } else if (currindex != lastindex && start.totalNanoseconds() > 0) {
              // Group index is different from last and start is set up:  close
              // a region and new a region
              stop = currTime;
              createsplitter = true;
              newsplitter = true;
            } else if (currindex == lastindex && start.totalNanoseconds() > 0) {
              // Still of the group index
              if (i == iend) {
                // Last entry in this section of log.  Need to flag to close the
                // pair
                stop = currTime;
                createsplitter = true;
                newsplitter = false;
              } else {
                // Do nothing
                ;
              }
            } else {
              // An impossible situation
              std::stringstream errmsg;
              double lastvalue = m_dblLog->nthValue(i - 1);
              errmsg << "Impossible to have currindex == lastindex == "
                     << currindex
                     << ", while start is not init.  Log Index = " << i
                     << "\t value = " << currValue << "\t, Index = " << index
                     << " in range " << logvalueranges[index] << ", "
                     << logvalueranges[index + 1]
                     << "; Last value = " << lastvalue;
              throw std::runtime_error(errmsg.str());
            }
          } // [In-bound: Inside interval]
          else {
            // [Situation] Fall between interval (which is not likley happen)
            currindex = -1;
            g_log.warning()
                << "Not likely to happen! Current value = " << currValue
                << " is  within value range but its index = " << index
                << " has no map to group index. "
                << "\n";
            if (start.totalNanoseconds() > 0) {
              // Close the interval pair if it has been started.
              stop = currTime;
              createsplitter = true;
            }
          } // [In-bound: Between interval]
        } else {
          // Out of a range: check whether there is a splitter started
          currindex = -1;
          if (start.totalNanoseconds() > 0) {
            // End situation
            stop = currTime;
            createsplitter = true;
          }
        } // [Out-bound]

      } // [CORRECT DIRECTION]
      else {
        // Log Index i falls out b/c out of wrong direction
        currindex = -1;

        // Condition to generate a Splitter (close parenthesis)
        if (!correctdir && start.totalNanoseconds() > 0) {
          stop = currTime;
          createsplitter = true;
        }
      }
    } // ENDIF (log entry in specified time)
    else {
      // Log Index i falls out b/c out of time range...
      currindex = -1;
    }

    // d) Create Splitter
    if (createsplitter) {
      // make_splitter(start, stop, lastindex, tol);
      makeSplitterInVector(vecSplitTime, vecSplitGroup, start, stop, lastindex,
                           tol_ns, laststoptime);

      // reset
      start = ZeroTime;
    }

    // e) Start new splitter: have to be here due to start cannot be updated
    // before a possible splitter generated
    if (newsplitter)
      start = currTime;

    // f) Break
    if (breakloop)
      break;

    // e) Update loop variable
    lastindex = currindex;

  } // For each log value

  // To fill the blanks at the end of log to make last entry of splitter is stop
  // time
  // To make it non-empty
  if (vecSplitTime.size() == 0) {
    start = m_dblLog->nthTime(istart);
    stop = m_dblLog->nthTime(iend);
    lastindex = -1;
    makeSplitterInVector(vecSplitTime, vecSplitGroup, start, stop, lastindex,
                         tol_ns, laststoptime);
  }

  return;
}

//-----------------------------------------------------------------------------------------------
/** Generate filters for an integer log
  * @param minvalue :: minimum allowed log value
  * @param maxvalue :: maximum allowed log value
  * @param filterIncrease :: include log value increasing period;
  * @param filterDecrease :: include log value decreasing period
  * @param runend :: end of run date and time
  */
void GenerateEventsFilter::processIntegerValueFilter(int minvalue, int maxvalue,
                                                     bool filterIncrease,
                                                     bool filterDecrease,
                                                     DateAndTime runend) {
  // Determine the filter mode and delta
  int delta = 0;
  bool singlevaluemode;
  if (minvalue == maxvalue) {
    singlevaluemode = true;
  } else {
    singlevaluemode = false;

    double deltadbl = getProperty("LogValueInterval");
    if (isEmpty(deltadbl))
      delta = maxvalue - minvalue + 1;
    else
      delta = static_cast<int>(deltadbl + 0.5);

    if (delta <= 0) {
      stringstream errss;
      errss << "In a non-single value mode, LogValueInterval cannot be 0 or "
               "negative for integer.  "
            << "Current input is " << deltadbl << ".";
      throw runtime_error(errss.str());
    } else
      g_log.information() << "Generate event-filter by integer log: step = "
                          << delta << "\n";
  }

  // Search along log to generate splitters
  size_t numlogentries = m_intLog->size();
  vector<DateAndTime> vecTimes = m_intLog->timesAsVector();
  vector<int> vecValue = m_intLog->valuesAsVector();

  time_duration timetol = DateAndTime::durationFromSeconds(
      m_logTimeTolerance * m_timeUnitConvertFactorToNS * 1.0E-9);
  int64_t timetolns = timetol.total_nanoseconds();

  DateAndTime splitstarttime(0);
  int pregroup = -1;
  DateAndTime laststoptime(0);

  g_log.debug() << "Number of integer log entries = " << numlogentries << ".\n";

  for (size_t i = 0; i < numlogentries; ++i) {
    int currvalue = vecValue[i];
    int currgroup = -1;

    // Determine whether this log value is allowed and then the ws group it
    // belonged to.
    if (currvalue >= minvalue && currvalue <= maxvalue) {
      // Log value is in specified range
      if ((i == 0) ||
          (i >= 1 && ((filterIncrease && vecValue[i] >= vecValue[i - 1]) ||
                      (filterDecrease && vecValue[i] <= vecValue[i - 1])))) {
        // First entry (regardless direction) and other entries considering
        // change of value
        if (singlevaluemode) {
          currgroup = 0;
        } else {
          currgroup = (currvalue - minvalue) / delta;
        }
      }
    }

    // Make a new splitter if condition is met
    bool statuschanged;
    if (pregroup >= 0 && currgroup < 0) {
      // previous log is in allowed region.  but this one is not.  create a
      // splitter
      if (splitstarttime.totalNanoseconds() == 0)
        throw runtime_error("Programming logic error.");

      makeSplitterInVector(m_vecSplitterTime, m_vecSplitterGroup,
                           splitstarttime, vecTimes[i], pregroup, timetolns,
                           laststoptime);
      laststoptime = vecTimes[i];

      splitstarttime = DateAndTime(0);
      statuschanged = true;
    } else if (pregroup < 0 && currgroup >= 0) {
      // previous log is not allowed, but this one is.  this is the start of a
      // new splitter
      splitstarttime = vecTimes[i];
      statuschanged = true;
    } else if (currgroup >= 0 && pregroup != currgroup) {
      // migrated to a new region
      if (splitstarttime.totalNanoseconds() == 0)
        throw runtime_error("Programming logic error (1).");
      makeSplitterInVector(m_vecSplitterTime, m_vecSplitterGroup,
                           splitstarttime, vecTimes[i], pregroup, timetolns,
                           laststoptime);
      laststoptime = vecTimes[i];

      splitstarttime = vecTimes[i];
      statuschanged = true;
    } else {
      // no need to do anything: status is not changed
      statuschanged = false;
    }

    // Update group/pregroup
    if (statuschanged)
      pregroup = currgroup;
  } // ENDOFLOOP on time series

  // Create the last splitter if existing
  if (pregroup >= 0) {
    // Last entry is in an allowed region.
    if (splitstarttime.totalNanoseconds() == 0)
      throw runtime_error("Programming logic error (1).");
    makeSplitterInVector(m_vecSplitterTime, m_vecSplitterGroup, splitstarttime,
                         runend, pregroup, timetolns, laststoptime);
  }

  // Write to the information workspace
  // FIXME - Consider to refactor this part with all other methods
  if (singlevaluemode) {
    TableRow newrow = m_filterInfoWS->appendRow();
    stringstream message;
    message << m_intLog->name() << " = " << minvalue;
    newrow << 0 << message.str();
  } else {
    int logvalue = minvalue;
    int wsindex = 0;
    while (logvalue <= maxvalue) {
      stringstream message;
      if (logvalue + delta - 1 > logvalue)
        message << m_intLog->name() << " = [" << logvalue << ", "
                << logvalue + delta - 1 << "]";
      else
        message << m_intLog->name() << " = " << logvalue;

      message << ". Value change direction: ";
      if (filterIncrease && filterDecrease)
        message << "Both.";
      else if (filterIncrease)
        message << "Increasing. ";
      else if (filterDecrease)
        message << "Decreasing. ";

      TableRow newrow = m_filterInfoWS->appendRow();
      newrow << wsindex << message.str();

      ++wsindex;
      logvalue += delta;
    }
  }

  g_log.information() << "Integer log " << m_intLog->name()
                      << ": Number of splitters = " << m_vecSplitterGroup.size()
                      << ", Number of split info = "
                      << m_filterInfoWS->rowCount() << ".\n";

  return;
}

//----------------------------------------------------------------------------------------------
/** Do a binary search in the following list
 * Warning: if the vector is not sorted, the error will happen.
 * This algorithm won't guarantee for it
 *
 * @param sorteddata :: Sorted data.
 * @param value :: Value to look up.
 *
 * return:  if value is out of range, then return datarange.size() + 1
 */
size_t GenerateEventsFilter::searchValue(const std::vector<double> &sorteddata,
                                         double value) {
  // Case of out-of-boundary
  size_t numdata = sorteddata.size();
  size_t outrange = numdata + 1;
  if (numdata == 0)
    return outrange;
  else if (value < sorteddata.front() || value > sorteddata.back())
    return outrange;

  // Binary search
  size_t index = static_cast<size_t>(
      std::lower_bound(sorteddata.begin(), sorteddata.end(), value) -
      sorteddata.begin());

  if (value < sorteddata[index] && index % 2 == 1) {
    // value to search is less than the boundary: use the index of the one just
    // smaller to the value to search
    --index;
  } else if (value == sorteddata[index] && index % 2 == 1) {
    // value to search is on the boudary, i..e, a,b,b,c,c,....,x,x,y
    ++index;

    // return if out of range
    if (index == sorteddata.size())
      return outrange;
  }

  return index;
}

//----------------------------------------------------------------------------------------------
/** Determine starting value changing direction
  */
int GenerateEventsFilter::determineChangingDirection(int startindex) {
  int direction = 0;

  // Search to earlier entries
  int index = startindex;
  while (direction == 0 && index > 0) {
    double diff = m_dblLog->nthValue(index) - m_dblLog->nthValue(index - 1);
    if (diff > 0)
      direction = 1;
    else if (diff < 0)
      direction = -1;

    --index;
  }

  if (direction != 0)
    return direction;

  // Search to later entries
  index = startindex;
  int maxindex = m_dblLog->size() - 1;
  while (direction == 0 && index < maxindex) {
    double diff = m_dblLog->nthValue(index + 1) - m_dblLog->nthValue(index);
    if (diff > 0)
      direction = 1;
    else if (diff < 0)
      direction = -1;

    ++index;
  }

  if (direction == 0)
    throw runtime_error("Sample log is flat.  Use option 'Both' instead! ");

  return direction;
}

//----------------------------------------------------------------------------------------------
/** Add a new splitter to vector of splitters.  It is used by FilterByTime only.
  */
void
GenerateEventsFilter::addNewTimeFilterSplitter(Kernel::DateAndTime starttime,
                                               Kernel::DateAndTime stoptime,
                                               int wsindex, string info) {
  if (m_forFastLog) {
    // For MatrixWorkspace splitter
    // Start of splitter
    if (m_vecSplitterTime.size() == 0) {
      // First splitter
      m_vecSplitterTime.push_back(starttime);
    } else if (m_vecSplitterTime.back() < starttime) {
      // Splitter to insert has a gap to previous splitter
      m_vecSplitterTime.push_back(starttime);
      m_vecSplitterGroup.push_back(-1);

    } else if (m_vecSplitterTime.back() == starttime) {
      // Splitter to insert is just behind previous one (no gap): nothing
    } else {
      // Impossible situation
      throw runtime_error("Logic error. Trying to insert a splitter, whose "
                          "start time is earlier than last splitter.");
    }
    // Stop of splitter
    m_vecSplitterTime.push_back(stoptime);
    // Group
    m_vecSplitterGroup.push_back(wsindex);
  } else {
    // For regular Splitter
    Kernel::SplittingInterval spiv(starttime, stoptime, wsindex);
    m_splitWS->addSplitter(spiv);
  }

  // Information
  if (info.size() > 0) {
    API::TableRow row = m_filterInfoWS->appendRow();
    row << wsindex << info;
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Create a splitter and add to the vector of time splitters
  * This method will be called intensively.
  */
DateAndTime GenerateEventsFilter::makeSplitterInVector(
    std::vector<Kernel::DateAndTime> &vecSplitTime,
    std::vector<int> &vecGroupIndex, Kernel::DateAndTime start,
    Kernel::DateAndTime stop, int group, int64_t tol_ns, DateAndTime lasttime) {
  DateAndTime starttime(start.totalNanoseconds() - tol_ns);
  DateAndTime stoptime(stop.totalNanoseconds() - tol_ns);
  // DateAndTime starttime = start-tolerance;
  // DateAndTime stoptime = stop-tolerance;

  size_t timevecsize = vecSplitTime.size();
  if (timevecsize > 0)
    lasttime = vecSplitTime.back();

  // Start time of splitter
  if (timevecsize == 0) {
    // First value
    vecSplitTime.push_back(starttime);
  } else if (lasttime < starttime) {
    // Stop time of previous splitter is earlier than start time of this
    // splitter (gap)
    vecSplitTime.push_back(starttime);
    vecGroupIndex.push_back(-1);
  } else if (lasttime > starttime) {
    // Impossible situation
    throw runtime_error("Impossible situation.");
  }

  // The last situation is "Stop time of previous splitter is the start time of
  // this splitter".
  // No action is required to take

  // Complete this splitter, i.e., stoptime and group
  // Stop time of splitter
  vecSplitTime.push_back(stoptime);
  // Group index
  vecGroupIndex.push_back(group);

  return stoptime;
}

//----------------------------------------------------------------------------------------------
/** Create a matrix workspace for filter from vector of splitter time and group
  */
void GenerateEventsFilter::generateSplittersInMatrixWorkspace() {
  g_log.information() << "Size of splitter vector: " << m_vecSplitterTime.size()
                      << ", " << m_vecSplitterGroup.size() << "\n";

  size_t sizex = m_vecSplitterTime.size();
  size_t sizey = m_vecSplitterGroup.size();

  if (sizex - sizey != 1) {
    throw runtime_error("Logic error on splitter vectors' size. ");
  }

  m_filterWS =
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, sizex, sizey);
  MantidVec &dataX = m_filterWS->dataX(0);
  for (size_t i = 0; i < sizex; ++i) {
    dataX[i] = static_cast<double>(m_vecSplitterTime[i].totalNanoseconds());
  }

  MantidVec &dataY = m_filterWS->dataY(0);
  for (size_t i = 0; i < sizey; ++i) {
    dataY[i] = static_cast<double>(m_vecSplitterGroup[i]);
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Generate matrix workspace for splitters from vector of splitter time and
 * group
  * in the parallel-version
  */
void GenerateEventsFilter::generateSplittersInMatrixWorkspaceParallel() {
  // Determine size of output matrix workspace
  size_t numtimes = 0;
  size_t numThreads = vecSplitterTimeSet.size();
  for (size_t i = 0; i < numThreads; ++i) {
    numtimes += vecGroupIndexSet[i].size();
    g_log.debug() << "[DB] Thread " << i << " have "
                  << vecGroupIndexSet[i].size() << " splitter "
                  << "\n";
  }
  ++numtimes;

  size_t sizex = numtimes;
  size_t sizey = numtimes - 1;

  m_filterWS =
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, sizex, sizey);
  MantidVec &dataX = m_filterWS->dataX(0);
  MantidVec &dataY = m_filterWS->dataY(0);

  size_t index = 0;
  for (size_t i = 0; i < numThreads; ++i) {
    for (size_t j = 0; j < vecGroupIndexSet[i].size(); ++j) {
      dataX[index] =
          static_cast<double>(vecSplitterTimeSet[i][j].totalNanoseconds());
      dataY[index] = static_cast<double>(vecGroupIndexSet[i][j]);
      ++index;
    }
  }
  dataX[index] =
      static_cast<double>(vecSplitterTimeSet.back().back().totalNanoseconds());

  return;
}

//----------------------------------------------------------------------------------------------
/** Convert splitters vector to splitters and add to SplittersWorskpace
  */
void GenerateEventsFilter::generateSplittersInSplitterWS() {
  for (size_t i = 0; i < m_vecSplitterGroup.size(); ++i) {
    int groupindex = m_vecSplitterGroup[i];
    if (groupindex >= 0) {
      DateAndTime start = m_vecSplitterTime[i];
      DateAndTime stop = m_vecSplitterTime[i + 1];
      Kernel::SplittingInterval newsplit(start, stop, groupindex);
      m_splitWS->addSplitter(newsplit);
    }
  }

  return;
}

//----------------------------------------------------------------------------------------------
/** Find run end time.  Here is how the run end time is defined ranked from
 * highest priority
  * 1. Run.endTime()
  * 2. Last proton charge log time
  * 3. Last event time
  * In order to consider the events in the last pulse,
  * 1. if proton charge does exist, then run end time will be extended by 1
 * pulse time
  * 2. otherwise, extended by 0.1 second (10 Hz)
  * Exception: None of the 3 conditions is found to determine run end time
  */
DateAndTime GenerateEventsFilter::findRunEnd() {
  // Try to get the run end from Run object
  DateAndTime runendtime(0);
  bool norunendset = false;
  try {
    runendtime = m_dataWS->run().endTime();
  } catch (std::runtime_error err) {
    norunendset = true;
  }

  g_log.debug() << "Check point 1 "
                << "Run end time = " << runendtime << "/"
                << runendtime.totalNanoseconds()
                << ", no run end set = " << norunendset << "\n";

  int64_t extended_ns = static_cast<int64_t>(1.0E8);
  if (m_dataWS->run().hasProperty("proton_charge")) {
    // Get last proton charge time and compare with run end time
    Kernel::TimeSeriesProperty<double> *protonchargelog =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
            m_dataWS->run().getProperty("proton_charge"));

    if (protonchargelog->size() > 1) {
      Kernel::DateAndTime tmpendtime = protonchargelog->lastTime();
      extended_ns = protonchargelog->nthTime(1).totalNanoseconds() -
                    protonchargelog->nthTime(0).totalNanoseconds();
      if (tmpendtime > runendtime) {
        // Use the last proton charge time
        runendtime = tmpendtime;
        g_log.debug() << "Check point 1B: "
                      << "Use last proton charge time = "
                      << tmpendtime.totalNanoseconds() << " as run end. "
                      << "\n";
      }
      norunendset = false;
    }

    g_log.debug() << "Check point 2A "
                  << " run end time = " << runendtime << "\n";
  } else if (norunendset && m_dataWS->getNumberEvents() > 0) {
    // No proton_charge or run_end: sort events and find the last event
    norunendset = false;

    for (size_t i = 0; i < m_dataWS->getNumberHistograms(); ++i) {
      const DataObjects::EventList &evlist = m_dataWS->getEventList(i);
      if (evlist.getNumberEvents() > 0) {
        // If event list is empty, the returned value may not make any sense
        DateAndTime lastpulse = evlist.getPulseTimeMax();
        if (lastpulse > runendtime)
          runendtime = lastpulse;
      }
    }
    g_log.debug() << "Check point 2B "
                  << " from last event: run end time = " << runendtime << " / "
                  << runendtime.totalNanoseconds() << "\n";
  }

  // Check whether run end time is set
  if (norunendset)
    throw runtime_error("Run end time cannot be determined. ");

  // Add 1 second to make sure that no event left behind either last pulse time
  // or last event time
  runendtime = DateAndTime(runendtime.totalNanoseconds() + extended_ns);

  return runendtime;
}

} // namespace Mantid
} // namespace Algorithms
