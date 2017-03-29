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
    : m_nDetectors(nDetectors), m_nTimeIndexes(nTimeIndexes), m_nBins(nBins) {}

void ScanningWorkspaceBuilder::setInstrument(
    boost::shared_ptr<const Geometry::Instrument> instrument) {
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

MatrixWorkspace_sptr ScanningWorkspaceBuilder::buildWorkspace() const {
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

void ScanningWorkspaceBuilder::verifyTimeIndexSize(
    size_t timeIndexSize, const std::string &description) const {
  if (timeIndexSize != m_nTimeIndexes) {
    throw std::logic_error(
        "Number of " + description +
        " supplied does not match the number of time indexes being requested.");
  }
}

void ScanningWorkspaceBuilder::verifyDetectorSize(
    size_t detectorSize, const std::string &description) const {
  if (detectorSize != m_nDetectors) {
    throw std::logic_error(
        "Number of " + description +
        " supplied does not match the number of detectors being requested.");
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
