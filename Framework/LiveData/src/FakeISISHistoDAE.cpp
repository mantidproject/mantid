//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidLiveData/FakeISISHistoDAE.h"
#include <numeric>

#include <Poco/Net/TCPServer.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Thread.h>

namespace Mantid {
namespace LiveData {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FakeISISHistoDAE)

typedef enum {
  ISISDSUnknown = 0,
  ISISDSInt32 = 1,
  ISISDSReal32 = 2,
  ISISDSReal64 = 3,
  ISISDSChar = 4
} ISISDSDataType;

typedef struct {
  int len;
  // cppcheck-suppress unusedStructMember
  int ver_major;
  // cppcheck-suppress unusedStructMember
  int ver_minor;
  // cppcheck-suppress unusedStructMember
  int pid;
  // cppcheck-suppress unusedStructMember
  int access_type; /* 0 =dae, 1 = crpt */
                   // cppcheck-suppress unusedStructMember
  int pad[1];
  // cppcheck-suppress unusedStructMember
  char user[32];
  // cppcheck-suppress unusedStructMember
  char host[64];
} isisds_open_t;

/** used for sends and replies once a connection open
 * try to align to 64 bits (8 bytes) boundaries
 */
typedef struct {
  int len;  /* of this structure plus any additional data (in bytes) */
  int type; /* ISISDSDataType */
  int ndims;
  int dims_array[11];
  char command[32];
  /* additional data (if any) will follow this */
} isisds_command_header_t;

/**
 * Implements Poco TCPServerConnection and does the actual job of interpreting
 * commands
 * from a client and sending data.
 */
class TestServerConnection : public Poco::Net::TCPServerConnection {
  int m_nPeriods;
  int m_nSpectra;
  int m_nBins;
  int m_nMonitors;
  int m_nMonitorBins;

public:
  /**
   * Constructor. Defines the simulated dataset dimensions.
   * @param soc :: A socket that provides communication with the client.
   * @param nper :: Number of periods in the simulated dataset.
   * @param nspec :: Number of spectra in the simulated dataset.
   * @param nbins :: Number of bins in the simulated dataset.
   */
  TestServerConnection(const Poco::Net::StreamSocket &soc, int nper = 1,
                       int nspec = 100, int nbins = 30)
      : Poco::Net::TCPServerConnection(soc), m_nPeriods(nper),
        m_nSpectra(nspec), m_nBins(nbins), m_nMonitors(3),
        m_nMonitorBins(nbins * 2) {
    char buffer[1024];
    socket().receiveBytes(&buffer, 1024);
    sendOK();
  }
  /// Destructor.
  ~TestServerConnection() {
    // std::cerr << "Test connection deleted" << std::endl;
  }
  /// Sends an OK message when there is nothing to send or an error occured
  void sendOK() {
    isisds_command_header_t comm;
    memset(&comm, 0, sizeof(comm));
    comm.len = sizeof(comm);
    comm.type = ISISDSUnknown;
    strncpy(comm.command, "OK", sizeof(comm.command));
    socket().sendBytes(&comm, comm.len);
  }
  /**
   * Send a text string
   * @param str :: A string to send
   */
  void sendString(const std::string &str) {
    isisds_command_header_t comm;
    memset(&comm, 0, sizeof(comm));
    comm.len = (int)sizeof(comm) + (int)str.size();
    comm.type = ISISDSChar;
    comm.ndims = 1;
    comm.dims_array[0] = (int)str.size();
    strncpy(comm.command, "OK", sizeof(comm.command));
    socket().sendBytes(&comm, sizeof(comm));
    socket().sendBytes(str.c_str(), (int)str.size());
  }
  /**
   * Send an int value
   * @param value :: A value to send
   */
  void sendInt(int value) {
    isisds_command_header_t comm;
    memset(&comm, 0, sizeof(comm));
    comm.len = static_cast<int>(sizeof(comm) + sizeof(int));
    comm.type = ISISDSInt32;
    comm.ndims = 1;
    comm.dims_array[0] = 1;
    strncpy(comm.command, "OK", sizeof(comm.command));
    socket().sendBytes(&comm, sizeof(comm));
    socket().sendBytes(&value, sizeof(int));
  }
  /**
   * Send some data
   * @param spec :: Staring spectra index
   * @param nos :: Number of spectra to send.
   */
  void sendData(int spec, int nos) {
    int period = 0;
    int istart = spec;
    const int ns1 = m_nSpectra + m_nMonitors + 1;
    if (m_nPeriods > 1) {
      period = spec / ns1;
      istart = spec - period * ns1;
    }
    if (period >= m_nPeriods || istart + nos > ns1) {
      sendOK();
    }
    const int nb1 = (istart <= m_nSpectra ? m_nBins : m_nMonitorBins) + 1;
    const int ndata = nos * nb1;
    std::vector<int> data(ndata);
    for (int i = 0; i < nos; ++i) {
      const int value = period * 1000 + istart + i;
      std::fill(data.begin() + i * nb1, data.begin() + (i + 1) * nb1, value);
    }
    isisds_command_header_t comm;
    memset(&comm, 0, sizeof(comm));
    comm.len = (int)(sizeof(comm) + sizeof(int) * ndata);
    comm.type = ISISDSInt32;
    comm.ndims = 2;
    comm.dims_array[0] = nos;
    comm.dims_array[1] = m_nBins + 1;
    strncpy(comm.command, "OK", sizeof(comm.command));
    socket().sendBytes(&comm, sizeof(comm));
    socket().sendBytes(data.data(), (int)sizeof(int) * ndata);
  }
  /**
   * Send an array of float numbers
   * @param arr :: An array to send
   */
  void sendFloatArray(const std::vector<float> &arr) {
    isisds_command_header_t comm;
    memset(&comm, 0, sizeof(comm));
    comm.len = (int)(sizeof(comm) + sizeof(float) * arr.size());
    comm.type = ISISDSReal32;
    comm.ndims = 1;
    comm.dims_array[0] = (int)arr.size();
    strncpy(comm.command, "OK", sizeof(comm.command));
    socket().sendBytes(&comm, sizeof(comm));
    socket().sendBytes(arr.data(), (int)(sizeof(float) * arr.size()));
  }
  /**
   * Send an array of int numbers
   * @param arr :: An array to send
   */
  void sendIntArray(const std::vector<int> &arr) {
    isisds_command_header_t comm;
    memset(&comm, 0, sizeof(comm));
    comm.len = (int)(sizeof(comm) + sizeof(int) * arr.size());
    comm.type = ISISDSInt32;
    comm.ndims = 1;
    comm.dims_array[0] = (int)arr.size();
    strncpy(comm.command, "OK", sizeof(comm.command));
    socket().sendBytes(&comm, sizeof(comm));
    socket().sendBytes(arr.data(), (int)(sizeof(int) * arr.size()));
  }
  /**
   * Main method that reads commands from the socket and send out the data.
   */
  void run() {
    for (;;) {
      char buffer[1024];
      isisds_command_header_t comm;
      int n = 0;
      try {
        socket().receiveBytes(&comm, sizeof(comm));
        n = socket().receiveBytes(&buffer, 1024);
        if (n == 0)
          break;
      } catch (...) {
        break;
      }
      if (comm.type == ISISDSChar) {
        std::string command(buffer, n);
        if (command == "NAME") {
          sendString("MUSR");
        } else if (command == "NPER") {
          sendInt(m_nPeriods);
        } else if (command == "NSP1") {
          sendInt(m_nSpectra);
        } else if (command == "NSP2") {
          sendInt(m_nMonitors);
        } else if (command == "NTC1") {
          sendInt(m_nBins);
        } else if (command == "NTC2") {
          sendInt(m_nMonitorBins);
        } else if (command == "NDET") {
          sendInt(m_nSpectra + m_nMonitors);
        } else if (command == "NMON") {
          sendInt(m_nMonitors);
        } else if (command == "RTCB1") {
          std::vector<float> bins(m_nBins + 1);
          const float dx = 100.0f;
          float x = 10000.0f;
          for (auto b = bins.begin(); b != bins.end(); ++b, x += dx) {
            *b = x;
          };
          sendFloatArray(bins);
        } else if (command == "RTCB2" ||
                   (command.size() > 5 && command.substr(0, 5) == "RTCB_")) {
          std::vector<float> bins(m_nMonitorBins + 1);
          const float dx = 10.0f;
          float x = 10000.0f;
          for (auto b = bins.begin(); b != bins.end(); ++b, x += dx) {
            *b = x;
          };
          sendFloatArray(bins);
        } else if (command == "RRPB") {
          std::vector<float> rrpb(32);
          rrpb[8] = 3.14f;
          sendFloatArray(rrpb);
        } else if (command == "UDET") {
          std::vector<int> udet(m_nSpectra + m_nMonitors);
          for (int i = 0; i < static_cast<int>(udet.size()); ++i) {
            udet[i] = (int)(1000 + i + 1);
          }
          sendIntArray(udet);
        } else if (command == "SPEC") {
          std::vector<int> spec(m_nSpectra + m_nMonitors);
          for (int i = 0; i < static_cast<int>(spec.size()); ++i) {
            spec[i] = (int)(i + 1);
          }
          sendIntArray(spec);
        } else if (command == "MDET") {
          std::vector<int> mdet(m_nMonitors);
          for (int i = 0; i < m_nMonitors; ++i) {
            mdet[i] = (int)(m_nSpectra + i + 1);
          }
          sendIntArray(mdet);
        } else {
          sendOK();
        }
      } else {
        std::string command(comm.command);
        if (command == "GETDAT") {
          int *spec_nos = reinterpret_cast<int *>(buffer);
          int spec = spec_nos[0];
          int nos = spec_nos[1];
          sendData(spec, nos);
        } else {
          sendOK();
        }
      }
    }
  }
};

/**
 * Implements Poco TCPServerConnectionFactory
 */
class TestServerConnectionFactory
    : public Poco::Net::TCPServerConnectionFactory {
  int m_nPeriods; ///< Number of periods in the fake dataset
  int m_nSpectra; ///< Number of spectra in the fake dataset
  int m_nBins;    ///< Number of bins in the fake dataset
public:
  /**
   * Constructor.
   * @param nper :: Number of periods in the simulated dataset.
   * @param nspec :: Number of spectra in the simulated dataset.
   * @param nbins :: Number of bins in the simulated dataset.
   */
  TestServerConnectionFactory(int nper = 1, int nspec = 100, int nbins = 30)
      : Poco::Net::TCPServerConnectionFactory(), m_nPeriods(nper),
        m_nSpectra(nspec), m_nBins(nbins) {}
  /**
   * The factory method.
   * @param socket :: The socket.
   */
  Poco::Net::TCPServerConnection *
  createConnection(const Poco::Net::StreamSocket &socket) {
    return new TestServerConnection(socket, m_nPeriods, m_nSpectra, m_nBins);
  }
};

using namespace Kernel;
using namespace API;

/// (Empty) Constructor
FakeISISHistoDAE::FakeISISHistoDAE() : m_server(NULL) {}

/// Destructor
FakeISISHistoDAE::~FakeISISHistoDAE() {
  if (m_server) {
    m_server->stop();
    delete m_server;
  }
}

/**
 * Declare the algorithm properties
 */
void FakeISISHistoDAE::init() {
  declareProperty(new PropertyWithValue<int>("NPeriods", 1, Direction::Input),
                  "Number of periods.");
  declareProperty(new PropertyWithValue<int>("NSpectra", 100, Direction::Input),
                  "Number of spectra.");
  declareProperty(new PropertyWithValue<int>("NBins", 30, Direction::Input),
                  "Number of bins.");
  declareProperty(new PropertyWithValue<int>("Port", 56789, Direction::Input),
                  "The port to broadcast on (default 56789, ISISDAE 6789).");
}

/**
 * Execute the algorithm.
 */
void FakeISISHistoDAE::exec() {
  int nper = getProperty("NPeriods");
  int nspec = getProperty("NSpectra");
  int nbins = getProperty("NBins");
  int port = getProperty("Port");

  Mutex::ScopedLock lock(m_mutex);
  Poco::Net::ServerSocket socket(static_cast<Poco::UInt16>(port));
  socket.listen();

  m_server = new Poco::Net::TCPServer(
      TestServerConnectionFactory::Ptr(
          new TestServerConnectionFactory(nper, nspec, nbins)),
      socket);
  m_server->start();
  // Keep going until you get cancelled
  while (true) {
    try {
      // Exit if the user presses cancel
      interruption_point();
    } catch (...) {
      break;
    }
    progress(0.0, "Fake DAE");

    // Sleep for 50 msec
    Poco::Thread::sleep(50);
  }
  if (m_server) {
    m_server->stop();
    m_server = NULL;
  }
  socket.close();
}

} // namespace LiveData
} // namespace Mantid
