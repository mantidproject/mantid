// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/UsageService.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/ParaViewVersion.h"

#include <Poco/ActiveResult.h>

#include <json/json.h>

namespace Mantid {
namespace Kernel {

/// static logger
Kernel::Logger g_log("UsageServiceImpl");

//----------------------------------------------------------------------------------------------
/** FeatureUsage
 */
FeatureUsage::FeatureUsage(const std::string &type, const std::string &name,
                           const bool internal)
    : type(type), name(name), internal(internal) {}

// Better brute force.
bool FeatureUsage::operator<(const FeatureUsage &r) const {
  if (type < r.type)
    return true;
  if (type > r.type)
    return false;
  // Otherwise type are equal
  if (name < r.name)
    return true;
  if (name > r.name)
    return false;
  // Otherwise name are equal
  if (static_cast<int>(internal) < static_cast<int>(r.internal))
    return true;
  if (static_cast<int>(internal) > static_cast<int>(r.internal))
    return false;
  // Otherwise all are equal
  return false;
}

::Json::Value FeatureUsage::asJson() const {
  ::Json::Value retVal;

  retVal["type"] = type;
  retVal["name"] = name;
  retVal["internal"] = internal;

  return retVal;
}

//----------------------------------------------------------------------------------------------
/** Constructor for UsageServiceImpl
 */
UsageServiceImpl::UsageServiceImpl()
    : m_timer(), m_timerTicks(0), m_timerTicksTarget(0), m_FeatureQueue(),
      m_FeatureQueueSizeThreshold(50), m_isEnabled(false), m_mutex(),
      m_application("python"),
      m_startTime(Types::Core::DateAndTime::getCurrentTime()),
      m_startupActiveMethod(this, &UsageServiceImpl::sendStartupAsyncImpl),
      m_featureActiveMethod(this, &UsageServiceImpl::sendFeatureAsyncImpl) {
  setInterval(60);
  auto url = Mantid::Kernel::ConfigService::Instance().getValue<std::string>(
      "usagereports.rooturl");
  if (!url.is_initialized()) {
    g_log.debug() << "Failed to load usage report url\n";
  } else {
    m_url = url.get();
    g_log.debug() << "Root usage reporting url is " << m_url << "\n";
  };
}

void UsageServiceImpl::setApplicationName(const std::string &name) {
  m_application = name;
}

std::string UsageServiceImpl::getApplicationName() const {
  return m_application;
}

void UsageServiceImpl::setInterval(const uint32_t seconds) {
  // set the ticks target to by 24 hours / interval
  m_timerTicksTarget = 24 * 60 * 60 / seconds;

  m_timer.setStartInterval((seconds * 1000));
  m_timer.setPeriodicInterval((seconds * 1000));
}

void UsageServiceImpl::registerStartup() {
  if (isEnabled()) {
    sendStartupReport();
  }
}

/** registerFeatureUsage
 */
void UsageServiceImpl::registerFeatureUsage(const std::string &type,
                                            const std::string &name,
                                            const bool internal) {
  if (isEnabled()) {
    std::lock_guard<std::mutex> _lock(m_mutex);
    m_FeatureQueue.push(FeatureUsage(type, name, internal));
  }
}

bool UsageServiceImpl::isEnabled() const { return m_isEnabled; }

void UsageServiceImpl::setEnabled(const bool enabled) {
  if (m_isEnabled != enabled) {
    if (enabled) {
      m_timer.start(Poco::TimerCallback<UsageServiceImpl>(
          *this, &UsageServiceImpl::timerCallback));
    } else {
      m_timer.stop();
    }
  }
  m_isEnabled = enabled;
}

void UsageServiceImpl::clear() {
  std::queue<FeatureUsage> empty;
  std::swap(m_FeatureQueue, empty);
}

void UsageServiceImpl::flush() {
  if (isEnabled()) {
    sendFeatureUsageReport(true);
  }
}

/** getUpTime returns the time for which the mantid instance has been running
 @return time_duration The time for which mantid has been running.
*/
Types::Core::time_duration UsageServiceImpl::getUpTime() {
  return Types::Core::DateAndTime::getCurrentTime() - m_startTime;
}

void UsageServiceImpl::shutdown() {
  try {
    // stop the timer
    setEnabled(false);
    // send any remaining feature usage records
    sendFeatureUsageReport(true);
  } catch (std::exception &ex) {
    g_log.error() << "Error during the shutdown of the UsageService. "
                  << ex.what();
  }
}

void UsageServiceImpl::sendStartupReport() {
  try {
    std::string message = this->generateStartupMessage();
    // send the report
    Poco::ActiveResult<int> result = m_startupActiveMethod(message);
  } catch (std::exception &ex) {
    g_log.debug() << "Send startup usage failure. " << ex.what() << '\n';
  }
}

void UsageServiceImpl::sendFeatureUsageReport(const bool synchronous = false) {
  try {
    std::string message = this->generateFeatureUsageMessage();
    if (!message.empty()) {
      if (synchronous) {
        sendFeatureAsyncImpl(message);
      } else {
        Poco::ActiveResult<int> result = m_featureActiveMethod(message);
      }
    }

  } catch (std::exception &ex) {
    g_log.debug() << "sendFeatureUsageReport failure. " << ex.what() << '\n';
  }
}

void UsageServiceImpl::timerCallback(Poco::Timer & /*unused*/) {
  m_timerTicks++;
  if (m_timerTicks > m_timerTicksTarget) {
    // send startup report
    sendStartupReport();
    m_timerTicks = 0;
  }

  // Check bufferlength
  if (m_FeatureQueue.size() > m_FeatureQueueSizeThreshold) {
    sendFeatureUsageReport();
  }
}

/**
 * This puts together the system information for the json document.
 */
::Json::Value UsageServiceImpl::generateFeatureHeader() {
  ::Json::Value header;

  // mantid version and sha1
  header["mantidVersion"] = MantidVersion::versionShort();

  return header;
}

/**
 * This puts together the system information for the json document.
 */
std::string UsageServiceImpl::generateStartupMessage() {
  ::Json::Value message;

  // username
  message["uid"] = Kernel::ChecksumHelper::md5FromString(
      ConfigService::Instance().getUsername());
  // hostname
  message["host"] = Kernel::ChecksumHelper::md5FromString(
      ConfigService::Instance().getComputerName());

  // os name, version, and architecture
  message["osName"] = ConfigService::Instance().getOSName();
  message["osArch"] = ConfigService::Instance().getOSArchitecture();
  message["osVersion"] = ConfigService::Instance().getOSVersion();
  message["osReadable"] = ConfigService::Instance().getOSVersionReadable();

#if defined(MAKE_VATES)
  // paraview
  message["ParaView"] = Kernel::ParaViewVersion::targetVersion();
#else
  message["ParaView"] = 0;
#endif

  // mantid version and sha1
  message["mantidVersion"] = MantidVersion::version();
  message["mantidSha1"] = MantidVersion::revisionFull();

  // mantid version and sha1
  message["dateTime"] = m_startTime.toISO8601String();

  message["application"] = m_application;

  ::Json::FastWriter writer;
  return writer.write(message);
}

std::string UsageServiceImpl::generateFeatureUsageMessage() {

  std::map<FeatureUsage, int> featureCountMap;

  if (!m_FeatureQueue.empty()) {
    // lock around emptying of the Q so any further threads have to wait
    std::lock_guard<std::mutex> _lock(m_mutex);
    // generate a map containing the counts of identical feature usage records
    while (!m_FeatureQueue.empty()) {
      auto featureUsage = m_FeatureQueue.front();
      m_FeatureQueue.pop();
      if (featureCountMap.find(featureUsage) == featureCountMap.end()) {
        featureCountMap[featureUsage] = 1;
      } else {
        featureCountMap[featureUsage]++;
      }
    }
  }

  if (!featureCountMap.empty()) {
    ::Json::FastWriter writer;
    ::Json::Value features;
    auto message = this->generateFeatureHeader();
    for (auto const &featureItem : featureCountMap) {
      ::Json::Value thisFeature = featureItem.first.asJson();
      thisFeature["count"] = featureItem.second;
      features.append(thisFeature);
    }
    if (!features.empty()) {
      message["features"] = features;
      return writer.write(message);
    }
  }
  return "";
}

//--------------------------------------------------------------------------------------------
/**
 * Asynchronous execution
 */

/**Async method for sending startup messages
 */
int UsageServiceImpl::sendStartupAsyncImpl(const std::string &message) {
  return this->sendReport(message, m_url + "/api/usage");
}

/**Async method for sending feature messages
 */
int UsageServiceImpl::sendFeatureAsyncImpl(const std::string &message) {
  return this->sendReport(message, m_url + "/api/feature");
}

int UsageServiceImpl::sendReport(const std::string &message,
                                 const std::string &url) {
  int status = -1;
  try {
    Kernel::InternetHelper helper;
    std::stringstream responseStream;
    helper.setTimeout(20);
    helper.setBody(message);
    status = helper.sendRequest(url, responseStream);
  } catch (Mantid::Kernel::Exception::InternetError &e) {
    status = e.errorCode();
    g_log.information() << "Call to \"" << url << "\" responded with " << status
                        << "\n"
                        << e.what() << "\n";
  }

  return status;
}

} // namespace Kernel
} // namespace Mantid
