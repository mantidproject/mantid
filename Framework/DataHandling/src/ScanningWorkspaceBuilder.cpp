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
    std::vector<std::vector<Kernel::V3D>>) {
  throw std::runtime_error("Not implemented yet!");
}
void ScanningWorkspaceBuilder::setRotations(
    std::vector<std::vector<Kernel::Quat>>) {
  throw std::runtime_error("Not implemented yet!");
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

  return outputWorkspace;
}

void ScanningWorkspaceBuilder::verifyTimeIndexSize(
    size_t inputSize, const std::string &description) {
  if (inputSize != m_nTimeIndexes) {
    throw std::logic_error(
        "Number of " + description +
        " supplied does not match the number of time indexes being requested.");
  }
}

void ScanningWorkspaceBuilder::validateInputs() {
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
