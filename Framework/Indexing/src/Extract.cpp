#include "MantidIndexing/Extract.h"
#include "MantidIndexing/IndexInfo.h"
#include "MantidTypes/SpectrumDefinition.h"

namespace Mantid {
namespace Indexing {

/// Extracts IndexInfo from source IndexInfo, extracting data for all indices
/// specified by vector.
IndexInfo extract(const IndexInfo &source, const std::vector<size_t> &indices) {
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
