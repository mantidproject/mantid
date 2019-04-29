// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_INDEXING_INDEXSET_H_
#define MANTID_INDEXING_INDEXSET_H_

#include <boost/iterator/iterator_facade.hpp>

#include <algorithm>
#include <set>
#include <stdexcept>
#include <vector>

namespace Mantid {
namespace Indexing {
namespace detail {

/** A base class for a set of indices. This uses the CRTP. We need
  DetectorIndexSet and SpectrumIndexSet with the same functionality, but the
  types should be incompatible. Thus, we use IndexSet<DetectorIndexSet> and
  IndexSet<SpectrumIndexSet>.

  @author Simon Heybrock
  @date 2017
*/
template <class T> class IndexSet {
public:
  class const_iterator
      : public boost::iterator_facade<const_iterator, size_t,
                                      boost::random_access_traversal_tag,
                                      size_t> {
  public:
    const_iterator(const IndexSet &indexSet, const size_t index)
        : m_indexSet(indexSet), m_index(index) {}
    const_iterator &operator=(const const_iterator &other) {
      if (&m_indexSet != &other.m_indexSet) {
        throw std::invalid_argument(
            "Cannot assign iterators from different IndexSet objects.");
      }
      m_index = other.m_index;
      return *this;
    }

  private:
    friend class boost::iterator_core_access;

    void increment() {
      if (m_index < m_indexSet.size())
        ++m_index;
    }

    bool equal(const const_iterator &other) const {
      return (&m_indexSet == &other.m_indexSet) && (m_index == other.m_index);
    }

    size_t dereference() const { return m_indexSet[m_index]; }

    void decrement() {
      if (m_index > 0)
        --m_index;
    }
    void advance(int64_t delta) {
      m_index = delta < 0 ? std::max(static_cast<int64_t>(0),
                                     static_cast<int64_t>(m_index) + delta)
                          : std::min(m_indexSet.size(),
                                     m_index + static_cast<size_t>(delta));
    }
    int64_t distance_to(const const_iterator &other) const {
      return static_cast<int64_t>(other.m_index) -
             static_cast<int64_t>(m_index);
    }

    const IndexSet &m_indexSet;
    size_t m_index;
  };

  const_iterator begin() const { return const_iterator(*this, 0); }
  const_iterator end() const { return const_iterator(*this, size()); }

  explicit IndexSet();
  IndexSet(size_t fullRange);
  IndexSet(int64_t min, int64_t max, size_t fullRange);
  IndexSet(const std::vector<size_t> &indices, size_t fullRange);

  /// Returns the size of the set.
  size_t size() const { return m_size; }

  /// Returns true if the set is empty.
  size_t empty() const { return m_size == 0; }

  /// Returns the element at given index (range 0...size()-1).
  size_t operator[](size_t index) const {
    // This is accessed frequently in loops and thus inlined.
    if (m_isRange)
      return m_min + index;
    return m_indices[index];
  }

  bool isContiguous() const noexcept;

protected:
  ~IndexSet() = default;

private:
  bool m_isRange = true;
  // Default here to avoid uninitialized warning for m_isRange = false.
  size_t m_min = 0;
  size_t m_size;
  std::vector<size_t> m_indices;
};

/// Default Constructor creates empty IndexSet of size 0.
template <class T> IndexSet<T>::IndexSet() : m_size(0) {}

/// Constructor for a set covering the full range from 0 to fullRange-1.
template <class T>
IndexSet<T>::IndexSet(size_t fullRange) : m_size(fullRange) {}

/// Constructor for a set covering the range from min to max. Range is verified
/// at construction time.
template <class T>
IndexSet<T>::IndexSet(int64_t min, int64_t max, size_t fullRange) {
  if (min < 0 || min > max)
    throw std::logic_error("IndexSet: specified min or max values are invalid");
  if (max >= static_cast<int64_t>(fullRange))
    throw std::out_of_range("IndexSet: specified max value is out of range");

  // Bounds checked, cast should be fine in all cases.
  m_min = static_cast<size_t>(min);
  m_size = static_cast<size_t>(max - min + 1);
}

/// Constructor for a set containing all specified indices. Range is verified at
/// construction time and duplicates cause an error.
template <class T>
IndexSet<T>::IndexSet(const std::vector<size_t> &indices, size_t fullRange)
    : m_isRange(false) {
  // Validate indices, using m_indices as buffer (reassigned later).
  m_indices = indices;
  std::sort(m_indices.begin(), m_indices.end());
  if (!m_indices.empty() && *m_indices.rbegin() >= fullRange)
    throw std::out_of_range("IndexSet: specified index is out of range");
  if (std::adjacent_find(m_indices.begin(), m_indices.end()) != m_indices.end())
    throw std::runtime_error("IndexSet: duplicate indices are not allowed");
  m_indices = indices;
  m_size = m_indices.size();
}

/**
 * Check if the index range is contiguous and in ascending order.
 */
template <class T> bool IndexSet<T>::isContiguous() const noexcept {
  if (!m_isRange || m_indices.size() > 1) {
    for (size_t i = 0; i < m_indices.size() - 1; ++i) {
      if (m_indices[i] + 1 != m_indices[i + 1]) {
        return false;
      }
    }
  }
  return true;
}

} // namespace detail
} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_INDEXSET_H_ */
