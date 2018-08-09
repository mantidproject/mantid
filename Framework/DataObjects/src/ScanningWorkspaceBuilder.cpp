#include "MantidDataObjects/ScanningWorkspaceBuilder.h"

#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidHistogramData/Points.h"
#include "MantidTypes/SpectrumDefinition.h"

using namespace Mantid::API;
using namespace Mantid::HistogramData;
using namespace Mantid::Indexing;

namespace Mantid {
namespace DataObjects {

/**
 * Create the scanning workspace builder. Time ranges must still be set before
 *this can be used.
 *
 * @param instrument A pointer to the base instrument for the workspace
 * @param nTimeIndexes The number of time indexes to create
 * @param nBins The number of bins (or points) for each spectrum
 * @param isPointData If true will use points for the x-axis instead of bins
 */
ScanningWorkspaceBuilder::ScanningWorkspaceBuilder(
    const boost::shared_ptr<const Geometry::Instrument> &instrument,
    const size_t nTimeIndexes, const size_t nBins, const bool isPointData)
    : m_nDetectors(instrument->getNumberDetectors()),
      m_nTimeIndexes(nTimeIndexes), m_nBins(nBins), m_instrument(instrument),
      m_histogram(BinEdges(nBins + 1, LinearGenerator(1.0, 1.0)),
                  Counts(nBins, 0.0)),
      m_indexingType(IndexingType::Default) {
  if (isPointData)
    m_histogram = HistogramData::Histogram(Points(nBins), Counts(nBins, 0.0));
}

/**
 * Set a histogram to be used for all the workspace spectra. This can be used to
 *set the correct bin edges, but only if the binning is identical for every
 *spectra.
 *
 * @param histogram A histogram with bin edges defined
 */
void ScanningWorkspaceBuilder::setHistogram(
    HistogramData::Histogram histogram) {
  if (histogram.size() != m_nBins)
    throw std::logic_error(
        "Histogram supplied does not have the correct size.");

  m_histogram = std::move(histogram);
}

/**
 * Set time ranges from a vector of start time, end time pairs.
 *
 * @param timeRanges A vector of DateAndTime pairs, corresponding to the start
 *and end times
 */
void ScanningWorkspaceBuilder::setTimeRanges(
    std::vector<std::pair<Types::Core::DateAndTime, Types::Core::DateAndTime>>
        timeRanges) {
  verifyTimeIndexSize(timeRanges.size(), "start time, end time pairs");
  m_timeRanges = std::move(timeRanges);
}

/**
 * Set time ranges from a start time and a vector of durations
 *
 * @param startTime A DateAndTime object corresponding to the start of the first
 *scan
 * @param durations A vector of doubles containing the duration in seconds
 */
void ScanningWorkspaceBuilder::setTimeRanges(
    const Types::Core::DateAndTime &startTime,
    const std::vector<double> &durations) {
  verifyTimeIndexSize(durations.size(), "time durations");

  std::vector<std::pair<Types::Core::DateAndTime, Types::Core::DateAndTime>>
      timeRanges = {
          std::pair<Types::Core::DateAndTime, Types::Core::DateAndTime>(
              startTime, startTime + durations[0])};

  for (size_t i = 1; i < m_nTimeIndexes; ++i) {
    const auto newStartTime = timeRanges[i - 1].second;
    const auto endTime = newStartTime + durations[i];
    timeRanges.push_back(
        std::pair<Types::Core::DateAndTime, Types::Core::DateAndTime>(
            newStartTime, endTime));
  }

  setTimeRanges(std::move(timeRanges));
}

/**
 * Supply a vector of vectors which contain positions. The inner vectors should
 *contain the position for each time index, the outer vector the vector for each
 *detector.
 *
 * @param positions A vector of vectors containing positions
 */
void ScanningWorkspaceBuilder::setPositions(
    std::vector<std::vector<Kernel::V3D>> positions) {

  if (!m_positions.empty() || !m_instrumentAngles.empty())
    throw std::logic_error("Can not set positions, as positions or instrument "
                           "angles have already been set.");

  for (const auto &vector : positions) {
    verifyTimeIndexSize(vector.size(), "positions");
  }
  verifyDetectorSize(positions.size(), "positions");

  m_positions = std::move(positions);
}

/**
 * Supply a vector of vectors which contain rotations. The inner vectors should
 *contain the rotation for each time index, the outer vector the vector for each
 *detector.
 *
 * @param rotations A vector of vectors containing rotations
 */
void ScanningWorkspaceBuilder::setRotations(
    std::vector<std::vector<Kernel::Quat>> rotations) {

  if (!m_rotations.empty() || !m_instrumentAngles.empty())
    throw std::logic_error("Can not set rotations, as rotations or instrument "
                           "angles have already been set.");

  for (const auto &vector : rotations) {
    verifyTimeIndexSize(vector.size(), "rotations");
  }
  verifyDetectorSize(rotations.size(), "rotations");

  m_rotations = std::move(rotations);
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
 * @param relativeRotations a vector of angles, the size matching the number of
 *time indexes
 * @param rotationPosition the position to rotate around, e.g. the sample
 *position
 * @param rotationAxis the axis to rotate around. e.g. the vertical axis to
 *rotate the instrument in the horizontal plane
 */
void ScanningWorkspaceBuilder::setRelativeRotationsForScans(
    std::vector<double> relativeRotations, const Kernel::V3D &rotationPosition,
    const Kernel::V3D &rotationAxis) {

  if (!m_positions.empty() || !m_rotations.empty())
    throw std::logic_error("Can not set instrument angles, as positions and/or "
                           "rotations have already been set.");

  verifyTimeIndexSize(relativeRotations.size(), "instrument angles");
  m_instrumentAngles = std::move(relativeRotations);
  m_rotationPosition = rotationPosition;
  m_rotationAxis = rotationAxis;
}

/**
 * Set the indexing type, either to time or detector oriented indexing.
 *
 * @param indexingType An index type enum
 */
void ScanningWorkspaceBuilder::setIndexingType(
    const IndexingType indexingType) {
  if (m_indexingType != IndexingType::Default)
    throw std::logic_error("Indexing type has been set already.");

  m_indexingType = indexingType;
}

/**
 * Verify everything has been set that is required and return the workspace.
 *
 * @return Workspace2D with the scanning information set
 */
MatrixWorkspace_sptr ScanningWorkspaceBuilder::buildWorkspace() const {
  validateInputs();

  auto outputWorkspace = create<Workspace2D>(
      m_instrument, m_nDetectors * m_nTimeIndexes, m_histogram);

  auto &outputDetectorInfo = outputWorkspace->mutableDetectorInfo();
  outputDetectorInfo.setScanInterval(m_timeRanges[0]);

  buildOutputDetectorInfo(outputDetectorInfo);

  if (!m_positions.empty())
    buildPositions(outputDetectorInfo);

  if (!m_rotations.empty())
    buildRotations(outputDetectorInfo);

  if (!m_instrumentAngles.empty())
    buildRelativeRotationsForScans(outputDetectorInfo);

  switch (m_indexingType) {
  case IndexingType::Default:
    outputWorkspace->setIndexInfo(
        Indexing::IndexInfo(m_nDetectors * m_nTimeIndexes));
    break;
  case IndexingType::TimeOriented:
    createTimeOrientedIndexInfo(*outputWorkspace);
    break;
  case IndexingType::DetectorOriented:
    createDetectorOrientedIndexInfo(*outputWorkspace);
    break;
  }

  return boost::shared_ptr<MatrixWorkspace>(std::move(outputWorkspace));
}

void ScanningWorkspaceBuilder::buildOutputDetectorInfo(
    Geometry::DetectorInfo &outputDetectorInfo) const {
  auto mergeWorkspace =
      create<Workspace2D>(m_instrument, m_nDetectors, m_histogram.binEdges());
  for (size_t i = 1; i < m_nTimeIndexes; ++i) {
    auto &mergeDetectorInfo = mergeWorkspace->mutableDetectorInfo();
    mergeDetectorInfo.setScanInterval(m_timeRanges[i]);
    outputDetectorInfo.merge(mergeDetectorInfo);
  }
}

void ScanningWorkspaceBuilder::buildRotations(
    Geometry::DetectorInfo &outputDetectorInfo) const {
  for (size_t i = 0; i < m_nDetectors; ++i) {
    for (size_t j = 0; j < m_nTimeIndexes; ++j) {
      outputDetectorInfo.setRotation({i, j}, m_rotations[i][j]);
    }
  }
}

void ScanningWorkspaceBuilder::buildPositions(
    Geometry::DetectorInfo &outputDetectorInfo) const {
  for (size_t i = 0; i < m_nDetectors; ++i) {
    for (size_t j = 0; j < m_nTimeIndexes; ++j) {
      outputDetectorInfo.setPosition({i, j}, m_positions[i][j]);
    }
  }
}

void ScanningWorkspaceBuilder::buildRelativeRotationsForScans(
    Geometry::DetectorInfo &outputDetectorInfo) const {
  for (size_t i = 0; i < outputDetectorInfo.size(); ++i) {
    for (size_t j = 0; j < outputDetectorInfo.scanCount(i); ++j) {
      if (outputDetectorInfo.isMonitor({i, j}))
        continue;
      auto position = outputDetectorInfo.position({i, j});
      const auto rotation = Kernel::Quat(m_instrumentAngles[j], m_rotationAxis);
      position -= m_rotationPosition;
      rotation.rotate(position);
      position += m_rotationPosition;
      outputDetectorInfo.setPosition({i, j}, position);
      const auto &oldRotation = outputDetectorInfo.rotation({i, j});
      outputDetectorInfo.setRotation({i, j}, rotation * oldRotation);
    }
  }
}

void ScanningWorkspaceBuilder::createTimeOrientedIndexInfo(
    MatrixWorkspace &ws) const {
  auto indexInfo = ws.indexInfo();
  auto spectrumDefinitions =
      std::vector<SpectrumDefinition>(m_nDetectors * m_nTimeIndexes);

  for (size_t detIndex = 0; detIndex < m_nDetectors; ++detIndex) {
    for (size_t timeIndex = 0; timeIndex < m_nTimeIndexes; ++timeIndex) {
      spectrumDefinitions[detIndex * m_nTimeIndexes + timeIndex].add(detIndex,
                                                                     timeIndex);
    }
  }

  indexInfo.setSpectrumDefinitions(spectrumDefinitions);
  ws.setIndexInfo(indexInfo);
}

void ScanningWorkspaceBuilder::createDetectorOrientedIndexInfo(
    MatrixWorkspace &ws) const {
  auto indexInfo = ws.indexInfo();
  auto spectrumDefinitions =
      std::vector<SpectrumDefinition>(m_nDetectors * m_nTimeIndexes);

  for (size_t timeIndex = 0; timeIndex < m_nTimeIndexes; ++timeIndex) {
    for (size_t detIndex = 0; detIndex < m_nDetectors; ++detIndex) {
      spectrumDefinitions[timeIndex * m_nDetectors + detIndex].add(detIndex,
                                                                   timeIndex);
    }
  }

  indexInfo.setSpectrumDefinitions(spectrumDefinitions);
  ws.setIndexInfo(indexInfo);
}

void ScanningWorkspaceBuilder::verifyTimeIndexSize(
    const size_t timeIndexSize, const std::string &description) const {
  if (timeIndexSize != m_nTimeIndexes) {
    throw std::logic_error(
        "Number of " + description +
        " supplied does not match the number of time indexes.");
  }
}

void ScanningWorkspaceBuilder::verifyDetectorSize(
    const size_t detectorSize, const std::string &description) const {
  if (detectorSize != m_nDetectors) {
    throw std::logic_error("Number of " + description +
                           " supplied does not match the number of detectors.");
  }
}

void ScanningWorkspaceBuilder::validateInputs() const {
  if (m_timeRanges.empty())
    throw std::logic_error("Can not build workspace - time ranges have not "
                           "been set. Please call setTimeRanges() before "
                           "building.");
}

} // namespace DataObjects
} // namespace Mantid
