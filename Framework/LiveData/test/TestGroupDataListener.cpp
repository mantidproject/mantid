#include "TestGroupDataListener.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"

using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::Kernel::ConfigService;

namespace Mantid {
namespace LiveData {
DECLARE_LISTENER(TestGroupDataListener)

/// Constructor
TestGroupDataListener::TestGroupDataListener() : ILiveListener(), m_buffer() {
  // Set up the first workspace buffer
  this->createWorkspace();

  m_dataReset = false;
}

/// Destructor
TestGroupDataListener::~TestGroupDataListener() = default;

bool TestGroupDataListener::connect(const Poco::Net::SocketAddress &) {
  // Do nothing.
  return true;
}

bool TestGroupDataListener::isConnected() { return true; }

ILiveListener::RunStatus TestGroupDataListener::runStatus() { return Running; }

int TestGroupDataListener::runNumber() const { return 0; }

void TestGroupDataListener::start(
    Kernel::DateAndTime /*startTime*/) // Ignore the start time
{
  return;
}

/** Create the default empty event workspace */
void TestGroupDataListener::createWorkspace() {
  // create a group
  m_buffer = WorkspaceCreationHelper::CreateWorkspaceGroup(3, 2, 10, "tst");
  // it must not be in the ADS
  API::AnalysisDataService::Instance().deepRemoveGroup("tst");
}

boost::shared_ptr<Workspace> TestGroupDataListener::extractData() {
  m_dataReset = false;

  // Copy the workspace pointer to a temporary variable
  API::WorkspaceGroup_sptr extracted = m_buffer;
  this->createWorkspace();

  return extracted;
}

} // namespace LiveData
} // namespace Mantid
