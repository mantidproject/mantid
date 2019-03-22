// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidIndexing/SpectrumNumberTranslator.h"

namespace Mantid {
namespace Indexing {

namespace {
// Helpers for accessing vectors of tuples used as a map. Lookup is by first
// element in a tuple. Templated to support const and non-const.
template <class T, class Key>
auto lower_bound(T &map, const Key key) -> decltype(map.begin()) {
  return std::lower_bound(map.begin(), map.end(),
                          std::make_pair(key, size_t{0}),
                          [](const typename T::value_type &a,
                             const typename T::value_type &b) -> bool {
                            return std::get<0>(a) < std::get<0>(b);
                          });
}

template <class T, class Key>
auto upper_bound(T &map, const Key key) -> decltype(map.begin()) {
  return std::upper_bound(map.begin(), map.end(),
                          std::make_pair(key, size_t{0}),
                          [](const typename T::value_type &a,
                             const typename T::value_type &b) -> bool {
                            return std::get<0>(a) < std::get<0>(b);
                          });
}

template <class T, class Key>
auto find(T &map, const Key key) -> decltype(map.begin()) {
  auto it = lower_bound(map, key);
  if ((it != map.end()) && (std::get<0>(*it) == key))
    return it;
  return map.end();
}
} // namespace

SpectrumNumberTranslator::SpectrumNumberTranslator(
    const std::vector<SpectrumNumber> &spectrumNumbers,
    const Partitioner &partitioner, const PartitionIndex &partition)
    : m_partition(partition), m_globalSpectrumNumbers(spectrumNumbers) {
  partitioner.checkValid(m_partition);

  size_t currentIndex = 0;
  for (size_t i = 0; i < m_globalSpectrumNumbers.size(); ++i) {
    auto partition = partitioner.indexOf(GlobalSpectrumIndex(i));
    auto number = m_globalSpectrumNumbers[i];
    m_spectrumNumberToPartition.emplace(number, partition);
    if (partition == m_partition) {
      m_spectrumNumberToIndex.emplace_back(number, currentIndex);
      m_globalToLocal.emplace_back(GlobalSpectrumIndex(i), currentIndex);
      if (partitioner.numberOfPartitions() > 1)
        m_spectrumNumbers.emplace_back(number);
      ++currentIndex;
    }
  }
  // Careful: Do NOT use maps of spectrum numbers for size check, since those
  // can (currently) be smaller than the actual size due to duplicate spectrum
  // numbers.
  m_isPartitioned = (m_globalToLocal.size() != m_globalSpectrumNumbers.size());

  // At this point, m_globalToLocal is sorted by construction so it can be used
  // as a "map" (using methods from the anonymous namespace above).
  // m_spectrumNumberToIndex is not sorted. It will be sorted in makeIndexSet()
  // if required. The reason for postponing this is a potentially expensive
  // setup that is not required unless specific vecsions of makeIndexSet() are
  // called.
}

SpectrumNumberTranslator::SpectrumNumberTranslator(
    const std::vector<SpectrumNumber> &spectrumNumbers,
    const SpectrumNumberTranslator &parent)
    : m_isPartitioned(parent.m_isPartitioned), m_partition(parent.m_partition),
      m_globalSpectrumNumbers(spectrumNumbers) {
  size_t currentIndex = 0;
  for (size_t i = 0; i < m_globalSpectrumNumbers.size(); ++i) {
    auto partition = parent.m_spectrumNumberToPartition.at(spectrumNumbers[i]);
    auto number = m_globalSpectrumNumbers[i];
    m_spectrumNumberToPartition.emplace(number, partition);
    if (partition == m_partition) {
      m_spectrumNumberToIndex.emplace_back(number, currentIndex);
      m_globalToLocal.emplace_back(GlobalSpectrumIndex(i), currentIndex);
      if (m_isPartitioned)
        m_spectrumNumbers.emplace_back(number);
      ++currentIndex;
    }
  }
}

const std::vector<SpectrumNumber> &
SpectrumNumberTranslator::globalSpectrumNumbers() const {
  return m_globalSpectrumNumbers;
}

SpectrumNumberTranslator::SpectrumNumberTranslator(
    const std::vector<GlobalSpectrumIndex> &globalIndices,
    const SpectrumNumberTranslator &parent)
    : SpectrumNumberTranslator(parent.spectrumNumbers(globalIndices), parent) {}

SpectrumIndexSet
SpectrumNumberTranslator::makeIndexSet(SpectrumNumber min,
                                       SpectrumNumber max) const {
  checkUniqueSpectrumNumbers();
  // Range check
  static_cast<void>(m_spectrumNumberToPartition.at(min));
  static_cast<void>(m_spectrumNumberToPartition.at(max));

  std::call_once(m_mapSetup,
                 &SpectrumNumberTranslator::setupSpectrumNumberToIndexMap,
                 this);
  std::vector<size_t> indices;
  const auto begin = lower_bound(m_spectrumNumberToIndex, min);
  const auto end = upper_bound(m_spectrumNumberToIndex, max);
  for (auto it = begin; it != end; ++it)
    indices.push_back(it->second);
  return SpectrumIndexSet(indices, m_spectrumNumberToIndex.size());
}

SpectrumIndexSet
SpectrumNumberTranslator::makeIndexSet(GlobalSpectrumIndex min,
                                       GlobalSpectrumIndex max) const {
  if (min > max)
    throw std::logic_error(
        "SpectrumIndexTranslator: specified min is larger than max.");
  if (max >= m_spectrumNumberToPartition.size())
    throw std::out_of_range(
        "SpectrumIndexTranslator: specified max value is out of range.");

  const auto begin = lower_bound(m_globalToLocal, min);
  const auto end = upper_bound(m_globalToLocal, max);
  if (begin == m_globalToLocal.end() || end == m_globalToLocal.begin() ||
      begin == end)
    return SpectrumIndexSet(0);
  return SpectrumIndexSet(begin->second, std::prev(end)->second,
                          m_globalToLocal.size());
}

SpectrumIndexSet SpectrumNumberTranslator::makeIndexSet(
    const std::vector<SpectrumNumber> &spectrumNumbers) const {
  checkUniqueSpectrumNumbers();
  std::call_once(m_mapSetup,
                 &SpectrumNumberTranslator::setupSpectrumNumberToIndexMap,
                 this);
  std::vector<size_t> indices;
  for (const auto &spectrumNumber : spectrumNumbers)
    if (m_spectrumNumberToPartition.at(spectrumNumber) == m_partition)
      indices.push_back(find(m_spectrumNumberToIndex, spectrumNumber)->second);
  return SpectrumIndexSet(indices, m_spectrumNumberToIndex.size());
}

SpectrumIndexSet SpectrumNumberTranslator::makeIndexSet(
    const std::vector<GlobalSpectrumIndex> &globalIndices) const {
  std::vector<size_t> indices;
  for (const auto &globalIndex : globalIndices) {
    if (globalIndex >= m_spectrumNumberToPartition.size())
      throw std::out_of_range(
          "SpectrumIndexTranslator: specified index is out of range.");
    const auto it = find(m_globalToLocal, globalIndex);
    if (it != m_globalToLocal.end())
      indices.push_back(it->second);
  }
  return SpectrumIndexSet(indices, m_globalToLocal.size());
}

PartitionIndex SpectrumNumberTranslator::partitionOf(
    const GlobalSpectrumIndex globalIndex) const {
  checkUniqueSpectrumNumbers();
  const auto spectrumNumber =
      m_globalSpectrumNumbers[static_cast<size_t>(globalIndex)];
  return m_spectrumNumberToPartition.at(spectrumNumber);
}

void SpectrumNumberTranslator::checkUniqueSpectrumNumbers() const {
  // To support legacy code that creates workspaces with duplicate spectrum
  // numbers we check for bad spectrum numbers only when needed, i.e., when
  // accessing index maps.
  if (m_globalSpectrumNumbers.size() != m_spectrumNumberToPartition.size())
    throw std::logic_error("SpectrumNumberTranslator: The vector of spectrum "
                           "numbers contained duplicate entries.");
}

void SpectrumNumberTranslator::setupSpectrumNumberToIndexMap() const {
  std::sort(m_spectrumNumberToIndex.begin(), m_spectrumNumberToIndex.end(),
            [](const std::pair<SpectrumNumber, size_t> &a,
               const std::pair<SpectrumNumber, size_t> &b) -> bool {
              return std::get<0>(a) < std::get<0>(b);
            });
}

std::vector<SpectrumNumber> SpectrumNumberTranslator::spectrumNumbers(
    const std::vector<GlobalSpectrumIndex> &globalIndices) const {
  std::vector<SpectrumNumber> spectrumNumbers;
  spectrumNumbers.reserve(globalIndices.size());
  std::transform(globalIndices.cbegin(), globalIndices.cend(),
                 std::back_inserter(spectrumNumbers), [this](const auto index) {
                   return m_globalSpectrumNumbers[static_cast<size_t>(index)];
                 });
  return spectrumNumbers;
}

} // namespace Indexing
} // namespace Mantid
