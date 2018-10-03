// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_ISISRUNLOGS_H_
#define MANTID_DATAHANDLING_ISISRUNLOGS_H_

#include "MantidKernel/LogParser.h"
#include "MantidKernel/System.h"

#include "MantidAPI/Run.h"

#include <memory>

namespace Mantid {
namespace DataHandling {

/**

  Defines a class to aid in creating ISIS specific run logs for periods, status
  etc.
  This adds:
    - status log: "running"
    - current period log: "period x"
    - all periods: "periods"
 */
class DLLExport ISISRunLogs {
public:
  /// Construct this object using a run that has the required ICP event log
  /// and the number of periods
  ISISRunLogs(const API::Run &icpRun);
  /// Adds the status log to the this run
  void addStatusLog(API::Run &exptRun);
  /// Adds period related logs
  void addPeriodLogs(const int period, API::Run &exptRun);
  /// Add 'period i' log.
  void addPeriodLog(const int period, API::Run &exptRun);

private:
  /// A LogParser object
  std::unique_ptr<Kernel::LogParser> m_logParser;
};

} // namespace DataHandling
} // namespace Mantid

#endif /* MANTID_DATAHANDLING_ISISRUNLOGS_H_ */
