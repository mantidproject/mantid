#include "MantidDataHandling/ScanningWorkspaceHelper.h"

#include "MantidAPI/DetectorInfo.h"
#include "MantidAPI/WorkspaceFactory.h"

using namespace Mantid::API;

namespace Mantid {
namespace DataHandling {

ScanningWorkspaceHelper::ScanningWorkspaceHelper(size_t nDetectors, size_t nTimeIndexes, size_t nBins)
    : m_nDetectors(nDetectors), m_nTimeIndexes(nTimeIndexes), m_nBins(nBins) {}

void ScanningWorkspaceHelper::setTimeRanges(std::vector<std::pair<Kernel::DateAndTime, Kernel::DateAndTime>> timeRanges) {
  verifyTimeIndexSize(timeRanges.size(), "start time, end time");
  m_timeRanges = timeRanges;
}

void ScanningWorkspaceHelper::setTimeRanges(Kernel::DateAndTime startTime, std::vector<double> durations) {
  verifyTimeIndexSize(durations.size(), "durations");

  std::vector<std::pair<Kernel::DateAndTime, Kernel::DateAndTime>> timeRanges = {std::pair<Kernel::DateAndTime, Kernel::DateAndTime>(startTime, startTime + durations[0])};

  for (size_t i = 1; i < m_nTimeIndexes; ++i) {
    const auto newStartTime = timeRanges[i - 1].second;
    const auto endTime = newStartTime + durations[i];
    timeRanges.push_back(std::pair<Kernel::DateAndTime, Kernel::DateAndTime>(newStartTime, endTime));
  }

  setTimeRanges(timeRanges);
}

void ScanningWorkspaceHelper::verifyTimeIndexSize(size_t inputSize, std::string description) {
  if (inputSize != m_nTimeIndexes) {
    throw std::invalid_argument("Number of " + description + " pairs supplied does not match the number of time indexes being requested.");
  }
}

MatrixWorkspace_sptr ScanningWorkspaceHelper::buildWorkspace() {
  auto outputWorkspace = WorkspaceFactory::Instance().create("Workspace2D", m_nDetectors * m_nTimeIndexes, m_nBins + 1, m_nBins);
  auto &outputDetectorInfo = outputWorkspace->mutableDetectorInfo();

  for (size_t i = 0; i < m_nTimeIndexes; ++i) {
    auto mergeWorkspace = WorkspaceFactory::Instance().create(outputWorkspace, m_nDetectors);
    auto &mergeDetectorInfo = mergeWorkspace->mutableDetectorInfo();
    for (size_t j = 0; j < m_nDetectors; ++j) {
      mergeDetectorInfo.setScanInterval(j, m_timeRanges[i]);
    }
    outputDetectorInfo.merge(mergeDetectorInfo);
  }

  return outputWorkspace;
}

} // namespace DataHandling
} // namespace Mantid
