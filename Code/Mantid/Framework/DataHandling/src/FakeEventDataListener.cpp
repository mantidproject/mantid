#include "MantidDataHandling/FakeEventDataListener.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/MersenneTwister.h"

using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{
  DECLARE_LISTENER(FakeEventDataListener)

  /// Constructor
  FakeEventDataListener::FakeEventDataListener() : ILiveListener(),
      m_buffer(), m_rand(new Kernel::MersenneTwister)
  {
  }
    
  /// Destructor
  FakeEventDataListener::~FakeEventDataListener()
  {
    delete m_rand;
  }
  
  bool FakeEventDataListener::connect(const Poco::Net::SocketAddress&)
  {
    // Do nothing for now. Later, put in stuff to help test failure modes.
    return true;
  }

  bool FakeEventDataListener::isConnected()
  {
    return true; // For the time being at least
  }

  void FakeEventDataListener::start(Kernel::DateAndTime /*startTime*/) // Ignore the start time for now at least
  {
    // Set up the workspace buffer (probably won't know its dimensions before this point)
    // 2 spectra event workspace for now. Will make larger later.
    // No instrument, meta-data etc - will need to figure out who's responsible for that
    m_buffer = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
                 WorkspaceFactory::Instance().create("EventWorkspace", 2, 2, 1) );
    // Set a sample tof range
    m_rand->setRange(40000,60000);
    m_rand->setSeed(Kernel::DateAndTime::get_current_time().total_nanoseconds());

    return;
  }

  boost::shared_ptr<MatrixWorkspace> FakeEventDataListener::extractData()
  {
    // TODO: Add protection against calling this before start() has been called

    /* For the very first try, just add a small number of uniformly distributed events.
     * Next: 1. Add some kind of distribution
     *       2. Continuously add events in a separate thread once start has been called
     */
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
    // TODO: Think about whether creating a new workspace at this point is scalable
    m_buffer = boost::dynamic_pointer_cast<EventWorkspace>(
                 WorkspaceFactory::Instance().create("EventWorkspace",2,2,1) );
    // Will need an 'initializeFromParent' here later on....

    return extracted;
  }

} // namespace Mantid
} // namespace DataHandling
