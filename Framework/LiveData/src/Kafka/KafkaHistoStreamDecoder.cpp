// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2008 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
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
#include "MantidLiveData/Kafka/IKafkaStreamDecoder.hxx"
#include "MantidNexusGeometry/JSONGeometryParser.h"

GNU_DIAG_OFF("conversion")
#include "private/Schema/df12_det_spec_map_generated.h"
#include "private/Schema/hs00_event_histogram_generated.h"
GNU_DIAG_ON("conversion")

#include <json/json.h>

#include <utility>

namespace {
const std::string PROTON_CHARGE_PROPERTY = "proton_charge";
const std::string RUN_NUMBER_PROPERTY = "run_number";
const std::string RUN_START_PROPERTY = "run_start";
/// Logger
Mantid::Kernel::Logger g_log("KafkaHistoStreamDecoder");

const std::string HISTO_MESSAGE_ID = "hs00";
} // namespace

using namespace HistoSchema;

namespace Mantid::LiveData {

// -----------------------------------------------------------------------------
// Public members
// -----------------------------------------------------------------------------

/**
 * Constructor
 * @param broker Kafka broker
 * @param histoTopic The name of the topic streaming the histo data
 * run mapping
 */
KafkaHistoStreamDecoder::KafkaHistoStreamDecoder(std::shared_ptr<IKafkaBroker> broker, const std::string &histoTopic,
                                                 const std::string &runInfoTopic, const std::string &sampleEnvTopic,
                                                 const std::string &chopperTopic)
    : IKafkaStreamDecoder(std::move(broker), histoTopic, runInfoTopic, sampleEnvTopic, chopperTopic, ""),
      m_workspace() {}

/**
 * Destructor.
 * Stops capturing from the stream
 */
KafkaHistoStreamDecoder::~KafkaHistoStreamDecoder() = default;

KafkaHistoStreamDecoder::KafkaHistoStreamDecoder(KafkaHistoStreamDecoder &&rval) noexcept
    : IKafkaStreamDecoder(std::move(rval)) {
  {
    std::lock_guard lck(m_mutex);
    m_buffer = std::move(rval.m_buffer);
  }
}

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

  API::MatrixWorkspace_sptr ws{DataObjects::create<DataObjects::Workspace2D>(*m_workspace, nspectra, binedges)};

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
  initLocalCaches(runStartStruct);

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
        checkRunEnd(topicName, checkOffsets, offset, partition, stopOffsets, reachedEnd);
        if (offset > stopOffsets[topicName][static_cast<size_t>(partition)]) {
          // If the offset is beyond the end of the current run, then skip to
          // the next iteration and don't process the message
          m_cbIterationEnd();
          continue;
        }
      }

      // Check if we have a histo message
      // Most will be event messages so we check for this type first
      if (flatbuffers::BufferHasIdentifier(reinterpret_cast<const uint8_t *>(buffer.c_str()),
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

void KafkaHistoStreamDecoder::initLocalCaches(const RunStartStruct &runStartData) {
  m_runId = runStartData.runId;

  const auto jsonGeometry = runStartData.nexusStructure;
  const auto instName = runStartData.instrumentName;

  DataObjects::Workspace2D_sptr histoBuffer;
  if (!runStartData.detSpecMapSpecified) {
    /* Load the instrument to get the number of spectra :c */
    auto ws = API::WorkspaceFactory::Instance().create("Workspace2D", 1, 2, 1);
    loadInstrument<API::MatrixWorkspace>(instName, ws, jsonGeometry);
    const auto nspec = ws->getInstrument()->getNumberDetectors();

    // Create buffer
    histoBuffer = std::static_pointer_cast<DataObjects::Workspace2D>(
        API::WorkspaceFactory::Instance().create("Workspace2D", nspec, 2, 1));
    histoBuffer->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
    histoBuffer->setYUnit("Counts");

    /* Need a mapping with spectra numbers starting at zero */
    histoBuffer->rebuildSpectraMapping(true, 0);
    histoBuffer->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
    histoBuffer->setYUnit("Counts");
  } else {
    // Create buffer
    histoBuffer = createBufferWorkspace<DataObjects::Workspace2D>(
        "Workspace2D", runStartData.numberOfSpectra, runStartData.spectrumNumbers.data(),
        runStartData.detectorIDs.data(), static_cast<uint32_t>(runStartData.detectorIDs.size()));
  }

  // Load the instrument if possible but continue if we can't
  if (!loadInstrument<DataObjects::Workspace2D>(instName, histoBuffer, jsonGeometry))
    g_log.warning("Instrument could not be loaded. Continuing without instrument");

  auto &mutableRun = histoBuffer->mutableRun();
  // Run start. Cache locally for computing frame times
  // Convert nanoseconds to seconds (and discard the extra precision)
  auto runStartTime = static_cast<time_t>(runStartData.startTime / 1000000000);
  m_runStart.set_from_time_t(runStartTime);
  auto timeString = m_runStart.toISO8601String();
  // Run number
  mutableRun.addProperty(RUN_START_PROPERTY, std::string(timeString));
  mutableRun.addProperty(RUN_NUMBER_PROPERTY, runStartData.runId);
  // Create the proton charge property
  mutableRun.addProperty(new Kernel::TimeSeriesProperty<double>(PROTON_CHARGE_PROPERTY));

  // Buffers for each period
  const size_t nperiods = runStartData.nPeriods;
  if (nperiods > 1) {
    throw std::runtime_error("KafkaHistoStreamDecoder - Does not support multi-period data.");
  }
  // New caches so LoadLiveData's output workspace needs to be replaced
  m_dataReset = true;

  m_workspace = histoBuffer;
}

void KafkaHistoStreamDecoder::sampleDataFromMessage(const std::string & /*buffer*/) {
  throw Kernel::Exception::NotImplementedError("This method will require "
                                               "implementation when processing "
                                               "sample environment messages.");
}

} // namespace Mantid::LiveData
