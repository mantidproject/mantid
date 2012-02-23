#include "MantidDataHandling/FakeEventDataListener.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/MersenneTwister.h"

namespace {
  // The data rate to (attempt to) generate in events/sec
  long DATA_RATE = 200;
}

using namespace Mantid::API;

namespace Mantid
{
namespace DataHandling
{
  DECLARE_LISTENER(FakeEventDataListener)

  /// Constructor
  FakeEventDataListener::FakeEventDataListener() : ILiveListener(),
      m_buffer(), m_rand(new Kernel::MersenneTwister), m_timer(), m_callbackloop(1)
  {
  }
    
  /// Destructor
  FakeEventDataListener::~FakeEventDataListener()
  {
    m_timer.stop();
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
    m_rand->setSeed(Kernel::DateAndTime::getCurrentTime().totalNanoseconds());

    // If necessary, calculate the number of events we need to generate on each call of generateEvents
    // Rather limited resolution of 2000 events/sec
    if ( DATA_RATE > 2000 )
    {
      m_callbackloop = DATA_RATE / 2000;
    }
    // Using a Poco::Timer here. Probably a real listener will want to use a Poco::Activity or ActiveMethod.
    m_timer.setPeriodicInterval( (DATA_RATE > 2000 ? 1 : 2000/DATA_RATE) );
    m_timer.start(Poco::TimerCallback<FakeEventDataListener>(*this,&FakeEventDataListener::generateEvents));

    return;
  }

  boost::shared_ptr<MatrixWorkspace> FakeEventDataListener::extractData()
  {
    /* For the very first try, just add a small number of uniformly distributed events.
     * Next: 1. Add some kind of distribution
     *       2. Continuously add events in a separate thread once start has been called
     */
    using namespace DataObjects;

    // Create a new, empty workspace of the same dimensions and assign to the buffer variable
    // TODO: Think about whether creating a new workspace at this point is scalable
    EventWorkspace_sptr temp = boost::dynamic_pointer_cast<EventWorkspace>(
                                 WorkspaceFactory::Instance().create("EventWorkspace",2,2,1) );
    // Will need an 'initializeFromParent' here later on....

    // Safety considerations suggest I should stop the thread here, but the below methods don't
    // seem to do what I'd expect and I haven't seen any problems from not having them (yet).
    //m_timer.stop();
    std::swap(m_buffer,temp);
    //m_timer.restart();

    return temp;
  }

  /** Callback method called at specified interval by timer.
   *  Used to fill buffer workspace with events between calls to extractData.
   */
  void FakeEventDataListener::generateEvents(Poco::Timer&)
  {
    for (long i = 0; i < m_callbackloop; ++i)
    {
      m_buffer->getEventList(0).addEventQuickly(DataObjects::TofEvent(m_rand->next()));
      m_buffer->getEventList(1).addEventQuickly(DataObjects::TofEvent(m_rand->next()));
    }

    return;
  }
} // namespace Mantid
} // namespace DataHandling
