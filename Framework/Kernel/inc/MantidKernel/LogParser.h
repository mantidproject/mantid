// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_LOGPARSER_H_
#define MANTID_KERNEL_LOGPARSER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"

#ifndef Q_MOC_RUN
#include <boost/shared_ptr.hpp>
#endif

#include <map>
#include <sstream>

namespace Mantid {
namespace Types {
namespace Core {
class DateAndTime;
}
} // namespace Types
namespace Kernel {

//-------------------------------------------------------------------------
// Forward declarations
//-------------------------------------------------------------------------
class Property;
template <typename T> class TimeSeriesProperty;

/**
LogParser parses the instrument log files to select records corresponding
to 'RUNNING' instrument status. It determines the values of the logged variables
at the beginning and the end of each RUNNING interval and keeps track of changes
within the interval.
*/
class MANTID_KERNEL_DLL LogParser {
public:
  /// Returns the name of the log that contains the current period number
  static const std::string currentPeriodLogName() { return "current_period"; }
  /// Returns the name of the log created that defines the status during a run
  static const std::string statusLogName() { return "running"; }
  /// Returns the name of the log that contains all of the periods
  static const std::string periodsLogName() { return "periods"; }

  /// Creates a TimeSeriesProperty of either double or string type depending on
  /// the log data
  /// Returns a pointer to the created property
  static Kernel::Property *createLogProperty(const std::string &logFName,
                                             const std::string &name);
  /// Check if the icp log commands are in the new style
  static bool isICPEventLogNewStyle(
      const std::multimap<Types::Core::DateAndTime, std::string> &logm);

public:
  /// Create given the icpevent log property
  LogParser(const Kernel::Property *log);

  /// Number of periods
  int nPeriods() const { return m_nOfPeriods; }

  /// Creates a TimeSeriesProperty<bool> showing times when a particular period
  /// was active
  Kernel::TimeSeriesProperty<bool> *createPeriodLog(int period) const;

  /// Creates a log value for the current period.
  Kernel::Property *createCurrentPeriodLog(const int &period) const;

  /// Creates a TimeSeriesProperty<int> with all data periods
  Kernel::Property *createAllPeriodsLog() const;

  /// Creates a TimeSeriesProperty<bool> with running status
  Kernel::TimeSeriesProperty<bool> *createRunningLog() const;

private:
  /// Parse the icp event log with old style commands
  void parseOldStyleCommands(
      const std::multimap<Types::Core::DateAndTime, std::string> &logm,
      Kernel::TimeSeriesProperty<int> *periods,
      Kernel::TimeSeriesProperty<bool> *status);

  /// Parse the icp event log with new style commands
  void parseNewStyleCommands(
      const std::multimap<Types::Core::DateAndTime, std::string> &logm,
      Kernel::TimeSeriesProperty<int> *periods,
      Kernel::TimeSeriesProperty<bool> *status);

  /// Available commands.
  enum class commands { NONE = 0, BEGIN, END, CHANGE_PERIOD, ABORT };

  /// Typedef for a map of string commands to an enum of strongly typed
  /// commands.
  using CommandMap = std::map<std::string, commands>;

  /// TimeSeriesProperty<int> containing data periods. Created by LogParser
  boost::shared_ptr<Kernel::Property> m_periods;

  /// TimeSeriesProperty<bool> containing running status. Created by LogParser
  boost::shared_ptr<Kernel::TimeSeriesProperty<bool>> m_status;

  /// Number of periods
  int m_nOfPeriods;

  /// Creates a map of all available old-style commands.
  CommandMap createCommandMap(bool newStyle) const;

  /// Try to parse period data.
  void tryParsePeriod(const std::string &scom,
                      const Types::Core::DateAndTime &time,
                      std::istringstream &idata,
                      Kernel::TimeSeriesProperty<int> *const periods);
};

/// Returns the mean value if the property is TimeSeriesProperty<double>
MANTID_KERNEL_DLL double timeMean(const Kernel::Property *p);

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_LOGPARSER_H_*/
