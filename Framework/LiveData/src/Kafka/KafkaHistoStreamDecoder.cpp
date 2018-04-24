#include "MantidLiveData/Kafka/KafkaHistoStreamDecoder.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidLiveData/Exception.h"

GCC_DIAG_OFF(conversion)
#include "private/Schema/ai34_det_counts_generated.h"
GCC_DIAG_ON(conversion)


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
  : m_broker(broker)
  , m_histoTopic(histoTopic)
  , m_instrumentName(instrumentName)
  , m_histoStream()
  , m_workspace()
  , m_indexMap()
  , m_indexOffset(0)
  , m_thread()
  , m_interrupt(false)
  , m_capturing(false)
  , m_exception()
{
  g_log.warning() << "KafkaHistoStreamDecoder" << "\n";

  // Initialize buffer workspace
  {
    std::lock_guard<std::mutex> lock(m_workspace_mutex);
    m_workspace = createBufferWorkspace();
    m_indexMap =
        m_workspace->getDetectorIDToWorkspaceIndexVector(m_indexOffset);
    g_log.warning() << "indexOffset: " << m_indexOffset << "\n";

  }
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
  g_log.warning() << "startCapture" << "\n";

  m_histoStream = m_broker->subscribe({m_histoTopic}, SubscribeAtOption::LATEST);

  m_thread = std::thread([this]() { this->captureImpl(); });
  m_thread.detach();
}

/**
 * Stop capturing from the stream. This is a blocking call until the capturing
 * function has completed
 */
void KafkaHistoStreamDecoder::stopCapture() {
  g_log.warning() << "stopCapture" << "\n";

  // This will interrupt the "event" loop
  m_interrupt = true;
  // Wait until the function has completed. The background thread
  // will exit automatically
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
  g_log.warning() << "hasData" << "\n";

  std::lock_guard<std::mutex> lock(m_workspace_mutex);
  return !!m_workspace;
}

/**
 * Check for an exception thrown by the background thread and rethrow
 * it if necessary. If no error occurred swap the current internal buffer
 * for a fresh one and return the old buffer.
 * @return A pointer to the data collected since the last call to this
 * method
 */
API::Workspace_sptr KafkaHistoStreamDecoder::extractData() {
  g_log.warning() << "extractData" << "\n";

  if (m_exception) {
    throw *m_exception;
  }

  auto workspace_ptr = extractDataImpl();
  return workspace_ptr;
}

// -----------------------------------------------------------------------------
// Private members
// -----------------------------------------------------------------------------

API::Workspace_sptr KafkaHistoStreamDecoder::extractDataImpl() {
  g_log.warning() << "extractDataImpl" << "\n";

  std::lock_guard<std::mutex> lock(m_workspace_mutex);

  if (!m_capturing) {
    throw Exception::NotYet("Local buffers not initialized.");
  }

  auto temp = copyBufferWorkspace(m_workspace);
  g_log.warning() << "swap" << "\n";
  std::swap(m_workspace, temp);
  g_log.warning() << "return" << "\n";
  return temp;
}

/**
 * Start decoding data from the streams into the internal buffers.
 * Implementation designed to be entry point for new thread of execution.
 * It catches all thrown exceptions.
 */
void KafkaHistoStreamDecoder::captureImpl() {
  g_log.warning() << "captureImpl" << "\n";

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
  g_log.warning("Event capture starting");

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

    auto histoMsg = GetPulseImage(
        reinterpret_cast<const uint8_t *>(buffer.c_str()));

    const auto &detectorIDs = *(histoMsg->detector_id());
    const auto &detectionCounts = *(histoMsg->detection_count());

    auto nCounts = detectionCounts.size();
    g_log.warning() << "Pulse Image Counts: " << nCounts << "\n";

    {
      std::lock_guard<std::mutex> lock(m_workspace_mutex);
      for (decltype(nCounts) i = 0; i < nCounts; ++i) {
        // Actually these are spectrum numbers
        const auto detID = detectorIDs[i];
        const auto detCount  = detectionCounts[i];
        const auto curCount =  m_workspace->counts(detID - 1);
        m_workspace->setCounts(detID - 1, curCount + detCount);
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  g_log.debug("Histo capture finished");
}

DataObjects::Workspace2D_sptr KafkaHistoStreamDecoder::createBufferWorkspace() {
  g_log.warning() << "createBufferWorkspace" << "\n";

  API::MatrixWorkspace_sptr workspace;

  try {
    auto alg =
        API::AlgorithmManager::Instance().createUnmanaged("LoadEmptyInstrument");
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

/**
 * Create new buffer workspace from an existing copy
 * @param parent A pointer to an existing workspace
 */
DataObjects::Workspace2D_sptr KafkaHistoStreamDecoder::copyBufferWorkspace(
    const DataObjects::Workspace2D_sptr &workspace) {
  g_log.warning() << "copyBufferWorkspace" << "\n";

  API::MatrixWorkspace_sptr copy = API::WorkspaceFactory::Instance().create(
      "Workspace2D", workspace->getNumberHistograms(), 2, 1);

  g_log.warning() << "initializeFromParent" << "\n";
  API::WorkspaceFactory::Instance().initializeFromParent(*workspace, *copy,
                                                         false);

  return boost::dynamic_pointer_cast<DataObjects::Workspace2D>(copy);
}


} // namespace LiveData
} // namespace Mantid
