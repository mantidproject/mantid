#include "MantidLiveData/ISISLiveEventDataListener.h"
#include "MantidLiveData/Exception.h"

#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"
#include "MantidAPI/ExperimentInfo.h"

#include "MantidKernel/UnitFactory.h"

// Time we'll wait on a receive call (in seconds)
const long RECV_TIMEOUT = 30;
// Sleep time in case we need to wait for the data to become available (in milliseconds)
const long RECV_WAIT = 100;

namespace Mantid
{
namespace LiveData
{

DECLARE_LISTENER(ISISLiveEventDataListener)

#define RECEIVE(buffer,msg)  \
{\
    long timeout = 0;\
    while( m_socket.available() < static_cast<int>(sizeof(buffer)) )\
    {\
        Poco::Thread::sleep(RECV_WAIT);\
        timeout += RECV_WAIT;\
        if ( timeout > RECV_TIMEOUT * 1000 ) throw std::runtime_error("Receive operation timed out.");\
    }\
    m_socket.receiveBytes(&buffer, sizeof(buffer));\
    if ( !buffer.isValid() )\
    {\
        throw std::runtime_error(msg);\
    }\
}

// Get a reference to the logger
Kernel::Logger& ISISLiveEventDataListener::g_log = Kernel::Logger::get("ISISLiveEventDataListener");

/**
 * The destructor
 */
ISISLiveEventDataListener::ISISLiveEventDataListener():API::ILiveListener(),
    m_isConnected(false),
    m_stopThread(false)
{
}

ISISLiveEventDataListener::~ISISLiveEventDataListener()
{
    // Stop the background thread
    if (m_thread.isRunning())
    {
      // Ask the thread to exit (and hope that it does - Poco doesn't
      // seem to have an equivalent to pthread_cancel
      m_stopThread = true;
      try {
      m_thread.join(RECV_TIMEOUT * 2 * 1000);  // *1000 because join() wants time in milliseconds
      } catch (Poco::TimeoutException &) {
        // And just what do we do here?!?
        // Log a message, sure, but other than that we can either hang the
        // Mantid process waiting for a thread that will apparently never exit
        // or segfault because the ADARA::read() is going to try to write to
        // a buffer that's going to be deleted.
        // Chose segfault - at least that's obvious.
        g_log.fatal() << "ISISLiveEventDataListener failed to shut down its background thread! "
                      << "This should never happen and Mantid is pretty much guaranteed to crash shortly.  "
                      << "Talk to the Mantid developer team." << std::endl;
      }

    }
}

bool ISISLiveEventDataListener::connect(const Poco::Net::SocketAddress &address)
{
    std::cerr << "Connecting to " << address.toString() << std::endl;
    // If we don't have an address, force a connection to the test server running on
    // localhost on the default port
    if (address.host().toString().compare( "0.0.0.0") == 0)
    {
      Poco::Net::SocketAddress tempAddress("localhost:10000");
      try {
        m_socket.connect( tempAddress);  // BLOCKING connect
      } catch (...) {
        g_log.error() << "Connection to " << tempAddress.toString() << " failed." << std::endl;
        return false;
      }
    }
    else
    {
        try {
          m_socket.connect( address);  // BLOCKING connect
        } catch (...) {
          g_log.debug() << "Connection to " << address.toString() << " failed." << std::endl;
          return false;
        }
    }

    m_socket.setReceiveTimeout( Poco::Timespan( RECV_TIMEOUT, 0)); // POCO timespan is seconds, microseconds
    g_log.debug() << "Connected to " << m_socket.address().toString() << std::endl;
    std::cerr << "Connected to " << m_socket.address().toString() << std::endl;

    m_isConnected = true;
    return m_isConnected;
}

void ISISLiveEventDataListener::start(Kernel::DateAndTime startTime)
{
    (void)startTime;
    m_thread.start( *this);
    std::cerr << "Start" << std::endl;
}

boost::shared_ptr<API::Workspace> ISISLiveEventDataListener::extractData()
{
    if ( !m_eventBuffer )
    {
        throw LiveData::Exception::NotYet("The workspace has not yet been initialized.");
    }

    //Make a brand new EventWorkspace
    DataObjects::EventWorkspace_sptr temp = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
        API::WorkspaceFactory::Instance().create("EventWorkspace", m_eventBuffer->getNumberHistograms(), 2, 1));

    //Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(m_eventBuffer, temp, false);

    // Clear out the old logs
    temp->mutableRun().clearTimeSeriesLogs();

    // Lock the mutex and swap the workspaces
    {
      Poco::ScopedLock<Poco::FastMutex> scopedLock( m_mutex);
      std::swap(m_eventBuffer, temp);
    }  // mutex automatically unlocks here

    return temp;
}

bool ISISLiveEventDataListener::isConnected()
{
    return m_isConnected;
}

API::ILiveListener::RunStatus ISISLiveEventDataListener::runStatus()
{
    return Running;
}

/** The main function for the background thread
 *
 * Loops until the forground thread requests it to stop.  Reads data from the network,
 * parses it and stores the resulting events (and other metadata) in a temporary
 * workspace.
 */
void ISISLiveEventDataListener::run()
{
    static char* junk_buffer[10000];

    try
    {
        TCPStreamEventDataSetup setup;
        RECEIVE(setup,"Wrong version");
        std::cerr << "run number " << setup.head_setup.run_number<< " instr " << setup.head_setup.inst_name  << std::endl;

        // initialize the buffer workspace
        initEventBuffer( setup );

        if (m_isConnected == false) // sanity check
        {
          throw std::runtime_error( std::string("ISISLiveEventDataListener::run(): No connection to the DAE."));
          return;  // should never be called, but here just in case exceptions are disabled
        }

        TCPStreamEventDataNeutron events;
        while (m_stopThread == false)
        {
            RECEIVE(events.head,"Corrupt stream - you should reconnect.");

            if ( !(events.head.type == TCPStreamEventHeader::Neutron) )
            {
                // run finished (?)
                m_stopThread = true;
                break;
            }

            std::cerr << "junk " << events.head.length - static_cast<uint32_t>(sizeof(events.head)) << std::endl;
            // ???
            m_socket.receiveBytes(junk_buffer, events.head.length - static_cast<uint32_t>(sizeof(events.head)));

            RECEIVE(events.head_n,"Corrupt stream - you should reconnect.");

            m_socket.receiveBytes(junk_buffer, events.head_n.length - static_cast<uint32_t>(sizeof(events.head_n)));
            events.data.resize(events.head_n.nevents);
            uint32_t nread = 0;
            while( nread < events.head_n.nevents )
            {
                int ntoread = m_socket.available() / static_cast<int>(sizeof(TCPStreamEventNeutron));
                if ( ntoread > (events.head_n.nevents - nread) )
                {
                    ntoread = events.head_n.nevents - nread;
                }
                if (ntoread > 0)
                {
                    m_socket.receiveBytes(&(events.data[nread]), ntoread * static_cast<int>(sizeof(TCPStreamEventNeutron)));
                    nread += ntoread;
                }
                else
                {
                    Poco::Thread::sleep(100);
                }
            }
            if (!events.isValid())
            {
                throw std::runtime_error("corrupt stream - you should reconnect");
            }

            // store the events
            saveEvents( events.data );

        }

    } catch (std::runtime_error &e) {  // exception handler for generic runtime exceptions

      g_log.fatal() << "Caught a runtime exception." << std::endl
                    << "Exception message: " << e.what() << std::endl
                    << "Thread will exit." << std::endl;
      m_isConnected = false;

      m_backgroundException = boost::shared_ptr<std::runtime_error>( new std::runtime_error( e));

    } catch (std::invalid_argument &e) { // TimeSeriesProperty (and possibly some other things) can
                                        // can throw these errors
      g_log.fatal() << "Caught an invalid argument exception." << std::endl
                    << "Exception message: "  << e.what() << std::endl
                    << "Thread will exit." << std::endl;
      m_isConnected = false;
      std::string newMsg( "Invalid argument exception thrown from the background thread: ");
      newMsg += e.what();
      m_backgroundException = boost::shared_ptr<std::runtime_error>( new std::runtime_error( newMsg) );

    } catch (...) {  // Default exception handler
      g_log.fatal() << "Uncaught exception in SNSLiveEventDataListener network read thread."
                    << "  Thread is exiting." << std::endl;
      m_isConnected = false;
      m_backgroundException =
          boost::shared_ptr<std::runtime_error>( new std::runtime_error( "Unknown error in backgound thread") );
    }

}

/**
 * Initialize the buffer event workspace.
 * @param setup :: Information on the data to be sent.
 */
void ISISLiveEventDataListener::initEventBuffer(const TCPStreamEventDataSetup &setup)
{
    // try to load the instrument
    std::string instrName(setup.head_setup.inst_name);
    // get the IDF for instrument in setup for the current date
    std::string instrFilename = API::ExperimentInfo::getInstrumentFilename( instrName );
    API::Algorithm_sptr alg = API::AlgorithmFactory::Instance().create("LoadEmptyInstrument",-1);
    alg->initialize();
    alg->setPropertyValue("Filename",instrFilename);
    alg->setProperty("MakeEventWorkspace", true);
    alg->setProperty("OutputWorkspace", "tmp");
    alg->setChild(true);
    //alg->executeAsChildAlg();
    alg->execute();
    // check if the instrument was loaded
    if ( !alg->isExecuted() )
    {
        throw std::runtime_error("Failed to load instrument " + instrName);
    }
    // get the workspace created by the algorithm
    API::MatrixWorkspace_sptr workspace = alg->getProperty("OutputWorkspace");
    if ( !workspace )
    {
        throw std::runtime_error("Couldn't create the workspace for instrument " + instrName);
    }

    // save this workspace as the event buffer
    m_eventBuffer = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>( workspace );
    if ( !m_eventBuffer )
    {
        throw std::runtime_error("Couldn't create an event workspace for instrument " + instrName);
    }
}

/**
 * Save received event data in the buffer workspace.
 * @param data :: A vector with events.
 */
void ISISLiveEventDataListener::saveEvents(const std::vector<TCPStreamEventNeutron> &data)
{
    Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);

    for(auto it = data.begin(); it != data.end(); ++it)
    {
        Mantid::DataObjects::TofEvent event( it->time_of_flight );
        m_eventBuffer->getEventList( it->spectrum ).addEventQuickly( event );
        std::cerr << it->time_of_flight << ' ' << it->spectrum << std::endl;
    }
}



}
}
