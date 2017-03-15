#include "MantidIndexing/SpectrumNumberTranslator.h"

namespace Mantid {
namespace Indexing {

SpectrumNumberTranslator::SpectrumNumberTranslator(
    const std::vector<SpectrumNumber> &spectrumNumbers,
    std::unique_ptr<Partitioner> partitioner, const PartitionIndex &partition)
    : m_partitioner(std::move(partitioner)), m_partition(partition),
      m_globalSpectrumNumbers(spectrumNumbers) {
  m_partitioner->checkValid(m_partition);

  size_t currentIndex = 0;
  for (size_t i = 0; i < m_globalSpectrumNumbers.size(); ++i) {
    auto partition = m_partitioner->indexOf(GlobalSpectrumIndex(i));
    auto number = m_globalSpectrumNumbers[i];
    m_partitions.emplace(number, partition);
    if (partition == m_partition) {
      m_indices.emplace(number, currentIndex);
      m_globalToLocal.emplace(GlobalSpectrumIndex(i), currentIndex);
      if (m_partitioner->numberOfPartitions() > 1)
        m_spectrumNumbers.emplace_back(number);
      ++currentIndex;
    }
  }
}

/// Returns the global number of spectra.
size_t SpectrumNumberTranslator::globalSize() const {
  return m_globalSpectrumNumbers.size();
}

/// Returns the local number of spectra.
size_t SpectrumNumberTranslator::localSize() const {
  return m_indices.size();
}

/// Returns the spectrum number for given index.
SpectrumNumber
SpectrumNumberTranslator::spectrumNumber(const size_t index) const {
  if (m_partitioner->numberOfPartitions() == 1)
    return m_globalSpectrumNumbers[index];
  return m_spectrumNumbers[index];
}

// Full set
SpectrumIndexSet SpectrumNumberTranslator::makeIndexSet() const {
  checkUniqueSpectrumNumbers();
  return SpectrumIndexSet(m_indices.size());
}

SpectrumIndexSet
SpectrumNumberTranslator::makeIndexSet(SpectrumNumber min,
                                       SpectrumNumber max) const {
  checkUniqueSpectrumNumbers();
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
  checkUniqueSpectrumNumbers();
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
  checkUniqueSpectrumNumbers();
  std::vector<size_t> indices;
  for (const auto &spectrumNumber : spectrumNumbers)
    if (m_partitions.at(spectrumNumber) == m_partition)
      indices.push_back(m_indices.at(spectrumNumber));
  return SpectrumIndexSet(indices, m_indices.size());
}

SpectrumIndexSet SpectrumNumberTranslator::makeIndexSet(
    const std::vector<GlobalSpectrumIndex> &globalIndices) const {
  checkUniqueSpectrumNumbers();
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

void SpectrumNumberTranslator::checkUniqueSpectrumNumbers() const {
  // To support legacy code that creates workspaces with duplicate spectrum
  // numbers we check for bad spectrum numbers only when needed, i.e., when
  // accessing index maps.
  if (m_globalSpectrumNumbers.size() != m_partitions.size())
    throw std::logic_error("SpectrumNumberTranslator: The vector of spectrum "
                           "numbers contained duplicate entries.");
}

} // namespace Indexing
} // namespace Mantid
