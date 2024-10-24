// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <algorithm>
#include <boost/iterator/iterator_facade.hpp>

namespace Mantid {
namespace Geometry {

/** InfoIterator

Base to allow users of the Info objects (DetectorInfo etc) access to data
via a random access iterator.

Note that the reference type (InfoItem<T>) causes the iterator to be treated as
std::input_iterator for the purposes of many std algorithms such as
std::advance. See https://en.cppreference.com/w/cpp/iterator/advance for example
*/
template <typename T, template <typename> class InfoItem>
class InfoIteratorBase : public boost::iterator_facade<InfoIteratorBase<T, InfoItem>, InfoItem<T>,
                                                       boost::random_access_traversal_tag, InfoItem<T>> {

public:
  /**
   * Constructor for base iterator
   * @param info : Info object (T) to provide iterator ontop of.
   * @param index : start point of iterator
   * @param totalSize : Represents maximum length of info. i.e. total number of
   * items that can be iterated over.
   */
  InfoIteratorBase(T &info, const size_t index, const size_t totalSize) : m_item(info, index), m_totalSize(totalSize) {
    if (index > totalSize)
      throw std::invalid_argument("Iterator start point cannot be greater than maximum size");
  }

private:
  // Allow boost iterator access
  friend class boost::iterator_core_access;

  void advance(int64_t delta) {
    m_item.m_index = delta < 0 ? std::max(static_cast<uint64_t>(0), static_cast<uint64_t>(m_item.m_index) + delta)
                               : std::min(m_totalSize, m_item.m_index + static_cast<size_t>(delta));
  }

  bool equal(const InfoIteratorBase<T, InfoItem> &other) const { return getIndex() == other.getIndex(); }

  void increment() {
    if (m_item.m_index < m_totalSize) {
      ++m_item.m_index;
    }
  }

  void decrement() {
    if (m_item.m_index > 0) {
      --m_item.m_index;
    }
  }

  size_t getIndex() const { return m_item.m_index; }

  void setIndex(const size_t index) { m_item.m_index = index; }

  InfoItem<T> dereference() const { return m_item; }

  uint64_t distance_to(const InfoIteratorBase<T, InfoItem> &other) const {
    return static_cast<uint64_t>(other.getIndex()) - static_cast<uint64_t>(getIndex());
  }

  InfoItem<T> m_item;
  size_t m_totalSize;
};
} // namespace Geometry
} // namespace Mantid
