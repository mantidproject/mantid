//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidDataHandling/FakeISISHistoDAE.h"
#include <numeric>

#include <Poco/Net/TCPServer.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Thread.h>

namespace Mantid
{
namespace DataHandling
{
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FakeISISHistoDAE)

typedef enum { ISISDSUnknown = 0, ISISDSInt32 = 1, ISISDSReal32 = 2, ISISDSReal64 = 3, ISISDSChar = 4 } ISISDSDataType;

typedef struct
{
	int len;
	int ver_major;
	int ver_minor;
	int pid;
	int access_type; /* 0 =dae, 1 = crpt */
  // cppcheck-suppress unusedStructMember
	int pad[1];
	char user[32];
	char host[64];
}  isisds_open_t;

/** used for sends and replies once a connection open
 * try to align to 64 bits (8 bytes) boundaries 
 */
typedef struct 
{
	int len;  /* of this structure plus any additional data (in bytes) */
 	int type; /* ISISDSDataType */
	int ndims;
	int dims_array[11];
	char command[32];
	/* additional data (if any) will follow this */
} isisds_command_header_t;

class TestServerConnection: public Poco::Net::TCPServerConnection
{
  int m_nPeriods;
  int m_nSpectra;
  int m_nBins;
public:
  TestServerConnection( const Poco::Net::StreamSocket & soc, int nper = 1, int nspec = 100, int nbins = 30 ):
  Poco::Net::TCPServerConnection(soc),
  m_nPeriods(nper),
  m_nSpectra(nspec),
  m_nBins(nbins)
  {
    char buffer[1024];
    int n = socket().receiveBytes(&buffer,1024);
    sendOK();
  }
  ~TestServerConnection()
  {
    //std::cerr << "Test connection deleted" << std::endl;
  }
  void sendOK()
  {
    isisds_command_header_t comm;
    memset( &comm, 0, sizeof(comm) );
    comm.len = sizeof(comm);
    comm.type = ISISDSUnknown;
    strncpy(comm.command, "OK", sizeof(comm.command));
    socket().sendBytes(&comm, comm.len);
  }
  void sendString(const std::string& str)
  {
    isisds_command_header_t comm;
    memset( &comm, 0, sizeof(comm) );
    comm.len = sizeof(comm) + (int)str.size();
    comm.type = ISISDSChar;
		comm.ndims = 1;
    comm.dims_array[0] = (int)str.size();
    strncpy(comm.command, "OK", sizeof(comm.command));
    socket().sendBytes(&comm, sizeof(comm));
    socket().sendBytes(str.c_str(), (int)str.size());
  }
  void sendInt(int value)
  {
    isisds_command_header_t comm;
    memset( &comm, 0, sizeof(comm) );
    comm.len = sizeof(comm) + sizeof(int);
    comm.type = ISISDSInt32;
		comm.ndims = 1;
    comm.dims_array[0] = 1;
    strncpy(comm.command, "OK", sizeof(comm.command));
    socket().sendBytes(&comm, sizeof(comm));
    socket().sendBytes(&value, sizeof(int));
  }
  void sendData(int spec, int nos)
  {
    int period = 0;
    int istart = spec;
    const int ns1 = m_nSpectra + 1;
    const int nb1 = m_nBins + 1;
    if ( m_nPeriods > 1 )
    {
      period = spec / ns1;
      istart = spec - period * ns1;
      if ( period >= m_nPeriods || istart + nos > ns1 )
      {
        sendOK();
      }
    }
    const int ndata = nos * nb1;
    std::vector<int> data( ndata );
    for(size_t i = 0; i < nos; ++i)
    {
      const int value = period * 1000 + istart + (int)i;
      std::fill( data.begin() + i * nb1, data.begin() + ( i + 1 ) * nb1, value );
    }
    isisds_command_header_t comm;
    memset( &comm, 0, sizeof(comm) );
    comm.len = sizeof(comm) + sizeof(int) * ndata;
    comm.type = ISISDSInt32;
		comm.ndims = 2;
    comm.dims_array[0] = nos;
    comm.dims_array[1] = m_nBins + 1;
    strncpy(comm.command, "OK", sizeof(comm.command));
    socket().sendBytes(&comm, sizeof(comm));
    socket().sendBytes(data.data(), sizeof(int) * ndata);
  }
  void sendFloatArray(const std::vector<float>& arr)
  {
    isisds_command_header_t comm;
    memset( &comm, 0, sizeof(comm) );
    comm.len = (int)(sizeof(comm) + sizeof(float) * arr.size());
    comm.type = ISISDSReal32;
		comm.ndims = 1;
    comm.dims_array[0] = (int)arr.size();
    strncpy( comm.command, "OK", sizeof(comm.command) );
    socket().sendBytes( &comm, sizeof(comm) );
    socket().sendBytes( arr.data(), (int)( sizeof(float) * arr.size() ) );
  }
  void sendIntArray(const std::vector<int>& arr)
  {
    isisds_command_header_t comm;
    memset( &comm, 0, sizeof(comm) );
    comm.len = (int)(sizeof(comm) + sizeof(int) * arr.size());
    comm.type = ISISDSInt32;
		comm.ndims = 1;
    comm.dims_array[0] = (int)arr.size();
    strncpy( comm.command, "OK", sizeof(comm.command) );
    socket().sendBytes( &comm, sizeof(comm) );
    socket().sendBytes( arr.data(), (int)( sizeof(int) * arr.size() ) );
  }
  void run()
  {
    for(;;)
    {
      char buffer[1024];
      isisds_command_header_t comm;
      int n = 0;
      try
      {
        n = socket().receiveBytes(&comm,sizeof(comm));
        n = socket().receiveBytes(&buffer,1024);
        if ( n == 0 ) break;
      }
      catch(...)
      {
        break;
      } 
      if ( comm.type == ISISDSChar )
      {
        std::string command(buffer, n);
        if ( command == "NAME" )
        {
          sendString("TESTHISTOLISTENER");
        }
        else if ( command == "NPER" )
        {
          sendInt( m_nPeriods );
        }
        else if ( command == "NSP1" )
        {
          sendInt( m_nSpectra );
        }
        else if ( command == "NTC1" )
        {
          sendInt( m_nBins );
        }
        else if ( command == "NDET" )
        {
          sendInt( m_nSpectra );
        }
        else if ( command == "RTCB1" )
        {
          std::vector<float> bins( m_nBins + 1 );
          const float dx = 0.1f;
          float x = 0;
          for(auto b = bins.begin(); b != bins.end(); ++b, x += dx)
          {
            *b = x;
          };
          sendFloatArray( bins );
        }
        else if ( command == "RRPB" )
        {
          std::vector<float> rrpb( 32 );
          rrpb[8] = 3.14f;
          sendFloatArray( rrpb );
        }
        else if ( command == "UDET" )
        {
          std::vector<int> udet( m_nSpectra );
          for(size_t i = 0; i < m_nSpectra; ++i)
          {
            udet[i] = (int)( 1000 + i + 1 );
          }
          sendIntArray( udet );
        }
        else if ( command == "SPEC" )
        {
          std::vector<int> spec( m_nSpectra );
          for(size_t i = 0; i < m_nSpectra; ++i)
          {
            spec[i] = (int)( i + 1 );
          }
          sendIntArray( spec );
        }
        else
        {
          sendOK();
        }
      }
      else 
      {
        std::string command(comm.command);
        if ( command == "GETDAT" )
        {
          int *spec_nos = reinterpret_cast<int*>(buffer);
          int spec = spec_nos[0];
          int nos = spec_nos[1];
          sendData( spec, nos );
        }
        else
        {
          sendOK();
        }
      }
    }
  }
};

class TestServerConnectionFactory: public Poco::Net::TCPServerConnectionFactory
{
  int m_nPeriods;
  int m_nSpectra;
  int m_nBins;
public:
  TestServerConnectionFactory(int nper = 1, int nspec = 100, int nbins = 30): 
  Poco::Net::TCPServerConnectionFactory(), 
  m_nPeriods(nper),
  m_nSpectra(nspec),
  m_nBins(nbins)
  {}
  Poco::Net::TCPServerConnection * createConnection(const Poco::Net::StreamSocket & socket)
  {
    return new TestServerConnection( socket, m_nPeriods, m_nSpectra, m_nBins );
  }
};

/// Sets documentation strings for this algorithm
void FakeISISHistoDAE::initDocs()
{
  this->setWikiSummary("Simulates ISIS histogram DAE. ");
  this->setOptionalMessage("Simulates ISIS histogram DAE.");
}


using namespace Kernel;
using namespace API;

/// (Empty) Constructor
FakeISISHistoDAE::FakeISISHistoDAE():m_server(NULL) 
{
}

/// Destructor
FakeISISHistoDAE::~FakeISISHistoDAE() 
{
  if ( m_server )
  {
    m_server->stop();
    delete m_server;
  }
}

void FakeISISHistoDAE::init()
{
  declareProperty(new PropertyWithValue<int>("NPeriods", 1, Direction::Input),"Number of periods.");
  declareProperty(new PropertyWithValue<int>("NSpectra", 100, Direction::Input),"Number of spectra.");
  declareProperty(new PropertyWithValue<int>("NBins", 30, Direction::Input),"Number of bins.");
}

void FakeISISHistoDAE::exec()
{
  Mutex::ScopedLock lock(m_mutex);
  Poco::Net::ServerSocket socket(6789);
  socket.listen();
  int nper = getProperty("NPeriods");
  int nspec = getProperty("NSpectra");
  int nbins = getProperty("NBins");
  m_server = new Poco::Net::TCPServer(TestServerConnectionFactory::Ptr( new TestServerConnectionFactory(nper,nspec,nbins) ), socket );
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
    progress( 0.0, "Fake DAE" );

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

} // namespace DataHandling
} // namespace Mantid

