#include "MantidWorkflowAlgorithms/SendUsage.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/InternetHelper.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/ParaViewVersion.h"
#include <Poco/Net/HTTPRequest.h>
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

/// The URL endpoint.
// const std::string URL("http://django-mantid.rhcloud.com/api/usage");
const std::string URL("http://127.0.0.1:8000/api/usage"); // dev location

/// The string for post method
const std::string POST(Poco::Net::HTTPRequest::HTTP_POST);

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
  // declareProperty("UsageString", "", Direction::Input);
  declareProperty("Json", "", Direction::Output);
  declareProperty("HtmlCode", -1, Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SendUsage::exec() {
  // generate the default header - this uses a cache variable
  this->generateHeader();

  // send the report
  if (doSend()) {
    sendReport();
  } else {
    g_log.debug("Sending usage reports is disabled\n");
  }
}

void SendUsage::sendReport(const std::string &body) {
  std::string text(g_header + currentDateAndTime() + body + "}");

  g_log.debug() << text << "\n"; // TODO should be debug
  this->setPropertyValue("Json", text);

  // set up the headers
  std::map<string, string> htmlHeaders;

  std::stringstream responseStream;
  Kernel::InternetHelper helper;
  int status = helper.sendRequest(URL, responseStream, htmlHeaders, POST, text);
  this->setProperty("HtmlCode", status);
  g_log.debug() << "Call responded with " << status << "\n"
                << responseStream.str() << "\n";
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
         << "\",";

  // paraview version or zero
  buffer << "\"ParaView\":\"";
  if (ConfigService::Instance().quickParaViewCheck()) {
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
