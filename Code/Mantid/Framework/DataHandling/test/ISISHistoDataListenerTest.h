#ifndef MANTID_DATAHANDLING_ISISHISTODATALISTENERTEST_H_
#define MANTID_DATAHANDLING_ISISHISTODATALISTENERTEST_H_

#include "MantidDataHandling/ISISHistoDataListener.h"
#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidGeometry/ISpectraDetectorMap.h"
#include <cxxtest/TestSuite.h>

#include <Poco/Net/TCPServer.h>
#include <Poco/Net/StreamSocket.h>

#include <algorithm>

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
  TestServerConnection( const Poco::Net::StreamSocket & soc ):
  Poco::Net::TCPServerConnection(soc),
  m_nPeriods(1),
  m_nSpectra(100),
  m_nBins(30)
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
      if ( period >= m_nPeriods || istart + nos >= ns1 )
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
public:
  TestServerConnectionFactory(): Poco::Net::TCPServerConnectionFactory() {}
  Poco::Net::TCPServerConnection * createConnection(const Poco::Net::StreamSocket & socket)
  {
    return new TestServerConnection( socket );
  }
};

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;

class ISISHistoDataListenerTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ISISHistoDataListenerTest *createSuite() { return new ISISHistoDataListenerTest(); }
  static void destroySuite( ISISHistoDataListenerTest *suite ) { delete suite; }

  ISISHistoDataListenerTest()
  {
    Mantid::API::FrameworkManager::Instance();
  }

  void test_Init()
  {

    Poco::Net::ServerSocket socket(6789);
    socket.listen();
    Poco::Net::TCPServer server(TestServerConnectionFactory::Ptr( new TestServerConnectionFactory ), socket );
    server.start();

    auto listener = Mantid::API::LiveListenerFactory::Instance().create("TESTHISTOLISTENER");
    TS_ASSERT( listener );
    TS_ASSERT( listener->isConnected() );

    int s[] = {1,2,3,10,11,95,96,97,98,99,100};
    std::vector<specid_t> specs;
    specs.assign( s, s + 11 );
    listener->setSpectra( specs );
    auto ws = listener->extractData();
    TS_ASSERT_EQUALS( ws->getNumberHistograms(), 11 );
    TS_ASSERT_EQUALS( ws->blocksize(), 30 );

    server.stop();

    auto x = ws->readX( 0 );
    TS_ASSERT_EQUALS( x.size(), 31 );
    TS_ASSERT_EQUALS( x[0], 0 );
    TS_ASSERT_DELTA( x[1], 0.1, 1e-6 );
    TS_ASSERT_DELTA( x[30], 3.0, 1e-6 );

    x = ws->readX( 4 );
    TS_ASSERT_EQUALS( x.size(), 31 );
    TS_ASSERT_EQUALS( x[0], 0 );
    TS_ASSERT_DELTA( x[1], 0.1, 1e-6 );
    TS_ASSERT_DELTA( x[30], 3.0, 1e-6 );

    auto y = ws->readY( 2 );
    TS_ASSERT_EQUALS( y[0], 3 );
    TS_ASSERT_EQUALS( y[5], 3 );
    TS_ASSERT_EQUALS( y[29], 3 );

    y = ws->readY( 4 );
    TS_ASSERT_EQUALS( y[0], 11 );
    TS_ASSERT_EQUALS( y[5], 11 );
    TS_ASSERT_EQUALS( y[29], 11 );

    y = ws->readY( 7 );
    TS_ASSERT_EQUALS( y[0], 97 );
    TS_ASSERT_EQUALS( y[5], 97 );
    TS_ASSERT_EQUALS( y[29], 97 );

    auto e = ws->readE( 2 );
    TS_ASSERT_EQUALS( e[0], sqrt(3.0) );
    TS_ASSERT_EQUALS( e[5], sqrt(3.0) );
    TS_ASSERT_EQUALS( e[29], sqrt(3.0) );

    e = ws->readE( 4 );
    TS_ASSERT_EQUALS( e[0], sqrt(11.0) );
    TS_ASSERT_EQUALS( e[5], sqrt(11.0) );
    TS_ASSERT_EQUALS( e[29], sqrt(11.0) );

    e = ws->readE( 7 );
    TS_ASSERT_EQUALS( e[0], sqrt(97.0) );
    TS_ASSERT_EQUALS( e[5], sqrt(97.0) );
    TS_ASSERT_EQUALS( e[29], sqrt(97.0) );

    auto& sm = ws->spectraMap();
    TS_ASSERT_EQUALS( sm.nSpectra(), 100 );

    auto d = sm.getDetectors( 1 );
    TS_ASSERT_EQUALS( d.size(), 1 );
    TS_ASSERT_EQUALS( d[0], 1001 );

    d = sm.getDetectors( 4 );
    TS_ASSERT_EQUALS( d.size(), 1 );
    TS_ASSERT_EQUALS( d[0], 1004 );

    d = sm.getDetectors( 100 );
    TS_ASSERT_EQUALS( d.size(), 1 );
    TS_ASSERT_EQUALS( d[0], 1100 );

  }
  

};


#endif /* MANTID_DATAHANDLING_ISISHISTODATALISTENERTEST_H_ */
