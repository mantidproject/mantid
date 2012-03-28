#include "TestDataListener.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ConfigService.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using Mantid::Kernel::ConfigService;

namespace Mantid
{
namespace DataHandling
{
  DECLARE_LISTENER(TestDataListener)

  /// Constructor
  TestDataListener::TestDataListener() : ILiveListener(),
      m_buffer(), m_rand(new Kernel::MersenneTwister)
  {
    // Set up the first workspace buffer
    this->createEmptyWorkspace();
    // Set a sample tof range
    m_rand->setRange(40000,60000);
    m_rand->setSeed(Kernel::DateAndTime::getCurrentTime().totalNanoseconds());

    m_timesCalled = 0;
    m_dataReset = false;
    if ( ! ConfigService::Instance().getValue("testdatalistener.reset_after",m_resetAfter) )
      m_resetAfter = 0;
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

  ILiveListener::RunStatus TestDataListener::runStatus()
  {
    // Always in a run - could be changed to do something more elaborate if needed for testing
    return RunStatus::Running;
  }

  void TestDataListener::start(Kernel::DateAndTime /*startTime*/) // Ignore the start time
  {
    return;
  }

  /** Create the default empty event workspace */
  void TestDataListener::createEmptyWorkspace()
  {
    // Create a new, empty workspace of the same dimensions and assign to the buffer variable
    m_buffer = boost::dynamic_pointer_cast<EventWorkspace>(
                 WorkspaceFactory::Instance().create("EventWorkspace",2,2,1) );
    // Give detector IDs
    for (size_t i=0; i<m_buffer->getNumberHistograms(); i++)
      m_buffer->getSpectrum(i)->setDetectorID( detid_t(i) );
    // Create in TOF units
    m_buffer->getAxis(0)->setUnit("TOF");
    // Load a fake instrument
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 10, 0.1);
    m_buffer->setInstrument(inst);
  }

  boost::shared_ptr<MatrixWorkspace> TestDataListener::extractData()
  {
    m_dataReset = false;

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
    this->createEmptyWorkspace();

    m_timesCalled++;

    if (m_resetAfter > 0 && m_timesCalled >= m_resetAfter)
    {
      m_dataReset = true;
      m_timesCalled = 0;
    }

    return extracted;
  }

} // namespace Mantid
} // namespace DataHandling
