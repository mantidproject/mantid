#include "MantidDataObjects/ScanningWorkspaceBuilder.h"

#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidHistogramData/LinearGenerator.h"
#include "MantidTypes/SpectrumDefinition.h"

using namespace Mantid::API;
using namespace Mantid::HistogramData;
using namespace Mantid::Indexing;

namespace Mantid {
namespace DataObjects {

ScanningWorkspaceBuilder::ScanningWorkspaceBuilder(const size_t nDetectors,
                                                   const size_t nTimeIndexes,
                                                   const size_t nBins)
    : m_nDetectors(nDetectors), m_nTimeIndexes(nTimeIndexes), m_nBins(nBins),
      m_histogram(BinEdges(nBins + 1, LinearGenerator(0.0, 1.0)),
                  Counts(std::vector<double>(nBins, 0.0))),
      m_indexingType(IndexingType::DEFAULT) {}

void ScanningWorkspaceBuilder::setInstrument(
    const boost::shared_ptr<const Geometry::Instrument> &instrument) {
  if (instrument->getNumberDetectors() < m_nDetectors) {
    throw std::logic_error("There are not enough detectors in the instrument "
                           "for the number of detectors set in the scanning "
                           "workspace builder.");
  }

  m_instrument = instrument;
}

/**
 * Set a histogram to be used for all the workspace spectra. This can be used to
 *set the correct bin edges, but only if the binning is identical for every
 *spectra.
 *
 * @param histogram A histogram with bin edges defined
 */
void ScanningWorkspaceBuilder::setHistogram(
    const HistogramData::Histogram &histogram) {
  if ((histogram.counts().size() != m_nBins) ||
      (histogram.binEdges().size() != m_nBins + 1))
    throw std::logic_error(
        "Histogram supplied does not have the correct size.");

  m_histogram = histogram;
}

/**
 * Set time ranges from a vector of start time, end time pairs.
 *
 * @param timeRanges A vector of DateAndTime pairs, corresponding to the start
 *and end times
 */
void ScanningWorkspaceBuilder::setTimeRanges(const std::vector<
    std::pair<Kernel::DateAndTime, Kernel::DateAndTime>> &timeRanges) {
  verifyTimeIndexSize(timeRanges.size(), "start time, end time pairs");
  m_timeRanges = timeRanges;
}

/**
 * Set time ranges from a start time and a vector of durations
 *
 * @param startTime A DateAndTime object corresponding to the start of the first
 *scan
 * @param durations A vector of doubles containing the duration in seconds
 */
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

/**
 * Supply a vector of vectors which contain positions. The inner vectors should
 *contain the position for each time index, the outer vector the vector for each
 *detector.
 *
 * @param positions A vector of vectors containing positions
 */
void ScanningWorkspaceBuilder::setPositions(
    const std::vector<std::vector<Kernel::V3D>> &positions) {

  if (!m_positions.empty() || !m_instrumentAngles.empty())
    throw std::logic_error("Can not set positions, as positions or instrument "
                           "angles have already been set.");

  for (const auto &vector : positions) {
    verifyTimeIndexSize(vector.size(), "positions");
  }
  verifyDetectorSize(positions.size(), "positions");

  m_positions = positions;
}

/**
 * Supply a vector of vectors which contain rotations. The inner vectors should
 *contain the rotation for each time index, the outer vector the vector for each
 *detector.
 *
 * @param rotations A vector of vectors containing rotations
 */
void ScanningWorkspaceBuilder::setRotations(
    const std::vector<std::vector<Kernel::Quat>> &rotations) {

  if (!m_rotations.empty() || !m_instrumentAngles.empty())
    throw std::logic_error("Can not set rotations, as rotations or instrument "
                           "angles have already been set.");

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
    const std::vector<double> &instrumentAngles) {

  if (!m_positions.empty() || !m_rotations.empty())
    throw std::logic_error("Can not set instrument angles, as positions and/or "
                           "rotations have already been set.");

  verifyTimeIndexSize(instrumentAngles.size(), "instrument angles");
  m_instrumentAngles = instrumentAngles;
}

/**
 * Set the indexing type, either to time or detector oriented indexing.
 *
 * @param indexingType An index type enum
 */
void ScanningWorkspaceBuilder::setIndexingType(
    const IndexingType indexingType) {
  if (m_indexingType != IndexingType::DEFAULT)
    throw std::logic_error("Indexing type has been set already.");

  m_indexingType = indexingType;
}

/**
 * Verify everything has been set that is required and return the workspace.
 *
 * @return Workspace2D with the scanning information set
 */
MatrixWorkspace_sptr ScanningWorkspaceBuilder::buildWorkspace() {
  validateInputs();

  auto outputWorkspace = create<Workspace2D>(
      m_instrument, m_nDetectors * m_nTimeIndexes, m_histogram);

  auto &outputDetectorInfo = outputWorkspace->mutableDetectorInfo();
  outputDetectorInfo.setScanInterval(0, m_timeRanges[0]);

  for (size_t i = 1; i < m_nTimeIndexes; ++i) {
    const auto mergeWorkspace =
        create<Workspace2D>(m_instrument, m_nDetectors, m_histogram.binEdges());
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
    outputWorkspace->setIndexInfo(
        Indexing::IndexInfo(m_nDetectors * m_nTimeIndexes));
    break;
  case IndexingType::TIME_ORIENTED:
    createTimeOrientedIndexInfo(*outputWorkspace);
    break;
  case IndexingType::DETECTOR_ORIENTED:
    createDetectorOrientedIndexInfo(*outputWorkspace);
    break;
  }

  return boost::shared_ptr<MatrixWorkspace>(std::move(outputWorkspace));
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
    MatrixWorkspace &ws) {
  auto indexInfo = ws.indexInfo();
  auto spectrumDefinitions = Kernel::make_cow<std::vector<SpectrumDefinition>>(
      m_nDetectors * m_nTimeIndexes);

  for (size_t detIndex = 0; detIndex < m_nDetectors; ++detIndex) {
    for (size_t timeIndex = 0; timeIndex < m_nTimeIndexes; ++timeIndex) {
      spectrumDefinitions.access()[detIndex * m_nTimeIndexes + timeIndex].add(
          detIndex, timeIndex);
    }
  }

  indexInfo.setSpectrumDefinitions(spectrumDefinitions);
  ws.setIndexInfo(indexInfo);
}

void ScanningWorkspaceBuilder::createDetectorOrientedIndexInfo(
    MatrixWorkspace &ws) {
  auto indexInfo = ws.indexInfo();
  auto spectrumDefinitions = Kernel::make_cow<std::vector<SpectrumDefinition>>(
      m_nDetectors * m_nTimeIndexes);

  for (size_t timeIndex = 0; timeIndex < m_nTimeIndexes; ++timeIndex) {
    for (size_t detIndex = 0; detIndex < m_nDetectors; ++detIndex) {
      spectrumDefinitions.access()[timeIndex * m_nDetectors + detIndex].add(
          detIndex, timeIndex);
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
  if (!m_instrument)
    throw std::logic_error("Can not build workspace - instrument has not been "
                           "set. Please call setInstrument() before building.");

  if (m_timeRanges.empty())
    throw std::logic_error("Can not build workspace - time ranges have not "
                           "been set. Please call setTimeRanges() before "
                           "building.");
}

} // namespace DataObjects
} // namespace Mantid
