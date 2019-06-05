// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidLiveData/ISIS/FakeISISEventDAE.h"
#include "MantidLiveData/ISIS/TCPEventStreamDefs.h"

#include "MantidKernel/MersenneTwister.h"
#include "MantidKernel/Timer.h"

#include <Poco/ActiveResult.h>
#include <Poco/Net/StreamSocket.h>
#include <Poco/Net/TCPServer.h>

namespace Mantid {
namespace LiveData {
// Register the algorithm into the algorithm factory
DECLARE_ALGORITHM(FakeISISEventDAE)

using namespace Kernel;
using namespace API;

namespace {
/**
 * Implements Poco TCPServerConnection and does the actual job of interpreting
 * commands
 * from a client and sending data.
 */
class TestServerConnection : public Poco::Net::TCPServerConnection {
  int m_nPeriods;
  int m_nSpectra;
  int m_Rate;
  int m_nEvents;
  boost::shared_ptr<Progress> m_prog;

public:
  /**
   * Constructor. Defines the simulated dataset dimensions.
   * @param soc :: A socket that provides communication with the client.
   */
  TestServerConnection(const Poco::Net::StreamSocket &soc, int nper, int nspec,
                       int rate, int nevents, boost::shared_ptr<Progress> prog)
      : Poco::Net::TCPServerConnection(soc), m_nPeriods(nper),
        m_nSpectra(nspec), m_Rate(rate), m_nEvents(nevents), m_prog(prog) {
    m_prog->report(0, "Client Connected");
    sendInitialSetup();
  }
  /// Sends an OK message when there is nothing to send or an error occured
  void sendOK() {
    std::string comm = "OK";
    socket().sendBytes(comm.c_str(), static_cast<int>(comm.size()));
  }

  /// Send initial setup header
  void sendInitialSetup() {
    TCPStreamEventDataSetup setup;
    setup.head_setup.run_number = 1234;
    strcpy(setup.head_setup.inst_name, "MUSR");
    socket().sendBytes(&setup, static_cast<int>(sizeof(setup)));
  }

  /**
   * Main method that sends out the data.
   */
  void run() override {
    Kernel::MersenneTwister tof(0, 10000.0, 20000.0);
    Kernel::MersenneTwister spec(1234, 0.0, static_cast<double>(m_nSpectra));
    Kernel::MersenneTwister period(0, 0.0, static_cast<double>(m_nPeriods));
    std::vector<TCPStreamEventNeutron> neutronVector(m_nEvents);

    Timer timer;
    int eventTotal = 0;

    for (;;) {
      Poco::Thread::sleep(m_Rate);
      TCPStreamEventDataNeutron data;
      data.head_n.nevents = m_nEvents;
      data.head_n.period = static_cast<uint32_t>(period.nextValue());

      socket().sendBytes(&data.head, static_cast<int>(sizeof(data.head)));
      socket().sendBytes(&data.head_n, static_cast<int>(sizeof(data.head_n)));

      for (uint32_t i = 0; i < data.head_n.nevents; ++i) {
        TCPStreamEventNeutron neutron;
        neutron.time_of_flight = static_cast<float>(tof.nextValue());
        neutron.spectrum = static_cast<uint32_t>(spec.nextValue());
        neutronVector[i] = neutron;
      }

      int bytesSent = 0;
      int targetSize =
          m_nEvents * static_cast<int>(sizeof(TCPStreamEventNeutron));
      while (bytesSent < targetSize) {
        bytesSent += socket().sendBytes(neutronVector.data() + bytesSent,
                                        targetSize - bytesSent);
      }

      // report progress
      float secondsElapsed = timer.elapsed(false);
      eventTotal += m_nEvents;
      // only report once per second
      if (secondsElapsed > 1) {
        float rate = static_cast<float>(eventTotal) / secondsElapsed;
        std::stringstream sstm;
        sstm << static_cast<int>(rate) << " events/sec";
        m_prog->report(0, sstm.str());

        eventTotal = 0;
        timer.reset();
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
  int m_Rate;
  int m_nEvents;
  boost::shared_ptr<Progress> m_prog;

public:
  /**
   * Constructor.
   */
  TestServerConnectionFactory(int nper, int nspec, int rate, int nevents,
                              boost::shared_ptr<Progress> prog)
      : Poco::Net::TCPServerConnectionFactory(), m_nPeriods(nper),
        m_nSpectra(nspec), m_Rate(rate), m_nEvents(nevents), m_prog(prog) {}
  /**
   * The factory method.
   * @param socket :: The socket.
   */
  Poco::Net::TCPServerConnection *
  createConnection(const Poco::Net::StreamSocket &socket) override {
    return new TestServerConnection(socket, m_nPeriods, m_nSpectra, m_Rate,
                                    m_nEvents, m_prog);
  }
};
} // namespace

/**
 * Declare the algorithm properties
 */
void FakeISISEventDAE::init() {
  declareProperty(
      std::make_unique<PropertyWithValue<int>>("NPeriods", 1, Direction::Input),
      "Number of periods.");
  declareProperty(std::make_unique<PropertyWithValue<int>>("NSpectra", 100,
                                                           Direction::Input),
                  "Number of spectra.");
  declareProperty(
      std::make_unique<PropertyWithValue<int>>("Rate", 20, Direction::Input),
      "Rate of sending the data: stream of NEvents events is sent "
      "every Rate milliseconds.");
  declareProperty(std::make_unique<PropertyWithValue<int>>("NEvents", 1000,
                                                           Direction::Input),
                  "Number of events in each packet.");
  declareProperty(
      std::make_unique<PropertyWithValue<int>>("Port", 59876, Direction::Input),
      "The port to broadcast on (default 59876, ISISDAE 10000).");
}

/**
 * Execute the algorithm.
 */
void FakeISISEventDAE::exec() {
  int nper = getProperty("NPeriods");
  int nspec = getProperty("NSpectra");
  int rate = getProperty("Rate");
  int nevents = getProperty("NEvents");
  int port = getProperty("Port");

  // start the live HistoDAE as well
  API::IAlgorithm_sptr histoDAE =
      createChildAlgorithm("FakeISISHistoDAE", -1.0, -1.0);
  histoDAE->setLoggingOffset(-2); // make most messages from the HistoDAE
                                  // invisible to default logging levels
  histoDAE->setProperty("NPeriods", nper);
  histoDAE->setProperty("NSpectra", nspec);
  histoDAE->setProperty("Port", port + 1);
  auto histoDAEHandle = histoDAE->executeAsync();

  auto prog = boost::make_shared<Progress>(this, 0.0, 1.0, 100);
  prog->setNotifyStep(0);
  prog->report(0, "Waiting for client");
  Poco::Net::ServerSocket socket(static_cast<Poco::UInt16>(port));
  socket.listen();
  Poco::Net::TCPServer server(
      TestServerConnectionFactory::Ptr(
          new TestServerConnectionFactory(nper, nspec, rate, nevents, prog)),
      socket);
  server.start();
  // Keep going until you get cancelled
  while (true) {
    try {
      // Exit if the user presses cancel
      interruption_point();
    } catch (...) {
      // This is caught as a result of cancel being set - so exit the loop
      break;
    }
    // Sleep for 50 msec
    Poco::Thread::sleep(50);
  }
  // It's most likely that we got here from a cancel request
  // so calling prog->report after this point
  // will generate another CancelException
  histoDAE->cancel();
  histoDAEHandle.wait();
  server.stop();
  socket.close();
}

} // namespace LiveData
} // namespace Mantid
