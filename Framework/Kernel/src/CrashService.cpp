#include "MantidKernel/CrashService.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/ParaViewVersion.h"
#include "MantidKernel/FacilityInfo.h"

#include <Poco/ActiveResult.h>

#include <json/json.h>

namespace Mantid {
namespace Kernel {

namespace {
/// static logger
Logger g_log("CrashServiceImpl");
}
const std::string CRASH_URL("http://crashreports.mantidproject.org/api/crash");
// const std::string STARTUP_URL(
//    "http://posttestserver.com/post.php?dir=Mantid"); // dev location
// http://posttestserver.com/data/

//----------------------------------------------------------------------------------------------
// Constructor for CrashServiceImpl
CrashServiceImpl::CrashServiceImpl(std::string application)
    : m_application(application), m_crashActiveMethod(this, &CrashServiceImpl::sendCrashAsyncImpl) {
  
}

void CrashServiceImpl::sendCrashReport() {
  try {
    std::string message = this->generateCrashMessage();

    // send the report
    Poco::ActiveResult<int> result = m_crashActiveMethod(message);
  } catch (std::exception &ex) {
    g_log.debug() << "Send crash report failure. " << ex.what() << '\n';
  }
}

std::string CrashServiceImpl::generateCrashMessage() {
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

  message["dateTime"] =
      Types::Core::DateAndTime::getCurrentTime().toISO8601String();

  message["application"] = m_application;

  message["facility"] = ConfigService::Instance().getFacility().name();

  ::Json::FastWriter writer;
  return writer.write(message);
}

int CrashServiceImpl::sendCrashAsyncImpl(const std::string &message) {
  return this->sendReport(message, CRASH_URL);
}

int CrashServiceImpl::sendReport(const std::string &message,
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
                        << "\n" << e.what() << "\n";
  }

  return status;
}

} // namespace Kernel
} // namespace Mantid