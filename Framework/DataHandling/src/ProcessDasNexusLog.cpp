#include "MantidDataHandling/ProcessDasNexusLog.h"
#include "MantidKernel/System.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidAPI/FileProperty.h"

#include <fstream>

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(ProcessDasNexusLog)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ProcessDasNexusLog::ProcessDasNexusLog() : Algorithm(), DeprecatedAlgorithm() {}

void ProcessDasNexusLog::init() {
  this->declareProperty(
      make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
          "InputWorkspace", "", Direction::InOut),
      "The name of the [[EventWorkspace]] to filter events from.");
  this->declareProperty("LogToProcess", "",
                        boost::make_shared<MandatoryValidator<std::string>>(),
                        "The name of sample log to process.");
  this->declareProperty(
      "ProcessedLog", "", boost::make_shared<MandatoryValidator<std::string>>(),
      "The name of the new sample log processed from DAS log.");
  this->declareProperty(
      make_unique<API::FileProperty>("OutputDirectory", "",
                                     API::FileProperty::Directory),
      "The directory for some other examination files to be written to.");
  this->declareProperty(
      "NumberOfOutputs", 4000,
      "Number of log entries to be written to a file for examination.");
  this->declareProperty(
      make_unique<API::FileProperty>("OutputLogFile", "",
                                     API::FileProperty::OptionalSave),
      "The file name for the output data file. ");
}

void ProcessDasNexusLog::exec() {
  // 1. Get input
  API::MatrixWorkspace_sptr inWS = getProperty("InputWorkspace");
  std::string inlogname = getProperty("LogToProcess");
  std::string outlogname = getProperty("ProcessedLog");
  int numentriesoutput = getProperty("NumberOfOutputs");
  std::string outputfilename = getProperty("OutputLogFile");

  // 2. Check Input
  // 1. Get log
  Kernel::Property *log(nullptr);
  try {
    log = inWS->run().getProperty(inlogname);
  } catch (Exception::NotFoundError &) {
    // Will trigger non-existent log message below
  }
  if (!log) {
    g_log.error() << "Log " << inlogname << " does not exist!\n";
    throw std::invalid_argument("Non-existent log name");
  }
  Kernel::TimeSeriesProperty<double> *tslog =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(log);
  if (!tslog) {
    g_log.error() << "Log " << inlogname << " is not time series log\n";
    throw std::invalid_argument("Log type error!");
  }

  // 3. Do some check for log statistic
  checkLog(inWS, inlogname);

  // 3. Convert Das log to log for absolute time
  std::vector<Kernel::DateAndTime> abstimevec;
  std::vector<double> orderedtofs;
  convertToAbsoluteTime(inWS, inlogname, abstimevec, orderedtofs);

  // 4. Add vector to log
  addLog(inWS, abstimevec, 1.0, outlogname, tslog->timesAsVector(), orderedtofs,
         false);

  // 5. Optionally write out log to
  if (numentriesoutput > 0) {
    this->writeLogtoFile(inWS, inlogname, static_cast<size_t>(numentriesoutput),
                         outputfilename);
  }
}

/*
 * Add and check log from processed absolute time stamps
 */
void ProcessDasNexusLog::addLog(API::MatrixWorkspace_sptr ws,
                                std::vector<Kernel::DateAndTime> timevec,
                                double unifylogvalue, std::string logname,
                                std::vector<Kernel::DateAndTime> pulsetimes,
                                std::vector<double> orderedtofs, bool docheck) {
  // 1. Do some static
  g_log.notice() << "Vector size = " << timevec.size() << '\n';
  double sum1dtms = 0.0; // sum(dt^2)
  double sum2dtms = 0.0; // sum(dt^2)
  size_t numinvert = 0;
  size_t numsame = 0;
  size_t numnormal = 0;
  double maxdtms = 0;
  double mindtms = 1.0E20;
  size_t numdtabove10p = 0;
  size_t numdtbelow10p = 0;

  double sampledtms = 0.00832646 * 1.0E6;
  double dtmsA10p = sampledtms * 1.1;
  double dtmsB10p = sampledtms / 1.0;

  for (size_t i = 1; i < timevec.size(); i++) {
    int64_t dtns =
        timevec[i].totalNanoseconds() - timevec[i - 1].totalNanoseconds();
    double dtms = static_cast<double>(dtns) * 1.0E-3;

    sum1dtms += dtms;
    sum2dtms += dtms * dtms;
    if (dtns == 0)
      numsame++;
    else if (dtns < 0)
      numinvert++;
    else
      numnormal++;

    if (dtms > maxdtms)
      maxdtms = dtms;
    if (dtms < mindtms)
      mindtms = dtms;

    if (dtms > dtmsA10p)
      numdtabove10p++;
    else if (dtms < dtmsB10p)
      numdtbelow10p++;

  } // ENDFOR

  double dt = sum1dtms / static_cast<double>(timevec.size()) * 1.0E-6;
  double stddt =
      sqrt(sum2dtms / static_cast<double>(timevec.size()) * 1.0E-12 - dt * dt);

  g_log.notice() << "Normal   dt = " << numnormal << '\n';
  g_log.notice() << "Zero     dt = " << numsame << '\n';
  g_log.notice() << "Negative dt = " << numinvert << '\n';
  g_log.notice() << "Avg d(T) = " << dt << " seconds +/- " << stddt
                 << ",  Frequency = " << 1.0 / dt << '\n';
  g_log.notice() << "d(T) (unit ms) is in range [" << mindtms << ", " << maxdtms
                 << "]\n";
  g_log.notice() << "Number of d(T) 10% larger than average  = "
                 << numdtabove10p << '\n';
  g_log.notice() << "Number of d(T) 10% smaller than average = "
                 << numdtbelow10p << '\n';

  g_log.notice() << "Size of timevec, pulsestimes, orderedtofs = "
                 << timevec.size() << ", " << pulsetimes.size() << ", "
                 << orderedtofs.size() << '\n';

  if (docheck) {
    exportErrorLog(ws, timevec, pulsetimes, orderedtofs, 1 / (0.5 * 240.1));
    calDistributions(timevec, 1 / (0.5 * 240.1));
  }

  // 2. Add log
  auto newlog = new Kernel::TimeSeriesProperty<double>(logname);
  for (auto &time : timevec) {
    newlog->addValue(time, unifylogvalue);
  }
  ws->mutableRun().addProperty(newlog, true);
}

/*
 * Export time stamps looking erroreous
 */
void ProcessDasNexusLog::exportErrorLog(
    API::MatrixWorkspace_sptr ws, std::vector<Kernel::DateAndTime> abstimevec,
    std::vector<Kernel::DateAndTime> pulsetimes,
    std::vector<double> orderedtofs, double dts) {
  std::string outputdir = getProperty("OutputDirectory");
  if (outputdir.back() != '/')
    outputdir += "/";

  std::string ofilename = outputdir + "errordeltatime.txt";
  g_log.notice() << ofilename << '\n';
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
          static_cast<double>(pulsetimes[i - 1].totalNanoseconds() -
                              t0.totalNanoseconds()) *
          1.0E-9;
      double deltapulsetimeSec2 =
          static_cast<double>(pulsetimes[i].totalNanoseconds() -
                              t0.totalNanoseconds()) *
          1.0E-9;
      int index1 = static_cast<int>(deltapulsetimeSec1 * 60);
      int index2 = static_cast<int>(deltapulsetimeSec2 * 60);

      ofs << "Error d(T) = " << tempdts << "   vs   Correct d(T) = " << dts
          << '\n';
      ofs << index1 << "\t\t" << pulsetimes[i - 1].totalNanoseconds() << "\t\t"
          << orderedtofs[i - 1] << '\n';
      ofs << index2 << "\t\t" << pulsetimes[i].totalNanoseconds() << "\t\t"
          << orderedtofs[i] << '\n';
    }
  }

  ofs.close();
}

/*
 * Output distributions in order for a better understanding of the log
 * @param dts: d(T) in second
 */
void ProcessDasNexusLog::calDistributions(
    std::vector<Kernel::DateAndTime> timevec, double dts) {
  // 1. Calculate percent deviation vs. number of cases
  std::vector<double> x1, y1;
  for (int i = -99; i < 100; i++) {
    x1.push_back(static_cast<double>(i));
    y1.push_back(0);
  }

  for (size_t i = 1; i < timevec.size(); i++) {
    double tempdts = static_cast<double>(timevec[i].totalNanoseconds() -
                                         timevec[i - 1].totalNanoseconds()) *
                     1.0E-9;
    int index = static_cast<int>((tempdts - dts) / dts * 100) + 99;
    if (index < 0)
      index = 0;
    else if (index > 199)
      index = 19;
    y1[static_cast<size_t>(index)]++;
  }

  /* Skip output */
  for (size_t i = 0; i < x1.size(); i++)
    g_log.notice() << i << "\t\t" << x1[i] << "\t\t" << y1[i] << '\n';
  /**/

  // 2. Calculate space distribution on error cases
  std::vector<double> x2s;
  std::vector<size_t> y2;

  size_t numperiods = 100;
  int64_t spanns =
      timevec.back().totalNanoseconds() - timevec.front().totalNanoseconds();
  double timestepsec =
      static_cast<double>(spanns) * 1.0E-9 / static_cast<double>(numperiods);

  for (size_t i = 0; i < numperiods; i++) {
    x2s.push_back(static_cast<double>(i) * timestepsec);
    y2.push_back(0);
  }

  size_t numbaddt = 0;
  for (size_t i = 1; i < timevec.size(); i++) {
    double tempdts = static_cast<double>(timevec[i].totalNanoseconds() -
                                         timevec[i - 1].totalNanoseconds()) *
                     1.0E-9;
    double dev = (tempdts - dts) / dts;
    bool baddt = false;
    if (fabs(dev) > 0.5)
      baddt = true;

    if (baddt) {
      numbaddt++;
      int index =
          static_cast<int>(static_cast<double>(timevec[i].totalNanoseconds() -
                                               timevec[0].totalNanoseconds()) *
                           1.0E-9 / timestepsec);
      if (index < 0)
        throw std::runtime_error("Impossible to have index less than 0");
      if (index >= static_cast<int>(numperiods)) {
        g_log.error() << "Logic error X\n";
        index = static_cast<int>(numperiods) - 1;
      }
      y2[static_cast<size_t>(index)]++;
    }
  } // ENDFOR

  /* Skip
  for (size_t i = 0; i < x2s.size(); i ++)
    g_log.notice() << i << "\t\t" << x2s[i] << "\t\t" << y2[i] << '\n';
    */
  g_log.notice() << "total number of wrong dt = " << numbaddt << '\n';
}

/*
 * Check log in workspace
 */
void ProcessDasNexusLog::checkLog(API::MatrixWorkspace_sptr ws,
                                  std::string logname) {
  // 1. Get log
  Kernel::Property *log = ws->run().getProperty(logname);
  if (!log) {
    g_log.error() << "Log " << logname << " does not exist!\n";
    throw std::invalid_argument("Non-exising log name");
  }
  Kernel::TimeSeriesProperty<double> *tslog =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(log);
  if (!tslog) {
    g_log.error() << "Log " << logname << " is not time series log\n";
    throw std::invalid_argument("Log type error!");
  }

  // 2. Survey
  std::vector<Kernel::DateAndTime> times = tslog->timesAsVector();
  g_log.information() << "Entries of times = " << times.size() << '\n';
  size_t countsame = 0;
  size_t countinverse = 0;
  for (size_t i = 1; i < times.size(); i++) {
    Kernel::DateAndTime tprev = times[i - 1];
    Kernel::DateAndTime tpres = times[i];
    if (tprev == tpres)
      countsame++;
    else if (tprev > tpres)
      countinverse++;
  }

  // 3. Output
  Kernel::DateAndTime t0(ws->run().getProperty("run_start")->value());
  Kernel::time_duration dts = times.front() - t0;
  Kernel::time_duration dtf = times.back() - t0;
  size_t f = times.size() - 1;

  g_log.information() << "Number of Equal Time Stamps    = " << countsame
                      << '\n';
  g_log.information() << "Number of Inverted Time Stamps = " << countinverse
                      << '\n';
  g_log.information() << "Run Start = " << t0.totalNanoseconds() << '\n';
  g_log.information() << "First Log (Absolute Time, Relative Time): "
                      << times[0].totalNanoseconds() << ", "
                      << Kernel::DateAndTime::nanosecondsFromDuration(dts)
                      << '\n';
  g_log.information() << "Last  Log (Absolute Time, Relative Time): "
                      << times[f].totalNanoseconds() << ", "
                      << Kernel::DateAndTime::nanosecondsFromDuration(dtf)
                      << '\n';
}

/*
 * Convert DAS log to a vector of absolute time
 * @param  orderedtofs: tofs with abstimevec
 */
void ProcessDasNexusLog::convertToAbsoluteTime(
    API::MatrixWorkspace_sptr ws, std::string logname,
    std::vector<Kernel::DateAndTime> &abstimevec,
    std::vector<double> &orderedtofs) {
  // 1. Get log
  Kernel::Property *log = ws->run().getProperty(logname);
  Kernel::TimeSeriesProperty<double> *tslog =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(log);
  if (!tslog)
    throw std::runtime_error("Invalid time series log: it could not be cast "
                             "(interpreted) as a time series property");
  std::vector<Kernel::DateAndTime> times = tslog->timesAsVector();
  std::vector<double> values = tslog->valuesAsVector();

  // 2. Get converted
  size_t numsamepulses = 0;
  std::vector<double> tofs;
  Kernel::DateAndTime prevtime(0);

  for (size_t i = 0; i < times.size(); i++) {
    Kernel::DateAndTime tnow = times[i];
    if (tnow > prevtime) {
      // (a) Process previous logs
      std::sort(tofs.begin(), tofs.end());
      for (double tof : tofs) {
        Kernel::DateAndTime temptime =
            prevtime + static_cast<int64_t>(tof * 100);
        abstimevec.push_back(temptime);
        orderedtofs.push_back(tof);
      }
      // (b) Clear
      tofs.clear();
      // (c) Update time
      prevtime = tnow;
    } else {
      numsamepulses++;
    }
    // (d) Push the current value
    tofs.push_back(values[i]);
  } // ENDFOR
  // Clear the last
  if (!tofs.empty()) {
    // (a) Process previous logs: note value is in unit of 100 nano-second
    std::sort(tofs.begin(), tofs.end());
    for (double tof : tofs) {
      Kernel::DateAndTime temptime = prevtime + static_cast<int64_t>(tof * 100);
      abstimevec.push_back(temptime);
      orderedtofs.push_back(tof);
    }
  } else {
    throw std::runtime_error("Impossible for this to happen!");
  }
} // END Function

/*
 * Write a certain number of log entries (from beginning) to file
 */
void ProcessDasNexusLog::writeLogtoFile(API::MatrixWorkspace_sptr ws,
                                        std::string logname,
                                        size_t numentriesoutput,
                                        std::string outputfilename) {
  // 1. Get log
  Kernel::Property *log = ws->run().getProperty(logname);
  Kernel::TimeSeriesProperty<double> *tslog =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(log);
  if (!tslog)
    throw std::runtime_error("Invalid time series log: it could not be cast "
                             "(interpreted) as a time series property");
  std::vector<Kernel::DateAndTime> times = tslog->timesAsVector();
  std::vector<double> values = tslog->valuesAsVector();

  // 2. Write out
  std::ofstream ofs;
  ofs.open(outputfilename.c_str(), std::ios::out);
  ofs << "# Absolute Time (nanosecond)\tPulse Time (nanosecond)\tTOF (ms)\n";

  Kernel::DateAndTime prevtime(0);
  std::vector<double> tofs;

  for (size_t i = 0; i < numentriesoutput; i++) {
    Kernel::DateAndTime tnow = times[i];

    if (tnow > prevtime) {
      // (a) Process previous logs
      std::sort(tofs.begin(), tofs.end());
      for (double tof : tofs) {
        Kernel::DateAndTime temptime =
            prevtime + static_cast<int64_t>(tof * 100);
        ofs << temptime.totalNanoseconds() << "\t" << tnow.totalNanoseconds()
            << "\t" << tof * 0.1 << '\n';
      }
      // (b) Clear
      tofs.clear();
      // (c) Update time
      prevtime = tnow;
    }

    // (d) Push the current value
    tofs.push_back(values[i]);
  } // ENDFOR
  // Clear the last
  if (!tofs.empty()) {
    // (a) Process previous logs: note value is in unit of 100 nano-second
    std::sort(tofs.begin(), tofs.end());
    for (double tof : tofs) {
      Kernel::DateAndTime temptime = prevtime + static_cast<int64_t>(tof * 100);
      ofs << temptime.totalNanoseconds() << "\t" << prevtime.totalNanoseconds()
          << "\t" << tof * 0.1 << '\n';
    }
  } else {
    throw std::runtime_error("Impossible for this to happen!");
  }

  ofs.close();
} // END Function

} // namespace Mantid
} // namespace DataHandling
