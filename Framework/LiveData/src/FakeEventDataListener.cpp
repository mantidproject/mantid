#include "MantidLiveData/FakeEventDataListener.h"
#include "MantidLiveData/Exception.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/WriteLock.h"
#include "MantidKernel/DateAndTime.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace LiveData {
DECLARE_LISTENER(FakeEventDataListener)

/// Constructor
FakeEventDataListener::FakeEventDataListener()
    : ILiveListener(), m_buffer(), m_rand(new Kernel::MersenneTwister(5489)),
      m_timer(), m_callbackloop(1), m_numExtractDataCalls(0), m_runNumber(1) {
  if (!ConfigService::Instance().getValue("fakeeventdatalistener.datarate",
                                          m_datarate))
    m_datarate = 200; // Default data rate. Low so that our lowest-powered
                      // buildserver can cope.
  // For auto-ending and restarting runs
  if (!ConfigService::Instance().getValue("fakeeventdatalistener.endrunevery",
                                          m_endRunEvery))
    m_endRunEvery = 0;
  if (!ConfigService::Instance().getValue("fakeeventdatalistener.notyettimes",
                                          m_notyettimes))
    m_notyettimes = 0;
}

/// Destructor
FakeEventDataListener::~FakeEventDataListener() {
  m_timer.stop();
  delete m_rand;
}

bool FakeEventDataListener::connect(const Poco::Net::SocketAddress &) {
  // Do nothing for now. Later, put in stuff to help test failure modes.
  return true;
}

bool FakeEventDataListener::isConnected() {
  return true; // For the time being at least
}

ILiveListener::RunStatus FakeEventDataListener::runStatus() {
  if (m_endRunEvery > 0 && DateAndTime::getCurrentTime() > m_nextEndRunTime) {
    // End a run once every m_endRunEvery seconds
    m_nextEndRunTime = DateAndTime::getCurrentTime() + m_endRunEvery;
    m_runNumber++;
    return EndRun;
  } else
    // Never end a run
    return Running;
}

int FakeEventDataListener::runNumber() const { return m_runNumber; }

void FakeEventDataListener::start(
    Kernel::DateAndTime /*startTime*/) // Ignore the start time for now at least
{
  // Set up the workspace buffer (probably won't know its dimensions before this
  // point)
  // 2 spectra event workspace for now. Will make larger later.
  // No instrument, meta-data etc - will need to figure out who's responsible
  // for that
  m_buffer = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", 2, 2, 1));
  // Set a sample tof range
  m_rand->setRange(40000, 60000);
  m_rand->setSeed(Kernel::DateAndTime::getCurrentTime().totalNanoseconds());

  // If necessary, calculate the number of events we need to generate on each
  // call of generateEvents
  // Rather limited resolution of 2000 events/sec
  if (m_datarate > 2000) {
    m_callbackloop = m_datarate / 2000;
  }
  // Using a Poco::Timer here. Probably a real listener will want to use a
  // Poco::Activity or ActiveMethod.
  m_timer.setPeriodicInterval((m_datarate > 2000 ? 1 : 2000 / m_datarate));
  m_timer.start(Poco::TimerCallback<FakeEventDataListener>(
      *this, &FakeEventDataListener::generateEvents));

  // When we are past this time, end the run.
  m_nextEndRunTime = DateAndTime::getCurrentTime() + m_endRunEvery;

  return;
}

boost::shared_ptr<Workspace> FakeEventDataListener::extractData() {
  // This is here to test the LoadLiveData side of the 'NotYet' exception
  // Note the post-increment of the call count in the comparison
  if (m_numExtractDataCalls++ < m_notyettimes)
    throw Exception::NotYet("No workspace yet!");

  /* For the very first try, just add a small number of uniformly distributed
   * events.
   * Next: 1. Add some kind of distribution
   *       2. Continuously add events in a separate thread once start has been
   * called
   */
  using namespace DataObjects;

  // Create a new, empty workspace of the same dimensions and assign to the
  // buffer variable
  EventWorkspace_sptr temp = boost::dynamic_pointer_cast<EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", 2, 2, 1));
  // Will need an 'initializeFromParent' here later on....

  // Safety considerations suggest I should stop the thread here, but the below
  // methods don't
  // seem to do what I'd expect and I haven't seen any problems from not having
  // them (yet).
  // m_timer.stop();
  // m_timer.restart();

  // Get an exclusive lock
  // will wait for generateEvents() to finish before swapping
  Mutex::ScopedLock _lock(m_mutex);
  std::swap(m_buffer, temp);

  // Add a run number
  temp->mutableRun().addLogData(
      new PropertyWithValue<int>("run_number", m_runNumber));

  return temp;
}

/** Callback method called at specified interval by timer.
 *  Used to fill buffer workspace with events between calls to extractData.
 */
void FakeEventDataListener::generateEvents(Poco::Timer &) {
  Mutex::ScopedLock _lock(m_mutex);
  for (long i = 0; i < m_callbackloop; ++i) {
    m_buffer->getEventList(0)
        .addEventQuickly(DataObjects::TofEvent(m_rand->nextValue()));
    m_buffer->getEventList(1)
        .addEventQuickly(DataObjects::TofEvent(m_rand->nextValue()));
  }

  return;
}
} // namespace LiveData
} // namespace Mantid
