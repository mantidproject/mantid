#include "MantidAPI/UsageService.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ParaViewVersion.h"

#include <Poco/ActiveResult.h>
#include <json/json.h>

namespace Mantid {
namespace API {

using namespace Kernel;

/// static logger
Kernel::Logger g_log("UsageService");

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
  jsonMap["name"] = type;
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
/** Constructor for UsageServiceImpl
 */
UsageServiceImpl::UsageServiceImpl()
    : m_timer(), m_timerTicks(0), m_timerTicksTarget(0), m_FeatureQueue(),
      m_FeatureQueueSizeThreshold(50), m_mutex(), m_cachedHeader() {
  if (isEnabled()) {
    int interval = 60;
    ConfigService::Instance().getValue("Usage.BufferCheck", interval);

    // set the ticks target to by 24 hours / interval
    m_timerTicksTarget = 24 * 60 * 60 / interval;

    m_timer.setPeriodicInterval((interval * 1000));
    m_timer.start(Poco::TimerCallback<UsageServiceImpl>(
        *this, &UsageServiceImpl::timerCallback));
  }
}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
UsageServiceImpl::~UsageServiceImpl() {}

void UsageServiceImpl::registerStartup() {
  if (isEnabled()) {
    sendStartupReport();
  }
}

//----------------------------------------------------------------------------------------------
/** registerFeatureUsage Overloads
*/
void UsageServiceImpl::registerFeatureUsage(const std::string &type,
                                            const std::string &name,
                                            const Kernel::DateAndTime &start,
                                            const float &duration,
                                            const std::string &details) {
  m_FeatureQueue.push(FeatureUsage(type, name, start, duration, details));
}

void UsageServiceImpl::registerFeatureUsage(const std::string &type,
                                            const std::string &name,
                                            const std::string &details) {
  registerFeatureUsage(type, name, DateAndTime::getCurrentTime(), 0.0, details);
}

void UsageServiceImpl::registerFeatureUsage(const Algorithm *alg,
                                            const float &duration = 0.0) {
  std::ostringstream oss;
  oss << alg->name() << ".v" << alg->version();
  registerFeatureUsage("Algorithm", oss.str(), DateAndTime::getCurrentTime(),
                       duration, alg->toString());
}

//----------------------------------------------------------------------------------------------

bool UsageServiceImpl::isEnabled() const {
  try {
    int enabled = 0;
    int retVal = Kernel::ConfigService::Instance().getValue(
        "usagereports.enabled", enabled);
    if ((retVal == 0) || (enabled == 0)) {
      return false; // exit early
    }
    return true;
  } catch (std::exception &ex) {
    g_log.debug()
        << "Error determining if usage reporting is enabled: assuming false. "
        << ex.what() << std::endl;
    return false;
  }
}

void UsageServiceImpl::flush() {
  if (isEnabled()) {
    sendFeatureUsageReport();
  }
}

void UsageServiceImpl::sendStartupReport() {
  try {
    auto algSendStartupUsage = AlgorithmManager::Instance().create("SendUsage");
    algSendStartupUsage->setAlgStartupLogging(false);
    Poco::ActiveResult<bool> result = algSendStartupUsage->executeAsync();
  } catch (Kernel::Exception::NotFoundError &) {
    g_log.debug() << "SendUsage algorithm is not available - cannot update "
                     "send usage information."
                  << std::endl;
  } catch (std::exception &ex) {
    g_log.debug() << "SendUsage algorithm failure. " << ex.what() << std::endl;
  }
}

void UsageServiceImpl::sendFeatureUsageReport() {
  try {
    ::Json::Value root;
    ::Json::FastWriter writer;
    ::Json::Reader reader;

    if (!m_FeatureQueue.empty()) {
      // lock around emptying of the Q so any further threads have to wait
      Kernel::Mutex::ScopedLock _lock(m_mutex);
      // generate json to submit
      while (!m_FeatureQueue.empty()) {
        auto featureUsage = m_FeatureQueue.front();
        root.append(featureUsage.asJson());
        m_FeatureQueue.pop();
      }
    }

    if (root.size() > 0) {
      std::string jsonString = writer.write(root);
      g_log.debug() << "FeatureUsage to send\n" << jsonString << std::endl;

      // TODO - submit json to server

    }

  } catch (std::exception &ex) {
    g_log.debug() << "sendFeatureUsageReport failure. " << ex.what()
                  << std::endl;
  }
}

void UsageServiceImpl::timerCallback(Poco::Timer &) {
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
::Json::Value UsageServiceImpl::generateHeader() {
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
    header["ParaView"] = ConfigService::Instance().pvPluginsAvailable() ? Kernel::ParaViewVersion::targetVersion() : 0;

    // mantid version and sha1
    header["mantidVersion"] = MantidVersion::version();
    header["mantidSha1"] = MantidVersion::revisionFull();

    //cache this for future use
    m_cachedHeader = header;
  }

  // mantid version and sha1
  header["dateTime"] = DateAndTime::getCurrentTime().toISO8601String();

  return header;
}


} // namespace API
} // namespace Mantid
