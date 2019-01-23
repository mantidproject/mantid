// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_USAGESERVICE_H_
#define MANTID_KERNEL_USAGESERVICE_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/SingletonHolder.h"
#include <MantidTypes/Core/DateAndTime.h>

#include <json/value.h>

#include <Poco/ActiveMethod.h>
#include <Poco/Timer.h>

#include <mutex>
#include <queue>

namespace Mantid {
namespace Kernel {

/** UsageReporter : The Usage reporter is responsible for collating, and sending
  all usage data.
  This  centralizes all the logic covering Usage Reporting including:
    - Detecting if reporting is enabled
    - Registering the startup of Mantid
    - Sending Startup usage reports, immediately, and every 24 hours thereafter
    - Registering feature usage, and storing in a feature usage buffer
    - Sending Feature usage reports on application exit, and when the feature
  usage buffer is above a size threshold.
*/

class FeatureUsage {
public:
  /// Constructor
  FeatureUsage(const std::string &type, const std::string &name,
               const bool internal);
  bool operator<(const FeatureUsage &r) const;

  ::Json::Value asJson() const;

  std::string type;
  std::string name;
  bool internal;
};

class MANTID_KERNEL_DLL UsageServiceImpl {
public:
  /// Sets the application name that has invoked Mantid
  void setApplicationName(const std::string &name);
  /// Returns the application name that has invoked Mantid
  std::string getApplicationName() const;
  /// Sets the interval that the timer checks for tasks
  void setInterval(const uint32_t seconds = 60);
  /// Registers the Startup of Mantid
  void registerStartup();
  /// Registers the use of a feature in mantid
  void registerFeatureUsage(const std::string &type, const std::string &name,
                            const bool internal);

  /// Returns true if usage reporting is enabled
  bool isEnabled() const;
  /// Sets whether the UsageReporter is enabled
  void setEnabled(const bool enabled);
  /// clear any buffers without sending any outstanding usage reports
  void clear();
  /// flushes any buffers and sends any outstanding usage reports
  void flush();
  void shutdown();
  /// gets the uptime of this mantid instance
  Types::Core::time_duration getUpTime();
  /// Gets the start time of this mantid instance
  Types::Core::DateAndTime getStartTime() { return m_startTime; }

protected:
  /// Constructor
  UsageServiceImpl();
  /// Destructor
  virtual ~UsageServiceImpl() = default;
  /// generates the message body for a startup message
  virtual std::string generateStartupMessage();
  /// generates the message body for a feature usage message
  virtual std::string generateFeatureUsageMessage();
  /// sends a report over the internet
  virtual int sendReport(const std::string &message, const std::string &url);

private:
  friend struct Mantid::Kernel::CreateUsingNew<UsageServiceImpl>;
  /// Private, unimplemented copy constructor
  UsageServiceImpl(const UsageServiceImpl &);
  /// Private, unimplemented copy assignment operator
  UsageServiceImpl &operator=(const UsageServiceImpl &);

  /// Send startup Report
  void sendStartupReport();
  /// Send featureUsageReport
  void sendFeatureUsageReport(const bool synchronous);

  int sendStartupAsyncImpl(const std::string &message);
  int sendFeatureAsyncImpl(const std::string &message);

  /// A method to handle the timerCallbacks
  void timerCallback(Poco::Timer &);

  // generate Json header for feature calls
  ::Json::Value generateFeatureHeader();

  /// a timer
  Poco::Timer m_timer;

  /// The number of timer ticks since the last reset
  uint32_t m_timerTicks;
  /// The number of timer ticks at which to reset
  uint32_t m_timerTicksTarget;

  std::queue<FeatureUsage> m_FeatureQueue;
  size_t m_FeatureQueueSizeThreshold;
  bool m_isEnabled;
  mutable std::mutex m_mutex;
  std::string m_application;
  Types::Core::DateAndTime m_startTime;

  /// Async method for sending startup notifications
  Poco::ActiveMethod<int, std::string, UsageServiceImpl> m_startupActiveMethod;
  /// Async method for sending feature notifications
  Poco::ActiveMethod<int, std::string, UsageServiceImpl> m_featureActiveMethod;

  /// Stores the base url of the usage system
  std::string m_url;
};

EXTERN_MANTID_KERNEL template class MANTID_KERNEL_DLL
    Mantid::Kernel::SingletonHolder<UsageServiceImpl>;
using UsageService = Mantid::Kernel::SingletonHolder<UsageServiceImpl>;

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_USAGESERVICE_H_ */
