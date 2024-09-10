// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/ErrorReporter.h"
#include "MantidJson/Json.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/FacilityInfo.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MantidVersion.h"

#include <Poco/ActiveResult.h>
#include <json/json.h>

#include <utility>

namespace Mantid::Kernel {

namespace {
/// static logger
Logger g_log("ErrorReporter");
} // namespace

//----------------------------------------------------------------------------------------------
// Constructor for ErrorReporter
/** Constructor
 */
ErrorReporter::ErrorReporter(const std::string &application, const Types::Core::time_duration &upTime,
                             const std::string &exitCode, const bool share)
    : ErrorReporter(application, upTime, exitCode, share, "", "", "", "") {}

/** Constructor
 */
ErrorReporter::ErrorReporter(const std::string &application, const Types::Core::time_duration &upTime,
                             const std::string &exitCode, const bool share, const std::string &name,
                             const std::string &email, const std::string &textBox)
    : ErrorReporter(application, upTime, exitCode, share, name, email, textBox, "") {}

ErrorReporter::ErrorReporter(std::string application, Types::Core::time_duration upTime, std::string exitCode,
                             const bool share, std::string name, std::string email, std::string textBox,
                             std::string traceback)
    : m_application(std::move(application)), m_exitCode(std::move(exitCode)), m_upTime(std::move(upTime)),
      m_share(share), m_name(std::move(name)), m_email(std::move(email)), m_textbox(std::move(textBox)),
      m_stacktrace(std::move(traceback)) {
  auto url = Mantid::Kernel::ConfigService::Instance().getValue<std::string>("errorreports.rooturl");
  if (!url.has_value()) {
    g_log.debug() << "Failed to load error report url\n";
  } else {
    m_url = url.value();
  }
}

/** Generates an error message and then calls an internet helper to send it
 */
Mantid::Kernel::InternetHelper::HTTPStatus ErrorReporter::sendErrorReport() {
  try {
    std::string message = this->generateErrorMessage();

    // send the report
    // Poco::ActiveResult<int> result = m_errorActiveMethod(message);
    const auto status = this->sendReport(message, m_url + "/api/error");
    return status;
  } catch (Kernel::Exception::InternetError &ex) {
    g_log.debug() << "Send error report failure. " << ex.what() << '\n';
    return static_cast<InternetHelper::HTTPStatus>(ex.errorCode());
  } catch (std::exception &ex) {
    g_log.debug() << "Send error report failure. " << ex.what() << '\n';
    return Kernel::InternetHelper::HTTPStatus::BAD_REQUEST;
  }
}

/** Generates an error message in json format
 */
std::string ErrorReporter::generateErrorMessage() const {
  ::Json::Value message;

  // username
  message["uid"] = Kernel::ChecksumHelper::md5FromString(ConfigService::Instance().getUsername());
  // hostname
  message["host"] = Kernel::ChecksumHelper::md5FromString(ConfigService::Instance().getComputerName());

  // os name, version, and architecture
  message["osName"] = ConfigService::Instance().getOSName();
  message["osArch"] = ConfigService::Instance().getOSArchitecture();
  message["osVersion"] = ConfigService::Instance().getOSVersion();
  message["osReadable"] = ConfigService::Instance().getOSVersionReadable();

  // legacy interface requires paraview version DON'T REMOVE
  message["ParaView"] = 0;

  // mantid version and sha1
  message["mantidVersion"] = MantidVersion::version();
  message["mantidSha1"] = MantidVersion::revisionFull();

  message["dateTime"] = Types::Core::DateAndTime::getCurrentTime().toISO8601String();

  message["upTime"] = to_simple_string(m_upTime);

  message["application"] = m_application;

  message["facility"] = ConfigService::Instance().getFacility().name();

  message["exitCode"] = m_exitCode;

  if (m_share) {
    message["textBox"] = m_textbox;
    message["email"] = m_email;
    message["name"] = m_name;
    message["stacktrace"] = m_stacktrace;
  } else {
    message["email"] = "";
    message["name"] = "";
    message["textBox"] = m_textbox;
    message["stacktrace"] = "";
  }

  return Mantid::JsonHelpers::jsonToString(message);
}

/** Submits a post request to the specified url with the message as the body
 @param message : String containg json formatted error message
 @param url : The url to send the post request to
*/
Kernel::InternetHelper::HTTPStatus ErrorReporter::sendReport(const std::string &message, const std::string &url) {
  InternetHelper::HTTPStatus status;
  try {
    Kernel::InternetHelper helper;
    std::stringstream responseStream;
    helper.setTimeout(20);
    helper.setBody(message);
    status = helper.sendRequest(url, responseStream);
  } catch (Mantid::Kernel::Exception::InternetError &e) {
    status = static_cast<InternetHelper::HTTPStatus>(e.errorCode());
    g_log.information() << "Call to \"" << url << "\" responded with " << static_cast<int>(status) << "\n"
                        << e.what() << "\n";
  }

  return status;
}

} // namespace Mantid::Kernel
