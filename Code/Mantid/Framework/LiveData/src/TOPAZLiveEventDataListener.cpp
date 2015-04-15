#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidLiveData/TOPAZLiveEventDataListener.h"
#include "MantidLiveData/Exception.h"
//#include "MantidDataObjects/Events.h"
//#include "MantidKernel/DateAndTime.h"
//#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
//#include "MantidKernel/WriteLock.h"
#include "MantidDataObjects/EventWorkspace.h"

#include <Poco/Net/NetException.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>
//#include <Poco/Net/SocketStream.h>
//#include <Poco/Timestamp.h>

// Includes for parsing the XML device descriptions
//#include "Poco/DOM/DOMParser.h"
//#include "Poco/DOM/Document.h"
//#include "Poco/DOM/AutoPtr.h"
//#include "Poco/DOM/NodeList.h"
//#include "Poco/DOM/NamedNodeMap.h"

//#include <Poco/Thread.h>
//#include <Poco/Runnable.h>

//#include <time.h>
//#include <sstream> // for ostringstream
#include <string>
#include <fstream>
#include <exception>

using namespace Mantid::Kernel;
using namespace Mantid::API;

#if 0
// Port numbers
// Basically, we're going to mimic the network protocol that iSawEV uses,
// since that's what the event_catcher util on TOPAZ knows to output
#define ISAW_EV_PORT            9000
#define TEST_ISAW_EV_PORT       9100

// Not sure we'll need these
#define ISAW_EV_DATA_PORT       9012
#define ISAW_EV_CMD_PORT        9013
#define TEST_ISAW_EV_DATA_PORT  9112
#define TEST_ISAW_EV_CMD_PORT   9113
#endif

// Time we'll wait on a receive call (in seconds)
// Also used when shutting down the thread so we know how long to wait there
#define RECV_TIMEOUT 30

// Names for a couple of time series properties
// TODO: Do we need the scan and pause properties?
//#define PAUSE_PROPERTY "pause"
//#define SCAN_PROPERTY "scan_index"
#define PROTON_CHARGE_PROPERTY "proton_charge"


// Data structures for the UDP packet that we receive.
// These were copied from the das_proto.h file in the LiveStreaming
// subversion repository
struct socket_header_struct
{
    int32_t iReceiveID;     // unique identifier for message, sequence #...
    int32_t iCommandID;     // determines kind of packets to come...
    int32_t iTotalBytes;    // # of total bytes in payload!
    int32_t Spare1;         // size of PULSE_ID payload in bytes...
    int32_t Spare2;         // just a spare
    int32_t Spare3;         // detector bank id
};

typedef struct socket_header_struct COMMAND_HEADER, *COMMAND_HEADER_PTR;

struct pulse_id_struct
{
    uint32_t pulseIDlow;
    uint32_t pulseIDhigh;
    uint64_t eventID;
    double charge;   // TODO: this *might* be in picoCoulombs, or it
                     // might be units of 10 picoCoulombs...
};

typedef struct pulse_id_struct PULSE_ID, *PULSE_ID_PTR;

struct neutron_event_struct
{
    uint32_t tof;     // Time-of-flight (we *think* units are 100ns -
                      // divide by 10 to get microseconds)
    uint32_t pixelId;
};


typedef struct neutron_event_struct NEUTRON_EVENT, *NEUTRON_EVENT_PTR;


/******************************************************************************
 * UDP Packet structure
 * 
 * From what I've been able to deduce so far, this is how the UDP packets
 * are organized:
 * 
 *      ---------------------------------
 *      |     socket_header_struct      |
 *      |-------------------------------|
 *      |     first pulse_id_struct     |                      
 *      |-------------------------------|
 *      |              ...              |
 *      |-------------------------------|
 *      |     n'th pulse_id_struct      |
 *      |-------------------------------|
 *      |   first neutron_event_struct  |
 *      |-------------------------------|
 *      |              ...              |
 *      |-------------------------------|
 *      |   n'th neutron_event_struct   |
 *      --------------------------------|
 * 
 * There will be one or more neutron event structs.  Excatly how many there
 * are is determined from socket_header_struct.Spare1.  This is the size in
 * bytes occupied by pulse_id_structs.  Dividing by sizeof(pulse_id_struct)
 * gives the number of structs.
 * 
 * The remaining data in the packet is neutron_event_structs.  
 * socket_header_struct.iTotalBytes - socket_header_struct.Spare1 is the size
 * of the event data and dividing by sizeof(neutron_event_struct) gives the
 * number of events.
 * 
 * If you treat the event data in the packet as an array of
 * neutron_event_structs, then the eventID value in each pulse_id_struct is
 * the index into that array of the first event associated with that pulse.
 * 
 * Note that socket_header_struct.iTotalBytes does NOT include the size of
 * the socket_header_struct itself.  Thus the total packet size is 
 * sizeof(socket_header_struct) + socket_header_struct.iTotalBytes.
 ****************************************************************************/


// Helper function to get a DateAndTime value from a pulse_id_struct
Mantid::Kernel::DateAndTime timeFromPulse(const pulse_id_struct *p) {
  uint32_t seconds = p->pulseIDhigh;
  uint32_t nanoseconds = p->pulseIDlow;

  // Make sure we pick the correct constructor (the Mac gets an ambiguous error)
  return DateAndTime(static_cast<int64_t>(seconds),
                     static_cast<int64_t>(nanoseconds));
}


namespace Mantid {
namespace LiveData {
DECLARE_LISTENER(TOPAZLiveEventDataListener)

namespace {
/// static logger
Kernel::Logger g_log("SNSLiveEventDataListener");
}

/// Constructor
TOPAZLiveEventDataListener::TOPAZLiveEventDataListener()
    : ILiveListener(), m_status(NoRun), m_udpBufSize(32768),
      m_runNumber(0),m_stopThread(false)
{

    m_udpBuf = new unsigned char[m_udpBufSize];
    
    // Note: not doing much actual initialization here.  For reasons that
    // are unclear, this object may get created and destroyed several times
    // in the process of opening up the StartLiveData dialog box.  As such
    // we really want the constructor to be as quick as possible.  Therefore,
    // most of the initialization is done at the top of the run() function.
}

/// Destructor
TOPAZLiveEventDataListener::~TOPAZLiveEventDataListener()
{
    
    if (m_thread.isRunning())
    {
        // Ask the thread to exit (and hope that it does - Poco doesn't
        // seem to have an equivalent to pthread_cancel
        m_stopThread = true;
        try {
            m_thread.join(RECV_TIMEOUT * 2 * 1000); 
            // *1000 because join() wants time in milliseconds
        } catch (Poco::TimeoutException &) {
            // And just what do we do here?!?
            // Log a message, sure, but other than that we can either hang the
            // Mantid process waiting for a thread that will apparently never
            // exit or segfault because the thread is going to try to write to
            // a buffer that's about to be deleted.
            // Chose segfault - at least that's obvious.
            g_log.fatal() << "TOPAZLiveEventDataListener failed to shut down "
                << "its background thread! This should never happen and "
                << "Mantid is pretty much guaranteed to crash shortly.  "
                << "Talk to the Mantid developer team." << std::endl;
        }
    }
    
    delete[] m_udpBuf;
}

/// Connect to the TOPAZ event_catcher util (which will supply us with
/// events).
/// @param address The address to attempt to connect to
/// @return Returns true if the connection succeeds.  False otherwise.
bool TOPAZLiveEventDataListener::connect(
    const Poco::Net::SocketAddress &address)
// The SocketAddress class will throw various exceptions if it encounters an
// error.  We're assuming the calling function will catch any exceptions that
// are important
// Note: Right now, it's the factory class that actually calls connect(), and
// it doesn't check the return value.  (It does, however, trap the Poco
// exceptions.)
{
    bool rv = false; // assume failure

    try {
        m_tcpSocket.connect(address); // BLOCKING connect
    } catch (...) {
        g_log.error() << "Connection to " << address.toString() << " failed."
                    << std::endl;
        return false;
    }
    
    Poco::Net::SocketAddress tcpAddress = m_tcpSocket.address();
    
    m_tcpSocket.setReceiveTimeout(Poco::Timespan(RECV_TIMEOUT, 0));
    // POCO timespan is seconds, microseconds
    
    g_log.debug() << "Connected to " << tcpAddress.toString()
                  << std::endl;
                
    // After connecting to the main port (either 9000 or 9100 depending on
    // whether or not we're in test mode), event_catcher will send us the port
    // number it's using for the event data.
    uint16_t dataPort;
    try {
        if (m_tcpSocket.receiveBytes(&dataPort, sizeof(dataPort)) != 
            sizeof(dataPort))
        {
                g_log.error() << "Failed to read entire data port num from " 
                        << tcpAddress.toString() << std::endl;
            return false;
        }
    } catch (...) {
        g_log.error() << "Error reading data port num from " 
                      << tcpAddress.toString() << std::endl;
        return false;
    }
    
    // Note: No, there's no byte-swapping done on dataPort.  It appears that
    // event_catcher does no byte swapping on any of its sends, so we can't
    // do any here.  Since event_catcher pretty much has to run on the same
    // machine as this code, endianness shouldn't actually be a problem.
    
    // Set up the socket address we'll use in the receiveFrom() calls
    // Oddly:  the only way to set the values on a SocketAddress is with
    // operator= or the constructor...
    // TODO: For now, we have to hard-code the address as localhost because
    // that's what event-catcher sends to.  It currently isn't smart enough
    // to send UDP packets back to the address that actually connected to
    // the TCP port...
    m_dataAddr = Poco::Net::SocketAddress(
        Poco::Net::IPAddress( "127.0.0.1"), dataPort);
       
    rv = m_isConnected = true;
    // Note: we leave m_tcpSocket connected.  Otherwise, event_catcher stops
    // sending events to us.
    return rv;
}

/// Test to see if the object has connected to the DAS

/// Test to see if the object has connected to the DAS
/// @return Returns true if connected.  False otherwise.
bool TOPAZLiveEventDataListener::isConnected() { return m_isConnected; }


/// Start the background thread

/// Starts the background thread which reads data from the network, parses
/// it and stores the resulting events in a temporary workspace.
/// @param startTime Ignored.  This class doesn't have the capability to
/// replay historical data.
void TOPAZLiveEventDataListener::start(Kernel::DateAndTime startTime)
{
    (void)startTime; // Keep the compiler from complaining about unsed variable
    
    
    // Initialize the workspace
    // NOTE: initWorkspace() may take several seconds to complete.  Most of the
    // time seems to be spent loading the workspace.
    initWorkspace();
    initMonitorWorkspace();
    m_workspaceInitialized = true;
    
    
    m_thread.start(*this);
}


/// The main function for the background thread

/// Loops until the forground thread requests it to stop.  Reads data from the
/// network, parses it and stores the resulting events (and other metadata) in
/// a temporary workspace.
void TOPAZLiveEventDataListener::run() {
  try {
  
    if (m_isConnected == false) // sanity check
    {
      throw std::runtime_error(std::string(
          "TOPAZLiveEventDataListener::run(): No connection to event_catcher."));
    }   
    
    m_dataSocket.bind(m_dataAddr);
    m_dataSocket.setReceiveTimeout(Poco::Timespan(
      RECV_TIMEOUT, 0)); // POCO timespan is seconds, microseconds
    
    Poco::Net::SocketAddress sendAddr;  // address of the sender
    // loop until the foreground thread tells us to stop
    while (m_stopThread == false) 
    {
        // it's possible that a stop request came in while we were sleeping...
        if (m_stopThread) {   
            break;
        }
        
        int bytesRead = 0;
        try {
          bytesRead = m_dataSocket.receiveFrom(m_udpBuf, m_udpBufSize, sendAddr);
        } catch (Poco::TimeoutException &) {
          if (m_stopThread == false) {
          // Don't need to stop processing or anything - just log a warning
          g_log.warning( "Timeout reading from the network.  "
                         "Is event_catcher still sending?");
          }
          
          continue;  // don't process the data in the buffer (since
                     // it's incomplete or otherwise bad)
                     
        } catch (Poco::Net::NetException &e) {
          std::string msg("m_dataSocket::receiveBytes(): ");
          msg += e.name();
          throw std::runtime_error(msg);
        }
        
        // If I understand things correctly, the data sitting in m_udpBuf will
        // be organized as a socket_header_struct, followed by one or more
        // pulse_id_structs, followed by zero or more neutron_event_structs.
        //
        // Lets start with some basic decoding and logging...
        COMMAND_HEADER_PTR hdr = (COMMAND_HEADER_PTR)m_udpBuf;
        unsigned long num_pulse_ids = hdr->Spare1 / sizeof(PULSE_ID);
        unsigned long num_events = (hdr->iTotalBytes - hdr->Spare1) /
                              sizeof(NEUTRON_EVENT);
        
                              
        g_log.debug() << "Received UDP Packet.  " << bytesRead << " bytes  "
                      << num_pulse_ids << " pulses  " << num_events
                      << " events\n" << std::endl;
        
        PULSE_ID_PTR pid = (PULSE_ID_PTR)(m_udpBuf + sizeof(COMMAND_HEADER));
        NEUTRON_EVENT_PTR events =
            (NEUTRON_EVENT_PTR)(m_udpBuf + sizeof(COMMAND_HEADER) +
                                (num_pulse_ids * sizeof(PULSE_ID)));
        
        for (unsigned i = 0; i < num_pulse_ids; i++)
        {
            g_log.debug() << "Pulse ID: " << pid[i].pulseIDhigh << ", " 
                        << pid[i].pulseIDlow << std::endl;
            g_log.debug() << "  Event ID: " << pid[i].eventID << std::endl;
            g_log.debug() << "  Charge: " << pid[i].charge << std::endl;
            
            // Figure out the event indexes that belong to this pulse
            unsigned long firstEvent = pid[i].eventID;
            unsigned long lastEvent;
            if (i == num_pulse_ids - 1) // last pulse in the packet?
            {
                lastEvent = num_events - 1;
            }
            else
            {
                if (firstEvent == pid[i+1].eventID)
                {
                    // this pulse had no events
                    continue;
                }
                lastEvent = pid[i+1].eventID - 1;
            }
            
            // Timestamp for the events
            Mantid::Kernel::DateAndTime eventTime = timeFromPulse(&pid[i]);
            
            Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);
            for (unsigned long j = firstEvent; j <= lastEvent; j++)
            {
                // Save the pulse charge in the logs
                // TODO:  We're not sure what the units are on the charge value
                // They *might* be picoCoulombs, or the might be units of 10pC
                // (in which case we need to multiply by 10 because the
                // property is definitely in picoCoulombs.)
                m_eventBuffer->mutableRun()
                    .getTimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY)
                    ->addValue(eventTime, pid[i].charge);
                
                // appendEvent needs tof to be in units of microseconds, but
                // it comes from the ADARA stream in units of 100ns.
                appendEvent(events[j].pixelId, events[j].tof / 10.0, eventTime);
            }
        } // TODO: Verify that the mutex is unlocked when we go back to 
          // the top of the for i loop        
    }
      
      

      
    // If we've gotten here, it's because the thread has thrown an otherwise
    // uncaught exception.  In such a case, the thread will exit and there's
    // nothing we can do about that.  We'll log an error and save a copy of
    // the exception object so that we can re-throw it from the foreground
    // thread (which will cause the algorithm to exit).
    // NOTE: For the default exception handler, we actually create a new
    // runtime_error object and throw that, since there's no exception object
    // passed in to the handler.
  } catch (std::runtime_error &e) {
    // exception handler for generic runtime exceptions
    g_log.fatal() << "Caught a runtime exception.\n"
                  << "Exception message: " << e.what() << ".\n"
                  << "Thread will exit."  << std::endl;
    m_isConnected = false;

    m_backgroundException =
        boost::shared_ptr<std::runtime_error>(new std::runtime_error(e));

  } catch (std::invalid_argument &e) {
    // TimeSeriesProperty (and possibly some other things) can throw
    // these errors
    g_log.fatal() << "Caught an invalid argument exception.\n"
                  << "Exception message: " << e.what() << ".\n"
                  << "Thread will exit." << std::endl;
    m_isConnected = false;
    
    std::string newMsg(
        "Invalid argument exception thrown from the background thread: ");
    newMsg += e.what();
    m_backgroundException =
        boost::shared_ptr<std::runtime_error>(new std::runtime_error(newMsg));
  } catch (Poco::Exception &e) { // Generic POCO exception handler
    g_log.fatal( "Uncaught POCO exception in TOPAZLiveEventDataListener network "
        "read thread.");
    g_log.fatal( std::string("Exception message:" ) + e.displayText());
    g_log.fatal( "Thread is exiting.");
    m_isConnected = false;

    m_backgroundException = boost::shared_ptr<std::runtime_error>(
        new std::runtime_error("Unknown error in backgound thread"));
  } catch (...) { // Default exception handler
    g_log.fatal( "Uncaught exception in TOPAZLiveEventDataListener network "
        "read thread.  Thread is exiting.");
    m_isConnected = false;

    m_backgroundException = boost::shared_ptr<std::runtime_error>(
        new std::runtime_error("Unknown error in backgound thread"));
  }

  return;
}


/// Workspace initialization

/// Set up the internal workspace where we'll accumulate events
void TOPAZLiveEventDataListener::initWorkspace()
{
    m_eventBuffer = boost::static_pointer_cast<DataObjects::EventWorkspace>(
        WorkspaceFactory::Instance().create("EventWorkspace", 1, 1, 1));
    // The numbers in the create() function don't matter - they'll get overwritten
    // down in initWorkspacePart2() when we load the instrument definition.

    // Create the time series properties we'll need
    Property *prop = new TimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY);
    m_eventBuffer->mutableRun().addLogData(prop);
    
    // Use the LoadEmptyInstrument algorithm to create a proper workspace
    // for the TOPAZ beamline
    boost::shared_ptr<Algorithm> loadInst =
        Mantid::API::AlgorithmManager::Instance().createUnmanaged(
            "LoadInstrument");
    loadInst->initialize();
    loadInst->setChild(true); // keep the workspace out of the ADS
    //  loadInst->setProperty("InstrumentXML", m_instrumentXML);
    loadInst->setProperty("InstrumentName", "TOPAZ");
    loadInst->setProperty("Workspace", m_eventBuffer);

    loadInst->execute();

    m_eventBuffer->padSpectra(); // expands the workspace to the size of the just
                                // loaded instrument

    // Set the units
    m_eventBuffer->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
    m_eventBuffer->setYUnit("Counts");

    m_indexMap = m_eventBuffer->getDetectorIDToWorkspaceIndexMap(
        true /* bool throwIfMultipleDets */);
    
    // The DAS sends out data using "DAS Pixel ID's".  These might need to be
    // translated into "Logical Pixel ID's".  For TOPAZ, it's a 1:1 mapping,
    // so we're not going to bother.  If we add another beamline, though,
    // this is where we'd load in the pixel mapping file.
    
}


/// Creates a monitor workspace sized to the number of monitors, with the
/// monitor IDs set
void TOPAZLiveEventDataListener::initMonitorWorkspace() {
  auto monitors = m_eventBuffer->getInstrument()->getMonitors();
  auto monitorsBuffer = WorkspaceFactory::Instance().create(
      "EventWorkspace", monitors.size(), 1, 1);
  WorkspaceFactory::Instance().initializeFromParent(m_eventBuffer,
                                                    monitorsBuffer, true);
  // Set the id numbers
  for (size_t i = 0; i < monitors.size(); ++i) {
    monitorsBuffer->getSpectrum(i)->setDetectorID(monitors[i]);
  }

  m_monitorIndexMap = monitorsBuffer->getDetectorIDToWorkspaceIndexMap(true);

  m_eventBuffer->setMonitorWorkspace(monitorsBuffer);
}


/// Adds an event to the workspace
void TOPAZLiveEventDataListener::appendEvent(
    uint32_t pixelId, double tof, const Mantid::Kernel::DateAndTime pulseTime)
// NOTE: This function does NOT lock the mutex!  Make sure you do that
// before calling this function!
{   
    // It'd be nice to use operator[], but we might end up inserting a value....
    // Have to use find() instead.
    detid2index_map::iterator it = m_indexMap.find(pixelId);
    if (it != m_indexMap.end()) {
        std::size_t workspaceIndex = it->second;
        Mantid::DataObjects::TofEvent event(tof, pulseTime);
        m_eventBuffer->getEventList(workspaceIndex).addEventQuickly(event);
    } else {
        // TODO: do we want to disable this warning?  Most of the time, we
        // shouldn't have any invalid ID's, but if we do, we'll probably 
        // get a lot and flood the log with messages...
        //g_log.warning() << "Invalid pixel ID: " << pixelId << " (TofF: "
        //                << tof << " microseconds)" << std::endl;
    }
}

/// Retrieve buffered data

/// Called by the foreground thread to fetch data that's accumulated in
/// the temporary workspace.  The temporary workspace is left empty and
/// ready to receive more data.
/// @return shared pointer to a workspace containing the accumulated data
boost::shared_ptr<Workspace> TOPAZLiveEventDataListener::extractData() {

  // Check to see if the background thread has thrown an exception.  If so,
  // re-throw it here.
  if (m_backgroundException) {
    throw(*m_backgroundException);
  }

  // Sanity check - make sure the workspace has been initialized
  if (!m_workspaceInitialized) {
    throw std::runtime_error("TOPAZLiveEventDataListener:  "
                             "The workspace has not been initialized.");
  }

  using namespace DataObjects;

  // Make a brand new EventWorkspace
  EventWorkspace_sptr temp = boost::dynamic_pointer_cast<EventWorkspace>(
      API::WorkspaceFactory::Instance().create(
          "EventWorkspace", m_eventBuffer->getNumberHistograms(), 2, 1));

  // Copy geometry over.
  API::WorkspaceFactory::Instance().initializeFromParent(m_eventBuffer, temp,
                                                         false);

  // Clear out the old logs, except for the most recent entry
  temp->mutableRun().clearOutdatedTimeSeriesLogValues();

  // Clear out old monitor logs
  // TODO: At present, there's no way for monitor logs to be added
  // to m_monitorLogs.  Either implement this feature, or remove
  // m_monitorLogs!
  for (unsigned i = 0; i < m_monitorLogs.size(); i++) {
    temp->mutableRun().removeProperty(m_monitorLogs[i]);
  }
  m_monitorLogs.clear();

  // Create a fresh monitor workspace and insert into the new 'main' workspace
  auto monitorBuffer = m_eventBuffer->monitorWorkspace();
  auto newMonitorBuffer = WorkspaceFactory::Instance().create(
      "EventWorkspace", monitorBuffer->getNumberHistograms(), 1, 1);
  WorkspaceFactory::Instance().initializeFromParent(monitorBuffer,
                                                    newMonitorBuffer, false);
  temp->setMonitorWorkspace(newMonitorBuffer);

  // Lock the mutex and swap the workspaces
  {
    Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);
    std::swap(m_eventBuffer, temp);
  } // mutex automatically unlocks here

  return temp;
}

/// Check the status of the current run

/// Called by the foreground thread check the status of the current run
/// @returns Returns an enum indicating beginning of a run, in the middle
/// of a run, ending a run or not in a run.
ILiveListener::RunStatus TOPAZLiveEventDataListener::runStatus() {

  // First up, check to see if the background thread has thrown an
  // exception.  If so, re-throw it here.
  if (m_backgroundException) {
    throw(*m_backgroundException);
  }

#if 0
  // Need to protect against m_status and m_deferredRunDetailsPkt
  // getting out of sync in the (currently only one) case where the
  // background thread has not been paused...
  Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);

  // The MonitorLiveData algorithm calls this function *after* the call to
  // extract data, which means the value we return should reflect the
  // value that's appropriate for the events that were returned when
  // extractData was called().
  ILiveListener::RunStatus rv = m_status;


  return rv;
#endif
  // until we figure out how to get run info from the stream, this is
  // all we can do
  return ILiveListener::RunStatus::NoRun;
}



} // namespace LiveData
} // namespace Mantid
