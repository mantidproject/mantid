// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidLiveData/Kafka/IKafkaStreamDecoder.h"
#include "MantidNexusGeometry/JSONInstrumentBuilder.h"

GNU_DIAG_OFF("conversion")
#include "../src/Kafka/private/Schema/tdct_timestamps_generated.h"
GNU_DIAG_ON("conversion")

namespace {
Mantid::Kernel::Logger logger("IKafkaStreamDecoder");
const std::string CHOPPER_MESSAGE_ID = "tdct";
} // namespace

namespace Mantid {
namespace LiveData {
/**
 * Create a buffer workspace of the correct size based on the values given.
 * @param workspaceClassName the name of the workspace class to be created e.g
 * Workspace2D or EventWorkspace
 * @param nspectra The number of unique spectrum numbers
 * @param spec An array of length ndet specifying the spectrum number of each
 * detector
 * @param udet An array of length ndet specifying the detector ID of each
 * detector
 * @param length The length of the spec/udet arrays
 * @return A new workspace of the appropriate size
 */
template <typename T>
std::shared_ptr<T> IKafkaStreamDecoder::createBufferWorkspace(const std::string &workspaceClassName, size_t nspectra,
                                                              const int32_t *spec, const int32_t *udet,
                                                              uint32_t length) {
  // Get spectra to detector mapping
  auto spdetMap = buildSpectrumToDetectorMap(spec, udet, length);
  assert(spdetMap.size() == nspectra);

  // Create histo workspace
  auto buffer =
      std::static_pointer_cast<T>(API::WorkspaceFactory::Instance().create(workspaceClassName, nspectra, 2, 1));

  // Set the units
  buffer->getAxis(0)->unit() = Kernel::UnitFactory::Instance().create("TOF");
  buffer->setYUnit("Counts");
  // Setup spectra-detector mapping.
  size_t wsIdx(0);
  for (const auto &spIter : spdetMap) {
    auto &spectrum = buffer->getSpectrum(wsIdx);
    spectrum.setSpectrumNo(spIter.first);
    spectrum.addDetectorIDs(spIter.second);
    ++wsIdx;
  }
  return buffer;
}

/**
 * Create new buffer workspace from an existing copy
 * @param workspaceClassName the name of the workspace class to be created e.g
 * Workspace2D or EventWorkspace
 * @param parent A reference to an existing workspace
 */
template <typename T>
std::shared_ptr<T> IKafkaStreamDecoder::createBufferWorkspace(const std::string &workspaceClassName,
                                                              const std::shared_ptr<T> &parent) {
  auto buffer = std::static_pointer_cast<T>(
      API::WorkspaceFactory::Instance().create(workspaceClassName, parent->getNumberHistograms(), 2, 1));
  // Copy meta data
  API::WorkspaceFactory::Instance().initializeFromParent(*parent, *buffer, false);
  // Clear out the old logs, except for the most recent entry
  buffer->mutableRun().clearOutdatedTimeSeriesLogValues();
  return buffer;
}

template <typename T> void loadFromAlgorithm(const std::string &name, std::shared_ptr<T> workspace) {
  auto alg = API::AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
  // Do not put the workspace in the ADS
  alg->setChild(true);
  alg->initialize();
  alg->setPropertyValue("InstrumentName", name);
  alg->setProperty("Workspace", workspace);
  alg->setProperty("RewriteSpectraMap", Kernel::OptionalBool(false));
  alg->execute();
}

/**
 * Run LoadInstrument for the given instrument name. If it cannot succeed it
 * does nothing to the internal workspace
 * @param name Name of an instrument to load
 * @param workspace A pointer to the workspace receiving the instrument
 * @param parseJSON flag which will extract geometry from
 */
template <typename T>
bool IKafkaStreamDecoder::loadInstrument(const std::string &name, std::shared_ptr<T> workspace,
                                         const std::string &jsonGeometry) {
  auto instrument = workspace->getInstrument();
  if (instrument->getNumberDetectors() != 0) // instrument already loaded.
    return true;

  if (!name.empty()) {
    try {
      if (jsonGeometry.empty())
        loadFromAlgorithm<T>(name, workspace);
      else {
        try {
          NexusGeometry::JSONInstrumentBuilder builder("{\"nexus_structure\":" + jsonGeometry + "}");
          workspace->setInstrument(builder.buildGeometry());
        } catch (std::exception &exc) {
          logger.warning() << "Unable to load instrument from nexus_structure provided in "
                              "run start message. Falling back on trying to use Mantid's "
                              "instrument repository. Error encountered was \""
                           << exc.what() << "\"\n";
          loadFromAlgorithm<T>(name, workspace);
        }
      }
      return true;
    } catch (std::exception &exc) {
      logger.warning() << "Error loading instrument '" << name << "': \"" << exc.what()
                       << "\". The streamed data will have no associated "
                          "instrument geometry. \n";
    }
  } else {
    logger.warning() << "Empty instrument name provided. \n";
  }
  return false;
}

/**
 * Add chopper timestamps to the mutable run info of all workspaces used to
 * buffer data from the kafka stream.
 * @param workspaces buffer workspaces storing kafka data.
 */
template <typename T> void IKafkaStreamDecoder::writeChopperTimestampsToWorkspaceLogs(std::vector<T> workspaces) {
  if (!m_chopperStream)
    return;

  std::string buffer;
  int64_t offset;
  int32_t partition;
  std::string topicName;
  m_chopperStream->consumeMessage(&buffer, offset, partition, topicName);

  if (buffer.empty())
    return;

  std::lock_guard<std::mutex> workspaceLock(m_mutex);

  if (flatbuffers::BufferHasIdentifier(reinterpret_cast<const uint8_t *>(buffer.c_str()), CHOPPER_MESSAGE_ID.c_str())) {
    auto chopperMsg = Gettimestamp(reinterpret_cast<const uint8_t *>(buffer.c_str()));

    const auto *timestamps = chopperMsg->timestamps();
    std::vector<uint64_t> mantidTimestamps;
    std::copy(timestamps->begin(), timestamps->end(), std::back_inserter(mantidTimestamps));
    auto name = chopperMsg->name()->str();
    for (auto &workspace : workspaces) {
      auto mutableRunInfo = workspace->mutableRun();
      Kernel::ArrayProperty<uint64_t> *property;
      if (mutableRunInfo.hasProperty(name)) {
        property = dynamic_cast<Kernel::ArrayProperty<uint64_t> *>(mutableRunInfo.getProperty(name));
      } else {
        property = new Mantid::Kernel::ArrayProperty<uint64_t>(name);
      }
      *property = mantidTimestamps;
    }
  }
}
} // namespace LiveData
} // namespace Mantid
