// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ErrorReporter.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/ParaViewVersion.h"

#include <Poco/ActiveResult.h>
#include <json/json.h>

namespace Mantid {
namespace Kernel {

namespace {
/// static logger
Logger g_log("ErrorReporter");
} // namespace

//----------------------------------------------------------------------------------------------
// Constructor for ErrorReporter
/** Constructor
 */
ErrorReporter::ErrorReporter(const std::string application,
                             const Types::Core::time_duration upTime,
                             const std::string exitCode, const bool share)
    : ErrorReporter(application, upTime, exitCode, share, "", "", "", "") {}

/** Constructor
 */
ErrorReporter::ErrorReporter(const std::string application,
                             const Types::Core::time_duration upTime,
                             const std::string exitCode, const bool share,
                             const std::string name, const std::string email,
                             const std::string textBox)
    : ErrorReporter(application, upTime, exitCode, share, name, email, textBox,
                    "") {}

ErrorReporter::ErrorReporter(const std::string application,
                             const Types::Core::time_duration upTime,
                             const std::string exitCode, const bool share,
                             const std::string name, const std::string email,
                             const std::string textBox,
                             const std::string recoveryFile)
    : m_application(application), m_exitCode(exitCode), m_upTime(upTime),
      m_share(share), m_name(name), m_email(email), m_textbox(textBox),
      m_recoveryFile(recoveryFile) {
  auto url = Mantid::Kernel::ConfigService::Instance().getValue<std::string>(
      "errorreports.rooturl");
  if (!url.is_initialized()) {
    g_log.debug() << "Failed to load error report url\n";
  } else {
    m_url = url.get();
  }
}

/** Generates an error message and then calls an internet helper to send it
 */
int ErrorReporter::sendErrorReport() {
  try {
    std::string message = this->generateErrorMessage();

    // send the report
    // Poco::ActiveResult<int> result = m_errorActiveMethod(message);
    auto status = this->sendReport(message, m_url + "/api/error");
    return status;
  } catch (std::exception &ex) {
    g_log.debug() << "Send error report failure. " << ex.what() << '\n';
    return -1;
  }
}

/** Generates an error message in json format
 */
std::string ErrorReporter::generateErrorMessage() {
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

  message["upTime"] = to_simple_string(m_upTime);

  message["application"] = m_application;

  message["facility"] = ConfigService::Instance().getFacility().name();

  message["exitCode"] = m_exitCode;

  if (m_share) {
    message["textBox"] = m_textbox;
    message["email"] = m_email;
    message["name"] = m_name;
    message["fileHash"] = m_recoveryFile;
  } else {
    message["email"] = "";
    message["name"] = "";
    message["fileHash"] = "";
    message["textBox"] = "";
  }

  ::Json::FastWriter writer;
  return writer.write(message);
}

/** Submits a post request to the specified url with the message as the body
 @param message : String containg json formatted error message
 @param url : The url to send the post request to
*/
int ErrorReporter::sendReport(const std::string &message,
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
