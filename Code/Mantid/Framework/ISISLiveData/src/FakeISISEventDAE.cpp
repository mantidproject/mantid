//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidISISLiveData/FakeISISEventDAE.h"
#include "MantidISISLiveData/TCPEventStreamDefs.h"

#include "MantidKernel/MersenneTwister.h"

#include <Poco/Net/TCPServer.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Thread.h>

#include <boost/random/uniform_int.hpp>

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
    int m_nPeriods;
    int m_nSpectra;
    int m_Rate;
    int m_nEvents;
public:
  /**
   * Constructor. Defines the simulated dataset dimensions.
   * @param soc :: A socket that provides communication with the client.
   */
  TestServerConnection( const Poco::Net::StreamSocket & soc, int nper, int nspec,int rate, int nevents ):
    Poco::Net::TCPServerConnection(soc),
    m_nPeriods(nper),
    m_nSpectra(nspec),
      m_Rate(rate),
      m_nEvents(nevents)
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
      Kernel::MersenneTwister tof(0,100.0,200.0);
      Kernel::MersenneTwister spec(1234,0.0,static_cast<double>(m_nSpectra));
      Kernel::MersenneTwister period(0,0.0,static_cast<double>(m_nPeriods));
      std::vector<TCPStreamEventNeutron> neutronVector (m_nEvents);
      for(;;)
      {
          Poco::Thread::sleep(m_Rate);
          TCPStreamEventDataNeutron data;
          data.head_n.nevents = m_nEvents;
          data.head_n.period  = static_cast<uint32_t>(period.nextValue());

          socket().sendBytes(&data.head,(int)sizeof(data.head));
          socket().sendBytes(&data.head_n,(int)sizeof(data.head_n));

          for(uint32_t i = 0; i < data.head_n.nevents; ++i)
          {
              TCPStreamEventNeutron neutron;
              neutron.time_of_flight = static_cast<float>(tof.nextValue());
              neutron.spectrum = static_cast<uint32_t>(spec.nextValue());
              neutronVector[i] = neutron;
          }
              
          int bytesSent = 0;
          int targetSize = m_nEvents * static_cast<int>(sizeof(TCPStreamEventNeutron));
          while (bytesSent < targetSize)
          {
            bytesSent += socket().sendBytes(neutronVector.data() + bytesSent,targetSize-bytesSent);
          }
          
      }
  }
};

/**
 * Implements Poco TCPServerConnectionFactory
 */
class TestServerConnectionFactory: public Poco::Net::TCPServerConnectionFactory
{
    int m_nPeriods; ///< Number of periods in the fake dataset
    int m_nSpectra; ///< Number of spectra in the fake dataset
    int m_Rate;
    int m_nEvents;
public:
  /**
   * Constructor.
   */
  TestServerConnectionFactory(int nper, int nspec,int rate, int nevents):
  Poco::Net::TCPServerConnectionFactory(),
  m_nPeriods(nper),
  m_nSpectra(nspec),
      m_Rate(rate),
      m_nEvents(nevents)
  {}
  /**
   * The factory method.
   * @param socket :: The socket.
   */
  Poco::Net::TCPServerConnection * createConnection(const Poco::Net::StreamSocket & socket)
  {
    return new TestServerConnection( socket, m_nPeriods, m_nSpectra, m_Rate, m_nEvents );
  }
};


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
    declareProperty(new PropertyWithValue<int>("NPeriods", 1, Direction::Input),"Number of periods.");
    declareProperty(new PropertyWithValue<int>("NSpectra", 100, Direction::Input),"Number of spectra.");
    declareProperty(new PropertyWithValue<int>("Rate", 20, Direction::Input),
                    "Rate of sending the data: stream of NEvents events is sent every Rate milliseconds.");
    declareProperty(new PropertyWithValue<int>("NEvents", 1000, Direction::Input),"Number of events in each packet.");
}

/**
 * Execute the algorithm.
 */
void FakeISISEventDAE::exec()
{
    int nper = getProperty("NPeriods");
    int nspec = getProperty("NSpectra");
    int rate = getProperty("Rate");
    int nevents = getProperty("NEvents");
  Mutex::ScopedLock lock(m_mutex);
  Poco::Net::ServerSocket socket(10000);
  socket.listen();
  m_server = new Poco::Net::TCPServer(
              TestServerConnectionFactory::Ptr( new TestServerConnectionFactory(nper,nspec,rate,nevents) ), socket );
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
