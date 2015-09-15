#include "MantidWorkflowAlgorithms/SendUsage.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/ParaViewVersion.h"
#include <sstream>

namespace Mantid {
namespace WorkflowAlgorithms {

using Mantid::Kernel::ConfigService;
using Mantid::Kernel::DateAndTime;
using Mantid::Kernel::Direction;
using Mantid::Kernel::MantidVersion;
using Mantid::API::WorkspaceProperty;
using std::string;

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(SendUsage)

string SendUsage::g_header = string();

namespace {
/// The key in Mantid::Kernel::ConfigService to use.
const std::string SEND_USAGE_CONFIG_KEY("usagereports.enabled");

/// The default status for html to return if it wasn't run
const int STATUS_DEFAULT = -1;

/// The URL endpoint.
const std::string URL("http://reports.mantidproject.org/api/usage");
// const std::string URL("http://127.0.0.1:8000/api/usage"); // dev location

/// @returns true if Kernel::ConfigService says the option is on.
bool doSend() {
  // create a variable to pass in
  int doSend = 1; // 0 = false, other = true

  // get the value from config service
  // the function returning something other than 0 means an error
  if (!ConfigService::Instance().getValue(SEND_USAGE_CONFIG_KEY, doSend)) {
    doSend = 1;
  }

  return (doSend != 0);
}

/// @returns Now formated for the json doc
string currentDateAndTime() {
  std::stringstream buffer;
  buffer << ",\"dateTime\":\""
         << DateAndTime::getCurrentTime().toISO8601String() << "\"";
  return buffer.str();
}
}

//----------------------------------------------------------------------------------------------
/** Constructor
 */
SendUsage::SendUsage() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
SendUsage::~SendUsage() {}

//----------------------------------------------------------------------------------------------

/// Algorithms name for identification. @see Algorithm::name
const std::string SendUsage::name() const { return "SendUsage"; }

/// Algorithm's version for identification. @see Algorithm::version
int SendUsage::version() const { return 1; }

/// Algorithm's category for identification. @see Algorithm::category
const std::string SendUsage::category() const { return "Workflow"; }

/// Algorithm's summary for use in the GUI and help. @see Algorithm::summary
const std::string SendUsage::summary() const {
  return "Send system usage back to mantid developers";
}

//----------------------------------------------------------------------------------------------
/** Initialize the algorithm's properties.
 */
void SendUsage::init() {
  declareProperty("Application", "mantidplot", "how mantid was invoked");
  declareProperty("Component", "", "leave blank for now");
  declareProperty("Json", "", Direction::Output);
  declareProperty("HtmlCode", STATUS_DEFAULT, Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SendUsage::exec() {
  // generate the default header - this uses a cache variable
  this->generateHeader();

  std::string json = this->generateJson();
  this->setPropertyValue("Json", json);

  // send the report
  if (doSend()) {
    sendReport(json);
  } else {
    g_log.debug("Sending usage reports is disabled\n");
  }
}

void SendUsage::sendReport(const std::string &json) {
  g_log.debug() << json << "\n";

  int status = STATUS_DEFAULT;

  try {
    Kernel::InternetHelper helper;
    std::stringstream responseStream;
    helper.setBody(json);
    status = helper.sendRequest(URL, responseStream);
    g_log.debug() << "Call to \"" << URL << "\" responded with " << status
                  << "\n" << responseStream.str() << "\n";
  }
  catch (Mantid::Kernel::Exception::InternetError &e) {
    status = e.errorCode();
    g_log.information() << "Call to \"" << URL << "\" responded with " << status
                        << "\n" << e.what() << "\n";
  }

  this->setProperty("HtmlCode", status);
}

std::string SendUsage::generateJson() {
  // later in life the additional parameters can be done after
  // the current date and time
  std::stringstream buffer;
  buffer << g_header << currentDateAndTime();

  // get the properties that were set
  std::string application = this->getPropertyValue("Application");
  if (!application.empty()) {
    buffer << ",\"application\":\"" << application << "\"";
  }
  std::string component = this->getPropertyValue("Component");
  if (!component.empty()) {
    buffer << ",\"component\":\"" << component << "\"";
  }

  // close the document
  buffer << "}";

  return buffer.str();
}

/**
 * This puts together the system information for the json document.
 * The only thing it is missing is a closing brace '}' so it can
 * be reused for other status messages at a later date.
 */
void SendUsage::generateHeader() {
  // see if the information is cached
  if (!g_header.empty()) {
    return;
  }

  // buffer for the json document
  std::stringstream buffer;
  buffer << "{";

  // username
  buffer << "\"uid\":\"" << Kernel::ChecksumHelper::md5FromString(
                                ConfigService::Instance().getUsername())
         << "\",";

  // hostname
  buffer << "\"host\":\"" << Kernel::ChecksumHelper::md5FromString(
                                 ConfigService::Instance().getComputerName())
         << "\",";

  // os name, version, and architecture
  buffer << "\"osName\":\"" << ConfigService::Instance().getOSName() << "\","
         << "\"osArch\":\"" << ConfigService::Instance().getOSArchitecture()
         << "\","
         << "\"osVersion\":\"" << ConfigService::Instance().getOSVersion()
         << "\","
         << "\"osReadable\":\""
         << ConfigService::Instance().getOSVersionReadable() << "\",";

  // paraview version or zero
  buffer << "\"ParaView\":\"";
  if (ConfigService::Instance().pvPluginsAvailable()) {
    buffer << Kernel::ParaViewVersion::targetVersion();
  } else {
    buffer << 0;
  }
  buffer << "\",";

  // mantid version and sha1
  buffer << "\"mantidVersion\":\"" << MantidVersion::version() << "\","
         << "\"mantidSha1\":\"" << MantidVersion::revisionFull() << "\"";

  g_header = buffer.str();
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
