#include "MantidIndexing/DetectorIDs.h"

#include <algorithm>

namespace Mantid {
namespace Indexing {

DetectorIDs::DetectorIDs(std::vector<detid_t> &&detectorIDs) {
  m_data.reserve(detectorIDs.size());
  for (auto &id : detectorIDs)
    m_data.push_back(std::vector<detid_t>{id});
}

DetectorIDs::DetectorIDs(std::vector<std::vector<detid_t>> &&detectorIDs) {
  m_data = std::move(detectorIDs);
  for (auto &ids : m_data) {
    std::sort(ids.begin(), ids.end());
    ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
  }
}

DetectorIDs::DetectorIDs(std::initializer_list<detid_t> &&ilist)
    : DetectorIDs(std::vector<detid_t>(ilist)) {}

size_t DetectorIDs::size() const { return m_data.size(); }

const std::vector<std::vector<detid_t>> &DetectorIDs::data() const {
  return m_data;
}

} // namespace Indexing
} // namespace Mantid
