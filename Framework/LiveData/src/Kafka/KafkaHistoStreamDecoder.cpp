// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidLiveData/Kafka/KafkaHistoStreamDecoder.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidLiveData/Exception.h"

GNU_DIAG_OFF("conversion")
#include "private/Schema/hs00_event_histogram_generated.h"
GNU_DIAG_ON("conversion")

namespace {
/// Logger
Mantid::Kernel::Logger g_log("KafkaHistoStreamDecoder");
} // namespace

namespace Mantid {
namespace LiveData {

// -----------------------------------------------------------------------------
// Public members
// -----------------------------------------------------------------------------

/**
 * Constructor
 * @param broker A reference to a Broker object for creating topic streams
 * @param histoTopic The name of the topic streaming the histo data
 * @param spDetTopic The name of the topic streaming the spectrum-detector
 * run mapping
 */
KafkaHistoStreamDecoder::KafkaHistoStreamDecoder(
    std::shared_ptr<IKafkaBroker> broker, const std::string &histoTopic,
    const std::string &instrumentName)
    : m_broker(broker), m_histoTopic(histoTopic),
      m_instrumentName(instrumentName), m_histoStream(), m_workspace(),
      m_buffer(), m_thread(), m_interrupt(false), m_capturing(false),
      m_exception() {
  // Initialize buffer workspace
  m_workspace = createBufferWorkspace();
}

/**
 * Destructor.
 * Stops capturing from the stream
 */
KafkaHistoStreamDecoder::~KafkaHistoStreamDecoder() { stopCapture(); }

/**
 * Start capturing from the stream on a separate thread. This is a non-blocking
 * call and will return after the thread has started
 */
void KafkaHistoStreamDecoder::startCapture(bool) {
  g_log.debug() << "Starting capture on topic: " << m_histoTopic << "\n";
  m_histoStream =
      m_broker->subscribe({m_histoTopic}, SubscribeAtOption::LATEST);

  m_thread = std::thread([this]() { this->captureImpl(); });
  m_thread.detach();
}

/**
 * Stop capturing from the stream. This is a blocking call until the capturing
 * function has completed
 */
void KafkaHistoStreamDecoder::stopCapture() {
  g_log.debug() << "Stopping capture\n";

  // This will interrupt the "event" loop
  m_interrupt = true;
  // Wait until the function has completed. The background thread will exit
  // automatically
  while (m_capturing) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  };
}

/**
 * Check if there is data available to extract
 * @return True if data has been accumulated so that extractData()
 * can be called, false otherwise
 */
bool KafkaHistoStreamDecoder::hasData() const {
  std::lock_guard<std::mutex> lock(m_buffer_mutex);
  return !m_buffer.empty();
}

/**
 * Check for an exception thrown by the background thread and rethrow
 * it if necessary.
 * @return A pointer to the data collected since the last call to this
 * method
 */
API::Workspace_sptr KafkaHistoStreamDecoder::extractData() {
  if (m_exception) {
    throw * m_exception;
  }

  auto workspace_ptr = extractDataImpl();
  return workspace_ptr;
}

// -----------------------------------------------------------------------------
// Private members
// -----------------------------------------------------------------------------

API::Workspace_sptr KafkaHistoStreamDecoder::extractDataImpl() {
  std::lock_guard<std::mutex> lock(m_buffer_mutex);

  if (!m_capturing) {
    throw Exception::NotYet("Local buffers not initialized.");
  }

  if (m_buffer.empty()) {
    throw Exception::NotYet("No message to process yet.");
  }

  auto histoMsg = GetEventHistogram(m_buffer.c_str());

  auto shape = histoMsg->current_shape();
  auto nbins = shape->Get(0) - 1;
  auto nspectra = static_cast<size_t>(shape->Get(1));

  auto metadata = histoMsg->dim_metadata();
  auto metadimx = metadata->GetAs<DimensionMetaData>(0);
  auto metadimy = metadata->GetAs<DimensionMetaData>(1);
  auto xbins = metadimx->bin_boundaries_as_ArrayDouble()->value();

  auto *bindata = xbins->data();
  HistogramData::BinEdges binedges(&bindata[0], &bindata[xbins->size()]);

  API::MatrixWorkspace_sptr ws{DataObjects::create<DataObjects::Workspace2D>(
      *m_workspace, nspectra, binedges)};

  ws->setIndexInfo(m_workspace->indexInfo());
  auto data = histoMsg->data_as_ArrayDouble()->value();

  // Set the units
  ws->getAxis(0)->setUnit(metadimx->unit()->c_str());
  ws->setYUnit(metadimy->unit()->c_str());

  std::vector<double> counts;
  for (size_t i = 0; i < nspectra; ++i) {
    const double *start = data->data() + (i * nbins);
    counts.assign(start, start + nbins);
    ws->setCounts(i, counts);
  }

  return ws;
}

/**
 * Start decoding data from the streams into the internal buffers.
 * Implementation designed to be entry point for new thread of execution.
 * It catches all thrown exceptions.
 */
void KafkaHistoStreamDecoder::captureImpl() {
  m_capturing = true;
  try {
    captureImplExcept();
  } catch (std::exception &exc) {
    m_exception = boost::make_shared<std::runtime_error>(exc.what());
  } catch (...) {
    m_exception = boost::make_shared<std::runtime_error>(
        "KafkaEventStreamDecoder: Unknown exception type caught.");
  }
  m_capturing = false;
}

/**
 * Exception-throwing variant of captureImpl(). Do not call this directly
 */
void KafkaHistoStreamDecoder::captureImplExcept() {
  g_log.information("Event capture starting");

  m_interrupt = false;
  std::string buffer;
  int64_t offset;
  int32_t partition;
  std::string topicName;

  while (!m_interrupt) {
    // Pull in events
    m_histoStream->consumeMessage(&buffer, offset, partition, topicName);

    // No events, wait for some to come along...
    if (buffer.empty()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    // Lock so we don't overwrite buffer while workspace is being extracted
    {
      std::lock_guard<std::mutex> lock(m_buffer_mutex);
      m_buffer = buffer;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  g_log.debug("Histo capture finished");
}

DataObjects::Workspace2D_sptr KafkaHistoStreamDecoder::createBufferWorkspace() {
  API::MatrixWorkspace_sptr workspace;

  try {
    auto alg = API::AlgorithmManager::Instance().createUnmanaged(
        "LoadEmptyInstrument");
    // Do not put the workspace in the ADS
    alg->setChild(true);
    alg->initialize();
    alg->setPropertyValue("InstrumentName", m_instrumentName);
    alg->setPropertyValue("OutputWorkspace", "ws");
    alg->execute();
    workspace = alg->getProperty("OutputWorkspace");
  } catch (std::exception &exc) {
    g_log.error() << "Error loading empty instrument '" << m_instrumentName
                  << "': " << exc.what() << "\n";
    throw;
  }

  return boost::dynamic_pointer_cast<DataObjects::Workspace2D>(workspace);
}

} // namespace LiveData
} // namespace Mantid
