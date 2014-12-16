#include "MantidLiveData/ISISLiveEventDataListener.h"
#include "MantidLiveData/Exception.h"

#include "MantidAPI/LiveListenerFactory.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidAPI/SpectrumDetectorMapping.h"
#include "MantidAPI/AlgorithmFactory.h"
#include "MantidAPI/Algorithm.h"

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/UnitFactory.h"

#ifdef GCC_VERSION
// Avoid compiler warnings on gcc from unused static constants in
// isisds_command.h
GCC_DIAG_OFF(unused - variable)
#endif
#include "LoadDAE/idc.h"

const char *PROTON_CHARGE_PROPERTY = "proton_charge";
const char *RUN_NUMBER_PROPERTY = "run_number";

namespace Mantid {
namespace LiveData {

DECLARE_LISTENER(ISISLiveEventDataListener)

namespace {
/// static logger
Kernel::Logger g_log("ISISLiveEventDataListener");
}

/**
 * The constructor
 */
ISISLiveEventDataListener::ISISLiveEventDataListener()
    : API::ILiveListener(), m_isConnected(false), m_stopThread(false),
      m_runNumber(0), m_daeHandle(), m_numberOfPeriods(0),
      m_numberOfSpectra(0) {
  m_warnings["period"] = "Period number is outside the range. Changed to 0.";
}

/**
 * The destructor
 */
ISISLiveEventDataListener::~ISISLiveEventDataListener() {
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
      g_log.fatal() << "ISISLiveEventDataListener failed to shut down its "
                       "background thread! "
                    << "This should never happen and Mantid is pretty much "
                       "guaranteed to crash shortly.  "
                    << "Talk to the Mantid developer team." << std::endl;
    }
  }
}

// connect the listener to DAE
bool
ISISLiveEventDataListener::connect(const Poco::Net::SocketAddress &address) {
  // If we don't have an address, force a connection to the test server running
  // on
  // localhost on the default port
  if (address.host().toString().compare("0.0.0.0") == 0) {
    Poco::Net::SocketAddress tempAddress("127.0.0.1:10000");
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

  std::string daeName = address.toString();
  // remove the port part
  auto i = daeName.find(':');
  if (i != std::string::npos) {
    daeName.erase(i);
  }

  if (daeName == "0.0.0.0") {
    // to connect to fake dae
    daeName = "127.0.0.1";
  }

  // set IDC reporter function for errors
  IDCsetreportfunc(&ISISLiveEventDataListener::IDCReporter);

  int retVal = 0;
  if (address.port() > 10000) {
    // we are using a custom port, set the DAE port as one higher
    retVal = IDCopen(daeName.c_str(), 0, 0, &m_daeHandle,
                     static_cast<uint16_t>(address.port() + 1));
  } else {
    // we are using the default port, take the default DAE port
    retVal = IDCopen(daeName.c_str(), 0, 0, &m_daeHandle);
  }
  if (retVal != 0) {
    m_daeHandle = NULL;
    return false;
  }

  m_numberOfPeriods = getInt("NPER");
  m_numberOfSpectra = getInt("NSP1");

  g_log.notice() << "Number of periods " << m_numberOfPeriods << std::endl;
  g_log.notice() << "Number of spectra " << m_numberOfSpectra << std::endl;

  TCPStreamEventDataSetup setup;
  Receive(setup, "Setup", "Wrong version");
  m_startTime.set_from_time_t(setup.head_setup.start_time);

  // initialize the buffer workspace
  initEventBuffer(setup);

  m_isConnected = true;
  return m_isConnected;
}

// start event collection
void ISISLiveEventDataListener::start(Kernel::DateAndTime startTime) {
  (void)startTime;
  m_thread.start(*this);
}

// return a workspace with collected events
boost::shared_ptr<API::Workspace> ISISLiveEventDataListener::extractData() {
  if (m_eventBuffer.empty() || !m_eventBuffer[0]) {
    // extractData() is called too early
    throw LiveData::Exception::NotYet(
        "The workspace has not yet been initialized.");
  }

  if (!m_isConnected) {
    // the background thread stopped because of an error. the error message has
    // been logged at this point
    throw std::runtime_error("Background thread stopped.");
  }

  Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);

  std::vector<DataObjects::EventWorkspace_sptr> outWorkspaces(
      m_numberOfPeriods);
  for (size_t i = 0; i < static_cast<size_t>(m_numberOfPeriods); ++i) {

    // Make a brand new EventWorkspace
    DataObjects::EventWorkspace_sptr temp =
        boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
            API::WorkspaceFactory::Instance().create(
                "EventWorkspace", m_eventBuffer[i]->getNumberHistograms(), 2,
                1));

    // Copy geometry over.
    API::WorkspaceFactory::Instance().initializeFromParent(m_eventBuffer[i],
                                                           temp, false);

    // Clear out the old logs
    temp->mutableRun().clearTimeSeriesLogs();

    // Swap the workspaces
    std::swap(m_eventBuffer[i], temp);

    outWorkspaces[i] = temp;
  }

  if (m_numberOfPeriods > 1) {
    // create a workspace group in case the data are multiperiod
    auto workspaceGroup = API::WorkspaceGroup_sptr(new API::WorkspaceGroup);
    for (size_t i = 0; i < static_cast<size_t>(m_numberOfPeriods); ++i) {
      workspaceGroup->addWorkspace(outWorkspaces[i]);
    }
    return workspaceGroup;
  }

  return outWorkspaces[0];
}

bool ISISLiveEventDataListener::isConnected() { return m_isConnected; }

API::ILiveListener::RunStatus ISISLiveEventDataListener::runStatus() {
  return Running;
}

int ISISLiveEventDataListener::runNumber() const { return m_runNumber; }

/** The main function for the background thread
 *
 * Loops until the forground thread requests it to stop.  Reads data from the
 *network,
 * parses it and stores the resulting events (and other metadata) in a temporary
 * workspace.
 */
void ISISLiveEventDataListener::run() {

  try {
    if (m_isConnected == false) // sanity check
    {
      throw std::runtime_error(std::string("No connection to the DAE."));
    }

    TCPStreamEventDataNeutron events;
    while (m_stopThread == false) {
      // get the header with the type of the packet
      Receive(events.head, "Events header",
              "Corrupt stream - you should reconnect.");
      if (!(events.head.type == TCPStreamEventHeader::Neutron)) {
        // don't know what to do with it - stop
        throw std::runtime_error("Unknown packet type.");
      }
      CollectJunk(events.head);

      // get the header with the sream size
      Receive(events.head_n, "Neutrons header",
              "Corrupt stream - you should reconnect.");
      CollectJunk(events.head_n);

      // absolute pulse (frame) time
      Mantid::Kernel::DateAndTime pulseTime =
          m_startTime + static_cast<double>(events.head_n.frame_time_zero);
      // Save the pulse charge in the logs
      double protons = static_cast<double>(events.head_n.protons);
      m_eventBuffer[0]
          ->mutableRun()
          .getTimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY)
          ->addValue(pulseTime, protons);

      events.data.resize(events.head_n.nevents);
      uint32_t nread = 0;
      // receive the events
      while (nread < events.head_n.nevents) {
        int ntoread = m_socket.available() /
                      static_cast<int>(sizeof(TCPStreamEventNeutron));
        if (ntoread > static_cast<int>(events.head_n.nevents - nread)) {
          ntoread = static_cast<int>(events.head_n.nevents - nread);
        }
        if (ntoread > 0) {
          m_socket.receiveBytes(
              &(events.data[nread]),
              ntoread * static_cast<int>(sizeof(TCPStreamEventNeutron)));
          nread += ntoread;
        } else {
          Poco::Thread::sleep(RECV_WAIT);
        }
      }
      if (!events.isValid()) {
        throw std::runtime_error("corrupt stream - you should reconnect");
      }

      // store the events
      saveEvents(events.data, pulseTime, events.head_n.period);
    }

  } catch (std::runtime_error &
               e) { // exception handler for generic runtime exceptions

    g_log.error() << "Caught a runtime exception." << std::endl
                  << "Exception message: " << e.what() << std::endl;
    m_isConnected = false;

    m_backgroundException =
        boost::shared_ptr<std::runtime_error>(new std::runtime_error(e));

  } catch (std::invalid_argument &
               e) { // TimeSeriesProperty (and possibly some other things) can
                    // can throw these errors
    g_log.error() << "Caught an invalid argument exception." << std::endl
                  << "Exception message: " << e.what() << std::endl;
    m_isConnected = false;
    std::string newMsg(
        "Invalid argument exception thrown from the background thread: ");
    newMsg += e.what();
    m_backgroundException =
        boost::shared_ptr<std::runtime_error>(new std::runtime_error(newMsg));

  } catch (...) { // Default exception handler
    g_log.error() << "Uncaught exception in ISISLiveEventDataListener network "
                     "read thread." << std::endl;
    m_isConnected = false;
    m_backgroundException = boost::shared_ptr<std::runtime_error>(
        new std::runtime_error("Unknown error in backgound thread"));
  }
}

/**
 * Initialize the buffer event workspace.
 * @param setup :: Information on the data to be sent.
 */
void ISISLiveEventDataListener::initEventBuffer(
    const TCPStreamEventDataSetup &setup) {
  // Create an event workspace for the output
  auto workspace = API::WorkspaceFactory::Instance().create(
      "EventWorkspace", m_numberOfSpectra, 2, 1);

  m_eventBuffer.resize(m_numberOfPeriods);

  // save this workspace as the event buffer
  m_eventBuffer[0] =
      boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(workspace);
  if (!m_eventBuffer[0]) {
    throw std::runtime_error("Failed to create an event workspace");
  }
  // Set the units
  m_eventBuffer[0]->getAxis(0)->unit() =
      Kernel::UnitFactory::Instance().create("TOF");
  m_eventBuffer[0]->setYUnit("Counts");

  // Set the spectra-detector maping
  loadSpectraMap();

  // Load the instrument
  std::string instrName(setup.head_setup.inst_name);
  loadInstrument(instrName);

  // Set the run number
  m_runNumber = setup.head_setup.run_number;
  std::string run_num = boost::lexical_cast<std::string>(m_runNumber);
  m_eventBuffer[0]->mutableRun().addLogData(
      new Mantid::Kernel::PropertyWithValue<std::string>(RUN_NUMBER_PROPERTY,
                                                         run_num));

  // Add the proton charge property
  m_eventBuffer[0]->mutableRun().addLogData(
      new Mantid::Kernel::TimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY));

  if (m_numberOfPeriods > 1) {
    for (size_t i = 1; i < static_cast<size_t>(m_numberOfPeriods); ++i) {
      // create an event workspace for each period
      m_eventBuffer[i] =
          boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(
              API::WorkspaceFactory::Instance().create(
                  "EventWorkspace", m_eventBuffer[0]->getNumberHistograms(), 2,
                  1));

      // Copy geometry over.
      API::WorkspaceFactory::Instance().initializeFromParent(
          m_eventBuffer[0], m_eventBuffer[i], false);
    }
  }
}

/**
 * Save received event data in the buffer workspace.
 * @param data :: A vector with events.
 */
void ISISLiveEventDataListener::saveEvents(
    const std::vector<TCPStreamEventNeutron> &data,
    const Kernel::DateAndTime &pulseTime, size_t period) {
  Poco::ScopedLock<Poco::FastMutex> scopedLock(m_mutex);

  if (period >= static_cast<size_t>(m_numberOfPeriods)) {
    auto warn = m_warnings.find("period");
    if (warn != m_warnings.end()) {
      g_log.warning() << warn->second << std::endl;
      m_warnings.erase(warn);
    }
    period = 0;
  }

  for (auto it = data.begin(); it != data.end(); ++it) {
    Mantid::DataObjects::TofEvent event(it->time_of_flight, pulseTime);
    m_eventBuffer[period]->getEventList(it->spectrum).addEventQuickly(event);
  }
}

/**
  * Set the spectra-detector map to the buffer workspace.
  */
void ISISLiveEventDataListener::loadSpectraMap() {
  // Read in the number of detectors
  int ndet = getInt("NDET");
  // Read in matching arrays of spectra indices and detector ids
  std::vector<int> udet;
  std::vector<int> spec;
  getIntArray("UDET", udet, ndet);
  getIntArray("SPEC", spec, ndet);
  // set up the mapping
  m_eventBuffer[0]->updateSpectraUsing(
      API::SpectrumDetectorMapping(spec, udet));
}

/**
  * Load the instrument
  * @param instrName :: Instrument name
  */
void ISISLiveEventDataListener::loadInstrument(const std::string &instrName) {
  // try to load the instrument. if it doesn't load give a warning and carry on
  if (instrName.empty()) {
    g_log.warning() << "Unable to read instrument name from DAE." << std::endl;
    return;
  }
  const char *warningMessage = "Failed to load instrument ";
  try {
    g_log.notice() << "Loading instrument " << instrName << " ... "
                   << std::endl;
    API::Algorithm_sptr alg =
        API::AlgorithmFactory::Instance().create("LoadInstrument", -1);
    alg->initialize();
    alg->setPropertyValue("InstrumentName", instrName);
    alg->setProperty("Workspace", m_eventBuffer[0]);
    alg->setProperty("RewriteSpectraMap", false);
    alg->setChild(true);
    alg->execute();
    // check if the instrument was loaded
    if (!alg->isExecuted()) {
      g_log.warning() << warningMessage << instrName << std::endl;
    }
    g_log.notice() << "Instrument loaded." << std::endl;
  } catch (std::exception &e) {
    g_log.warning() << warningMessage << instrName << std::endl;
    g_log.warning() << e.what() << instrName << std::endl;
  }
}

int ISISLiveEventDataListener::getInt(const std::string &par) const {
  int sv_dims_array[1] = {1}, sv_ndims = 1, buffer;
  int stat =
      IDCgetpari(m_daeHandle, par.c_str(), &buffer, sv_dims_array, &sv_ndims);
  if (stat != 0) {
    throw std::runtime_error("Unable to read " + par + " from DAE");
  }
  return buffer;
}

void ISISLiveEventDataListener::getIntArray(const std::string &par,
                                            std::vector<int> &arr,
                                            const size_t dim) {
  int dims = static_cast<int>(dim), ndims = 1;
  arr.resize(dim);
  if (IDCgetpari(m_daeHandle, par.c_str(), arr.data(), &dims, &ndims) != 0) {
    throw std::runtime_error("Unable to read " + par + " from DAE");
  }
}

/** Function called by IDC routines to report an error. Passes the error through
* to the logger
* @param status ::  The status code of the error (disregarded)
* @param code ::    The error code (disregarded)
* @param message :: The error message - passed to the logger at error level
*/
void ISISLiveEventDataListener::IDCReporter(int status, int code,
                                            const char *message) {
  (void)status;
  (void)code; // Avoid compiler warning
  g_log.error(message);
}
}
}
