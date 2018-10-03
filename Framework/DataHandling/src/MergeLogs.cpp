// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidDataHandling/MergeLogs.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

DECLARE_ALGORITHM(Merge2WorkspaceLogs)

void Merge2WorkspaceLogs::init() {

  declareProperty(make_unique<API::WorkspaceProperty<API::MatrixWorkspace>>(
                      "Workspace", "Anonymous", Direction::InOut),
                  "Workspace to have logs merged");
  declareProperty("LogName1", "", "The name of the first log to be merged.");
  declareProperty("LogName2", "", "The name of the second log to be merged.");
  declareProperty("MergedLogName", "",
                  "The name of the new log as the result "
                  "of log 1 being merged with log 2.");
  declareProperty("ResetLogValue", false,
                  "Reset both logs' values to unity for each one.");
  declareProperty("LogValue1", 0.0, "Unity value of log 1.");
  declareProperty("LogValue2", 1.0, "Unity value of log 2.");
}

void Merge2WorkspaceLogs::exec() {

  // 1. Get value
  matrixWS = this->getProperty("Workspace");
  std::string logname1 = this->getProperty("LogName1");
  std::string logname2 = this->getProperty("LogName2");
  std::string mlogname = this->getProperty("MergedLogName");
  bool resetlogvalue = this->getProperty("ResetLogValue");
  double logvalue1 = this->getProperty("LogValue1");
  double logvalue2 = this->getProperty("LogValue2");

  // 2. Check
  if (logname1.empty() || logname2.empty() || mlogname.empty()) {
    g_log.error() << "One or more than one log name is not given!\n";
    throw std::invalid_argument("One or more than one log name is not give");
  }

  if (resetlogvalue && fabs(logvalue1 - logvalue2) < 1.0E-9) {
    g_log.warning()
        << "User re-defined log values of two logs are very close!\n";
  }

  // 3. Merge log
  this->mergeLogs(logname1, logname2, mlogname, resetlogvalue, logvalue1,
                  logvalue2);
}

/*
 * Merge 2 TimeSeries log together for the third one
 * @param ilogname1:  name of log 1 to be merged
 * @param ilogname2:  name of log 2 to be merged
 * @param ologname:   name of the merged log to be added to workspace
 */
void Merge2WorkspaceLogs::mergeLogs(std::string ilogname1,
                                    std::string ilogname2, std::string ologname,
                                    bool resetlogvalue, double logvalue1,
                                    double logvalue2) {

  // 1. Get log
  Kernel::TimeSeriesProperty<double> *p1 = getTimeSeriesLog(ilogname1);
  Kernel::TimeSeriesProperty<double> *p2 = getTimeSeriesLog(ilogname2);

  std::vector<Types::Core::DateAndTime> times1 = p1->timesAsVector();
  std::vector<Types::Core::DateAndTime> times2 = p2->timesAsVector();

  auto rp = new Kernel::TimeSeriesProperty<double>(ologname);

  // 2. Merge
  size_t index1 = 0;
  size_t index2 = 0;
  bool icont = true;

  Types::Core::DateAndTime tmptime;
  double tmpvalue;
  bool launch1 = true;
  ;
  bool nocomparison = false;

  std::cout << "Merging!!\n";

  while (icont) {
    // std::cout << "index1 = " << index1 << ", index2 = " << index2 << ",
    // launch1 = " << launch1 << ", nocomparison = " << nocomparison <<
    // '\n';

    // i. Determine which log to work on
    if (!nocomparison) {
      launch1 = times1[index1] < times2[index2];
    }

    // ii. Retrieve data from source log
    if (launch1) {
      // Add log1
      tmptime = times1[index1];
      if (resetlogvalue) {
        tmpvalue = logvalue1;
      } else {
        tmpvalue = p1->getSingleValue(tmptime);
      }
    } else {
      // Add log 2
      tmptime = times2[index2];
      if (resetlogvalue) {
        tmpvalue = logvalue2;
      } else {
        tmpvalue = p2->getSingleValue(tmptime);
      }
    }

    // iii. Add log
    rp->addValue(tmptime, tmpvalue);

    // iv. Increase step
    if (launch1) {
      index1++;
    } else {
      index2++;
    }

    // v. Determine status
    if (nocomparison) {
      // no comparison case: transition to terminate while
      if (launch1 && index1 >= times1.size()) {
        icont = false;
      } else if (!launch1 && index2 >= times2.size()) {
        icont = false;
      }
    } else {
      // still in comparison: transition to no-comparison
      if (launch1 && index1 >= times1.size()) {
        nocomparison = true;
        launch1 = false;
      } else if (!launch1 && index2 >= times2.size()) {
        nocomparison = true;
        launch1 = true;
      }
    } // ENDIFELSE nocomparison
  }   // ENDWHILE

  // 3. Check and add new log
  int newlogsize = rp->size();
  if (size_t(newlogsize) != (times1.size() + times2.size())) {
    g_log.error() << "Resulted log size is not equal to the sum of two source "
                     "log sizes\n";
    throw;
  }

  matrixWS->mutableRun().addProperty(rp);
}

/*
 * Get reference to Time Sereis log
 */
Kernel::TimeSeriesProperty<double> *
Merge2WorkspaceLogs::getTimeSeriesLog(std::string logname) {

  // 1. Get property
  Kernel::Property *prop = matrixWS->run().getLogData(logname);
  if (!prop) {
    g_log.error() << "Unable to find log " << logname << " of workspace "
                  << matrixWS->getName() << '\n';
    throw;
  }

  // 2. Convert to TimeSeries

  Kernel::TimeSeriesProperty<double> *timeprop =
      dynamic_cast<Kernel::TimeSeriesProperty<double> *>(prop);
  if (!timeprop) {
    g_log.error() << "Property (log) " << logname
                  << " is not of class TimeSeriesProperty!\n";
    throw;
  }

  return timeprop;
}

} // namespace DataHandling
} // namespace Mantid
