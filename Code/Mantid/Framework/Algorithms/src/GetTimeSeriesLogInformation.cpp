#include "MantidAlgorithms/GetTimeSeriesLogInformation.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FileProperty.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidAPI/IEventList.h"
#include "MantidAPI/TableRow.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/EventList.h"
#include "MantidDataObjects/Events.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <algorithm>
#include <fstream>
#include "MantidKernel/ListValidator.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;

using namespace std;

namespace Mantid {
namespace Algorithms {

DECLARE_ALGORITHM(GetTimeSeriesLogInformation)
//----------------------------------------------------------------------------------------------
/** Constructor
 */
GetTimeSeriesLogInformation::GetTimeSeriesLogInformation() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
GetTimeSeriesLogInformation::~GetTimeSeriesLogInformation() {}

/** Definition of all input arguments
 */
void GetTimeSeriesLogInformation::init() {
  declareProperty(
      new API::WorkspaceProperty<MatrixWorkspace>("InputWorkspace", "Anonymous",
                                                  Direction::InOut),
      "Input EventWorkspace.  Each spectrum corresponds to 1 pixel");

  declareProperty(new WorkspaceProperty<MatrixWorkspace>(
                      "OutputWorkspace", "Dummy", Direction::Output),
                  "Name of the workspace of log delta T distribution. ");

  auto tablewsprop = new WorkspaceProperty<TableWorkspace>(
      "InformationWorkspace", "", Direction::Output);
  declareProperty(
      tablewsprop,
      "Name of optional log statistic information output Tableworkspace.");

  declareProperty("LogName", "", "Log's name to filter events.");

  std::vector<std::string> timeoptions;
  timeoptions.push_back("Absolute Time (nano second)");
  timeoptions.push_back("Relative Time (second)");
  declareProperty(
      "TimeRangeOption", "Relative Time (second)",
      boost::make_shared<StringListValidator>(timeoptions),
      "User defined time range (T0, Tf) is of absolute time (second). ");

  declareProperty(
      "FilterStartTime", EMPTY_DBL(),
      "Earliest time of the events to be selected.  "
      "It can be absolute time (ns), relative time (second) or percentage.");
  declareProperty(
      "FilterStopTime", EMPTY_DBL(),
      "Latest time of the events to be selected.  "
      "It can be absolute time (ns), relative time (second) or percentage.");

  declareProperty(
      "TimeStepBinResolution", 0.0001,
      "Time resolution (second) for time stamp delta T disibution. ");

  declareProperty("IgnoreNegativeTimeInterval", false,
                  "If true, then the time interval with negative number will "
                  "be neglected in doing statistic.");

  return;
}

/** Main execution
 */
void GetTimeSeriesLogInformation::exec() {
  // 1. Get wrokspace, log property and etc.
  m_dataWS = this->getProperty("InputWorkspace");
  if (!m_dataWS) {
    throw runtime_error(
        "Inputworkspace cannot be parsed to a MatrixWorkspace.");
  }

  string logname = getProperty("LogName");
  if (logname.size() == 0)
    throw runtime_error("Input log value cannot be an empty string. ");

  Kernel::Property *log = m_dataWS->run().getProperty(logname);
  if (!log) {
    stringstream errmsg;
    errmsg << "Property " << logname << " does not exit in sample of workspace "
           << m_dataWS->getName() << ".";
    g_log.error(errmsg.str());
    throw std::invalid_argument(errmsg.str());
  } else {
    m_log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>(log);
    if (!m_log) {
      stringstream errmsg;
      errmsg << "Log " << logname
             << " is found, but is not a double type time series log";
      g_log.error(errmsg.str());
      throw std::invalid_argument(errmsg.str());
    }
  }

  m_timeVec = m_log->timesAsVector();
  m_valueVec = m_log->valuesAsVector();

  m_starttime = m_dataWS->run().startTime();
  m_endtime = m_dataWS->run().endTime();

  m_ignoreNegativeTime = getProperty("IgnoreNegativeTimeInterval");

  // 2. Process start time and end time
  processTimeRange();

  // 3. Check time stamps
  checkLogBasicInforamtion();

  // 4. Calculate distribution of delta T
  double resolution = getProperty("TimeStepBinResolution");
  Workspace2D_sptr timestatws = calDistributions(m_timeVec, resolution);

  // 5. Check whether the log is alternating
  checkLogValueChanging(m_timeVec, m_valueVec, 0.1);

  // 6. Export error log
  if (false) {
    double userinputdt = 1 / 240.1;
    exportErrorLog(m_dataWS, m_timeVec, userinputdt);
  }

  // -1. Finish set output properties.
  TableWorkspace_sptr infows = generateStatisticTable();
  setProperty("InformationWorkspace", infows);

  this->setProperty("OutputWorkspace",
                    boost::dynamic_pointer_cast<MatrixWorkspace>(timestatws));

  // 4. Do more staticts (examine)
  // std::string outputdir = this->getProperty("OutputDirectory");
  // examLog(logname, outputdir);

  return;
}

/** Do statistic on user proposed range and examine the log
 * inside the given time range.
 */
void GetTimeSeriesLogInformation::processTimeRange() {
  // Orignal
  m_intInfoMap.insert(make_pair("Items", m_log->size()));

  // Input time
  double t0r = this->getProperty("FilterStartTime");
  double tfr = this->getProperty("FilterStopTime");

  // Time unit option
  string timeoption = this->getProperty("TimeRangeOption");
  int timecase = 0;
  if (timeoption.compare("Absolute Time (nano second)") == 0)
    timecase = 1;
  else if (timeoption.compare("Relative Time (second)") == 0)
    timecase = 0;
  else
    timecase = -1;

  double duration = static_cast<double>(m_timeVec.back().totalNanoseconds() -
                                        m_timeVec[0].totalNanoseconds()) *
                    1.0E-9;

  // Process start time
  if (t0r == EMPTY_DBL()) {
    // Default
    mFilterT0 = m_timeVec[0];
  } else {
    switch (timecase) {
    case 0:
      // Relative time (second)
      mFilterT0 = calculateRelativeTime(t0r);
      break;
    case 1:
      // Absolute time (nano second)
      mFilterT0 = getAbsoluteTime(t0r);
      break;
    case -1:
      mFilterT0 = calculateRelativeTime(t0r * duration);
      break;
    default:
      throw runtime_error("Coding error!");
      break;
    }
  } // Filter start time

  // Process filter end time
  if (tfr == EMPTY_DBL()) {
    // Default
    mFilterTf = m_endtime;
  } else {
    switch (timecase) {
    // Set with input
    case 0:
      // Relative time (second)
      mFilterTf = calculateRelativeTime(tfr);
      break;
    case 1:
      // Absolute time (nano second)
      mFilterTf = getAbsoluteTime(tfr);
      break;
    case -1:
      mFilterTf = calculateRelativeTime(tfr * duration);
      break;
    default:
      throw runtime_error("Coding error!");
      break;
    }
  }

  // Check validity
  if (mFilterTf.totalNanoseconds() <= mFilterT0.totalNanoseconds()) {
    stringstream errmsg;
    errmsg << "User defined filter starting time @ " << mFilterT0
           << " (T = " << t0r
           << ") is later than or equal to filer ending time @ " << mFilterTf
           << " (T = " << tfr << ").";
    g_log.error(errmsg.str());
    throw std::invalid_argument(errmsg.str());
  }

  return;
}

/** Convert a value in nanosecond to DateAndTime.  The value is treated as an
 * absolute time from
  * 1990.01.01
  */
Kernel::DateAndTime
GetTimeSeriesLogInformation::getAbsoluteTime(double abstimens) {
  DateAndTime temptime(static_cast<int64_t>(abstimens));

  return temptime;
}

/** Calculate the time from a given relative time from run start
  * @param deltatime :: double as a relative time to run start time in second
  */
Kernel::DateAndTime
GetTimeSeriesLogInformation::calculateRelativeTime(double deltatime) {
  int64_t totaltime =
      m_starttime.totalNanoseconds() + static_cast<int64_t>(deltatime * 1.0E9);
  DateAndTime abstime(totaltime);

  return abstime;
}

/** Generate statistic information table workspace
  */
TableWorkspace_sptr GetTimeSeriesLogInformation::generateStatisticTable() {
  TableWorkspace_sptr tablews(new TableWorkspace());

  tablews->addColumn("str", "Name");
  tablews->addColumn("double", "Value");

  // 1. Integer part
  for (auto intmapiter = m_intInfoMap.begin(); intmapiter != m_intInfoMap.end();
       ++intmapiter) {
    string name = intmapiter->first;
    size_t value = intmapiter->second;

    TableRow newrow = tablews->appendRow();
    newrow << name << static_cast<double>(value);
  }

  // 2. Double part
  map<string, double>::iterator dblmapiter;
  for (dblmapiter = m_dblInfoMap.begin(); dblmapiter != m_dblInfoMap.end();
       ++dblmapiter) {
    string name = dblmapiter->first;
    double value = dblmapiter->second;

    TableRow newrow = tablews->appendRow();
    newrow << name << value;
  }

  return tablews;
}

/** Export time stamps looking erroreous
 *  @param dts :: standard delta T in second
 *  @param ws  ::  shared pointer to a matrix workspace, which has the log to
 *study
 *  @param abstimevec :: vector of log time
 *
 *  This algorithm should be reconsidered how to work with it.
 */
void GetTimeSeriesLogInformation::exportErrorLog(MatrixWorkspace_sptr ws,
                                                 vector<DateAndTime> abstimevec,
                                                 double dts) {
  std::string outputdir = getProperty("OutputDirectory");
  if (!outputdir.empty() && outputdir[outputdir.size() - 1] != '/')
    outputdir += "/";

  std::string ofilename = outputdir + "errordeltatime.txt";
  g_log.notice() << ofilename << std::endl;
  std::ofstream ofs;
  ofs.open(ofilename.c_str(), std::ios::out);

  size_t numbaddt = 0;
  Kernel::DateAndTime t0(ws->run().getProperty("run_start")->value());

  for (size_t i = 1; i < abstimevec.size(); i++) {
    double tempdts = static_cast<double>(abstimevec[i].totalNanoseconds() -
                                         abstimevec[i - 1].totalNanoseconds()) *
                     1.0E-9;
    double dev = (tempdts - dts) / dts;
    bool baddt = false;
    if (fabs(dev) > 0.5)
      baddt = true;

    if (baddt) {
      numbaddt++;
      double deltapulsetimeSec1 =
          static_cast<double>(abstimevec[i - 1].totalNanoseconds() -
                              t0.totalNanoseconds()) *
          1.0E-9;
      double deltapulsetimeSec2 =
          static_cast<double>(abstimevec[i].totalNanoseconds() -
                              t0.totalNanoseconds()) *
          1.0E-9;
      int index1 = static_cast<int>(deltapulsetimeSec1 * 60);
      int index2 = static_cast<int>(deltapulsetimeSec2 * 60);

      ofs << "Error d(T) = " << tempdts << "   vs   Correct d(T) = " << dts
          << std::endl;
      ofs << index1 << "\t\t" << abstimevec[i - 1].totalNanoseconds() << "\t\t"
          << index2 << "\t\t" << abstimevec[i].totalNanoseconds() << std::endl;
    }
  }

  ofs.close();
}

/** Output distributions in order for a better understanding of the log
  * Result is written to a Workspace2D
  *
  * @param timevec  :: a vector of time stamps
  * @param stepsize :: resolution of the delta time count bin
  */
Workspace2D_sptr GetTimeSeriesLogInformation::calDistributions(
    std::vector<Kernel::DateAndTime> timevec, double stepsize) {
  // 1. Get a vector of delta T (in unit of seconds)
  double dtmin = static_cast<double>(timevec.back().totalNanoseconds() -
                                     timevec[0].totalNanoseconds()) *
                 1.0E-9;
  double dtmax = 0.0;

  vector<double> vecdt(timevec.size() - 1, 0.0);
  for (size_t i = 1; i < timevec.size(); ++i) {
    vecdt[i - 1] = static_cast<double>(timevec[i].totalNanoseconds() -
                                       timevec[i - 1].totalNanoseconds()) *
                   1.0E-9;
    if (vecdt[i - 1] < dtmin)
      dtmin = vecdt[i - 1];
    else if (vecdt[i - 1] > dtmax)
      dtmax = vecdt[i - 1];
  }

  // 2. Create a vector of counts
  size_t numbins;
  if (m_ignoreNegativeTime && dtmin < 0) {
    numbins = static_cast<size_t>(ceil((dtmax) / stepsize)) + 2;
  } else {
    numbins = static_cast<size_t>(ceil((dtmax - dtmin) / stepsize)) + 2;
  }

  g_log.notice() << "Distribution has " << numbins << " bins.  Delta T = ("
                 << dtmin << ", " << dtmax << ")\n";

  Workspace2D_sptr distws = boost::dynamic_pointer_cast<Workspace2D>(
      API::WorkspaceFactory::Instance().create("Workspace2D", 1, numbins,
                                               numbins));
  MantidVec &vecDeltaT = distws->dataX(0);
  MantidVec &vecCount = distws->dataY(0);

  double countmin = dtmin;
  if (m_ignoreNegativeTime && dtmin < 0)
    countmin = 0;

  for (size_t i = 0; i < numbins; ++i)
    vecDeltaT[i] = countmin + (static_cast<double>(i) - 1) * stepsize;
  for (size_t i = 0; i < numbins; ++i)
    vecCount[i] = 0;

  // 3. Count
  for (size_t i = 0; i < vecdt.size(); ++i) {
    double dt = vecdt[i];
    int index;
    if (dt < 0 && m_ignoreNegativeTime) {
      index = 0;
    } else {
      vector<double>::iterator viter =
          lower_bound(vecDeltaT.begin(), vecDeltaT.end(), dt);
      index = static_cast<int>(viter - vecDeltaT.begin());
      if (index >= static_cast<int>(vecDeltaT.size())) {
        // Out of upper boundary
        g_log.error() << "Find index = " << index
                      << " > vecX.size = " << vecDeltaT.size() << ".\n";
      } else if (vecdt[i] < vecDeltaT[index]) {
        --index;
      }

      if (index < 0)
        throw runtime_error("How can this happen.");
    }
    vecCount[index] += 1;
  }

  return distws;
}

/** Check log in workspace including ... ...
 */
void GetTimeSeriesLogInformation::checkLogBasicInforamtion() {
  // 1. Time correctness: same time, disordered time
  size_t countsame = 0;
  size_t countinverse = 0;
  for (size_t i = 1; i < m_timeVec.size(); i++) {
    Kernel::DateAndTime tprev = m_timeVec[i - 1];
    Kernel::DateAndTime tpres = m_timeVec[i];
    if (tprev == tpres)
      countsame++;
    else if (tprev > tpres)
      countinverse++;
  }

  //   Written to summary map
  /*
  Kernel::time_duration dts = m_timeVec[0]-m_starttime;
  Kernel::time_duration dtf = m_timeVec.back() - m_timeVec[0];
  size_t f = m_timeVec.size()-1;
  */

  m_intInfoMap.insert(make_pair("Number of Time Stamps", m_timeVec.size()));
  m_intInfoMap.insert(make_pair("Number of Equal Time Stamps", countsame));
  m_intInfoMap.insert(
      make_pair("Number of Reversed Time Stamps", countinverse));

  // 2. Average and standard deviation (delta t)
  double runduration_sec = static_cast<double>(m_endtime.totalNanoseconds() -
                                               m_starttime.totalNanoseconds()) *
                           1.0E-9;
  if (runduration_sec < 0.0) {
    g_log.warning() << "It shows that the run duration is not right.  "
                    << "Run start = " << m_starttime.toFormattedString() << "; "
                    << "Run End = " << m_endtime.toFormattedString() << ".\n";
    g_log.notice() << "Log start time = " << m_timeVec[0].toFormattedString()
                   << "; "
                   << "Log end time = " << m_timeVec.back().toFormattedString()
                   << ".\n";
  }

  double sum_deltaT1 = 0.0; // sum(dt  ) in second
  double sum_deltaT2 = 0.0; // sum(dt^2) in second^2
  double max_dt = 0;
  double min_dt = 0;
  if (runduration_sec > 0)
    min_dt = runduration_sec;

  size_t numpts = m_timeVec.size();
  for (size_t i = 0; i < numpts - 1; ++i) {
    int64_t dtns =
        m_timeVec[i + 1].totalNanoseconds() - m_timeVec[i].totalNanoseconds();
    double dt = static_cast<double>(dtns) * 1.0E-9; // in second

    if (dt < 0) {
      g_log.warning() << "Reversed dT: dt = " << dt << " between "
                      << m_timeVec[i].toFormattedString() << " and "
                      << m_timeVec[i + 1].toFormattedString()
                      << " @ index = " << i << ".\n";
    }

    sum_deltaT1 += dt;
    sum_deltaT2 += dt * dt;

    if (dt > max_dt)
      max_dt = dt;
    if (dt < min_dt)
      min_dt = dt;
  }

  double avg_dt = sum_deltaT1 / static_cast<double>(numpts - 1);
  double std_dt =
      sqrt(sum_deltaT2 / static_cast<double>(numpts - 1) - avg_dt * avg_dt);

  m_dblInfoMap.insert(make_pair("Average(dT)", avg_dt));
  m_dblInfoMap.insert(make_pair("Sigma(dt)", std_dt));
  m_dblInfoMap.insert(make_pair("Min(dT)", min_dt));
  m_dblInfoMap.insert(make_pair("Max(dT)", max_dt));

  // 3. Count number of time intervals beyond 10% of deviation
  /* Temporarily disabled
  for (size_t i = 1; ; i ++)
  {
    int64_t dtns =
  m_timeVec[i].totalNanoseconds()-m_timeVec[i-1].totalNanoseconds();
    double dtms = static_cast<double>(dtns)*1.0E-3;

    if (dtms > dtmsA10p)
      numdtabove10p ++;
    else if (dtms < dtmsB10p)
      numdtbelow10p ++;

  } // ENDFOR
  */

  // 4. Output
  /* Temporily disabled
  g_log.notice() << "Run Start = " << t0.totalNanoseconds() << std::endl;
  g_log.notice() << "First Log: " << "Absolute Time = " <<
  m_timeVec[0].totalNanoseconds() << "(ns), "
                 << "Relative Time = " <<
  DateAndTime::nanosecondsFromDuration(dts) << "(ns) \n";
  g_log.notice() << "Last  Log: " << "Absolute Time = " <<
  m_timeVec[f].totalNanoseconds() << "(ns), "
                 << "Relative Time = " <<
  DateAndTime::nanosecondsFromDuration(dtf) << "(ns) \n";
  g_log.notice() << "Normal   dt = " << numnormal << std::endl;
  g_log.notice() << "Zero     dt = " << numsame << std::endl;
  g_log.notice() << "Negative dt = " << numinvert << std::endl;
  g_log.notice() << "Avg d(T) = " << dt << " seconds +/- " << stddt << ",
  Frequency = " << 1.0/dt << std::endl;
  g_log.notice() << "d(T) (unit ms) is in range [" << mindtms << ", " << maxdtms
  << "]"<< std::endl;
  g_log.notice() << "Number of d(T) 10% larger than average  = " <<
  numdtabove10p << std::endl;
  g_log.notice() << "Number of d(T) 10% smaller than average = " <<
  numdtbelow10p << std::endl;

  g_log.notice() << "Size of timevec = " << m_timeVec.size() << std::endl;
  */

  return;
}

/** Check whether log values are changing from 2 adjacent time stamps
 *  @param delta :: if adjacent log values differs less than this number, then
 * it is not considered as alternating
 *  @param timevec :: vector of DateAndTime as the all the time stamps in a time
 * series log
 *  @param values :: vector double of as the all the values in the time series
 * log to study.
 */
void GetTimeSeriesLogInformation::checkLogValueChanging(
    vector<DateAndTime> timevec, vector<double> values, double delta) {
  std::stringstream ss;
  ss << "Alternating Threashold = " << delta << std::endl;

  size_t numchange = 0;
  for (size_t i = 1; i < values.size(); i++) {
    double tempdelta = values[i] - values[i - 1];
    if (fabs(tempdelta) < delta) {
      // Value are 'same'
      numchange++;

      // An error message
      ss << "@ " << i << "\tDelta = " << tempdelta << "\t\tTime From "
         << timevec[i - 1].totalNanoseconds() << " to "
         << timevec[i].totalNanoseconds() << std::endl;
    }
  }

  m_intInfoMap.insert(
      make_pair("Number of adjacent time stamp w/o value change", numchange));
  g_log.debug() << ss.str();

  return;
}

} // namespace Mantid
} // namespace Algorithms
