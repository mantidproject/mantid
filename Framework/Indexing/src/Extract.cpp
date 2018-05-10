#include "MantidIndexing/Extract.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidIndexing/SpectrumIndexSet.h"
#include "MantidTypes/SpectrumDefinition.h"

namespace Mantid {
namespace Indexing {

namespace {
void checkStorageMode(const IndexInfo &indexInfo) {
  using namespace Parallel;
  if (indexInfo.storageMode() == StorageMode::Distributed)
    throw std::runtime_error("extract() does not support " +
                             Parallel::toString(StorageMode::Distributed));
}
} // namespace

/// Extracts IndexInfo from source IndexInfo, extracting data for all indices
/// specified by index set.
IndexInfo extract(const IndexInfo &source, const SpectrumIndexSet &indices) {
  return extract(source, std::vector<size_t>(indices.begin(), indices.end()));
}

/// Extracts IndexInfo from source IndexInfo, extracting data for all indices
/// specified by vector.
IndexInfo extract(const IndexInfo &source, const std::vector<size_t> &indices) {
  checkStorageMode(source);
  std::vector<SpectrumNumber> specNums;
  std::vector<SpectrumDefinition> specDefs;
  const auto &sourceDefs = source.spectrumDefinitions();
  for (const auto &i : indices) {
    specNums.emplace_back(source.spectrumNumber(i));
    specDefs.emplace_back((*sourceDefs)[i]);
  }
  IndexInfo result(std::move(specNums));
  result.setSpectrumDefinitions(std::move(specDefs));
  return result;
}

/// Extracts IndexInfo from source IndexInfo, extracting data for all indices
/// specified by range.
IndexInfo extract(const IndexInfo &source, const size_t minIndex,
                  const size_t maxIndex) {
  checkStorageMode(source);
  std::vector<SpectrumNumber> specNums;
  std::vector<SpectrumDefinition> specDefs;
  const auto &sourceDefs = source.spectrumDefinitions();
  for (size_t i = minIndex; i <= maxIndex; ++i) {
    specNums.emplace_back(source.spectrumNumber(i));
    specDefs.emplace_back((*sourceDefs)[i]);
  }
  IndexInfo result(std::move(specNums));
  result.setSpectrumDefinitions(std::move(specDefs));
  return result;
}

} // namespace Indexing
} // namespace Mantid
