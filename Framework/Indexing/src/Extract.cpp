#include "MantidIndexing/DetectorID.h"
#include "MantidIndexing/Extract.h"
#include "MantidIndexing/IndexInfo.h"

namespace Mantid {
namespace Indexing {

/// Extracts IndexInfo from source IndexInfo, extracting data for all indices
/// specified by vector.
IndexInfo extract(const IndexInfo &source, const std::vector<size_t> &indices) {
  std::vector<SpectrumNumber> specNums;
  std::vector<std::vector<DetectorID>> detIDs;
  for (const auto &i : indices) {
    specNums.emplace_back(source.spectrumNumber(i));
    detIDs.emplace_back(source.detectorIDs(i));
  }
  IndexInfo result(std::move(specNums));
  result.setDetectorIDs(std::move(detIDs));
  return result;
}

/// Extracts IndexInfo from source IndexInfo, extracting data for all indices
/// specified by range.
IndexInfo extract(const IndexInfo &source, const size_t minIndex,
                  const size_t maxIndex) {
  std::vector<SpectrumNumber> specNums;
  std::vector<std::vector<DetectorID>> detIDs;
  for (size_t i = minIndex; i <= maxIndex; ++i) {
    specNums.emplace_back(source.spectrumNumber(i));
    detIDs.emplace_back(source.detectorIDs(i));
  }
  IndexInfo result(std::move(specNums));
  result.setDetectorIDs(std::move(detIDs));
  return result;
}

} // namespace Indexing
} // namespace Mantid
