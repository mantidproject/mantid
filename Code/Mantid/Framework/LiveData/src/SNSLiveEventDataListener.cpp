#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidLiveData/SNSLiveEventDataListener.h"
#include "MantidLiveData/Exception.h"
#include "MantidDataObjects/Events.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/WriteLock.h"

#include <Poco/Net/NetException.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/SocketStream.h>
#include <Poco/Timestamp.h>

// Includes for parsing the XML device descriptions
#include "Poco/DOM/DOMParser.h"
#include "Poco/DOM/Document.h"
#include "Poco/DOM/AutoPtr.h"
#include "Poco/DOM/NodeList.h"
#include "Poco/DOM/NamedNodeMap.h"

#include <Poco/Thread.h>
#include <Poco/Runnable.h>

#include <time.h>
#include <sstream> // for ostringstream
#include <string>
#include <exception>

using namespace Mantid::Kernel;
using namespace Mantid::API;

// Time we'll wait on a receive call (in seconds)
// Also used when shutting down the thread so we know how long to wait there
#define RECV_TIMEOUT 30

// Names for a couple of time series properties
#define PAUSE_PROPERTY "pause"
#define SCAN_PROPERTY "scan_index"
#define PROTON_CHARGE_PROPERTY "proton_charge"

// Helper function to get a DateAndTime value from an ADARA packet header
Mantid::Kernel::DateAndTime timeFromPacket(const ADARA::PacketHeader &hdr) {
  uint32_t seconds = (uint32_t)(hdr.pulseId() >> 32);
  uint32_t nanoseconds = hdr.pulseId() & 0xFFFFFFFF;

  // Make sure we pick the correct constructor (the Mac gets an ambiguous error)
  return DateAndTime(static_cast<int64_t>(seconds),
                     static_cast<int64_t>(nanoseconds));
}

namespace Mantid {
namespace LiveData {
DECLARE_LISTENER(SNSLiveEventDataListener);
// The DECLARE_LISTENER macro seems to confuse some editors' syntax checking.
// The
// semi-colon limits the complaints to one line.  It has no actual effect on the
// code.

namespace {
/// static logger
Kernel::Logger g_log("SNSLiveEventDataListener");
}

/// Constructor
SNSLiveEventDataListener::SNSLiveEventDataListener()
    : ILiveListener(), ADARA::Parser(), m_status(NoRun), m_runNumber(0),
      m_workspaceInitialized(false), m_socket(), m_isConnected(false),
      m_pauseNetRead(false), m_stopThread(false), m_runPaused(false),
      m_ignorePackets(true), m_filterUntilRunStart(false)
// ADARA::Parser() will accept values for buffer size and max packet size, but
// the
// defaults will work fine
{

  // Perform all the workspace initialization steps (including actually creating
  // the workspace) that need to happen prior to receiving any packets.
  initWorkspacePart1();

  // Initialize m_keepPausedEvents from the config file.
  // NOTE: To the best of my knowledge, the existence of this property is not
  // documented
  // anywhere and this lack of documentation is deliberate.
  if (!ConfigService::Instance().getValue("livelistener.keeppausedevents",
                                          m_keepPausedEvents)) {
    // If the property hasn't been set, assume false
    m_keepPausedEvents = 0;
  }
}

/// Destructor
SNSLiveEventDataListener::~SNSLiveEventDataListener() {
  // Stop the background thread
  if (m_thread.isRunning()) {
    // Ask the thread to exit (and hope that it does - Poco doesn't
    // seem to have an equivalent to pthread_cancel
    m_stopThread = true;
    try {
      m_thread.join(RECV_TIMEOUT * 2 *
                    1000); // *1000 because join() wants time in milliseconds
    } catch (Poco::TimeoutException &) {
      // And just what do we do here?!?
      // Log a message, sure, but other than that we can either hang the
      // Mantid process waiting for a thread that will apparently never exit
      // or segfault because the ADARA::read() is going to try to write to
      // a buffer that's going to be deleted.
      // Chose segfault - at least that's obvious.
      g_log.fatal() << "SNSLiveEventDataListener failed to shut down its "
                       "background thread! "
                    << "This should never happen and Mantid is pretty much "
                       "guaranteed to crash shortly.  "
                    << "Talk to the Mantid developer team." << std::endl;
    }
  }
}

/// Connect to the SMS daemon.

/// Attempts to connect to the SMS daemon at the specified address.  Note:
/// if the address is '0.0.0.0', it looks on localhost:31415 (useful for
/// debugging and testing).
/// @param address The address to attempt to connect to
/// @return Returns true if the connection succeeds.  False otherwise.
bool SNSLiveEventDataListener::connect(const Poco::Net::SocketAddress &address)
// The SocketAddress class will throw various exceptions if it encounters an
// error
// We're assuming the calling function will catch any exceptions that are
// important
// Note: Right now, it's the factory class that actually calls connect(), and it
// doesn't check the return value.  (It does, however, trap the Poco
// exceptions.)
{
  bool rv = false; // assume failure

  // If we don't have an address, force a connection to the test server running
  // on
  // localhost on the default port
  if (address.host().toString().compare("0.0.0.0") == 0) {
    Poco::Net::SocketAddress tempAddress("localhost:31415");
    try {
      m_socket.connect(tempAddress); // BLOCKING connect
    } catch (...) {
      g_log.error() << "Connection to " << tempAddress.toString() << " failed."
                    << std::endl;
      return false;
    }
  } else {
    try {
      m_socket.connect(address); // BLOCKING connect
    } catch (...) {
      g_log.debug() << "Connection to " << address.toString() << " failed."
                    << std::endl;
      return false;
    }
  }

  m_socket.setReceiveTimeout(Poco::Timespan(
      RECV_TIMEOUT, 0)); // POCO timespan is seconds, microseconds
  g_log.debug() << "Connected to " << m_socket.address().toString()
                << std::endl;

  rv = m_isConnected = true;
  return rv;
}

/// Test to see if the object has connected to the SMS daemon

/// Test to see if the object has connected to the SMS daemon
/// @return Returns true if connected.  False otherwise.
bool SNSLiveEventDataListener::isConnected() { return m_isConnected; }

/// Start the background thread

/// Starts the background thread which reads data from the network, parses it
/// and
/// stores the resulting events in a temporary workspace.
/// @param startTime Specifies how much historical data the SMS should send
/// before continuing
/// the current 'live' data.  Use 0 to indicate no historical data.
void SNSLiveEventDataListener::start(Kernel::DateAndTime startTime) {
  // Save the startTime and kick off the background thread
  // (Can't really do anything else until we send the hello packet and the SMS
  // sends us
  // back the various metadata packets
  m_startTime = startTime;

  if (m_startTime.totalNanoseconds() == 1000000000) {
    // 1 billion nanoseconds - ie: 1 second past the EPOCH
    // Start live listener sends us this when it wants to start processing
    // at the start of the previous run (and it doesn't know when the previous
    // run started).  This value for a start time will cause the SMS to replay
    // all of its historical data and it will be up to us to filter out
    // everything
    // before the start of the previous run.
    // See the description of the 'Client Hello' packet in the SNS DAS design
    // doc
    // for more details
    m_filterUntilRunStart = true;
  }
  m_thread.start(*this);
}

/// The main function for the background thread

/// Loops until the forground thread requests it to stop.  Reads data from the
/// network,
/// parses it and stores the resulting events (and other metadata) in a
/// temporary
/// workspace.
void SNSLiveEventDataListener::run() {
  try {
    if (m_isConnected == false) // sanity check
    {
      throw std::runtime_error(std::string(
          "SNSLiveEventDataListener::run(): No connection to SMS server."));
    }

    // First thing to do is send a hello packet
    uint32_t helloPkt[5] = {4, ADARA::PacketType::CLIENT_HELLO_V0, 0, 0, 0};
    Poco::Timestamp now;
    uint32_t now_usec = (uint32_t)(now.epochMicroseconds() - now.epochTime());
    helloPkt[2] = (uint32_t)(now.epochTime() - ADARA::EPICS_EPOCH_OFFSET);
    helloPkt[3] = (uint32_t)now_usec * 1000;
    helloPkt[4] =
        (uint32_t)(m_startTime.totalNanoseconds() /
                   1000000000); // divide by a billion to get time in seconds

    if (m_socket.sendBytes(helloPkt, sizeof(helloPkt)) != sizeof(helloPkt))
    // Yes, I know a send isn't guaranteed to send the whole buffer in one call.
    // I'm treating such a case as an error anyway.
    {
      g_log.error("SNSLiveEventDataListener::run(): Failed to send client "
                  "hello packet. Thread exiting.");
      m_stopThread = true;
    }

    while (m_stopThread ==
           false) // loop until the foreground thread tells us to stop
    {

      while (m_pauseNetRead && m_stopThread == false) {
        // foreground thread doesn't want us to process any more packets until
        // it's ready.  See comments in rxPacket( const ADARA::RunStatusPkt
        // &pkt)
        Poco::Thread::sleep(100); // 100 milliseconds
      }

      if (m_stopThread) {
        // it's possible that a stop request came in while we were sleeping...
        break;
      }

      // Get some more data from our socket and put it in the parser's buffer
      unsigned int bufFillLen = bufferFillLength();
      if (bufFillLen) {
        uint8_t *bufFillAddr = bufferFillAddress();
        int bytesRead = 0;
        try {
          bytesRead = m_socket.receiveBytes(bufFillAddr, bufFillLen);
        } catch (Poco::TimeoutException &) {
          // Don't need to stop processing or anything - just log a warning
          g_log.warning(
              "Timeout reading from the network.  Is SMS still sending?");
        } catch (Poco::Net::NetException &e) {
          std::string msg("Parser::read(): ");
          msg += e.name();
          throw std::runtime_error(msg);
        }

        if (bytesRead > 0) {
          bufferBytesAppended(bytesRead);
        }
      }
      int packetsParsed = bufferParse();
      if (packetsParsed == 0) {
        // No packets were parsed.  Sleep a little to let some data accumulate
        // before calling read again.  (Keeps us from spinlocking the cpu...)
        Poco::Thread::sleep(10); // 10 milliseconds
      }
    }

    // If we've gotten here, it's because the thread has thrown an otherwise
    // uncaught exception.  In such a case, the thread will exit and there's
    // nothing we can do about that.  We'll log an error and save a copy of the
    // exception object so that we can re-throw it from the foreground thread
    // (which
    // will cause the algorithm to exit).
    // NOTE: For the default exception handler, we actually create a new
    // runtime_error
    // object and throw that, since there's no exception object passed in to the
    // handler.
  } catch (ADARA::invalid_packet &e) { // exception handler for invalid packets
    // For now, log it and let the thread exit.  In the future, we might
    // try to recover from this.  (A bad event packet could probably just
    // be ignored, for example)
    g_log.fatal() << "Caught an invalid packet exception in "
                     "SNSLiveEventDataListener network read thread.\n"
                  << "Exception message is: " << e.what() << ".\n"
                  << "Thread is exiting.\n";

    m_isConnected = false;

    m_backgroundException =
        boost::shared_ptr<std::runtime_error>(new ADARA::invalid_packet(e));

  } catch (std::runtime_error &
               e) { // exception handler for generic runtime exceptions
    g_log.fatal() << "Caught a runtime exception.\n"
                  << "Exception message: " << e.what() << ".\n"
                  << "Thread will exit.\n";
    m_isConnected = false;

    m_backgroundException =
        boost::shared_ptr<std::runtime_error>(new std::runtime_error(e));

  } catch (std::invalid_argument &
               e) { // TimeSeriesProperty (and possibly some other things) can
                    // can throw these errors
    g_log.fatal() << "Caught an invalid argument exception.\n"
                  << "Exception message: " << e.what() << ".\n"
                  << "Thread will exit.\n";
    m_isConnected = false;
    m_workspaceInitialized = true; // see the comments in the default exception
                                   // handler for why we set this value.
    std::string newMsg(
        "Invalid argument exception thrown from the background thread: ");
    newMsg += e.what();
    m_backgroundException =
        boost::shared_ptr<std::runtime_error>(new std::runtime_error(newMsg));

  } catch (...) { // Default exception handler
    g_log.fatal(
        "Uncaught exception in SNSLiveEventDataListener network read thread."
        " Thread is exiting.");
    m_isConnected = false;

    m_backgroundException = boost::shared_ptr<std::runtime_error>(
        new std::runtime_error("Unknown error in backgound thread"));
  }

  return;
}

/// Parse a banked event packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::BankedEventPkt packets.  Parsed events are stored
/// in the temporary workspace until the forground thread retrieves
/// them.
/// @see extractData()
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::BankedEventPkt &pkt) {
  // Check to see if we should process this packet (depending on what
  // the user selected for start up options, the SMS might be replaying
  // historical data that we don't care about).
  if (ignorePacket(pkt)) {
    return false;
  }

  // Assuming we're going to process this packet, check to see if we need
  // to finish our initialization steps
  // We should already be initialized by the time we get here, but
  // we'll give it one last try anyway since we'll have to throw
  // the packet away if we can't initialize.
  if (!m_workspaceInitialized) {
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }

    // If we weren't ready to init, or the init failed, that's an error and
    // we can't process this packet at all.
    if (!m_workspaceInitialized) {
      g_log.error("Cannot process BankedEventPacket because workspace isn't "
                  "initialized.");
      // Note: One error message per BankedEventPkt is likely to absolutely
      // flood the error log.
      // Might want to think about rate limiting this somehow...

      return false; // We still return false (ie: "no error") because there's no
                    // reason to stop
                    // parsing the data stream
    }
  }

  // A counter that we use for logging purposes
  unsigned totalEvents = 0;

  // First, check to see if the run has been paused.  We don't process
  // the events if we're paused unless the user has specifically overridden
  // this behavior with the livelistener.keeppausedevents property.
  if (m_runPaused && m_keepPausedEvents == false) {
    return false;
  }

  // Append the events
  g_log.debug() << "----- Pulse ID: " << pkt.pulseId() << " -----\n";
  // Scope braces
  {
    Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);

    // Timestamp for the events
    Mantid::Kernel::DateAndTime eventTime = timeFromPacket(pkt);

    // Save the pulse charge in the logs (*10 because we want the units to be
    // picoCulombs, and ADARA sends them out in units of 10pC)
    m_eventBuffer->mutableRun()
        .getTimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY)
        ->addValue(eventTime, pkt.pulseCharge() * 10);

    // Iterate through each event
    const ADARA::Event *event = pkt.firstEvent();
    unsigned lastBankID = pkt.curBankId();
    // A counter that we use for logging purposes
    unsigned eventsPerBank = 0;
    while (event != NULL) {
      eventsPerBank++;
      totalEvents++;
      if (lastBankID < 0xFFFFFFFE) // Bank ID -1 & -2 are special cases and are
                                   // not valid pixels
      {
        // appendEvent needs tof to be in units of microseconds, but it comes
        // from the ADARA stream in units of 100ns.
        if (pkt.getSourceCORFlag()) {
          appendEvent(event->pixel, event->tof / 10.0, eventTime);
        } else {
          appendEvent(event->pixel,
                      (event->tof + pkt.getSourceTOFOffset()) / 10.0,
                      eventTime);
        }
      }

      event = pkt.nextEvent();
      if (pkt.curBankId() != lastBankID) {
        g_log.debug() << "BankID " << lastBankID << " had " << eventsPerBank
                      << " events\n";

        lastBankID = pkt.curBankId();
        eventsPerBank = 0;
      }
    }
  } // mutex automatically unlocks here

  g_log.debug() << "Total Events: " << totalEvents << "\n";
  g_log.debug("-------------------------------");

  return false;
}

/// Parse a beam monitor event packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::BeamMonitorPkt packets.  Parsed events are counted and
/// the counts are accumulated in the temporary workspace until the forground
/// thread retrieves them.
/// @see extractData()
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::BeamMonitorPkt &pkt) {
  // Check to see if we should process this packet (depending on what
  // the user selected for start up options, the SMS might be replaying
  // historical data that we don't care about).
  if (ignorePacket(pkt)) {
    return false;
  }

  // We'll likely be modifying m_eventBuffer (specifically,
  // m_eventBuffer->m_monitorWorkspace),
  // so lock the mutex
  Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);

  auto monitorBuffer = boost::static_pointer_cast<DataObjects::EventWorkspace>(
      m_eventBuffer->monitorWorkspace());
  const auto pktTime = timeFromPacket(pkt);

  while (pkt.nextSection()) {
    unsigned monitorID = pkt.getSectionMonitorID();

    if (monitorID > 5) {
      // Currently, we only handle monitors 0-5.  At the present time, that's
      // sufficient.
      g_log.error() << "Mantid cannot handle monitor ID's higher than 5.  If "
                    << monitorID << " is actually valid, then an appropriate "
                                    "entry must be made to the "
                    << " ADDABLE list at the top of Framework/API/src/Run.cpp"
                    << std::endl;
    } else {
      std::string monName("monitor");
      monName +=
          (char)(monitorID + 48); // The +48 converts to the ASCII character
      monName += "_counts";
      // Note: The monitor name must exactly match one of the entries in the
      // ADDABLE
      // list at the top of Run.cpp!

      int events = pkt.getSectionEventCount();
      if (m_eventBuffer->run().hasProperty(monName)) {
        events += m_eventBuffer->run().getPropertyValueAsType<int>(monName);
      } else {
        // First time we've received this monitor.  Add it to our list
        m_monitorLogs.push_back(monName);
      }

      // Update the property value (overwriting the old value if there was one)
      m_eventBuffer->mutableRun().addProperty<int>(monName, events, true);

      auto it = m_monitorIndexMap.find(
          -1 * monitorID); // Monitor IDs are negated in Mantid IDFs
      if (it != m_monitorIndexMap.end()) {
        bool risingEdge;
        uint32_t cycle, tof;
        while (pkt.nextEvent(risingEdge, cycle, tof)) {
          // Add the event. Note that they're in units of 100 ns in the packet,
          // need to change to microseconds.
          monitorBuffer->getEventList(it->second)
              .addEventQuickly(DataObjects::TofEvent(tof / 10.0, pktTime));
        }
      } else {
        g_log.error() << "Event from unknown monitor ID (" << monitorID
                      << ") seen.\n";
      }
    }
  }

  return false;
}

/// Parse a geometry packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::GeometryPkt packets.  The data is used to initialize
/// the temporary workspace.
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::GeometryPkt &pkt) {
  // Note: Deliberately NOT calling ignorePacket() because we always parse
  // these packets

  // TODO: For now, I'm assuming that we only need to process one of these
  // packets the first time it comes in and we can ignore any others.
  if (m_workspaceInitialized == false) {
    m_instrumentXML = pkt.info(); // save the xml so we can pass it to the
                                  // LoadInstrument algorithm

    // Now parse the XML for required logfile parameters (we can't call the
    // LoadInstrument alg until we received a value for such parameters).
    //
    // What we're looking for is a node called "parameter" that has a child
    // node called "logfile".  We need to save the id attribute on the
    // logfile node.

    Poco::XML::DOMParser parser;
    Poco::AutoPtr<Poco::XML::Document> doc =
        parser.parseString(m_instrumentXML);

    const Poco::AutoPtr<Poco::XML::NodeList> nodes =
        doc->getElementsByTagName("parameter");
    // Oddly, NodeLists don't seem to have any provision for iterators.  Also,
    // the length() function actually traverses the list to get the count,
    // so we should probably call it once and store it in a variable instead
    // of putting it at the top of a for loop...
    long unsigned nodesLength = nodes->length();
    for (long unsigned i = 0; i < nodesLength; i++) {
      Poco::XML::Node *node = nodes->item(i);
      const Poco::AutoPtr<Poco::XML::NodeList> childNodes = node->childNodes();

      long unsigned childNodesLength = childNodes->length();
      for (long unsigned j = 0; j < childNodesLength; j++) {
        Poco::XML::Node *childNode = childNodes->item(j);
        if (childNode->nodeName() == "logfile") {
          // Found one!
          Poco::AutoPtr<Poco::XML::NamedNodeMap> attr = childNode->attributes();
          long unsigned attrLength = attr->length();
          for (long unsigned k = 0; k < attrLength; k++) {
            Poco::XML::Node *attrNode = attr->item(k);
            if (attrNode->nodeName() == "id") {
              m_requiredLogs.push_back(attrNode->nodeValue());
            }
          }
        }
      }
    }
  }

  // Check to see if we can complete the initialzation steps
  if (!m_workspaceInitialized) {
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }
  }

  return false;
}

/// Parse a beamline info packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::BeamlineInfoPkt packets.  The data is used to initialize
/// the temporary workspace.
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::BeamlineInfoPkt &pkt) {
  // Note: Deliberately NOT calling ignorePacket() because we always parse
  // these packets

  // We only need to process a beamlineinfo packet once
  if (m_workspaceInitialized == false) {
    // We need the instrument name
    m_instrumentName = pkt.longName();
  }

  // Check to see if we can complete the initialzation steps
  if (!m_workspaceInitialized) {
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }
  }

  return false;
}

/// Parse a run status packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::RunStatusPkt packets.
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::RunStatusPkt &pkt) {
  // grab the time from the packet - we'll use it down in
  // initializeWorkspacePart2()
  // Note that we need this value even if we otherwise ignore the packet
  m_dataStartTime = timeFromPacket(pkt);

  // Check to see if we should process the rest of this packet (depending
  // on what the user selected for start up options, the SMS might be
  // replaying historical data that we don't care about).
  if (ignorePacket(pkt, pkt.status())) {
    return false;
  }

  Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);

  const bool haveRunNumber = m_eventBuffer->run().hasProperty("run_number");

  if (pkt.status() == ADARA::RunStatus::NEW_RUN) {
    // Starting a new run:  update m_status and add the run_start & run_number
    // properties

    if (m_status != NoRun) {
      // Previous status should have been NoRun.  Spit out a warning if it's
      // not.
      g_log.warning()
          << "Unexpected start of run.  Run status should have been " << NoRun
          << " (NoRun), but was " << m_status << std::endl;
    }

    if (m_workspaceInitialized) {
      m_status = BeginRun;
    } else {
      // Pay close attention here - this gets complicated!
      //
      // Setting m_status to "Running" is something of a little white lie.  We
      // are
      // in fact at the beginning of a run.  However, since we haven't yet
      // initialized the workspace, this must be one of the first packets we've
      // actually received.  (Probably, the user selected the option to replay
      // history starting from the start of the current run.) Normally, when
      // pkt->status() is NEW_RUN, we'd set the m_pauseNetRead flag to true
      // (see below).  That would cause us to halt reading packets until the
      // flag was reset down in runStatus().  Having m_status set to BeginRun
      // would also cause runStatus() to reset all the data we need to
      // initialize
      // the workspace in preparation for a new run.  In most cases, this is
      // exactly
      // what we want.
      //
      // HOWEVER, in this particular case, we can't set m_pauseNetRead.  If we
      // do, we
      // will not read the Geometry and BeamMonitor packets that have the data
      // we
      // need to complete the workspace initialization. Until we complete the
      // initialization, the extractData() function won't complete successfully
      // and
      // the runStatus() function will thus never be called.  Since
      // m_pauseNetRead
      // is reset down in runStatus(), the whole live listener subsystem
      // basically
      // deadlocks.
      //
      // So, we can't set m_pauseNetRead.  That's OK, because we don't actually
      // have
      // any data from a previous run that we need to keep separate from this
      // run
      // (which was the whole purpose of m_pauseNetRead).  However, when the
      // runStatus() function sees m_status == BeginRun (or EndRun), it sets
      // m_workspaceInitialized to false and clears all the old data we used to
      // initialize the workspace.  It does this because it thinks a run
      // transition
      // has happened and new initialization data will be arriving shortly.  As
      // such, it implicitly assumes that m_pauseNetRead was set and we stopped
      // reading packets.  In this particular case, we can't set m_pauseNetRead,
      // and we're guaranteed to have initialized the workspace before
      // runStatus()
      // would ever be called. (See the previous paragraph.)  As such, the
      // initialization data that runStatus() would clear is actually the data
      // that we need.

      // So, by setting m_status to Running, we avoid runStatus() wiping out our
      // workspace initialization.  We then call setRunDetails() (which would
      // normally happen down in runStatus(), except that we've just gone out
      // of our way to make sure that part of runStatus() *DOESN'T* get
      // executed) and everything runs as it should.

      // It's debatable whether runStatus() should retain that implicit
      // asumption of
      // m_pauseNetRead being true, or should explicitly check its state in
      // addition
      // to m_status.  Either way, you're still going to need several paragraphs
      // of
      // comments to explain what the heck is going on.
      m_status = Running;
      setRunDetails(pkt);
    }

    // Add the run_number property
    if (m_status == BeginRun) {
      if (haveRunNumber) {
        // run_number should not exist at this point, and if it does, we can't
        // do much about it.
        g_log.warning(
            "run_number property already exists.  Current value will be "
            "ignored.\n"
            "(This should never happen.  Talk to the Mantid developers.)");
      } else {
        // Save a copy of the packet so we can call setRunDetails() later (after
        // extractData() has been called to fetch any data remaining from before
        // this run start.
        // Note: need to actually copy the contents (not just a pointer) because
        // pkt will go away when this function returns.  And since packets don't
        // have
        // default constructors, we can only keep a pointer as a member, and
        // thus
        // have to actually allocate our deferred packet with new.
        // Fortunately, this doesn't happen to often, so performance isn't an
        // issue.
        m_deferredRunDetailsPkt = boost::shared_ptr<ADARA::RunStatusPkt>(
            new ADARA::RunStatusPkt(pkt));
      }
    }

    // See detailed comments below for what the m_pauseNetRead flag does and the
    // comments above about m_status for why we don't always set it.
    if (m_workspaceInitialized) {
      m_pauseNetRead = true;
    }

  } else if (pkt.status() == ADARA::RunStatus::END_RUN) {
    // Run has ended:  update m_status and set the flag to stop parsing network
    // packets.
    // (see comments below for why)

    if ((m_status != Running) && (m_status != BeginRun)) {
      // Previous status should have been Running or BeginRun.  Spit out a
      // warning if it's not.  (If it's BeginRun, that's fine.  Itjust means
      // that the run ended before extractData() was called.)
      g_log.warning() << "Unexpected end of run.  Run status should have been "
                      << Running << " (Running), but was " << m_status
                      << std::endl;
    }
    m_status = EndRun;

    // Set the flag to make us stop reading from the network.
    // Stopping network reads solves a number of problems:
    // 1) We don't need to manage a second buffer in order to keep the events
    // in the just ended run separate from the events in the next run.
    // 2) We don't have to deal with the case where the next run has already
    // started but extractData() hasn't been called to fetch the last events
    // from the previous run.
    // 3) We don't have to worry about the case where more than one run has
    // started and finished between calls to extractData()  (ie: 5 second runs
    // and a 10 second update interval.  Yes, that's an operator error, but I
    // still
    // don't want to worry about it.)
    //
    // Because of this, however, if extractData() isn't called at least once per
    // run,
    // the network packets may start to back up and SMS may eventually
    // disconnect us.
    // This flag will be cleared down in runStatus(), which is guaranteed to be
    // called
    // after extractData().
    m_pauseNetRead = true;

    // Set the run number & start time if we don't already have it
    if (!haveRunNumber) {
      setRunDetails(pkt);
    }
  } else if (pkt.status() == ADARA::RunStatus::STATE && !haveRunNumber) {
    setRunDetails(pkt);
  }

  // Note: all other possibilities for pkt.status() can be ignored

  // Check to see if we can/should complete the initialzation steps
  if (!m_workspaceInitialized) {
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }
  }

  return m_pauseNetRead;
  // If we've set m_pauseNetRead, it means we want to stop processing packets.
  // In that case, we need to return true so that we'll break out of the read()
  // loop
  // in the packet parser.
}

void SNSLiveEventDataListener::setRunDetails(const ADARA::RunStatusPkt &pkt) {
  m_runNumber = pkt.runNumber();
  m_eventBuffer->mutableRun().addProperty(
      "run_number", Strings::toString<int>(pkt.runNumber()));
  g_log.notice() << "Run number is " << m_runNumber << std::endl;

  // runStart() is in the EPICS epoch - ie Jan 1, 1990.  Convert to Unix epoch
  time_t runStartTime = pkt.runStart() + ADARA::EPICS_EPOCH_OFFSET;

  // Add the run_start property
  char timeString[64]; // largest the string should end up is 20 (plus a null
                       // terminator)
  strftime(timeString, 64, "%Y-%m-%dT%H:%M:%SZ", gmtime(&runStartTime));
  // addProperty() wants the time as an ISO 8601 string
  m_eventBuffer->mutableRun().addProperty("run_start", std::string(timeString));
}

/// Parse a variable value packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::VariableU32Pkt packets.  The extracted value is stored
/// in the sample log of the temporary workspace.
/// @warning The specified variable must have already been described in a
/// ADARA::DeviceDescriptorPkt packet.
/// @see SNSLiveEventDataListener::rxPacket( const ADARA::DeviceDescriptorPkt &)
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::VariableU32Pkt &pkt) {
  unsigned devId = pkt.devId();
  unsigned pvId = pkt.varId();

  // Check to see if we should process this packet now.  If not, add it to the
  // variable map because we might need to process it later
  if (ignorePacket(pkt)) {
    boost::shared_ptr<ADARA::Packet> ptr(new ADARA::VariableU32Pkt(pkt));
    m_variableMap.insert(std::make_pair(std::make_pair(devId, pvId), ptr));
  } else {
    // Look up the name of this variable
    NameMapType::const_iterator it =
        m_nameMap.find(std::make_pair(devId, pvId));

    if (it == m_nameMap.end()) {
      g_log.error()
          << "Ignoring variable value packet for device " << devId
          << ", variable " << pvId
          << " because we haven't received a device descriptor packet for it."
          << std::endl;
    } else {
      {
        Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);
        m_eventBuffer->mutableRun()
            .getTimeSeriesProperty<int>((*it).second)
            ->addValue(timeFromPacket(pkt), pkt.value());
      }
    }
  }

  // Check to see if we can complete the initialzation steps
  if (!m_workspaceInitialized) {
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }
  }

  return false;
}

/// Parse a variable value packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::VariableDoublePkt packets.  The extracted value is stored
/// in the sample log of the temporary workspace.
/// @warning The specified variable must have already been described in a
/// ADARA::DeviceDescriptorPkt packet.
/// @see SNSLiveEventDataListener::rxPacket( const ADARA::DeviceDescriptorPkt &)
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::VariableDoublePkt &pkt) {
  unsigned devId = pkt.devId();
  unsigned pvId = pkt.varId();

  // Check to see if we should process this packet now.  If not, add it to the
  // variable map because we might need to process it later
  if (ignorePacket(pkt)) {
    boost::shared_ptr<ADARA::Packet> ptr(new ADARA::VariableDoublePkt(pkt));
    m_variableMap.insert(std::make_pair(std::make_pair(devId, pvId), ptr));
  } else {
    // Look up the name of this variable
    NameMapType::const_iterator it =
        m_nameMap.find(std::make_pair(devId, pvId));

    if (it == m_nameMap.end()) {
      g_log.error()
          << "Ignoring variable value packet for device " << devId
          << ", variable " << pvId
          << " because we haven't received a device descriptor packet for it."
          << std::endl;
    } else {
      {
        Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);
        m_eventBuffer->mutableRun()
            .getTimeSeriesProperty<double>((*it).second)
            ->addValue(timeFromPacket(pkt), pkt.value());
      }
    }
  }

  // Check to see if we can complete the initialzation steps
  if (!m_workspaceInitialized) {
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }
  }

  return false;
}

/// Parse a variable value packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::VariableStringPkt packets.  The extracted value is stored
/// in the sample log of the temporary workspace.
/// @warning The specified variable must have already been described in a
/// ADARA::DeviceDescriptorPkt packet.
/// @see SNSLiveEventDataListener::rxPacket( const ADARA::DeviceDescriptorPkt &)
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
/// @remarks As of Februrary 2013, the SMS does not actually send out packets
/// of this type.  As such, this particular function has received very little
/// testing.
bool SNSLiveEventDataListener::rxPacket(const ADARA::VariableStringPkt &pkt) {
  unsigned devId = pkt.devId();
  unsigned pvId = pkt.varId();

  // Check to see if we should process this packet now.  If not, add it to the
  // variable map because we might need to process it later
  if (ignorePacket(pkt)) {
    boost::shared_ptr<ADARA::Packet> ptr(new ADARA::VariableStringPkt(pkt));
    m_variableMap.insert(std::make_pair(std::make_pair(devId, pvId), ptr));
  } else {
    // Look up the name of this variable
    NameMapType::const_iterator it =
        m_nameMap.find(std::make_pair(devId, pvId));

    if (it == m_nameMap.end()) {
      g_log.error()
          << "Ignoring variable value packet for device " << devId
          << ", variable " << pvId
          << " because we haven't received a device descriptor packet for it."
          << std::endl;
    } else {
      {
        Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);
        m_eventBuffer->mutableRun()
            .getTimeSeriesProperty<std::string>((*it).second)
            ->addValue(timeFromPacket(pkt), pkt.value());
      }
    }
  }

  // Check to see if we can complete the initialzation steps
  if (!m_workspaceInitialized) {
    if (readyForInitPart2()) {
      initWorkspacePart2();
    }
  }

  return false;
}

/// Parse a device decriptor packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::DeviceDecriptorPkt packets.  These packets contain
/// XML text decribing variables that will be received in subsequent
/// ADARA::VariableU32Pkt, ADARA::VariableDoublePkt and
/// ADARA::VariableStringPkt packets.
/// @see rxPacket( const ADARA::VariableU32Pkt &)
/// @see rxPacket( const ADARA::VariableDoublePkt &)
/// @see rxPacket( const ADARA::VariableStringPkt &)
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::DeviceDescriptorPkt &pkt) {
  // Note: Deliberately NOT calling ignorePacket() because we always parse
  // these packets

  Poco::XML::DOMParser parser;
  Poco::AutoPtr<Poco::XML::Document> doc =
      parser.parseMemory(pkt.description().c_str(), pkt.description().length());
  const Poco::XML::Node *deviceNode = doc->firstChild();

  // The 'device' should be the root element of the document.  I'm just being
  // paranoid here.
  while (deviceNode && deviceNode->nodeName() != "device") {
    deviceNode = deviceNode->nextSibling();
  }

  if (!deviceNode) {
    g_log.error("Device descriptor packet did not contain a device element!!  "
                "This should never happen!");
    return false;
  }

  // Find the process_variables element
  // Note: for now, I'm ignoring the 'device_name' & 'enumeration' elements
  // because I don't
  // think I need them

  const Poco::XML::Node *node = deviceNode->firstChild();
  while (node && node->nodeName() != "process_variables") {
    node = node->nextSibling();
  }

  if (!node) {
    g_log.warning("Device descriptor packet did not contain a "
                  "process_variables element.");
    return false;
  }

  node = node->firstChild();
  while (node) {
    // iterate through each individual variable...
    if (node->nodeName() == "process_variable") {
      // we need the name, ID and type
      const Poco::XML::Node *pvNode = node->firstChild();
      std::string pvName;
      std::string pvId;
      unsigned pvIdNum;
      std::string pvUnits;
      std::string pvType;
      while (pvNode) {
        const Poco::XML::Node *textElement = pvNode->firstChild();
        if (textElement) {
          if (pvNode->nodeName() == "pv_name") {
            pvName = textElement->nodeValue();
          } else if (pvNode->nodeName() == "pv_id") {
            pvId = textElement->nodeValue();
            std::istringstream(pvId) >> pvIdNum;
          } else if (pvNode->nodeName() == "pv_type") {
            pvType = textElement->nodeValue();
          } else if (pvNode->nodeName() == "pv_units") {
            pvUnits = textElement->nodeValue();
          }
        }

        pvNode = pvNode->nextSibling();
      }

      // We need at least the name, id & type before we can create the property
      // (Units are optional)
      if (pvName.empty() || pvId.empty() || pvType.empty()) {
        if (pvName.empty()) {
          pvName = "<UNKNOWN>";
        }
        g_log.warning() << "Ignoring process variable " << pvName
                        << " because it was missing required fields."
                        << std::endl;
      } else {
        // Check the nameMap - we may have already received a description for
        // this
        // device.  (SMS will re-send DeviceDescriptor packets under certain
        // circumstances.)
        NameMapType::const_iterator it =
            m_nameMap.find(std::make_pair(pkt.devId(), pvIdNum));
        if (it == m_nameMap.end()) {
          // create the property in the workspace - this is a little bit kludgy
          // because
          // the type is specified as a string in the XML, but we pass the
          // actual keyword
          // to the template declaration.  Hense all the if...else if...else
          // stuff...
          Property *prop = NULL;
          if (pvType == "double") {
            prop = new TimeSeriesProperty<double>(pvName);
          } else if ((pvType == "integer") || (pvType == "unsigned") ||
                     (pvType == "unsigned integer") ||
                     (pvType.find("enum_") == 0))
          // Note: Mantid doesn't currently support unsigned int properties
          // Note: We're treating enums as ints (at least for now)
          // Note: ADARA doesn't currently define an integer variable value
          // packet (only unsigned)
          {
            prop = new TimeSeriesProperty<int>(pvName);
          } else if (pvType == "string") {
            prop = new TimeSeriesProperty<std::string>(pvName);
          } else {
            // invalid type string
            g_log.warning() << "Ignoring process variable " << pvName
                            << " because it had an unrecognized type ("
                            << pvType << ")." << std::endl;
          }

          if (prop) {
            if (!pvUnits.empty()) {
              prop->setUnits(pvUnits);
            }
            {
              // Note: it's possible for us receive device descriptor packets in
              // the middle
              // of a run (after the call to initWorkspacePart2), so we really
              // do need to
              // the lock the mutex here.
              Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);
              m_eventBuffer->mutableRun().addLogData(prop);
            }

            // Add the pv id, device id and pv name to the name map so we can
            // find the
            // name when we process the variable value packets
            m_nameMap[std::make_pair(pkt.devId(), pvIdNum)] = pvName;
          }
        }
      }
    }

    node = node->nextSibling();
  }

  return false;
}

/// Parse a stream annotation packet

/// Overrides the default function defined in ADARA::Parser and processes
/// data from ADARA::AnnotationPkt packets.  Scan start, Scan stop, Pause
/// & Resume annotations are stored as time series properties in the
/// workspace.
/// @param pkt The packet to be parsed
/// @return Returns false if there were no problems.  Returns true if there
/// was an error and packet parsing should be interrupted
bool SNSLiveEventDataListener::rxPacket(const ADARA::AnnotationPkt &pkt) {
  // Check to see if we should process this packet (depending on what
  // the user selected for start up options, the SMS might be replaying
  // historical data that we don't care about).
  if (ignorePacket(pkt)) {
    return false;
  }

  {
    Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);
    // We have to lock the mutex prior to calling mutableRun()
    switch (pkt.type()) {
    case ADARA::MarkerType::GENERIC:
      // Do nothing.  We log the comment field below for all types
      break;

    case ADARA::MarkerType::SCAN_START:
      m_eventBuffer->mutableRun()
          .getTimeSeriesProperty<int>(SCAN_PROPERTY)
          ->addValue(timeFromPacket(pkt), pkt.scanIndex());
      g_log.information() << "Scan Start: " << pkt.scanIndex() << std::endl;
      break;

    case ADARA::MarkerType::SCAN_STOP:
      m_eventBuffer->mutableRun()
          .getTimeSeriesProperty<int>(SCAN_PROPERTY)
          ->addValue(timeFromPacket(pkt), 0);
      g_log.information() << "Scan Stop:  " << pkt.scanIndex() << std::endl;
      break;

    case ADARA::MarkerType::PAUSE:
      m_eventBuffer->mutableRun()
          .getTimeSeriesProperty<int>(PAUSE_PROPERTY)
          ->addValue(timeFromPacket(pkt), 1);
      g_log.information() << "Run paused" << std::endl;
      m_runPaused = true;
      break;

    case ADARA::MarkerType::RESUME:
      m_eventBuffer->mutableRun()
          .getTimeSeriesProperty<int>(PAUSE_PROPERTY)
          ->addValue(timeFromPacket(pkt), 0);
      g_log.information() << "Run resumed" << std::endl;
      m_runPaused = false;
      break;

    case ADARA::MarkerType::OVERALL_RUN_COMMENT:
      // Do nothing.  We log the comment field below for all types
      break;
    }
  } // mutex auto unlocks here

  // if there's a comment in the packet, log it at the info level
  std::string comment = pkt.comment();
  if (comment.size() > 0) {
    g_log.information() << "Annotation: " << comment << std::endl;
  }

  return false;
}

/// First part of the workspace initialization

/// Performs various initialization steps that can (and, in some
/// cases, must) be done prior to receiving any packets from the SMS daemon.
void SNSLiveEventDataListener::initWorkspacePart1() {
  m_eventBuffer = boost::static_pointer_cast<DataObjects::EventWorkspace>(
      WorkspaceFactory::Instance().create("EventWorkspace", 1, 1, 1));
  // The numbers in the create() function don't matter - they'll get overwritten
  // down in initWorkspacePart2() when we load the instrument definition.

  // We also know we'll need 3 time series properties on the workspace.  Create
  // them
  // now. (We may end up adding values to the pause and scan properties before
  // we
  // can call initWorkspacePart2().)
  Property *prop = new TimeSeriesProperty<int>(PAUSE_PROPERTY);
  m_eventBuffer->mutableRun().addLogData(prop);
  prop = new TimeSeriesProperty<int>(SCAN_PROPERTY);
  m_eventBuffer->mutableRun().addLogData(prop);
  prop = new TimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY);
  m_eventBuffer->mutableRun().addLogData(prop);
}

/// Second part of the workspace initialization

/// Finishes the workspace initialization using data from
/// various packets received from the SMS daemon
void SNSLiveEventDataListener::initWorkspacePart2() {
  // Use the LoadEmptyInstrument algorithm to create a proper workspace
  // for whatever beamline we're on
  boost::shared_ptr<Algorithm> loadInst =
      Mantid::API::AlgorithmManager::Instance().createUnmanaged(
          "LoadInstrument");
  loadInst->initialize();
  loadInst->setChild(true); // keep the workspace out of the ADS
  loadInst->setProperty("InstrumentXML", m_instrumentXML);
  loadInst->setProperty("InstrumentName", m_instrumentName);
  loadInst->setProperty("Workspace", m_eventBuffer);

  loadInst->execute();

  m_requiredLogs.clear(); // Clear the list.  If we have to initialize the
                          // workspace again,
  // (at the start of another run, for example), the list will be
  // repopulated when we receive the next geometry packet.

  m_eventBuffer->padSpectra(); // expands the workspace to the size of the just
                               // loaded instrument

  // Set the units
  m_eventBuffer->getAxis(0)->unit() = UnitFactory::Instance().create("TOF");
  m_eventBuffer->setYUnit("Counts");

  m_indexMap = m_eventBuffer->getDetectorIDToWorkspaceIndexMap(
      true /* bool throwIfMultipleDets */);

  // We always want to have at least one value for the the scan index time
  // series.  We may have
  // already gotten a scan start packet by the time we get here and therefor
  // don't need to do
  // anything.  If not, we need to put a 0 into the time series.
  if (m_eventBuffer->mutableRun()
          .getTimeSeriesProperty<int>(SCAN_PROPERTY)
          ->size() == 0) {
    m_eventBuffer->mutableRun()
        .getTimeSeriesProperty<int>(SCAN_PROPERTY)
        ->addValue(m_dataStartTime, 0);
  }

  initMonitorWorkspace();

  m_workspaceInitialized = true;
}

/// Creates a monitor workspace sized to the number of monitors, with the
/// monitor IDs set
void SNSLiveEventDataListener::initMonitorWorkspace() {
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

// Check to see if we have data for all of the logs listed in m_requiredLogs.
// NOTE: This function does not lock the mutex!  The calling function must
// ensure that m_eventBuffer won't change while the function runs (either by
// locking the mutex, or by the simple fact of never calling it once the
// workspace
// has been initialized...)
bool SNSLiveEventDataListener::haveRequiredLogs() {
  bool allFound = true;
  Run &run = m_eventBuffer->mutableRun();
  auto it = m_requiredLogs.begin();
  while (it != m_requiredLogs.end() && allFound) {
    if (!run.hasProperty(*it)) {
      allFound = false;
    } else if (run.getProperty(*it)->size() == 0) {
      allFound = false;
    }

    it++;
  }

  return allFound;
}

/// Adds an event to the workspace
void SNSLiveEventDataListener::appendEvent(
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
    g_log.warning() << "Invalid pixel ID: " << pixelId << " (TofF: " << tof
                    << " microseconds)" << std::endl;
  }
}

/// Retrieve buffered data

/// Called by the foreground thread to fetch data that's accumulated in
/// the temporary workspace.  The temporary workspace is left empty and
/// ready to receive more data.
/// @return shared pointer to a workspace containing the accumulated data
boost::shared_ptr<Workspace> SNSLiveEventDataListener::extractData() {
  // Check to see if the background thread has thrown an exception.  If so,
  // re-throw it here.
  if (m_backgroundException) {
    throw(*m_backgroundException);
  }

  // Check whether the background thread has actually initialized the workspace
  // (Which won't happen until the SMS sends it the packet with the geometry
  // information in it.)
  // First wait up to 10 seconds, then if it's still not initialized throw a
  // NotYet
  // exception so that the user has the opportunity to cancel.
  static const double maxBlockTime = 10.0;
  const DateAndTime endTime = DateAndTime::getCurrentTime() + maxBlockTime;
  while ((!m_workspaceInitialized) &&
         (DateAndTime::getCurrentTime() < endTime)) {
    Poco::Thread::sleep(100); // 100 milliseconds
  }
  if (!m_workspaceInitialized) {
    throw Exception::NotYet("The workspace has not yet been initialized.");
  }

  // Throw if the request was for data from the start of a run, but we're not
  // yet in a run.
  if (m_ignorePackets) // This variable is (un)set in ignorePacket()
  {
    throw Exception::NotYet("Waiting for a run to start.");
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
ILiveListener::RunStatus SNSLiveEventDataListener::runStatus() {
  // First up, check to see if the background thread has thrown an
  // exception.  If so, re-throw it here.
  if (m_backgroundException) {
    throw(*m_backgroundException);
  }

  // Need to protect against m_status and m_deferredRunDetailsPkt
  // getting out of sync in the (currently only one) case where the
  // background thread has not been paused...
  Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);

  // The MonitorLiveData algorithm calls this function *after* the call to
  // extract data, which means the value we return should reflect the
  // value that's appropriate for the events that were returned when
  // extractData was called().
  ILiveListener::RunStatus rv = m_status;

  // It's only appropriate to return EndRun once (ie: when we've just
  // returned the last events from the run).  After that, we need to
  // change the status to NoRun.
  // The same logic applies to BeginRun and Running
  if (m_status == BeginRun || m_status == EndRun) {
    // At run transitions, replace the old workspace with a new one
    // (This ensures that we're not using log data and/or geometry from
    // a previous run that are no longer valid.  SMS is guaranteed to
    // send us new device descriptor packets at the start of every run.)
    m_workspaceInitialized = false;

    // These next 3 are what we check for in readyForInitPart2()
    m_instrumentXML.clear();
    m_instrumentName.clear();
    if (m_status == EndRun) {
      // Don't clear this for BeginRun because it was set up in the parser
      // for the RunStatus packet that signaled the beginning of a new
      // run and is thus already set to the correct value.
      m_dataStartTime = Kernel::DateAndTime();
    }

    // NOTE: It's probably not necessary to clear the instrument name
    // and instrument XML (which is the geometry info) because these
    // values don't ever change.  (Or at least, changing them requires
    // changing the SMS config and restarting it and that would cause us
    // to restart the live listener algorithm.)  That said, SMS is
    // guaranteed to send this info out with every run transition, so
    // we might as well ensure that we always use up-to-date data.

    m_nameMap.clear();
    initWorkspacePart1();

    if (m_status == BeginRun) {
      // Set the run details using the packet we saved from the rxPacket()
      // function
      setRunDetails(*m_deferredRunDetailsPkt);
      m_deferredRunDetailsPkt.reset(); // shared_ptr, so we don't use delete
      m_status = Running;
    } else if (m_status == EndRun) {
      m_status = NoRun;
    }
  }

  m_pauseNetRead = false; // make sure the network reads start back up

  return rv;
}

// Called by the rxPacket() functions to determine if the packet should be
// processed
// (Depending on when it last indexed its data, SMS might send us packets that
// are
// older than we requested.)
// Returns false if the packet should be processed, true if is should be ignored
bool
SNSLiveEventDataListener::ignorePacket(const ADARA::PacketHeader &hdr,
                                       const ADARA::RunStatus::Enum status) {
  // Since we're filtering based on time (either the absolute timestamp or
  // nothing
  // before the start of the most recent run), once we've determined a given
  // packet should be processed, we know all packets after that should also be
  // processed.  Thus, we can reduce most calls to this function to a simple
  // boolean test...
  if (!m_ignorePackets)
    return false;

  // Are we looking for the start of the run?
  if (m_filterUntilRunStart) {
    if (hdr.type() == ADARA::PacketType::RUN_STATUS_V0 &&
        status == ADARA::RunStatus::NEW_RUN) {
      // A new run is starting...
      m_ignorePackets = false;
    }
  } else // Filter based solely on time
  {
    if (timeFromPacket(hdr) >= m_startTime) {
      m_ignorePackets = false;
    }
  }

  // If we've just hit our start-up condition, then process
  // all the variable value packets we've been hanging on to.
  if (!m_ignorePackets) {
    replayVariableCache();
  }

  return m_ignorePackets;
}

// Process all the variable value packets stored in m_variableMap
void SNSLiveEventDataListener::replayVariableCache() {
  auto it = m_variableMap.begin();
  while (it != m_variableMap.end()) {
    rxPacket(*(*it).second); // call rxPacket() on the stored packet
    it++;
  }

  m_variableMap.clear(); // empty the map to save a little ram
}

} // namespace LiveData
} // namespace Mantid
