#include "MantidWorkflowAlgorithms/SendUsage.h"
#include "MantidKernel/ChecksumHelper.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/MantidVersion.h"
#include "MantidKernel/ParaViewVersion.h"

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
  declareProperty("UsageString", "", Direction::Output);
}

//----------------------------------------------------------------------------------------------
/** Execute the algorithm.
 */
void SendUsage::exec() {
  this->generateHeader();

  // close up the json
  g_log.warning() << g_header << "}" << std::endl;
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

  // current date and time
  buffer << "\"dateTime\":\"" << DateAndTime::getCurrentTime().toISO8601String()
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
