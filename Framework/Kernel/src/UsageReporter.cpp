#include "MantidKernel/UsageReporter.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ParaViewVersion.h"

#include <Poco/ActiveMethod.h>
#include <json/json.h>
#include <stdlib.h>

namespace Mantid {
namespace Kernel {

/// static logger
Kernel::Logger g_log("UsageReporter");

// const std::string STARTUP_URL("http://reports.mantidproject.org/api/usage");
const std::string STARTUP_URL(
    "http://posttestserver.com/post.php?dir=Mantid"); // dev location
                                                      // http://posttestserver.com/data/
// const std::string FEATURE_URL("http://reports.mantidproject.org/api/usage");
const std::string FEATURE_URL(
    "http://posttestserver.com/post.php?dir=Mantid"); // dev location

//----------------------------------------------------------------------------------------------
/** FeatureUsage
*/
FeatureUsage::FeatureUsage(const std::string &type, const std::string &name,
                           const Kernel::DateAndTime &start,
                           const float &duration, const std::string &details)
    : type(type), name(name), start(start), duration(duration),
      details(details) {}

::Json::Value FeatureUsage::asJson() const {
  ::Json::Value jsonMap;
  jsonMap["type"] = type;
  jsonMap["name"] = name;
  jsonMap["start"] = start.toISO8601String();
  jsonMap["duration"] = duration;
  jsonMap["details"] = details;

  return jsonMap;
}

std::string FeatureUsage::asString() const {
  ::Json::FastWriter writer;
  return writer.write(asJson());
}

//----------------------------------------------------------------------------------------------
/** Constructor for UsageReporter
 */
UsageReporter::UsageReporter()
    : m_timer(), m_timerTicks(0), m_timerTicksTarget(0), m_FeatureQueue(),
      m_FeatureQueueSizeThreshold(50), m_isEnabled(false), m_mutex(),
      m_cachedHeader() {
  setInterval(60);
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
UsageReporter::~UsageReporter() {
  try {
    try {
      // stop the timer
      m_timer.restart(0);
      // send any remaining feature usage records
      flush();
    } catch (std::exception &ex) {
      g_log.error() << "Error during the closedown of the UsageService. "
                    << ex.what();
    }
  } catch (...) { // do not allow exceptions to leave the destructor
  }
}

void UsageReporter::setInterval(const uint32_t seconds) {
  // set the ticks target to by 24 hours / interval
  m_timerTicksTarget = 24 * 60 * 60 / seconds;

  m_timer.setPeriodicInterval((seconds * 1000));
  if (isEnabled()) {
    m_timer.start(Poco::TimerCallback<UsageReporter>(
        *this, &UsageReporter::timerCallback));
  }
}

void UsageReporter::registerStartup() {
  if (isEnabled()) {
    sendStartupReport();
  }
}

//----------------------------------------------------------------------------------------------
/** registerFeatureUsage Overloads
*/
void UsageReporter::registerFeatureUsage(const std::string &type,
                                            const std::string &name,
                                            const Kernel::DateAndTime &start,
                                            const float &duration,
                                            const std::string &details) {
  m_FeatureQueue.push(FeatureUsage(type, name, start, duration, details));
}

void UsageReporter::registerFeatureUsage(const std::string &type,
                                            const std::string &name,
                                            const std::string &details) {
  registerFeatureUsage(type, name, DateAndTime::getCurrentTime(), 0.0, details);
}

//----------------------------------------------------------------------------------------------

bool UsageReporter::isEnabled() const { return m_isEnabled; }

void UsageReporter::setEnabled(const bool enabled) {
  if (m_isEnabled != enabled) {
    if (enabled) {
      m_timer.start(Poco::TimerCallback<UsageReporter>(
        *this, &UsageReporter::timerCallback));
    } else {
      m_timer.stop();
    }
  }
  m_isEnabled = enabled;
}

void UsageReporter::flush() {
  if (isEnabled()) {
    sendFeatureUsageReport(true);
  }
}

void UsageReporter::sendStartupReport() {
  try {
    std::string message = this->generateStartupMessage();

    // send the report
    // sendReport(message, STARTUP_URL);
    Poco::ActiveResult<int> result = this->sendStartupAsync(message);
  } catch (std::exception &ex) {
    g_log.debug() << "Send startup usage failure. " << ex.what() << std::endl;
  }
}

void UsageReporter::sendFeatureUsageReport(const bool synchronous = false) {
  try {
    std::string message = this->generateFeatureUsageMessage();
    //g_log.debug() << "FeatureUsage to send\n" << message << std::endl;
    if (!message.empty()) {
      if (synchronous) {
        sendFeatureAsyncImpl(message);
      } else {
        Poco::ActiveResult<int> result = this->sendFeatureAsync(message);
      }
    }

  } catch (std::exception &ex) {
    g_log.debug() << "sendFeatureUsageReport failure. " << ex.what()
                  << std::endl;
  }
}

void UsageReporter::timerCallback(Poco::Timer &) {
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
::Json::Value UsageReporter::generateHeader() {
  ::Json::Value header = m_cachedHeader;

  if (header.size() == 0) {
    // username
    header["uid"] = Kernel::ChecksumHelper::md5FromString(
        ConfigService::Instance().getUsername());
    // hostname
    header["host"] = Kernel::ChecksumHelper::md5FromString(
        ConfigService::Instance().getComputerName());

    // os name, version, and architecture
    header["osName"] = ConfigService::Instance().getOSName();
    header["osArch"] = ConfigService::Instance().getOSArchitecture();
    header["osVersion"] = ConfigService::Instance().getOSVersion();
    header["osReadable"] = ConfigService::Instance().getOSVersionReadable();

    // paraview version or zero
    if (ConfigService::Instance().pvPluginsAvailable()) {
      header["ParaView"] = Kernel::ParaViewVersion::targetVersion();
    } else {
      header["ParaView"] = 0;
    }

    // mantid version and sha1
    header["mantidVersion"] = MantidVersion::version();
    header["mantidSha1"] = MantidVersion::revisionFull();

    // cache this for future use
    m_cachedHeader = header;
  }

  // mantid version and sha1
  header["dateTime"] = DateAndTime::getCurrentTime().toISO8601String();

  return header;
}

std::string UsageReporter::generateStartupMessage() {
  auto message = this->generateHeader();

  // get the properties that were set#
  message["application"] = "mantidplot";

  // The server possibly knows about component
  // but we are not using it here.
  // message["component"] = "";

  ::Json::FastWriter writer;
  return writer.write(message);
}

std::string UsageReporter::generateFeatureUsageMessage() {

  auto message = this->generateHeader();
  ::Json::FastWriter writer;
  ::Json::Value features;
  size_t featureCount = 0;

  if (!m_FeatureQueue.empty()) {
    // lock around emptying of the Q so any further threads have to wait
    Kernel::Mutex::ScopedLock _lock(m_mutex);
    // generate json to submit
    while (!m_FeatureQueue.empty()) {
      auto featureUsage = m_FeatureQueue.front();
      features.append(featureUsage.asJson());
      m_FeatureQueue.pop();
      featureCount++;
    }
  }

  if (featureCount > 0) {
    message["features"] = features;
    return writer.write(message);
  }
  return "";
}

//--------------------------------------------------------------------------------------------
/**
* Asynchronous execution
*/
Poco::ActiveResult<int>
UsageReporter::sendStartupAsync(const std::string &message) {
  auto sendAsync = new Poco::ActiveMethod<int, std::string, UsageReporter>(
      this, &UsageReporter::sendStartupAsyncImpl);
  return (*sendAsync)(message);
}

/**Async method for sending startup messages
*/
int UsageReporter::sendStartupAsyncImpl(const std::string &message) {
  return this->sendReport(message, STARTUP_URL);
}

/**Async method for sending feature messages
*/
Poco::ActiveResult<int>
UsageReporter::sendFeatureAsync(const std::string &message) {
  auto sendAsync = new Poco::ActiveMethod<int, std::string, UsageReporter>(
      this, &UsageReporter::sendFeatureAsyncImpl);
  return (*sendAsync)(message);
}

/**Async method for sending feature messages
*/
int UsageReporter::sendFeatureAsyncImpl(const std::string &message) {
  return this->sendReport(message, FEATURE_URL);
}

int UsageReporter::sendReport(const std::string &message,
                                 const std::string &url) {
  int status = -1;
  try {
    Kernel::InternetHelper helper;
    std::stringstream responseStream;
    helper.setTimeout(2);
    helper.setBody(message);
    status = helper.sendRequest(url, responseStream);
  } catch (Mantid::Kernel::Exception::InternetError &e) {
    status = e.errorCode();
    g_log.information() << "Call to \"" << url << "\" responded with " << status
                        << "\n" << e.what() << "\n";
  }

  return status;
}

} // namespace API
} // namespace Mantid
