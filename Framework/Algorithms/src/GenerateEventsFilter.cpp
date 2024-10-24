// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAlgorithms/GenerateEventsFilter.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/VisibleWhenProperty.h"

#include <boost/math/special_functions/round.hpp>
#include <utility>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using Types::Core::DateAndTime;
using Types::Core::time_duration;

using namespace std;

namespace Mantid::Algorithms {
DECLARE_ALGORITHM(GenerateEventsFilter)

/** Constructor
 */
GenerateEventsFilter::GenerateEventsFilter()
    : API::Algorithm(), m_dataWS(), m_splitWS(), m_filterWS(), m_filterInfoWS(), m_startTime(), m_stopTime(),
      m_runEndTime(), m_timeUnitConvertFactorToNS(0.), m_dblLog(nullptr), m_intLog(nullptr), m_logAtCentre(false),
      m_logTimeTolerance(0.), m_forFastLog(false), m_splitters(), m_vecSplitterTime(), m_vecSplitterGroup(),
      m_useParallel(false), m_vecSplitterTimeSet(), m_vecGroupIndexSet() {}

/** Declare input
 */
void GenerateEventsFilter::init() {
  // Input/Output Workspaces
  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>("InputWorkspace", "", Direction::Input),
      "An input Matrix workspace.");

  declareProperty(std::make_unique<API::WorkspaceProperty<API::Workspace>>("OutputWorkspace", "", Direction::Output),
                  "The name to use for the output SplittersWorkspace object, ie the filter.");

  declareProperty(
      std::make_unique<API::WorkspaceProperty<API::ITableWorkspace>>("InformationWorkspace", "", Direction::Output),
      "Optional output for the information of each splitter workspace index");

  declareProperty("FastLog", false, "Fast log will make output workspace to be a matrix workspace. ");

  // Time (general) range
  declareProperty("StartTime", "",
                  "The start time, such that all events before this time are filtered out: it could be \n"
                  "(1) relative time to run start time in unit as specified property 'UnitOfTime'\n"
                  "(2) absolute time\n"
                  "Absolute time takes a string in format as 1990-01-01T00:00:00, while the relative time takes "
                  "a string representing an integer or a floating-point number.");

  declareProperty("StopTime", "",
                  "The stop time, such that all events after this time are filtered out: it could be \n"
                  "(1) relative time to run start time in unit as specified property 'UnitOfTime'\n"
                  "(2) absolute time\n"
                  "Absolute time takes a string in format as 1990-01-01T00:00:00, while the relative time takes "
                  "a string representing an integer or a floating-point number.");

  // Split by time (only) in steps
  declareProperty(
      std::make_unique<ArrayProperty<double>>("TimeInterval"),
      "Array for lengths of time intervals for splitters: \n"
      "if the array is empty, then there will be one splitter created from StartTime and StopTime; \n"
      "if the array has one value, then if this value is positive, all splitters will have same time intervals, else "
      "the time intervals will be exponentially increasing;\n"
      "if the size of the array is larger than one, then the splitters can have various time interval values.");
  setPropertySettings("TimeInterval", std::make_unique<VisibleWhenProperty>("LogName", IS_EQUAL_TO, ""));

  std::vector<std::string> timeoptions{"Seconds", "Nanoseconds", "Percent"};
  declareProperty("UnitOfTime", "Seconds", std::make_shared<Kernel::StringListValidator>(timeoptions),
                  "This option determines the units of options 'StartTime', 'StopTime' and 'DeltaTime'. "
                  "Allowed units are 'Seconds' or 'Nanoseconds', counting from the run start time, "
                  "or a 'Percentage' of the total run time.");

  // Split by log value (only) in steps
  declareProperty(
      "LogName", "",
      "Name of the sample log to use to filter - for example, the pulse charge is recorded in 'ProtonCharge'.");

  declareProperty("MinimumLogValue", EMPTY_DBL(), "Minimum log value for which to keep events.");
  setPropertySettings("MinimumLogValue", std::make_unique<VisibleWhenProperty>("LogName", IS_NOT_EQUAL_TO, ""));

  declareProperty("MaximumLogValue", EMPTY_DBL(), "Maximum log value for which to keep events.");
  setPropertySettings("MaximumLogValue", std::make_unique<VisibleWhenProperty>("LogName", IS_NOT_EQUAL_TO, ""));

  declareProperty(
      "LogValueInterval", EMPTY_DBL(),
      "Delta of log value to be sliced into from min log value and max log value; if not given, then only value ");
  setPropertySettings("LogValueInterval", std::make_unique<VisibleWhenProperty>("LogName", IS_NOT_EQUAL_TO, ""));

  std::vector<std::string> filteroptions{"Both", "Increase", "Decrease"};
  declareProperty("FilterLogValueByChangingDirection", "Both",
                  std::make_shared<Kernel::StringListValidator>(filteroptions),
                  "d(log value)/dt can be positive and negative, they can be put to different splitters\n"
                  "there are 3 options, 'Both', 'Increase' and 'Decrease' corresponding to d(log value)/dt being any "
                  "value, positive only and negative only respectively.");
  setPropertySettings("FilterLogValueByChangingDirection",
                      std::make_unique<VisibleWhenProperty>("LogName", IS_NOT_EQUAL_TO, ""));

  declareProperty("TimeTolerance", 0.0,
                  "Tolerance, in seconds, for the event times to keep.  It is used in the case to filter by single "
                  "value. How TimeTolerance is applied is highly correlated to LogBoundary and PulseFilter.  Check the "
                  "help or algorithm documents for details.");
  setPropertySettings("TimeTolerance", std::make_unique<VisibleWhenProperty>("LogName", IS_NOT_EQUAL_TO, ""));

  vector<string> logboundoptions{"Centre", "Left", "Other"};
  auto logvalidator = std::make_shared<StringListValidator>(logboundoptions);
  declareProperty("LogBoundary", "Centre", logvalidator,
                  "How to treat log values as being measured in the centre of time. "
                  "There are three options, 'Centre', 'Left' and 'Other'. "
                  "This value must be set to Left if the sample log is recorded upon changing,"
                  "which applies to most of the sample environment devices in SNS.");
  setPropertySettings("LogBoundary", std::make_unique<VisibleWhenProperty>("LogName", IS_NOT_EQUAL_TO, ""));

  declareProperty(
      "LogValueTolerance", EMPTY_DBL(),
      "Tolerance of the log value to be included in filter, used in the case to filter by multiple values.");
  setPropertySettings("LogValueTolerance", std::make_unique<VisibleWhenProperty>("LogName", IS_NOT_EQUAL_TO, ""));

  // Output workspaces' title and name
  declareProperty("TitleOfSplitters", "", "Title of output splitters workspace and information workspace.");

  // Linear or parallel
  vector<string> processoptions{"Serial", "Parallel"};
  auto procvalidator = std::make_shared<StringListValidator>(processoptions);
  declareProperty("UseParallelProcessing", "Serial", procvalidator,
                  "Use multiple cores to generate events filter by log values: \n"
                  "Serial: Use a single core, good for slow log; \n"
                  "Parallel: Use multiple cores, appropriate for fast log. ");

  declareProperty("NumberOfThreads", EMPTY_INT(), "Number of threads forced to use in the parallel mode. ");
  declareProperty("UseReverseLogarithmic", false, "Use reverse logarithm for the time filtering.");
}

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
}

//----------------------------------------------------------------------------------------------
/** Process user properties
 */
void GenerateEventsFilter::processInOutWorkspaces() {
  // Input data workspace
  m_dataWS = this->getProperty("InputWorkspace");

  // Output splitter information workspace
  std::string title = getProperty("TitleOfSplitters");
  if (title.empty()) {
    // Using default
    title = "Splitters";
  }
  m_filterInfoWS = std::make_shared<TableWorkspace>();
  m_filterInfoWS->setTitle(title);
  m_filterInfoWS->addColumn("int", "workspacegroup");
  m_filterInfoWS->addColumn("str", "title");

  // Output Splitters workspace: MatrixWorkspace (optioned) will be generated in
  // last step
  m_forFastLog = getProperty("FastLog");
  if (!m_forFastLog) {
    m_splitWS = std::shared_ptr<DataObjects::SplittersWorkspace>(new DataObjects::SplittersWorkspace());
    m_splitWS->setTitle(title);
  }

  string algtype = getPropertyValue("UseParallelProcessing");
  if (algtype == "Serial")
    m_useParallel = false;
  else if (algtype == "Parallel")
    m_useParallel = true;
  else
    throw std::runtime_error("Impossible to have 3rd type other than Serial and Parallel. ");

  // Conflict
  if (m_useParallel && !m_forFastLog) {
    g_log.warning("Parallelization is for fast log only.  Automatically turn "
                  "FastLog on. ");
    m_forFastLog = true;
  }
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
  bool defaultstart = s_inpt0.empty();
  bool defaultstop = s_inptf.empty();

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
  if (timeunit == "Seconds") {
    // (second)
    m_timeUnitConvertFactorToNS = 1.0E9;
  } else if (timeunit == "Nanoseconds") {
    // (nano-seconds)
    m_timeUnitConvertFactorToNS = 1.0;
  } else if (timeunit == "Percent") {
    // (percent of total run time)
    int64_t runtime_ns = m_runEndTime.totalNanoseconds() - runstarttime.totalNanoseconds();
    auto runtimed_ns = static_cast<double>(runtime_ns);
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
    double inpt0 = std::stod(s_inpt0.c_str());
    if (inpt0 < 0) {
      stringstream errss;
      errss << "Input relative StartTime " << inpt0 << " cannot be negative. ";
      throw std::invalid_argument(errss.str());
    }
    int64_t t0_ns = runstarttime.totalNanoseconds() + static_cast<int64_t>(inpt0 * m_timeUnitConvertFactorToNS);
    m_startTime = Types::Core::DateAndTime(t0_ns);
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
    double inptf = std::stod(s_inptf.c_str());
    int64_t tf_ns = runstarttime.totalNanoseconds() + static_cast<int64_t>(inptf * m_timeUnitConvertFactorToNS);
    m_stopTime = Types::Core::DateAndTime(tf_ns);
  }

  // Check start/stop time
  //  if (m_startTime.totalNanoseconds() >= m_stopTime.totalNanoseconds()) {
  if (m_startTime >= m_stopTime) {
    stringstream errss;
    errss << "Input StartTime " << m_startTime.toISO8601String() << " is equal or later than "
          << "input StopTime " << m_stopTime.toFormattedString();
    throw runtime_error(errss.str());
  }

  g_log.information() << "Filter: StartTime = " << m_startTime << ", StopTime = " << m_stopTime
                      << "; Run start = " << runstarttime.toISO8601String()
                      << ", Run stop = " << m_runEndTime.toISO8601String() << "\n";
}

//----------------------------------------------------------------------------------------------
/** Set splitters by time value / interval only
 */
void GenerateEventsFilter::setFilterByTimeOnly() {
  vector<double> vec_timeintervals = this->getProperty("TimeInterval");

  bool singleslot = false;
  if (vec_timeintervals.empty()) {
    singleslot = true;
  } else {
    // Check that there is at least one non-zero time value/interval
    if (std::all_of(vec_timeintervals.begin(), vec_timeintervals.end(), [](double i) { return i == 0; }))
      throw std::invalid_argument("If TimeInterval has one or more values, at "
                                  "least one of those values must be non-zero.");
  }

  // Progress
  int64_t totaltime = m_stopTime.totalNanoseconds() - m_startTime.totalNanoseconds();

  g_log.information() << "Filter by time: start @ " << m_startTime.totalNanoseconds() << "; "
                      << "stop @ " << m_stopTime.totalNanoseconds() << "\n";

  if (singleslot) {
    int wsindex = 0;

    // Default and thus just one interval
    std::stringstream ss;
    ss << "Time.Interval.From." << m_startTime << ".To." << m_stopTime;
    addNewTimeFilterSplitter(m_startTime, m_stopTime, wsindex, ss.str());
  } else if (vec_timeintervals.size() == 1) {
    double timeinterval = vec_timeintervals[0];
    int64_t timeslot = 0;

    int64_t runStartTime = m_dataWS->run().startTime().totalNanoseconds();
    bool isLogarithmic = (timeinterval < 0);
    m_isReverseLogarithmic = this->getProperty("UseReverseLogarithmic");

    if (m_isReverseLogarithmic && !isLogarithmic) {
      g_log.warning("UseReverseLogarithmic checked but linear time interval provided. Using linear time interval.");
      m_isReverseLogarithmic = false;
    }

    if (isLogarithmic && m_startTime.totalNanoseconds() == runStartTime)
      throw runtime_error("Cannot do logarithmic time interval if the start time is the same as the start of the run.");

    auto deltatime_ns = static_cast<int64_t>(timeinterval * m_timeUnitConvertFactorToNS);
    double factor = std::fabs(timeinterval);

    int64_t startTime_ns = m_startTime.totalNanoseconds();
    int64_t endTime_ns = m_stopTime.totalNanoseconds();

    double relativeStartTime_ns = static_cast<double>(startTime_ns - runStartTime);
    double relativeEndTime_ns = static_cast<double>(endTime_ns - runStartTime);

    int64_t curtime_ns = !m_isReverseLogarithmic ? startTime_ns - runStartTime : endTime_ns - runStartTime;

    int64_t initialReverseLogStep = startTime_ns - runStartTime;

    int totalNumberOfSlices;

    // we compute the total expected number of slices
    if (isLogarithmic) {
      // if logarithmic, first an approximation of the value, then the final value depends if reverseLogarithmic is
      // used, because of the way the last bin is managed
      double logSize = std::log(relativeEndTime_ns / relativeStartTime_ns) / std::log1p(factor);
      if (!m_isReverseLogarithmic) {
        totalNumberOfSlices = static_cast<int>(std::ceil(logSize));
      } else {
        if (logSize < 1) {
          totalNumberOfSlices = 1;
        } else {
          double previousBin = std::pow(1 + factor, std::floor(logSize) - 1);

          // we check if the last bin can fit without being smaller than the previous one
          if (relativeEndTime_ns - relativeStartTime_ns * previousBin * (1 + factor) >
              relativeStartTime_ns * previousBin * factor) {
            // case where it can
            totalNumberOfSlices = static_cast<int>(std::ceil(logSize));
          } else {
            // case where it cannot and is merged with the previous one
            totalNumberOfSlices = static_cast<int>(std::floor(logSize));
          }
        }
      }
    } else {
      // nice linear case
      totalNumberOfSlices =
          static_cast<int>((relativeEndTime_ns - relativeStartTime_ns) / static_cast<double>(deltatime_ns));
    }

    int wsindex = !m_isReverseLogarithmic ? 0 : totalNumberOfSlices - 1;

    while ((!m_isReverseLogarithmic && curtime_ns + runStartTime < endTime_ns) ||
           (m_isReverseLogarithmic && curtime_ns + runStartTime > startTime_ns)) {
      // Calculate next time
      int64_t nexttime_ns; // note that this is the time since the start of the run

      if (isLogarithmic) {
        if (m_isReverseLogarithmic) {
          int64_t step = initialReverseLogStep + endTime_ns - runStartTime - curtime_ns;
          nexttime_ns = curtime_ns - static_cast<int64_t>(static_cast<double>(step) * factor);
        } else
          nexttime_ns = static_cast<int64_t>(static_cast<double>(curtime_ns) * (1 + factor));

      } else
        nexttime_ns = curtime_ns + deltatime_ns;

      if (nexttime_ns + runStartTime > m_stopTime.totalNanoseconds())
        nexttime_ns = m_stopTime.totalNanoseconds() - runStartTime;

      // in the reverseLog case, we make sure that the "last" bin cannot be smaller than the previous one.
      if (runStartTime + nexttime_ns - (curtime_ns - nexttime_ns) < m_startTime.totalNanoseconds())
        nexttime_ns = m_startTime.totalNanoseconds() - runStartTime;

      // Create splitter and information
      Types::Core::DateAndTime t0(std::min(curtime_ns, nexttime_ns) + runStartTime);
      Types::Core::DateAndTime tf(std::max(curtime_ns, nexttime_ns) + runStartTime);
      std::stringstream ss;
      ss << "Time.Interval.From." << t0 << ".to." << tf;

      addNewTimeFilterSplitter(t0, tf, wsindex, ss.str());

      // Update loop variable and progress
      curtime_ns = nexttime_ns;

      int64_t newtimeslot;
      if (!m_isReverseLogarithmic) {
        wsindex++;
        newtimeslot = (endTime_ns - (curtime_ns + runStartTime)) * 90 / totaltime;
      } else {
        wsindex--;
        newtimeslot = (curtime_ns + runStartTime - startTime_ns) * 90 / totaltime;
      }

      if (newtimeslot > timeslot) {
        // There is change and update progress
        timeslot = newtimeslot;
        double prog = 0.1 + double(timeslot) / 100.0;
        progress(prog);
      }
    } // END-WHILE

  } // END-IF-ELSE
  else {
    // Explicitly N time intervals with various interval

    // Construct a vector for time intervals in nanosecond
    size_t numtimeintervals = vec_timeintervals.size();
    std::vector<int64_t> vec_dtimens(numtimeintervals);
    for (size_t id = 0; id < numtimeintervals; ++id) {
      auto deltatime_ns = static_cast<int64_t>(vec_timeintervals[id] * m_timeUnitConvertFactorToNS);
      vec_dtimens[id] = deltatime_ns;
    }

    // Build the splitters
    int64_t timeslot = 0;

    int64_t curtime_ns = m_startTime.totalNanoseconds();
    int wsindex = 0;
    while (curtime_ns < m_stopTime.totalNanoseconds()) {
      for (size_t id = 0; id < numtimeintervals; ++id) {
        // get next time interval value
        int64_t deltatime_ns = vec_dtimens[id];
        // Calculate next.time
        int64_t nexttime_ns = curtime_ns + deltatime_ns;
        bool breaklater = false;
        if (nexttime_ns > m_stopTime.totalNanoseconds()) {
          nexttime_ns = m_stopTime.totalNanoseconds();
          breaklater = true;
        }

        // Create splitter and information
        Types::Core::DateAndTime t0(curtime_ns);
        Types::Core::DateAndTime tf(nexttime_ns);
        std::stringstream ss;
        ss << "Time.Interval.From." << t0 << ".to." << tf;

        addNewTimeFilterSplitter(t0, tf, wsindex, ss.str());

        // Update loop variable
        curtime_ns = nexttime_ns;
        ++wsindex;

        // Update progress
        int64_t newtimeslot = (curtime_ns - m_startTime.totalNanoseconds()) * 90 / totaltime;
        if (newtimeslot > timeslot) {
          // There is change and update progress
          timeslot = newtimeslot;
          double prog = 0.1 + double(timeslot) / 100.0;
          progress(prog);
        }

        if (breaklater)
          break;
      } // END-FOR
    } // END-WHILE
  }
} // namespace Algorithms

//----------------------------------------------------------------------------------------------
/** Generate filters by log values.
 * @param logname :: name of the log to filter with
 */
void GenerateEventsFilter::setFilterByLogValue(const std::string &logname) {
  // Obtain reference of sample log to filter with
  m_dblLog = dynamic_cast<TimeSeriesProperty<double> *>(m_dataWS->run().getProperty(logname));
  m_intLog = dynamic_cast<TimeSeriesProperty<int> *>(m_dataWS->run().getProperty(logname));
  if (!m_dblLog && !m_intLog) {
    stringstream errmsg;
    errmsg << "Log " << logname << " does not exist or is not TimeSeriesProperty in double or integer.";
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
  std::string filterdirection = getProperty("FilterLogValueByChangingDirection");
  bool filterIncrease;
  bool filterDecrease;
  if (filterdirection == "Both") {
    filterIncrease = true;
    filterDecrease = true;
  } else if (filterdirection == "Increase") {

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
  m_logAtCentre = bool(logboundary == "Centre");

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
      errmsg << "Fatal Error: Input minimum log value " << minvalue << " is larger than maximum log value " << maxvalue;
      throw runtime_error(errmsg.str());
    } else {
      g_log.debug() << "Filter by log value: min = " << minvalue << ", max = " << maxvalue
                    << ", process single value? = " << toProcessSingleValueFilter << ", delta value = " << deltaValue
                    << "\n";
    }

    // Filter double value log
    if (toProcessSingleValueFilter) {
      // Generate a filter for a single log value
      processSingleValueFilter(minvalue, maxvalue, filterIncrease, filterDecrease);
    } else {
      // Generate filters for a series of log value
      processMultipleValueFilters(minvalue, deltaValue, maxvalue, filterIncrease, filterDecrease);
    }
  } else {
    // Integer TimeSeriesProperty log
    // Process min/max allowed value
    int minvaluei, maxvaluei;
    if (minvalue == EMPTY_DBL()) {
      minvaluei = m_intLog->minValue();
      minvalue = static_cast<double>(minvaluei);
    } else
      minvaluei = boost::math::iround(minvalue);

    if (maxvalue == EMPTY_DBL()) {
      maxvaluei = m_intLog->maxValue();
      maxvalue = static_cast<double>(maxvaluei);
    } else
      maxvaluei = boost::math::iround(maxvalue);

    if (minvalue > maxvalue) {
      stringstream errmsg;
      errmsg << "Fatal Error: Input minimum log value " << minvalue << " is larger than maximum log value " << maxvalue;
      throw runtime_error(errmsg.str());
    } else {
      g_log.information() << "Generate event-filter for integer log: min = " << minvaluei << ", "
                          << "max = " << maxvaluei << "\n";
    }

    // Split along log
    DateAndTime runendtime = m_dataWS->run().endTime();
    processIntegerValueFilter(minvaluei, maxvaluei, filterIncrease, filterDecrease, runendtime);

  } // ENDIFELSE: Double/Integer Log

  g_log.information() << "Minimum value = " << minvalue << ", "
                      << "maximum value = " << maxvalue << ".\n";
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
void GenerateEventsFilter::processSingleValueFilter(double minvalue, double maxvalue, bool filterincrease,
                                                    bool filterdecrease) {
  // Get parameters time-tolerance and log-boundary
  double timetolerance = this->getProperty("TimeTolerance");
  auto timetolerance_ns = static_cast<int64_t>(timetolerance * m_timeUnitConvertFactorToNS);

  std::string logboundary = this->getProperty("LogBoundary");
  transform(logboundary.begin(), logboundary.end(), logboundary.begin(), ::tolower);

  // Generate filter
  // std::vector<Kernel::SplittingInterval> splitters;
  int wsindex = 0;
  makeFilterBySingleValue(minvalue, maxvalue, static_cast<double>(timetolerance_ns) * 1.0E-9, logboundary == "centre",
                          filterincrease, filterdecrease, m_startTime, m_stopTime, wsindex);

  // Create information table workspace
  if (!m_filterInfoWS)
    throw runtime_error("m_filterInfoWS has not been initialized.");

  API::TableRow row = m_filterInfoWS->appendRow();
  std::stringstream ss;
  ss << "Log." << m_dblLog->name() << ".From." << minvalue << ".To." << maxvalue << ".Value-change-direction:";
  if (filterincrease && filterdecrease) {
    ss << "both";
  } else if (filterincrease) {
    ss << "increase";
  } else {
    ss << "decrease";
  }
  row << 0 << ss.str();
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
void GenerateEventsFilter::processMultipleValueFilters(double minvalue, double valueinterval, double maxvalue,
                                                       bool filterincrease, bool filterdecrease) {
  // Read more input
  if (valueinterval <= 0)
    throw std::invalid_argument("Multiple values filter must have LogValueInterval larger than ZERO.");
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
    indexwsindexmap.emplace(index, wsindex);

    // Log interval/value boundary
    double lowbound = curvalue - valuetolerance;
    double upbound = curvalue + valueinterval - valuetolerance;
    logvalueranges.emplace_back(lowbound);
    logvalueranges.emplace_back(upbound);

    // Workgroup information
    std::stringstream ss;
    ss << "Log." << m_dblLog->name() << ".From." << lowbound << ".To." << upbound << ".Value-change-direction:";
    if (filterincrease && filterdecrease) {
      ss << "both";
    } else if (filterincrease) {
      ss << "increase";
    } else {
      ss << "decrease";
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
  for (auto &mit : indexwsindexmap) {
    dbsplitss << "Index " << mit.first << ":  WS-group = " << mit.second << ". Log value range: ["
              << logvalueranges[mit.first * 2] << ", " << logvalueranges[mit.first * 2 + 1] << ").\n";
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
    if (minlogvalue > upperboundinterval0 || maxlogvalue < lowerboundlastinterval) {
      g_log.warning() << "User specifies log interval from " << minvalue - valuetolerance << " to "
                      << maxvalue - valuetolerance << " with interval size = " << valueinterval << "; Log "
                      << m_dblLog->name() << " has range " << minlogvalue << " to " << maxlogvalue
                      << ".  Therefore some workgroup index may not have any splitter.\n";
    }
  }

  // Generate event filters by log value
  std::string logboundary = this->getProperty("LogBoundary");
  transform(logboundary.begin(), logboundary.end(), logboundary.begin(), ::tolower);

  if (m_useParallel) {
    // Make filters in parallel
    makeMultipleFiltersByValuesParallel(indexwsindexmap, logvalueranges, logboundary == "centre", filterincrease,
                                        filterdecrease, m_startTime, m_stopTime);
  } else {
    // Make filters in serial
    makeMultipleFiltersByValues(indexwsindexmap, logvalueranges, logboundary == "centre", filterincrease,
                                filterdecrease, m_startTime, m_stopTime);
  }
}

//-----------------------------------------------------------------------------------------------
/**
 * Fill a SplittingIntervalVec that will filter the events by matching
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
void GenerateEventsFilter::makeFilterBySingleValue(double min, double max, double TimeTolerance, bool centre,
                                                   bool filterIncrease, bool filterDecrease, DateAndTime startTime,
                                                   Types::Core::DateAndTime stopTime, int wsindex) {
  // Do nothing if the log is empty.
  if (m_dblLog->size() == 0) {
    g_log.warning() << "There is no entry in this property " << this->name() << "\n";
    return;
  }

  // Clear splitters in vector format
  m_vecSplitterGroup.clear();
  m_vecSplitterTime.clear();

  // Initialize control parameters
  bool lastGood = false;
  time_duration tol = DateAndTime::durationFromSeconds(TimeTolerance);
  int numgood = 0;
  DateAndTime currT, start, stop;

  size_t progslot = 0;
  for (int i = 0; i < m_dblLog->size(); i++) {
    // The new entry
    currT = m_dblLog->nthTime(i);

    // A good value?
    bool isGood = identifyLogEntry(i, currT, lastGood, min, max, startTime, stopTime, filterIncrease, filterDecrease);
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

        std::string empty("");
        addNewTimeFilterSplitter(start, stop, wsindex, empty);

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

    std::string empty("");
    addNewTimeFilterSplitter(start, stop, wsindex, empty);
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
bool GenerateEventsFilter::identifyLogEntry(const int &index, const Types::Core::DateAndTime &currT,
                                            const bool &lastgood, const double &minvalue, const double &maxvalue,
                                            const Types::Core::DateAndTime &startT,
                                            const Types::Core::DateAndTime &stopT, const bool &filterIncrease,
                                            const bool &filterDecrease) {
  double val = m_dblLog->nthValue(index);

  // Identify by time and value
  bool isgood = (val >= minvalue && val < maxvalue) && (currT >= startT && currT < stopT);

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
/** Fill a SplittingIntervalVec that will filter the events by matching
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
void GenerateEventsFilter::makeMultipleFiltersByValues(map<size_t, int> indexwsindexmap,
                                                       const vector<double> &logvalueranges, bool centre,
                                                       bool filterIncrease, bool filterDecrease, DateAndTime startTime,
                                                       DateAndTime stopTime) {
  g_log.notice("Starting method 'makeMultipleFiltersByValues'. ");

  // Return if the log is empty.
  int logsize = m_dblLog->size();
  if (logsize == 0) {
    g_log.warning() << "There is no entry in this property " << m_dblLog->name() << '\n';
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
  m_vecSplitterTimeSet.emplace_back(tempvectimes);
  m_vecGroupIndexSet.emplace_back(tempvecgroup);
  int istart = 0;
  auto iend = static_cast<int>(logsize - 1);

  makeMultipleFiltersByValuesPartialLog(istart, iend, m_vecSplitterTime, m_vecSplitterGroup, std::move(indexwsindexmap),
                                        logvalueranges, tol, filterIncrease, filterDecrease, startTime, stopTime);

  progress(1.0);
}

//-----------------------------------------------------------------------------------------------
/** Fill a SplittingIntervalVec that will filter the events by matching
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
void GenerateEventsFilter::makeMultipleFiltersByValuesParallel(const map<size_t, int> &indexwsindexmap,
                                                               const vector<double> &logvalueranges, bool centre,
                                                               bool filterIncrease, bool filterDecrease,
                                                               DateAndTime startTime, DateAndTime stopTime) {
  // Return if the log is empty.
  int logsize = m_dblLog->size();
  if (logsize == 0) {
    g_log.warning() << "There is no entry in this property " << m_dblLog->name() << '\n';
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

    vecStart.emplace_back(istart);
    vecEnd.emplace_back(iend);
  }

  {
    stringstream dbss;
    dbss << "Number of thread = " << numThreads << ", Log Size = " << logsize << "\n";
    for (size_t i = 0; i < vecStart.size(); ++i) {
      dbss << "Thread " << i << ": Log range: [" << vecStart[i] << ", " << vecEnd[i] << ") "
           << "Size = " << vecEnd[i] - vecStart[i] << "\n";
    }
    g_log.information(dbss.str());
  }

  // Create partial vectors
  m_vecSplitterTimeSet.clear();
  m_vecGroupIndexSet.clear();
  for (int i = 0; i < numThreads; ++i) {
    vector<DateAndTime> tempvectimes;
    tempvectimes.reserve(m_dblLog->size());
    vector<int> tempvecgroup;
    tempvecgroup.reserve(m_dblLog->size());
    m_vecSplitterTimeSet.emplace_back(tempvectimes);
    m_vecGroupIndexSet.emplace_back(tempvecgroup);
  }

  // Create event filters/splitters in parallel

    PRAGMA_OMP(parallel for schedule(dynamic, 1) )
    for (int i = 0; i < numThreads; ++i) {
      PARALLEL_START_INTERRUPT_REGION

      int istart = vecStart[i];
      int iend = vecEnd[i];

      makeMultipleFiltersByValuesPartialLog(istart, iend, m_vecSplitterTimeSet[i], m_vecGroupIndexSet[i],
                                            indexwsindexmap, logvalueranges, tol, filterIncrease, filterDecrease,
                                            startTime, stopTime);
      PARALLEL_END_INTERRUPT_REGION
    }
    PARALLEL_CHECK_INTERRUPT_REGION

    // Concatenate splitters on different threads together
    for (int i = 1; i < numThreads; ++i) {
      if (m_vecSplitterTimeSet[i - 1].back() == m_vecSplitterTimeSet[i].front()) {
        // T_(i).back() = T_(i+1).front()
        if (m_vecGroupIndexSet[i - 1].back() == m_vecGroupIndexSet[i].front()) {
          // G_(i).back() = G_(i+1).front(), combine these adjacent 2 splitters
          // Rule out impossible situation
          if (m_vecGroupIndexSet[i - 1].back() == -1) {
            // Throw with detailed error message
            stringstream errss;
            errss << "Previous vector of group index set (" << (i - 1) << ") is equal to -1. "
                  << "It is not likely to happen!  Size of previous vector of "
                     "group is "
                  << m_vecGroupIndexSet[i - 1].size() << ". \nSuggest to use sequential mode. ";
            throw runtime_error(errss.str());
          }

          // Pop back last element
          m_vecGroupIndexSet[i - 1].pop_back();
          DateAndTime newt0 = m_vecSplitterTimeSet[i - 1].front();
          m_vecSplitterTimeSet[i - 1].pop_back();
          DateAndTime origtime = m_vecSplitterTimeSet[i][0];
          m_vecSplitterTimeSet[i][0] = newt0;
          g_log.debug() << "Splitter at the end of thread " << i << " is extended from " << origtime << " to " << newt0
                        << "\n";
        } else {
          // 2 different and continous spliiter: ideal case without any
          // operation
          ;
        }
      } else {
        // T_(i).back() != T_(i+1).front(): need to fill the gap in time
        int lastindex = m_vecGroupIndexSet[i - 1].back();
        int firstindex = m_vecGroupIndexSet[i].front();

        if (lastindex != -1 && firstindex != -1) {
          // T_stop < T_start, I_stop != -1, I_start != 1. : Insert a minus-one
          // entry to make it complete
          m_vecGroupIndexSet[i - 1].emplace_back(-1);
          m_vecSplitterTimeSet[i - 1].emplace_back(m_vecSplitterTimeSet[i].front());
        } else if (lastindex == -1 && m_vecGroupIndexSet[i - 1].size() == 1) {
          // Empty splitter of the thread. Extend this to next
          m_vecSplitterTimeSet[i - 1].back() = m_vecSplitterTimeSet[i].front();
          g_log.debug() << "Thread = " << i << ", change ending time of " << i - 1 << " to "
                        << m_vecSplitterTimeSet[i].front() << "\n";
        } else if (firstindex == -1 && m_vecGroupIndexSet[i].size() == 1) {
          // Empty splitter of the thread. Extend last one to this
          m_vecSplitterTimeSet[i].front() = m_vecSplitterTimeSet[i - 1].back();
          g_log.debug() << "Thread = " << i << ", change starting time to " << m_vecSplitterTimeSet[i].front() << "\n";
        } else {
          throw runtime_error("It is not possible to have start or end of "
                              "filter to be minus-one index. ");
        }
      }
    }

    progress(1.0);
}

//----------------------------------------------------------------------------------------------
/** Make filters by multiple log values of partial log
 */
void GenerateEventsFilter::makeMultipleFiltersByValuesPartialLog(
    int istart, int iend, std::vector<Types::Core::DateAndTime> &vecSplitTime, std::vector<int> &vecSplitGroup,
    map<size_t, int> indexwsindexmap, const vector<double> &logvalueranges, const time_duration &tol,
    bool filterIncrease, bool filterDecrease, DateAndTime startTime, DateAndTime stopTime) {
  // Check
  int logsize = m_dblLog->size();
  if (istart < 0 || iend >= logsize)
    throw runtime_error("Input index of makeMultipleFiltersByValuesPartialLog "
                        "is out of boundary. ");

  int64_t tol_ns = tol.total_nanoseconds();

  // Define loop control parameters
  const Types::Core::DateAndTime ZeroTime(0);
  int lastindex = -1;
  int currindex;
  DateAndTime currTime, start, stop;
  // size_t progslot = 0;

  g_log.information() << "Log time coverage (index: " << istart << ", " << iend << ") from "
                      << m_dblLog->nthTime(istart) << ", " << m_dblLog->nthTime(iend) << "\n";

  DateAndTime laststoptime(0);
  int lastlogindex = m_dblLog->size() - 1;

  int prevDirection = determineChangingDirection(istart);

  for (int i = istart; i <= iend; i++) {
    // Initialize status flags and new entry
    bool breakloop = false;
    bool createsplitter = false;

    currTime = m_dblLog->nthTime(i);
    if ((i + 1 < iend) && (currTime == m_dblLog->nthTime(i + 1)))
      continue; // skip to the next value
    double currValue = m_dblLog->nthValue(i);

    // Filter out by time and direction (optional)
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

      bool valueWithinMinMax = true;
      if (index > logvalueranges.size()) {
        // Out of range
        valueWithinMinMax = false;
      }

      if (g_log.is(Logger::Priority::PRIO_DEBUG)) {
        stringstream dbss;
        dbss << "[DBx257] Examine Log Index " << i << ", Value = " << currValue << ", Data Range Index = " << index
             << "; "
             << "Group Index = " << indexwsindexmap[index / 2]
             << " (log value range vector size = " << logvalueranges.size() << "): ";
        if (index == 0)
          dbss << logvalueranges[index] << ", " << logvalueranges[index + 1];
        else if (index == logvalueranges.size())
          dbss << logvalueranges[index - 1] << ", " << logvalueranges[index];
        else if (valueWithinMinMax)
          dbss << logvalueranges[index - 1] << ", " << logvalueranges[index] << ", " << logvalueranges[index + 1];
        g_log.debug(dbss.str());
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
            errmsg << "Impossible to have currindex == lastindex == " << currindex
                   << ", while start is not init.  Log Index = " << i << "\t value = " << currValue
                   << "\t, Index = " << index << " in range " << logvalueranges[index] << ", "
                   << logvalueranges[index + 1] << "; Last value = " << lastvalue;
            throw std::runtime_error(errmsg.str());
          }
        } // [In-bound: Inside interval]
        else {
          // [Situation] Fall between interval (which is not likley happen)
          currindex = -1;
          g_log.warning() << "Not likely to happen! Current value = " << currValue
                          << " is  within value range but its index = " << index << " has no map to group index. "
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
      if (start.totalNanoseconds() > 0) {
        stop = currTime;
        createsplitter = true;
      }
    }

    // d) Create Splitter
    if (createsplitter) {
      // make_splitter(start, stop, lastindex, tol);
      makeSplitterInVector(vecSplitTime, vecSplitGroup, start, stop, lastindex, tol_ns, laststoptime);

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
  if (vecSplitTime.empty()) {
    start = m_dblLog->nthTime(istart);
    stop = m_dblLog->nthTime(iend);
    lastindex = -1;
    makeSplitterInVector(vecSplitTime, vecSplitGroup, start, stop, lastindex, tol_ns, laststoptime);
  }
}

//-----------------------------------------------------------------------------------------------
/** Generate filters for an integer log
 * @param minvalue :: minimum allowed log value
 * @param maxvalue :: maximum allowed log value
 * @param filterIncrease :: include log value increasing period;
 * @param filterDecrease :: include log value decreasing period
 * @param runend :: end of run date and time
 */
void GenerateEventsFilter::processIntegerValueFilter(int minvalue, int maxvalue, bool filterIncrease,
                                                     bool filterDecrease, DateAndTime runend) {
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
      delta = boost::math::iround(deltadbl);

    if (delta <= 0) {
      stringstream errss;
      errss << "In a non-single value mode, LogValueInterval cannot be 0 or "
               "negative for integer.  "
            << "Current input is " << deltadbl << ".";
      throw runtime_error(errss.str());
    } else
      g_log.information() << "Generate event-filter by integer log: step = " << delta << "\n";
  }

  // Search along log to generate splitters
  size_t numlogentries = m_intLog->size();
  vector<DateAndTime> vecTimes = m_intLog->timesAsVector();
  vector<int> vecValue = m_intLog->valuesAsVector();

  time_duration timetol = DateAndTime::durationFromSeconds(m_logTimeTolerance * m_timeUnitConvertFactorToNS * 1.0E-9);
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
      if ((i == 0) || (filterIncrease && vecValue[i] >= vecValue[i - 1]) ||
          (filterDecrease && vecValue[i] <= vecValue[i - 1])) {
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

      makeSplitterInVector(m_vecSplitterTime, m_vecSplitterGroup, splitstarttime, vecTimes[i], pregroup, timetolns,
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
      makeSplitterInVector(m_vecSplitterTime, m_vecSplitterGroup, splitstarttime, vecTimes[i], pregroup, timetolns,
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
    makeSplitterInVector(m_vecSplitterTime, m_vecSplitterGroup, splitstarttime, runend, pregroup, timetolns,
                         laststoptime);
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
        message << m_intLog->name() << "=[" << logvalue << "," << logvalue + delta - 1 << "]";
      else
        message << m_intLog->name() << "=" << logvalue;

      message << ".Value change direction:";
      if (filterIncrease && filterDecrease)
        message << "Both";
      else if (filterIncrease)
        message << "Increasing";
      else if (filterDecrease)
        message << "Decreasing";

      TableRow newrow = m_filterInfoWS->appendRow();
      newrow << wsindex << message.str();

      ++wsindex;
      logvalue += delta;
    }
  }

  g_log.information() << "Integer log " << m_intLog->name() << ": Number of splitters = " << m_vecSplitterGroup.size()
                      << ", Number of split info = " << m_filterInfoWS->rowCount() << ".\n";
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
size_t GenerateEventsFilter::searchValue(const std::vector<double> &sorteddata, double value) {
  // Case of out-of-boundary
  size_t numdata = sorteddata.size();
  size_t outrange = numdata + 1;
  if (numdata == 0)
    return outrange;
  else if (value < sorteddata.front() || value > sorteddata.back())
    return outrange;

  // Binary search
  size_t index =
      static_cast<size_t>(std::lower_bound(sorteddata.begin(), sorteddata.end(), value) - sorteddata.begin());

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
void GenerateEventsFilter::addNewTimeFilterSplitter(Types::Core::DateAndTime starttime,
                                                    Types::Core::DateAndTime stoptime, int wsindex,
                                                    const string &info) {
  if (m_forFastLog) {
    // For MatrixWorkspace splitter
    // Start of splitter
    if (m_vecSplitterTime.empty()) {
      // First splitter
      m_vecSplitterTime.emplace_back(starttime);
    } else if (m_vecSplitterTime.back() < starttime) {
      // Splitter to insert has a gap to previous splitter
      m_vecSplitterTime.emplace_back(starttime);
      m_vecSplitterGroup.emplace_back(-1);

    } else if (m_vecSplitterTime.back() == starttime) {
      // Splitter to insert is just behind previous one (no gap): nothing
    } else {
      // Impossible situation
      throw runtime_error("Logic error. Trying to insert a splitter, whose "
                          "start time is earlier than last splitter.");
    }
    // Stop of splitter
    m_vecSplitterTime.emplace_back(stoptime);
    // Group
    m_vecSplitterGroup.emplace_back(wsindex);
  } else {
    // For regular Splitter
    Kernel::SplittingInterval spiv(starttime, stoptime, wsindex);
    m_splitWS->addSplitter(spiv);
  }

  // Information
  if (!info.empty()) {
    if (!m_isReverseLogarithmic) {
      API::TableRow row = m_filterInfoWS->appendRow();
      row << wsindex << info;
    } else {
      m_filterInfoWS->insertRow(0);
      API::TableRow row = m_filterInfoWS->getRow(0);
      row << wsindex << info;
    }
  }
}

//----------------------------------------------------------------------------------------------
/** Create a splitter and add to the vector of time splitters
 * This method will be called intensively.
 */
DateAndTime GenerateEventsFilter::makeSplitterInVector(std::vector<Types::Core::DateAndTime> &vecSplitTime,
                                                       std::vector<int> &vecGroupIndex, Types::Core::DateAndTime start,
                                                       Types::Core::DateAndTime stop, int group, int64_t tol_ns,
                                                       DateAndTime lasttime) {
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
    vecSplitTime.emplace_back(starttime);
  } else if (lasttime < starttime) {
    // Stop time of previous splitter is earlier than start time of this
    // splitter (gap)
    vecSplitTime.emplace_back(starttime);
    vecGroupIndex.emplace_back(-1);
  } else if (lasttime > starttime) {
    // Impossible situation
    throw runtime_error("Impossible situation.");
  }

  // The last situation is "Stop time of previous splitter is the start time of
  // this splitter".
  // No action is required to take

  // Complete this splitter, i.e., stoptime and group
  // Stop time of splitter
  vecSplitTime.emplace_back(stoptime);
  // Group index
  vecGroupIndex.emplace_back(group);

  return stoptime;
}

//----------------------------------------------------------------------------------------------
/** Create a matrix workspace for filter from vector of splitter time and group
 */
void GenerateEventsFilter::generateSplittersInMatrixWorkspace() {
  g_log.information() << "Size of splitter vector: " << m_vecSplitterTime.size() << ", " << m_vecSplitterGroup.size()
                      << "\n";

  size_t sizex = m_vecSplitterTime.size();
  size_t sizey = m_vecSplitterGroup.size();

  if (sizex - sizey != 1) {
    throw runtime_error("Logic error on splitter vectors' size. ");
  }

  m_filterWS = create<Workspace2D>(1, BinEdges(sizex));
  auto &dataX = m_filterWS->mutableX(0);
  for (size_t i = 0; i < sizex; ++i) {
    // x is in the unit as second
    dataX[i] = static_cast<double>(m_vecSplitterTime[i].totalNanoseconds()) * 1.E-9;
  }

  auto &dataY = m_filterWS->mutableY(0);
  for (size_t i = 0; i < sizey; ++i) {
    dataY[i] = static_cast<double>(m_vecSplitterGroup[i]);
  }
}

//----------------------------------------------------------------------------------------------
/** Generate matrix workspace for splitters from vector of splitter time and
 * group
 * in the parallel-version
 */
void GenerateEventsFilter::generateSplittersInMatrixWorkspaceParallel() {
  // Determine size of output matrix workspace
  size_t numtimes = 0;
  size_t numThreads = m_vecSplitterTimeSet.size();
  for (size_t i = 0; i < numThreads; ++i) {
    numtimes += m_vecGroupIndexSet[i].size();
    g_log.debug() << "[DB] Thread " << i << " have " << m_vecGroupIndexSet[i].size() << " splitter "
                  << "\n";
  }
  ++numtimes;

  size_t sizex = numtimes;

  m_filterWS = create<Workspace2D>(1, BinEdges(sizex));
  auto &dataX = m_filterWS->mutableX(0);
  auto &dataY = m_filterWS->mutableY(0);

  size_t index = 0;
  for (size_t i = 0; i < numThreads; ++i) {
    for (size_t j = 0; j < m_vecGroupIndexSet[i].size(); ++j) {
      // x is in the unit as second
      dataX[index] = static_cast<double>(m_vecSplitterTimeSet[i][j].totalNanoseconds()) * 1.E-9;
      dataY[index] = static_cast<double>(m_vecGroupIndexSet[i][j]);
      ++index;
    }
  }
  // x is in the unit as second
  dataX[index] = static_cast<double>(m_vecSplitterTimeSet.back().back().totalNanoseconds()) * 1.E-9;
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
}

//----------------------------------------------------------------------------------------------
/**
 * In order to consider the events in the last pulse,
 * 1. if proton charge does exist, then run end time will be extended by 1
 * pulse time
 * 2. otherwise, extended by 0.1 second (10 Hz)
 * Exception: unable to determine run end time
 */
DateAndTime GenerateEventsFilter::findRunEnd() {
  // Try to get the run end from Run object
  DateAndTime runendtime(0);
  bool norunendset = false;
  try {
    runendtime = m_dataWS->run().endTime();
  } catch (const std::runtime_error &) {
    norunendset = true;
  }

  if (norunendset) {
    DataObjects::EventWorkspace_const_sptr eventWS = std::dynamic_pointer_cast<const EventWorkspace>(m_dataWS);
    if (!eventWS || eventWS->getNumberEvents() == 0) {
      throw std::runtime_error("Run end time cannot be determined.");
    } else {
      runendtime = eventWS->getPulseTimeMax();
    }
  }

  auto extended_ns = static_cast<int64_t>(1.0E8); // 0.1 seconds
  if (m_dataWS->run().hasProperty("proton_charge")) {
    Kernel::TimeSeriesProperty<double> *protonchargelog =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(m_dataWS->run().getProperty("proton_charge"));
    if (protonchargelog->size() > 1) {
      extended_ns = protonchargelog->nthTime(1).totalNanoseconds() - protonchargelog->nthTime(0).totalNanoseconds();
    }
  }

  // Add last pulse time or 0.1 seconds to make sure that no event left behind
  runendtime = DateAndTime(runendtime.totalNanoseconds() + extended_ns);

  return runendtime;
}

} // namespace Mantid::Algorithms
