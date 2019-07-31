// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
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
#include "MantidIndexing/IndexInfo.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidLiveData/Exception.h"
#include "MantidLiveData/Kafka/IKafkaStreamDecoder.tcc"

GNU_DIAG_OFF("conversion")
#include "private/Schema/df12_det_spec_map_generated.h"
#include "private/Schema/hs00_event_histogram_generated.h"
GNU_DIAG_ON("conversion")

using namespace HistoSchema;

namespace {
const std::string PROTON_CHARGE_PROPERTY = "proton_charge";
const std::string RUN_NUMBER_PROPERTY = "run_number";
const std::string RUN_START_PROPERTY = "run_start";
/// Logger
Mantid::Kernel::Logger g_log("KafkaHistoStreamDecoder");

const std::string HISTO_MESSAGE_ID = "hs00";
} // namespace

namespace Mantid {
namespace LiveData {

// -----------------------------------------------------------------------------
// Public members
// -----------------------------------------------------------------------------

/**
 * Constructor
 * @param broker Kafka broker
 * @param histoTopic The name of the topic streaming the histo data
 * @param spDetTopic The name of the topic streaming the spectrum-detector
 * run mapping
 */
KafkaHistoStreamDecoder::KafkaHistoStreamDecoder(
    std::shared_ptr<IKafkaBroker> broker, const std::string &histoTopic,
    const std::string &runInfoTopic, const std::string &spDetTopic,
    const std::string &sampleEnvTopic)
    : IKafkaStreamDecoder(broker, histoTopic, runInfoTopic, spDetTopic,
                          sampleEnvTopic),
      m_workspace() {}

/**
 * Destructor.
 * Stops capturing from the stream
 */
KafkaHistoStreamDecoder::~KafkaHistoStreamDecoder() {}

/**
 * Check if there is data available to extract
 * @return True if data has been accumulated so that extractData()
 * can be called, false otherwise
 */
bool KafkaHistoStreamDecoder::hasData() const noexcept {
  std::lock_guard<std::mutex> lock(m_mutex);
  return !m_buffer.empty();
}

// -----------------------------------------------------------------------------
// Private members
// -----------------------------------------------------------------------------

API::Workspace_sptr KafkaHistoStreamDecoder::extractDataImpl() {
  std::lock_guard<std::mutex> lock(m_mutex);

  if (!m_capturing) {
    throw Exception::NotYet("Local buffers not initialized.");
  }

  if (m_buffer.empty()) {
    throw Exception::NotYet("No message to process yet.");
  }

  // Retrieve flatbuffer struct describing histogram
  const auto *histoMsg = GetEventHistogram(m_buffer.c_str());

  const auto *shape = histoMsg->current_shape();
  auto nbins = shape->Get(0) - 1;
  auto nspectra = static_cast<size_t>(shape->Get(1));

  auto metadata = histoMsg->dim_metadata();
  auto metadimx = metadata->GetAs<DimensionMetaData>(0);
  auto metadimy = metadata->GetAs<DimensionMetaData>(1);
  auto xbins = metadimx->bin_boundaries_as_ArrayDouble()->value();

  // Compiler warnings if one tries to use xbins->begin()/end()
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
 * Exception-throwing variant of captureImpl(). Do not call this directly
 */
void KafkaHistoStreamDecoder::captureImplExcept() {
  g_log.information("Event capture starting");

  m_interrupt = false; // Allow MonitorLiveData or user to interrupt
  m_endRun = false;
  m_runStatusSeen = false; // Flag to ensure MonitorLiveData observes end of run
  std::string buffer;
  std::string runBuffer;
  int64_t offset;
  int32_t partition;
  std::string topicName;
  auto runStartStruct = getRunStartMessage(runBuffer);
  m_spDetStream->consumeMessage(&buffer, offset, partition, topicName);
  initLocalCaches(buffer, runStartStruct);

  // Keep track of whether we've reached the end of a run
  std::unordered_map<std::string, std::vector<int64_t>> stopOffsets;
  std::unordered_map<std::string, std::vector<bool>> reachedEnd;
  bool checkOffsets = false;

  while (!m_interrupt) {
    if (m_endRun) {
      waitForRunEndObservation();
      continue;
    } else {
      waitForDataExtraction();
    }

    {
      // Lock so we don't overwrite buffer while workspace is being extracted or
      // try to access data before it is read.
      std::lock_guard<std::mutex> lock(m_mutex);
      // Pull in data
      m_dataStream->consumeMessage(&buffer, offset, partition, topicName);

      // No events, wait for some to come along...
      if (buffer.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        m_cbIterationEnd();
        continue;
      }

      if (checkOffsets) {
        checkRunEnd(topicName, checkOffsets, offset, partition, stopOffsets,
                    reachedEnd);
        if (offset > stopOffsets[topicName][static_cast<size_t>(partition)]) {
          // If the offset is beyond the end of the current run, then skip to
          // the next iteration and don't process the message
          m_cbIterationEnd();
          continue;
        }
      }

      // Check if we have a histo message
      // Most will be event messages so we check for this type first
      if (flatbuffers::BufferHasIdentifier(
              reinterpret_cast<const uint8_t *>(buffer.c_str()),
              HISTO_MESSAGE_ID.c_str())) {
        // Data being accumulated before being streamed so no need to store
        // messages.
        m_buffer = buffer;
      } else
        checkRunMessage(buffer, checkOffsets, stopOffsets, reachedEnd);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    m_cbIterationEnd();
  }
  g_log.debug("Histo capture finished");
}

void KafkaHistoStreamDecoder::initLocalCaches(
    const std::string &rawMsgBuffer, const RunStartStruct &runStartData) {
  if (rawMsgBuffer.empty()) {
    throw std::runtime_error("KafkaEventStreamDecoder::initLocalCaches() - "
                             "Empty message received from spectrum-detector "
                             "topic. Unable to continue");
  }
  auto spDetMsg = GetSpectraDetectorMapping(
      reinterpret_cast<const uint8_t *>(rawMsgBuffer.c_str()));
  auto nspec = static_cast<uint32_t>(spDetMsg->n_spectra());
  auto nudet = spDetMsg->detector_id()->size();
  if (nudet != nspec) {
    std::ostringstream os;
    os << "KafkaEventStreamDecoder::initLocalEventBuffer() - Invalid "
          "spectra/detector mapping. Expected matched length arrays but "
          "found nspec="
       << nspec << ", ndet=" << nudet;
    throw std::runtime_error(os.str());
  }

  m_runNumber = runStartData.runNumber;

  // Create buffer
  auto histoBuffer = createBufferWorkspace<DataObjects::Workspace2D>(
      "Workspace2D", static_cast<size_t>(spDetMsg->n_spectra()),
      spDetMsg->spectrum()->data(), spDetMsg->detector_id()->data(), nudet);

  // Load the instrument if possible but continue if we can't
  auto instName = runStartData.instrumentName;
  if (!instName.empty())
    loadInstrument<DataObjects::Workspace2D>(instName, histoBuffer);
  else
    g_log.warning(
        "Empty instrument name received. Continuing without instrument");

  auto &mutableRun = histoBuffer->mutableRun();
  // Run start. Cache locally for computing frame times
  // Convert nanoseconds to seconds (and discard the extra precision)
  auto runStartTime = static_cast<time_t>(runStartData.startTime / 1000000000);
  m_runStart.set_from_time_t(runStartTime);
  auto timeString = m_runStart.toISO8601String();
  // Run number
  mutableRun.addProperty(RUN_START_PROPERTY, std::string(timeString));
  mutableRun.addProperty(RUN_NUMBER_PROPERTY,
                         std::to_string(runStartData.runNumber));
  // Create the proton charge property
  mutableRun.addProperty(
      new Kernel::TimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY));

  // Cache spec->index mapping. We assume it is the same across all periods
  m_specToIdx =
      histoBuffer->getSpectrumToWorkspaceIndexVector(m_specToIdxOffset);

  // Buffers for each period
  const size_t nperiods = runStartData.nPeriods;
  if (nperiods > 1) {
    throw std::runtime_error(
        "KafkaHistoStreamDecoder - Does not support multi-period data.");
  }
  // New caches so LoadLiveData's output workspace needs to be replaced
  m_dataReset = true;

  m_workspace = histoBuffer;
}

void KafkaHistoStreamDecoder::sampleDataFromMessage(
    const std::string & /*buffer*/) {
  throw Kernel::Exception::NotImplementedError("This method will require "
                                               "implementation when processing "
                                               "sample environment messages.");
}

} // namespace LiveData
} // namespace Mantid
