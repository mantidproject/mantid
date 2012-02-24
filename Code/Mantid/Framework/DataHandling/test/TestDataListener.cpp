#include "TestDataListener.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/MersenneTwister.h"

using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{
  DECLARE_LISTENER(TestDataListener)

  /// Constructor
  TestDataListener::TestDataListener() : ILiveListener(),
      m_buffer(), m_rand(new Kernel::MersenneTwister)
  {
    // Set up the workspace buffer
    m_buffer = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
                 WorkspaceFactory::Instance().create("EventWorkspace", 2, 2, 1) );
    // Set a sample tof range
    m_rand->setRange(40000,60000);
    m_rand->setSeed(Kernel::DateAndTime::getCurrentTime().totalNanoseconds());
  }
    
  /// Destructor
  TestDataListener::~TestDataListener()
  {
    delete m_rand;
  }
  
  bool TestDataListener::connect(const Poco::Net::SocketAddress&)
  {
    // Do nothing for now. Later, put in stuff to help test failure modes.
    return true;
  }

  bool TestDataListener::isConnected()
  {
    return true; // For the time being at least
  }

  void TestDataListener::start(Kernel::DateAndTime /*startTime*/) // Ignore the start time
  {
    return;
  }

  boost::shared_ptr<MatrixWorkspace> TestDataListener::extractData()
  {
    // Add a small number of uniformly distributed events to each event list.
    using namespace DataObjects;
    EventList & el1 = m_buffer->getEventList(0);
    EventList & el2 = m_buffer->getEventList(1);
    for (int i = 0; i < 100; ++i)
    {
      el1.addEventQuickly(TofEvent(m_rand->next()));
      el2.addEventQuickly(TofEvent(m_rand->next()));
    }

    // Copy the workspace pointer to a temporary variable
    EventWorkspace_sptr extracted = m_buffer;
    // Create a new, empty workspace of the same dimensions and assign to the buffer variable
    m_buffer = boost::dynamic_pointer_cast<EventWorkspace>(
                 WorkspaceFactory::Instance().create("EventWorkspace",2,2,1) );

    return extracted;
  }

} // namespace Mantid
} // namespace DataHandling
