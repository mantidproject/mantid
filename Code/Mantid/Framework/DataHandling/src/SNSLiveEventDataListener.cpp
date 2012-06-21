#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/ADARAParser.h"
#include "MantidDataHandling/SNSLiveEventDataListener.h"
#include "MantidDataObjects/Events.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/WriteLock.h"

#include <Poco/Net/NetException.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketStream.h>
#include <Poco/Timestamp.h>

#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include <sstream>    // for ostringstream
#include <string>
#include <exception>

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Time we'll wait on a receive call (in milliseconds)
// Also used when shutting down the thread so we know how long to wait there
#define RECV_TIMEOUT_MS 100

namespace Mantid
{
namespace DataHandling
{
  DECLARE_LISTENER(SNSLiveEventDataListener)
  ;
  // The DECLARE_LISTENER macro seems to confuse some editors' syntax checking.  The
  // semi-colon limits the complaints to one line.  It has no actual effect on the code.

  // Get a reference to the logger
  Kernel::Logger& SNSLiveEventDataListener::g_log = Kernel::Logger::get("SNSLiveEventDataListener");

  /// Constructor
  SNSLiveEventDataListener::SNSLiveEventDataListener()
    : ILiveListener(), ADARA::Parser(),
      m_buffer(), m_workspaceInitialized( false), m_socket(),
      m_isConnected( false), m_stopThread( false)
    // ADARA::Parser() will accept values for buffer size and max packet size, but the
    // defaults will work fine
  {
  }
    
  /// Destructor
  SNSLiveEventDataListener::~SNSLiveEventDataListener()
  {
    // Stop the background thread
    if (m_thread.isRunning())
    {
      // Ask the thread to exit (and hope that it does - Poco doesn't
      // seem to have an equivalent to pthread_cancel
      m_stopThread = true;
      try {
      m_thread.join(RECV_TIMEOUT_MS * 2);
      } catch (Poco::TimeoutException e) {
        // And just what do we do here?!?
        // Log a message, sure, but other than that we can either hang the
        // Mantid process waiting for a thread that will apparently never exit
        // or segfault because the ADARA::read() is going to try to write to
        // a buffer that's going to be deleted.
        // Chose segfault - at least that's obvious.
        g_log.fatal() << "SNSLiveEventDataListener failed to shut down its background thread! "
                      << "This should never happen and Mantid is pretty much guaranteed to crash shortly.  "
                      << "Talk to the Mantid developer team." << std::endl;
      }

    }
  }

  bool SNSLiveEventDataListener::connect(const Poco::Net::SocketAddress& address)
  // The SocketAddress class will throw various exceptions if it encounters an error
  // We're assuming the calling function will catch any exceptions that are important
  // Note: Right now, it's the factory class that actually calls connect(), and it
  // doesn't check the return value.  (It does, however, trap the Poco exceptions.)
  {
    bool rv = false;  // assume failure

    // If we don't have an address, force a connection to the test server running on
    // localhost on the default port
    if (address.host().toString().compare( "0.0.0.0") == 0)
    {
      Poco::Net::SocketAddress tempAddress("localhost:31415");
      m_socket.connect( tempAddress);  // BLOCKING connect
    }
    else
    {
      m_socket.connect( address);  // BLOCKING connect
    }

    m_socket.setReceiveTimeout( Poco::Timespan( 0, RECV_TIMEOUT_MS * 1000)); // POCO timespan is seconds, microseconds
    g_log.information() << "Connected to " << m_socket.address().toString() << std::endl;

    rv = m_isConnected = true;
    return rv;
  }

  bool SNSLiveEventDataListener::isConnected()
  {
    return m_isConnected;
  }

  void SNSLiveEventDataListener::start(Kernel::DateAndTime startTime)
  {
    // Save the startTime and kick off the background thread
    // (Can't really do anything else until we send the hello packet and the SMS sends us
    // back the various metadata packets
    m_startTime = startTime;   
    m_thread.start( *this);
  }

  void SNSLiveEventDataListener::run()
  {
    if (m_isConnected == false) // sanity check
    {
      throw std::runtime_error( std::string("SNSLiveEventDataListener::run(): No connection to SMS server."));
      return;  // should never be called, but here just in case exceptions are disabled
    }

    // First thing to do is send a hello packet
    uint32_t helloPkt[5] = { 4, ADARA::PacketType::CLIENT_HELLO_V0, 0, 0, 0};
    Poco::Timestamp now;
    uint32_t now_usec = (uint32_t)(now.epochMicroseconds() - now.epochTime());
    helloPkt[2] = (uint32_t)(now.epochTime() - ADARA::EPICS_EPOCH_OFFSET);
    helloPkt[3] = (uint32_t)now_usec * 1000;
    helloPkt[4] = (uint32_t)(m_startTime.totalNanoseconds() / 1000000000);  // divide by a billion to get time in seconds

    if ( m_socket.sendBytes(helloPkt, sizeof(helloPkt)) != sizeof(helloPkt))
    // Yes, I know a send isn't guarenteed to send the whole buffer in one call.
    // I'm treating such a case as an error anyway.
    {
      g_log.error()
        << "SNSLiveEventDataListener::run(): Failed to send client hello packet.  Thread exiting."
        << std::endl;
      m_stopThread = true;
    }

    while (m_stopThread == false)  // loop until the foreground thread tells us to stop
    {
      // Read the packets, accumulate them in m_buffer...
      read( m_socket);

      // Check the heartbeat
#define HEARTBEAT_TIMEOUT (60 * 5)  // 5 minutes
      time_duration elapsed = Kernel::DateAndTime().getCurrentTime() - m_heartbeat;
      if ( elapsed.total_seconds() > HEARTBEAT_TIMEOUT)
      {
        // SMS seems to have gone away.  Log an error
        g_log.error() << "SMS server has sent no data for " << HEARTBEAT_TIMEOUT
                      << " seconds.  Is it still running?" << std::endl;
      }
    }

    return;
  }


  bool SNSLiveEventDataListener::rxPacket( const ADARA::RTDLPkt &pkt)
  {
    m_heartbeat = Kernel::DateAndTime::getCurrentTime();

    // At the moment, all we need from the RTDL packets is the pulse
    // time and the raw flag.  (We reference them when processing the
    // banked event packets.)
    m_rtdlPulseId = pkt.pulseId();
    m_rtdlRawFlag = pkt.rawTOF();

    return true;
  }

  // Note:  Before we can process a particular BankedEventPkt, we must have
  // received the RTDLPkt for pulse ID specified in the BankedEventPkt.
  // Normally this won't be an issue since the SMS will send out RTDLPkts
  // before it sends BankedEventPkts.
  bool SNSLiveEventDataListener::rxPacket( const ADARA::BankedEventPkt &pkt)
  {

    // A few counters that we use for logging purposes
    unsigned eventsPerBank;
    unsigned totalEvents = 0;

    m_heartbeat = Kernel::DateAndTime::getCurrentTime();

    // First step - make sure the RTDL packet we've saved matches the
    // banked event packet we've just received and  make sure its RAW flag
    // is false
    if (pkt.pulseId() != m_rtdlPulseId)
    {
      // Wrong RTDL packet.  Fail!
      g_log.error() << "Ignoring data from Pulse ID" << pkt.pulseId()
                    << "because we have not received an RTDL packet for that pulse!"
                    << std::endl;
      return false;
    }

    if (m_rtdlRawFlag)
    {
      // As yet, we can't handle raw TofF values.
      g_log.warning() << "Ignoring data from Pulse ID " << pkt.pulseId()
                      << " because the raw flag was set." << std::endl;
      return false;
    }

    // TODO: Create a log with the pulse time and charge!!  (TimeSeriesProperties)

    // Append the events
    g_log.information() << "----- Pulse ID: " << pkt.pulseId() << " -----" << std::endl;
    m_mutex.lock();
    // Iterate through bank sections
    const ADARA::EventBank *bank = pkt.firstBank();
    while (bank != NULL)
    {
      if (pkt.curBankId() <= 0xFFFe)
      // Note: We ignore bank ID's >= 0xFFFE  (-2 is for error pixels and -1 is
      // for unmappable pixels);
      {
        eventsPerBank=0;

        // Iterate through each event
        const ADARA::Event *event = pkt.firstEvent();
        while (event != NULL)
        {
          eventsPerBank++;
          totalEvents++;

          // appendEvent needs tof to be in units of microseconds, but it comes
          // from the ADARA stream in units of 100ns.
          appendEvent(event->pixel, event->tof / 10.0, pkt.pulseId());
          event = pkt.nextEvent();
        }
        g_log.debug() << "BankID " << pkt.curBankId() << " had " << eventsPerBank
                      << " events" << std::endl;
      }
      else
      {
        g_log.information() << "Ignoring Bank ID: " << pkt.curBankId() << std::endl;
      }
      bank = pkt.nextBank();
    }
    m_mutex.unlock();
    g_log.information() << "Total Events: " << totalEvents << std::endl;
    g_log.information() << "-------------------------------" << std::endl;

    return true;
  }

  bool SNSLiveEventDataListener::rxPacket( const ADARA::HeartbeatPkt &)
  {
    // We don't actually need anything out of the heartbeat packet - we just
    // need to know that it arrived (and thus the SMS is still online)
    m_heartbeat = Kernel::DateAndTime::getCurrentTime();
    return true;
  }


  bool SNSLiveEventDataListener::rxPacket( const ADARA::GeometryPkt &pkt)
  {
    m_heartbeat = Kernel::DateAndTime::getCurrentTime();

    // TODO: For now, I'm assuming that we only need to process one of these
    // packets the first time it comes in and we can ignore any others.
    if (m_workspaceInitialized == false)
    {
      m_instrumentXML = pkt.info();

      // if we also have the instrument name (from the run status packet), then
      // we can initialize our workspace
      if (m_instrumentName.size() > 0)
      {
        initWorkspace();
      }
    }

    return true;
  }

  bool SNSLiveEventDataListener::rxPacket( const ADARA::BeamlineInfoPkt &pkt)
  {
    m_heartbeat = Kernel::DateAndTime::getCurrentTime();

    // We only need to process a beamlineinfo packet once
    if (m_workspaceInitialized == false)
    {
      // We need the instrument name (we'll use the shortname);
      m_instrumentName = pkt.shortName();

      // If we've also got the XML definition (from the Geometry packet), then
      // we can create our workspace.  Otherwise, we'll just wait
      if (m_instrumentXML.size() > 0)
      {
        initWorkspace();
      }
    }

    return true;
  }

  void SNSLiveEventDataListener::initWorkspace()
  {
    // Use the LoadEmptyInstrument algorithm to create a proper workspace
    // for whatever beamline we're on

    m_buffer = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>
        (WorkspaceFactory::Instance().create("EventWorkspace", 1, 1, 1));
    // The numbers in the create() function don't matter

    boost::shared_ptr<Algorithm>loadInst = Mantid::API::AlgorithmManager::Instance().createUnmanaged( "LoadInstrument");
    loadInst->initialize();
    loadInst->setChild( true);  // keep the workspace out of the ADS
    loadInst->setProperty("InstrumentXML", m_instrumentXML);
    loadInst->setProperty("InstrumentName", m_instrumentName);
    loadInst->setProperty("Workspace", m_buffer);

    loadInst->execute();

    m_buffer->padSpectra();  // expands the workspace to the size of the just loaded instrument

    // Set the units
    m_buffer->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    m_buffer->setYUnit( "Counts");

    m_indexMap = m_buffer->getDetectorIDToWorkspaceIndexMap( true /* bool throwIfMultipleDets */ );

    m_workspaceInitialized = true;
  }

  void SNSLiveEventDataListener::appendEvent(uint32_t pixelId, double tof,
                                             const Mantid::Kernel::DateAndTime pulseTime)
  // NOTE: This function does NOT lock the mutex!  Make sure you do that
  // before calling this function!
  {
    // It'd be nice to use operator[], but we might end up inserting a value....
    // Have to use find() instead.
    detid2index_map::iterator it = m_indexMap->find( pixelId);
    if (it != m_indexMap->end())
    {
      std::size_t workspaceIndex = it->second;
      Mantid::DataObjects::TofEvent event( tof, pulseTime);
      m_buffer->getEventList( workspaceIndex).addEventQuickly( event);
    }
    else
    {
      g_log.warning() << "Invalid pixel ID: " << pixelId << " (TofF: " << tof
                      << " microseconds)" << std::endl;
    }
  }

  boost::shared_ptr<Workspace> SNSLiveEventDataListener::extractData()
  {

    // Block until the background thread has actually initialized the workspace
    // (Which won't happen until the SMS sends it the packet with the geometry
    // information in it.)
    // Note:  Yes, I discussed this with the other developers and they all agreed
    // that blocking was the proper behavior.  We can't return NULL, or some
    // invalid workspace.
    while (m_workspaceInitialized == false)
    {
      Poco::Thread::sleep( 100);  // 100 milliseconds
    }

    using namespace DataObjects;

    //Make a brand new EventWorkspace
    EventWorkspace_sptr temp = boost::dynamic_pointer_cast<EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", m_buffer->getNumberHistograms(), 2, 1));

    //Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(m_buffer, temp, false);

    // Get an exclusive lock
    m_mutex.lock();    
    std::swap(m_buffer, temp);
    m_mutex.unlock();

    return temp;
  }


  ILiveListener::RunStatus SNSLiveEventDataListener::runStatus()
  {
      // TODO: Implement this!!
      return ILiveListener::NoRun;
  }
  
  
} // namespace Mantid
} // namespace DataHandling
