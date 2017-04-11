#include "MantidDataHandling/ScanningWorkspaceBuilder.h"

#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidGeometry/Instrument.h"

using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

ScanningWorkspaceBuilder::ScanningWorkspaceBuilder(size_t nDetectors,
                                                   size_t nTimeIndexes,
                                                   size_t nBins)
    : m_nDetectors(nDetectors), m_nTimeIndexes(nTimeIndexes), m_nBins(nBins),
      m_indexInfo({}, {}) {}

void ScanningWorkspaceBuilder::setInstrument(
    boost::shared_ptr<const Geometry::Instrument> instrument) {
  if (instrument->getNumberDetectors() < m_nDetectors) {
    throw std::logic_error("There are not enough detectors in the instrument "
                           "for the number of detectors set in the scanning "
                           "workspace builder.");
  }

  m_instrument = instrument;
}

void ScanningWorkspaceBuilder::setTimeRanges(const std::vector<
    std::pair<Kernel::DateAndTime, Kernel::DateAndTime>> &timeRanges) {
  verifyTimeIndexSize(timeRanges.size(), "start time, end time pairs");
  m_timeRanges = timeRanges;
}

void ScanningWorkspaceBuilder::setTimeRanges(
    const Kernel::DateAndTime &startTime,
    const std::vector<double> &durations) {
  verifyTimeIndexSize(durations.size(), "time durations");

  std::vector<std::pair<Kernel::DateAndTime, Kernel::DateAndTime>> timeRanges =
      {std::pair<Kernel::DateAndTime, Kernel::DateAndTime>(
          startTime, startTime + durations[0])};

  for (size_t i = 1; i < m_nTimeIndexes; ++i) {
    const auto newStartTime = timeRanges[i - 1].second;
    const auto endTime = newStartTime + durations[i];
    timeRanges.push_back(std::pair<Kernel::DateAndTime, Kernel::DateAndTime>(
        newStartTime, endTime));
  }

  setTimeRanges(timeRanges);
}

void ScanningWorkspaceBuilder::setPositions(
    std::vector<std::vector<Kernel::V3D>> &positions) {
  for (const auto &vector : positions) {
    verifyTimeIndexSize(vector.size(), "positions");
  }
  verifyDetectorSize(positions.size(), "positions");

  m_positions = positions;
}

void ScanningWorkspaceBuilder::setRotations(
    std::vector<std::vector<Kernel::Quat>> &rotations) {
  for (const auto &vector : rotations) {
    verifyTimeIndexSize(vector.size(), "rotations");
  }
  verifyDetectorSize(rotations.size(), "rotations");

  m_rotations = rotations;
}

/**
 * Set a vector of rotations corresponding to each time index. These angles
 *rotate the detector banks around the source, setting the corresponding
 *positions and rotations of the detectors.
 *
 * Here explicit assumptions are made - that the source is at (0, 0, 0), and the
 *rotation is in the X-Z plane. This corresponds to the common case of moving
 *detectors to increase angular coverage.
 *
 * @param instrumentAngles a vector of angles, the size matching the number of
 *time indexes
 */
void ScanningWorkspaceBuilder::setInstrumentAngles(
    std::vector<double> &instrumentAngles) {

  if (!m_positions.empty() || !m_rotations.empty())
    throw std::logic_error("Can not set instrument angles, as positions and/or "
                           "rotations have already been set.");

  verifyTimeIndexSize(instrumentAngles.size(), "instrument angles");
  m_instrumentAngles = instrumentAngles;
}

void ScanningWorkspaceBuilder::setIndexingType(IndexingType indexingType) {
  if (m_indexingType != IndexingType::DEFAULT)
    throw std::logic_error("Indexing type has been set already.");

  m_indexingType = indexingType;
}

MatrixWorkspace_sptr ScanningWorkspaceBuilder::buildWorkspace() {
  validateInputs();

  auto outputWorkspace = WorkspaceFactory::Instance().create(
      "Workspace2D", m_nDetectors * m_nTimeIndexes, m_nBins + 1, m_nBins);
  outputWorkspace->setInstrument(m_instrument);

  MatrixWorkspace_const_sptr parentWorkspace = outputWorkspace->clone();

  auto &outputDetectorInfo = outputWorkspace->mutableDetectorInfo();
  outputDetectorInfo.setScanInterval(0, m_timeRanges[0]);

  for (size_t i = 1; i < m_nTimeIndexes; ++i) {
    auto mergeWorkspace =
        WorkspaceFactory::Instance().create(parentWorkspace, m_nDetectors);
    auto &mergeDetectorInfo = mergeWorkspace->mutableDetectorInfo();
    for (size_t j = 0; j < m_nDetectors; ++j) {
      mergeDetectorInfo.setScanInterval(j, m_timeRanges[i]);
    }
    outputDetectorInfo.merge(mergeDetectorInfo);
  }

  if (!m_positions.empty())
    buildPositions(outputDetectorInfo);

  if (!m_rotations.empty())
    buildRotations(outputDetectorInfo);

  if (!m_instrumentAngles.empty())
    buildInstrumentAngles(outputDetectorInfo);

  switch (m_indexingType) {
  case IndexingType::DEFAULT:
  case IndexingType::TIME_ORIENTED:
    createTimeOrientedIndexInfo(outputDetectorInfo);
    break;
  case IndexingType::DETECTOR_ORIENTED:
    createDetectorOrientedIndexInfo(outputDetectorInfo);
  }

  outputWorkspace->setIndexInfo(m_indexInfo);

  return outputWorkspace;
}

void ScanningWorkspaceBuilder::buildRotations(
    DetectorInfo &outputDetectorInfo) const {
  for (size_t i = 0; i < m_nDetectors; ++i) {
    for (size_t j = 0; j < m_nTimeIndexes; ++j) {
      outputDetectorInfo.setRotation({i, j}, m_rotations[i][j]);
    }
  }
}

void ScanningWorkspaceBuilder::buildPositions(
    DetectorInfo &outputDetectorInfo) const {
  for (size_t i = 0; i < m_nDetectors; ++i) {
    for (size_t j = 0; j < m_nTimeIndexes; ++j) {
      outputDetectorInfo.setPosition({i, j}, m_positions[i][j]);
    }
  }
}

void ScanningWorkspaceBuilder::buildInstrumentAngles(
    DetectorInfo &outputDetectorInfo) const {
  for (size_t i = 0; i < outputDetectorInfo.size(); ++i) {
    for (size_t j = 0; j < outputDetectorInfo.scanCount(i); ++j) {
      auto position = outputDetectorInfo.position({i, j});
      const auto rotation =
          Kernel::Quat(m_instrumentAngles[j], Kernel::V3D(0, 1, 0));
      rotation.rotate(position);
      outputDetectorInfo.setPosition({i, j}, position);
      outputDetectorInfo.setRotation({i, j}, rotation);
    }
  }
}

void ScanningWorkspaceBuilder::createTimeOrientedIndexInfo(
    const DetectorInfo &detectorInfo) {
  const auto &detectorIDs = detectorInfo.detectorIDs();

  std::vector<specnum_t> spectra;
  std::vector<std::vector<detid_t>> detectorID;

  for (size_t i = 0; i < m_nDetectors; ++i) {
    for (size_t j = 0; j < m_nTimeIndexes; ++j) {
      spectra.push_back(int(j + i * m_nTimeIndexes + 1));
      std::vector<detid_t> detectors = {detectorIDs[i]};
      detectorID.push_back(detectors);
    }
  }

  m_indexInfo = Indexing::IndexInfo(std::move(spectra), std::move(detectorID));
}

void ScanningWorkspaceBuilder::createDetectorOrientedIndexInfo(
    const DetectorInfo &detectorInfo) {
  const auto &detectorIDs = detectorInfo.detectorIDs();

  std::vector<specnum_t> spectra;
  std::vector<std::vector<detid_t>> detectorID;

  for (size_t i = 0; i < m_nTimeIndexes; ++i) {
    for (size_t j = 0; j < m_nDetectors; ++j) {
      spectra.push_back(int(j + i * m_nDetectors + 1));
      std::vector<detid_t> detectors = {detectorIDs[j]};
      detectorID.push_back(detectors);
    }
  }

  m_indexInfo = Indexing::IndexInfo(std::move(spectra), std::move(detectorID));
}

void ScanningWorkspaceBuilder::verifyTimeIndexSize(
    size_t timeIndexSize, const std::string &description) const {
  if (timeIndexSize != m_nTimeIndexes) {
    throw std::logic_error(
        "Number of " + description +
        " supplied does not match the number of time indexes.");
  }
}

void ScanningWorkspaceBuilder::verifyDetectorSize(
    size_t detectorSize, const std::string &description) const {
  if (detectorSize != m_nDetectors) {
    throw std::logic_error("Number of " + description +
                           " supplied does not match the number of detectors.");
  }
}

void ScanningWorkspaceBuilder::validateInputs() const {
  if (!m_instrument)
    throw std::logic_error("Can not build workspace - instrument has not been "
                           "set. Please call setInstrument() before building.");

  if (m_timeRanges.empty())
    throw std::logic_error("Can not build workspace - time ranges have not "
                           "been set. Please call setTimeRanges() before "
                           "building.");
}

} // namespace DataHandling
} // namespace Mantid
