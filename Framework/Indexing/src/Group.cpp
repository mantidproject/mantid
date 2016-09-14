#include "MantidIndexing/Group.h"
#include "MantidIndexing/IndexInfo.h"

namespace Mantid {
namespace Indexing {

IndexInfo group(const IndexInfo &source, std::vector<specnum_t> &&specNums,
                const std::vector<std::vector<size_t>> &grouping) {
  if (specNums.size() != grouping.size())
    throw std::runtime_error("Indexing::group: Size mismatch between spectrum "
                             "number and grouping vectors");
  std::vector<std::vector<detid_t>> detIDs;
  for (const auto &group : grouping) {
    detIDs.emplace_back(std::vector<detid_t>());
    for (const auto &i : group) {
      const auto &IDs = source.detectorIDs(i);
      auto &newIDs = detIDs.back();
      newIDs.insert(newIDs.end(), IDs.begin(), IDs.end());
    }
  }
  return {std::move(specNums), std::move(detIDs)};
}

} // namespace Indexing
} // namespace Mantid
