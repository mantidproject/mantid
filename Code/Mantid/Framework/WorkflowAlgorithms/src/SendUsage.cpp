#include "MantidWorkflowAlgorithms/SendUsage.h"
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
int SendUsage::version() const {
  return 1;
}

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
  // buffer the json document
  std::stringstream buffer;
  buffer << "{";

  // username - TODO should be hashed
  buffer << "\"uid\":\"" << ConfigService::Instance().getUsername() << "\",";

  // hostname - TODO should be hashed
  buffer << "\"host\":\"" << ConfigService::Instance().getComputerName()
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
  if (ConfigService::Instance().quickParaViewCheck())
    buffer << Kernel::ParaViewVersion::targetVersion();
  else
    buffer << 0;
  buffer << "\",";

  // mantid version and sha1
  buffer << "\"mantidVersion\":\"" << MantidVersion::version() << "\","
         << "\"mantidSha1\":\"" << MantidVersion::revisionFull() << "\"";

  // close up the json
  buffer << "}";

  g_log.warning() << buffer.str() << std::endl;
}

} // namespace WorkflowAlgorithms
} // namespace Mantid
