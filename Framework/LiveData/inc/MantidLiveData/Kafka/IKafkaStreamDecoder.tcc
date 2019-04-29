#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidLiveData/Kafka/IKafkaStreamDecoder.h"

namespace {
Mantid::Kernel::Logger logger("IKafkaStreamDecoder");
}

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
boost::shared_ptr<T> IKafkaStreamDecoder::createBufferWorkspace(
    const std::string &workspaceClassName, size_t nspectra, const int32_t *spec,
    const int32_t *udet, uint32_t length) {
  // Get spectra to detector mapping
  auto spdetMap = buildSpectrumToDetectorMap(spec, udet, length);
  assert(spdetMap.size() == nspectra);

  // Create histo workspace
  auto buffer =
      boost::static_pointer_cast<T>(API::WorkspaceFactory::Instance().create(
          workspaceClassName, nspectra, 2, 1));

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
boost::shared_ptr<T> IKafkaStreamDecoder::createBufferWorkspace(
    const std::string &workspaceClassName, const boost::shared_ptr<T> &parent) {
  auto buffer =
      boost::static_pointer_cast<T>(API::WorkspaceFactory::Instance().create(
          workspaceClassName, parent->getNumberHistograms(), 2, 1));
  // Copy meta data
  API::WorkspaceFactory::Instance().initializeFromParent(*parent, *buffer,
                                                         false);
  // Clear out the old logs, except for the most recent entry
  buffer->mutableRun().clearOutdatedTimeSeriesLogValues();
  return buffer;
}

/**
 * Run LoadInstrument for the given instrument name. If it cannot succeed it
 * does nothing to the internal workspace
 * @param name Name of an instrument to load
 * @param workspace A pointer to the workspace receiving the instrument
 */
template <typename T>
void IKafkaStreamDecoder::loadInstrument(const std::string &name,
                                         boost::shared_ptr<T> workspace) {
  if (name.empty()) {
    logger.warning("Empty instrument name found");
    return;
  }
  try {
    auto alg =
        API::AlgorithmManager::Instance().createUnmanaged("LoadInstrument");
    // Do not put the workspace in the ADS
    alg->setChild(true);
    alg->initialize();
    alg->setPropertyValue("InstrumentName", name);
    alg->setProperty("Workspace", workspace);
    alg->setProperty("RewriteSpectraMap", Kernel::OptionalBool(false));
    alg->execute();
  } catch (std::exception &exc) {
    logger.warning() << "Error loading instrument '" << name
                    << "': " << exc.what() << "\n";
  }
}
} // namespace LiveData
} // namespace Mantid
