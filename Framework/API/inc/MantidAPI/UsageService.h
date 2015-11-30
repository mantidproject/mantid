#ifndef MANTID_API_USAGESERVICE_H_
#define MANTID_API_USAGESERVICE_H_

#include "MantidAPI/DllConfig.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/SingletonHolder.h"
#include "MantidKernel/MultiThreaded.h"
#include <Poco/Timer.h>
#include <json/value.h>
#include <queue>

namespace Json
{
class Value;
}

namespace Mantid {
namespace API {

class Algorithm;

/** UsageService : The Usage Service is responsible for collating, and sending
  all usage data.
  This  centralizes all the logic covering Usage Reporting including:

    - Detecting if reporting is enabled
    - Registering the startup of Mantid
    - Sending Startup usage reports, immediately, and every 24 hours thereafter
    - Registering feature usage, and storing in a feature usage buffer
    - Sending Feature usage reports on application exit, and when the feature
  usage buffer is above a size threshold.

  Copyright &copy; 2015 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
  National Laboratory & European Spallation Source

  This file is part of Mantid.

  Mantid is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 3 of the License, or
  (at your option) any later version.

  Mantid is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class FeatureUsage {
public:
  /// Constructor
  FeatureUsage(const std::string &type, const std::string &name,
               const Kernel::DateAndTime &start, const float& duration,
               const std::string &details);

  ::Json::Value asJson() const;
  std::string asString() const;

  std::string type;
  std::string name;
  Kernel::DateAndTime start;
  float duration;
  std::string details;
};

class MANTID_API_DLL UsageServiceImpl {
public:
  /// Registers the Startup of Mantid
  void registerStartup();
  /// Registers the use of a feature in mantid
  void registerFeatureUsage(const std::string &type, const std::string &name,
                            const Kernel::DateAndTime &start,
                            const float& duration, const std::string &details);
  void registerFeatureUsage(const std::string &type, const std::string &name,
                            const std::string &details = "");
  void registerFeatureUsage(const Algorithm* alg, const float& duration);

  /// Returns true if usage reporting is enabled
  bool isEnabled() const;

  /// flushes any buffers and sends any outstanding usage reports
  void flush();

private:
  friend struct Mantid::Kernel::CreateUsingNew<UsageServiceImpl>;

  /// Constructor
  UsageServiceImpl();
  /// Private, unimplemented copy constructor
  UsageServiceImpl(const UsageServiceImpl &);
  /// Private, unimplemented copy assignment operator
  UsageServiceImpl &operator=(const UsageServiceImpl &);
  /// Destructor
  ~UsageServiceImpl();

  /// Send startup Report
  void sendStartupReport();
  /// Send featureUsageReport
  void sendFeatureUsageReport();

  /// A method to handle the timerCallbacks
  void timerCallback(Poco::Timer &);
  /// Generate jsonfor calls to usage service
  ::Json::Value generateHeader();

  /// a timer
  Poco::Timer m_timer;

  /// The number of timer ticks since the last reset
  uint32_t m_timerTicks;
  /// The number of timer ticks at which to reset
  uint32_t m_timerTicksTarget;

  std::queue<FeatureUsage> m_FeatureQueue;
  size_t m_FeatureQueueSizeThreshold;
  mutable Kernel::Mutex m_mutex;

  ::Json::Value m_cachedHeader;
};

/// Forward declaration of a specialization of SingletonHolder for
/// UsageServiceImpl (needed for dllexport/dllimport) and a typedef for
/// it.
#ifdef _WIN32
// this breaks new namespace declaration rules; need to find a better fix
template class MANTID_API_DLL Mantid::Kernel::SingletonHolder<UsageServiceImpl>;
#endif /* _WIN32 */
typedef MANTID_API_DLL Mantid::Kernel::SingletonHolder<UsageServiceImpl>
    UsageService;

} // namespace API
} // namespace Mantid

#endif /* MANTID_API_USAGESERVICE_H_ */