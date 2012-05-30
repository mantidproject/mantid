#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataHandling/ADARAParser.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidDataHandling/SNSLiveEventDataListener.h"
#include "MantidDataObjects/Events.h"
#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/WriteLock.h"

#include <Poco/Net/NetException.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketStream.h>

#include <errno.h>
#include <sys/time.h> // for gettimeofday()
#include <unistd.h>
#include <fcntl.h>

#include <sstream>    // for ostringstream
#include <string>
#include <exception>

#include <Poco/Thread.h>
#include <Poco/Runnable.h>

using namespace Mantid::Kernel;
using namespace Mantid::API;


namespace Mantid
{
namespace DataHandling
{
  DECLARE_LISTENER(SNSLiveEventDataListener)
  ;
  // The DECLARE_LISTENER macro seems to confuse some editors' syntax checking.  The
  // semi-colon limits the complaints to one line.  It has no actual effect on the code.

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
  }

  /************************************************
  bool SNSLiveEventDataListener::connect(const Poco::Net::SocketAddress& address)
  {
    bool rv = false;  // assume failure

    // <sigh>  Have to do this the hard way because Dave D.'s packet parser wants a file
    // descriptor and POCO won't give me one for its StreamSocket....
    struct addrinfo hints;
    memset( &hints, 0, sizeof( hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = address.family();

    std::ostringstream port;
    port << address.port();
    std::string ip_addr = address.host().toString();

    struct addrinfo *infoList, *oneInfo;
    getaddrinfo( ip_addr.c_str(), port.str().c_str(), &hints, &infoList);
    oneInfo = infoList;
    while ( oneInfo != NULL)
    {
      m_sockfd = socket( oneInfo->ai_family, oneInfo->ai_socktype, oneInfo->ai_protocol);
      if ( m_sockfd == -1)
      {
        oneInfo = oneInfo->ai_next;
      }
      else
      {
        if (::connect(m_sockfd, oneInfo->ai_addr, oneInfo->ai_addrlen) == -1)
        {
          close( m_sockfd);
          m_sockfd = -1;
          oneInfo = oneInfo->ai_next;
        }
        else
        {
          // Success!  Set oneInfo to null so we break out of the while loop
          oneInfo = NULL;

          rv = m_isConnected = true;
        }
      }
    }
    ***********************************************************/


  bool SNSLiveEventDataListener::connect(const Poco::Net::SocketAddress& address)
  // The SocketAddress class will throw various exceptions if it encounters an error
  // We're assuming the calling function will catch any exceptions that are important
  // Note: Right now, it's the factory class that actually calls connect(), and it
  // doesn't check the return value.  (It does, however, trap the Poco exceptions.)
  {
    bool rv = false;  // assume failure

    m_socket.connect( address);  // BLOCKING connect
    //m_socket.connectNB( address);  // NON-BLOCKING connect
    m_socket.setReceiveTimeout( Poco::Timespan( 0, 100 * 1000));  // 100 millisec timeout

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
    struct timeval now;
    gettimeofday( &now, NULL);
    helloPkt[2] = (uint32_t)(now.tv_sec - ADARA::EPICS_EPOCH_OFFSET);
    helloPkt[3] = (uint32_t)now.tv_usec * 1000;
    helloPkt[4] = (uint32_t)(m_startTime.totalNanoseconds() / 1000000000);  // divide by a billion to get time in seconds

    if ( m_socket.sendBytes(helloPkt, sizeof(helloPkt)) != sizeof(helloPkt))
    //if ( send( m_sockfd, helloPkt, sizeof(helloPkt), 0) != sizeof(helloPkt))
    {
      std::string msg( "SNSLiveEventDataListener::run(): Failed to send client hello packet.  Err: ");
      msg += strerror(errno);
      throw std::runtime_error( msg);
    }

#if 0
    ////////////////////////////////////////////////////////////////////////////////
    //This is the code for creating an empty workspace.  It'll probably
    //go into the rxPacket function for whatever packet has the instrument
    //definition data in it.

    // Use the LoadEmptyInstrument algorithm to create a proper workspace
    // for whatever beamline we're on
    // Note:  this assumes we got the instrument name from the SMS in
    // response to the helloclient packet we sent.  See connect()

    LoadEmptyInstrument loaderLiveView;
    loaderLiveView.initialize();

    //assert( loaderSLS.isInitialized() );


    std::string definitionFile;
    // We need the name of the instrument definition file.  It's probably
    // something along the lines of "<Instrument Name>_Definition.xml"
    // and I'm supposed to get the instrument name from SMS...
    //
    // UPDATE: Dave says SMS will send the entire IDF file, not just an
    // instrument name.  For now, I'll probably write it to a temporary file
    // and use the loader's Filename property.  In the future, it'd probably
    // be better to be able to pass the entire IDF (presumably as a string)
    // straight into the loader...

    loaderLiveView.setPropertyValue("Filename", definitionFile);
    m_wsName = "LiveViewWS";
    loaderLiveView.setPropertyValue("OutputWorkspace", m_wsName);

    loaderLiveView.execute();

    //assert( loaderSLS.isExecuted() );

    m_buffer = AnalysisDataService::Instance().retrieveWS<DataObjects::EventWorkspace>(wsName);
    m_indexMap = getDetectorIDToWorkspaceIndexMap( true /* bool throwIfMultipleDets */ );
    m_workspaceInitialized = true;
    ///////////////////////////////////////////////////////////////////////////////////
#endif


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
        // TODO: HOW?!?
      }
    }

    return;
  }


  bool SNSLiveEventDataListener::rxPacket( const ADARA::RTDLPkt &pkt)
  {
    // At the moment, all we need from the RTDL packets is the pulse
    // time and the raw flag.  (We reference them when processing the
    // banked event packets.)
    m_rtdlPulseTime = pkt.timestamp();
    m_rtdlRawFlag = pkt.rawTOF();

    return true;
  }

  // Note:  Before we can process a particular BankedEventPkt, we must have
  // received the RTDLPkt for pulse ID specified in the BankedEventPkt.
  // Normally this won't be an issue since the SMS will send out RTDLPkts
  // before it sends BankedEventPkts.
  bool SNSLiveEventDataListener::rxPacket( const ADARA::BankedEventPkt &pkt)
  {

    // First step - make sure the RTDL packet we've saved matches the
    // banked event packet we've just received and  make sure its RAW flag
    // is false
    if (pkt.compare_timestamp( m_rtdlPulseTime) == false)
    {
      // Wrong RTDL packet.  Fail!
      // TODO: Should we set an error message somewhere?
      return false;
    }

    if (m_rtdlRawFlag)
    {
      // As yet, we can't handle raw TofF values.
      // TODO: Should we set an error message somewhere?
      return false;
    }

    // Create a log with the pulse time and charge!!  (TimeSeriesProperties)

    // Next convert the Pulse ID fields into a DateAndTime object
    const struct timespec &pulseTime = pkt.timestamp();
    Mantid::Kernel::DateAndTime pulseID( pulseTime.tv_sec, pulseTime.tv_nsec);

    m_mutex.lock();
    // Append the events
        // Iterate through bank sections
        // Note: We ignore bank ID's >= 0xFFFE  (-2 is for error pixels and -1 is for
        // unmappable pixels);
            // Iterate through each event
            //appendEvent( )
    m_mutex.unlock();

    return false; // TODO: implement me!
  }

  void SNSLiveEventDataListener::appendEvent(uint32_t pixelId, double tof,
                                             const Mantid::Kernel::DateAndTime pulseTime)
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
  }

  boost::shared_ptr<MatrixWorkspace> SNSLiveEventDataListener::extractData()
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

    // Create a new, empty workspace of the same dimensions and assign to the
    // buffer variable  

    //Make a brand new EventWorkspace
    // TODO: Think about whether creating a new workspace at this point is scalable
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
