/*WIKI*

Simulates ISIS event DAE. It runs continuously until canceled and listens to port 10000 for connection.
When connected starts sending event packets.

*WIKI*/
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidISISLiveData/FakeISISEventDAE.h"
#include "MantidISISLiveData/TCPEventStreamDefs.h"

#include <Poco/Net/TCPServer.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Thread.h>

#include <numeric>

namespace Mantid
{
namespace ISISLiveData
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FakeISISEventDAE);

/**
 * Implements Poco TCPServerConnection and does the actual job of interpreting commands
 * from a client and sending data.
 */
class TestServerConnection: public Poco::Net::TCPServerConnection
{
public:
  /**
   * Constructor. Defines the simulated dataset dimensions.
   * @param soc :: A socket that provides communication with the client.
   */
  TestServerConnection( const Poco::Net::StreamSocket & soc ):
      Poco::Net::TCPServerConnection(soc)
  {
    sendInitialSetup();
  }
  /// Destructor.
  ~TestServerConnection()
  {
    //std::cerr << "Test connection deleted" << std::endl;
  }
  /// Sends an OK message when there is nothing to send or an error occured
  void sendOK()
  {
    std::string comm = "OK";
    socket().sendBytes(comm.c_str(), (int)comm.size());
  }

  /// Send initial setup header
  void sendInitialSetup()
  {
      TCPStreamEventDataSetup setup;
      setup.head_setup.run_number = 1234;
      strcpy(setup.head_setup.inst_name,"MUSR");
      socket().sendBytes(&setup,(int)sizeof(setup));
  }

  /**
   * Main method that sends out the data.
   */
  void run()
  {
      for(;;)
      {
          Poco::Thread::sleep(1000);
          TCPStreamEventDataNeutron data;
          data.head_n.nevents = 100;

          socket().sendBytes(&data.head,(int)sizeof(data.head));
          socket().sendBytes(&data.head_n,(int)sizeof(data.head_n));

          for(uint32_t i = 0; i < data.head_n.nevents; ++i)
          {
              TCPStreamEventNeutron neutron;
              neutron.time_of_flight = 150.01f;
              neutron.spectrum = 3;
              socket().sendBytes(&neutron,(int)sizeof(neutron));
          }
      }
      TCPStreamEventDataSetup setup;
      setup.head_setup.run_number = 1234;
      strcpy(setup.head_setup.inst_name,"MUSR");
      socket().sendBytes(&setup,(int)sizeof(setup));
  }
};

/**
 * Implements Poco TCPServerConnectionFactory
 */
class TestServerConnectionFactory: public Poco::Net::TCPServerConnectionFactory
{
public:
  /**
   * Constructor.
   */
  TestServerConnectionFactory():
  Poco::Net::TCPServerConnectionFactory()
  {}
  /**
   * The factory method.
   * @param socket :: The socket.
   */
  Poco::Net::TCPServerConnection * createConnection(const Poco::Net::StreamSocket & socket)
  {
    return new TestServerConnection( socket );
  }
};

/// Sets documentation strings for this algorithm
void FakeISISEventDAE::initDocs()
{
  this->setWikiSummary("Simulates ISIS event DAE. ");
  this->setOptionalMessage("Simulates ISIS event DAE.");
}


using namespace Kernel;
using namespace API;

/// (Empty) Constructor
FakeISISEventDAE::FakeISISEventDAE():m_server(NULL)
{
}

/// Destructor
FakeISISEventDAE::~FakeISISEventDAE()
{
  if ( m_server )
  {
    m_server->stop();
    delete m_server;
  }
}

/**
 * Declare the algorithm properties
 */
void FakeISISEventDAE::init()
{
}

/**
 * Execute the algorithm.
 */
void FakeISISEventDAE::exec()
{
  Mutex::ScopedLock lock(m_mutex);
  Poco::Net::ServerSocket socket(10000);
  socket.listen();
  m_server = new Poco::Net::TCPServer(TestServerConnectionFactory::Ptr( new TestServerConnectionFactory() ), socket );
  m_server->start();
  // Keep going until you get cancelled
  while (true)
  {
    try
    {
    // Exit if the user presses cancel
      interruption_point();
    }
    catch(...)
    {
      break;
    }
    progress( 0.0, "Fake ISIS event DAE" );

    // Sleep for 50 msec
    Poco::Thread::sleep(50);
  }
  if ( m_server )
  {
    m_server->stop();
    m_server = NULL;
  }
  socket.close();
}

} // namespace LiveData
} // namespace Mantid

