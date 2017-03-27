#include "MantidIndexing/SpectrumNumberTranslator.h"

namespace Mantid {
namespace Indexing {

SpectrumNumberTranslator::SpectrumNumberTranslator(
    const std::vector<SpectrumNumber> &spectrumNumbers,
    const Partitioner &partitioner, const PartitionIndex &partition)
    : m_partition(partition) {
  partitioner.checkValid(m_partition);

  size_t currentIndex = 0;
  for (size_t i = 0; i < spectrumNumbers.size(); ++i) {
    auto partition = partitioner.indexOf(GlobalSpectrumIndex(i));
    auto number = spectrumNumbers[i];
    m_partitions.emplace(number, partition);
    if (partition == m_partition) {
      m_indices.emplace(number, currentIndex);
      m_globalToLocal.emplace(GlobalSpectrumIndex(i), currentIndex);
      ++currentIndex;
    }
  }
  if (spectrumNumbers.size() != m_partitions.size())
    throw std::logic_error("SpectrumNumberTranslator: The vector of spectrum "
                           "numbers contained duplicate entries.");
}

// Full set
SpectrumIndexSet SpectrumNumberTranslator::makeIndexSet() const {
  return SpectrumIndexSet(m_indices.size());
}

SpectrumIndexSet
SpectrumNumberTranslator::makeIndexSet(SpectrumNumber min,
                                       SpectrumNumber max) const {
  // Range check
  static_cast<void>(m_partitions.at(min));
  static_cast<void>(m_partitions.at(max));

  // The order of spectrum numbers can be arbitrary so we need to iterate.
  std::vector<size_t> indices;
  for (const auto &index : m_indices) {
    if (index.first >= min && index.first <= max)
      indices.push_back(index.second);
  }
  return SpectrumIndexSet(indices, m_indices.size());
}

SpectrumIndexSet
SpectrumNumberTranslator::makeIndexSet(GlobalSpectrumIndex min,
                                       GlobalSpectrumIndex max) const {
  if (min > max)
    throw std::logic_error(
        "SpectrumIndexTranslator: specified min is larger than max.");
  if (max >= m_partitions.size())
    throw std::out_of_range(
        "SpectrumIndexTranslator: specified max value is out of range.");

  const auto begin = m_globalToLocal.lower_bound(min);
  const auto end = m_globalToLocal.upper_bound(max);
  if (begin == m_globalToLocal.end() || end == m_globalToLocal.begin() ||
      begin == end)
    return SpectrumIndexSet(0);
  return SpectrumIndexSet(begin->second, std::prev(end)->second,
                          m_globalToLocal.size());
}

SpectrumIndexSet SpectrumNumberTranslator::makeIndexSet(
    const std::vector<SpectrumNumber> &spectrumNumbers) const {
  std::vector<size_t> indices;
  for (const auto &spectrumNumber : spectrumNumbers)
    if (m_partitions.at(spectrumNumber) == m_partition)
      indices.push_back(m_indices.at(spectrumNumber));
  return SpectrumIndexSet(indices, m_indices.size());
}

SpectrumIndexSet SpectrumNumberTranslator::makeIndexSet(
    const std::vector<GlobalSpectrumIndex> &globalIndices) const {
  std::vector<size_t> indices;
  for (const auto &globalIndex : globalIndices) {
    if (globalIndex >= m_partitions.size())
      throw std::out_of_range(
          "SpectrumIndexTranslator: specified index is out of range.");
    const auto it = m_globalToLocal.find(globalIndex);
    if (it != m_globalToLocal.end())
      indices.push_back(std::distance(m_globalToLocal.begin(), it));
  }
  return SpectrumIndexSet(indices, m_globalToLocal.size());
}

} // namespace Indexing
} // namespace Mantid
