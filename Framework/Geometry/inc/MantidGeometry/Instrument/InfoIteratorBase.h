// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_INFOITERATORBASE_H_
#define MANTID_GEOMETRY_INFOITERATORBASE_H_

#include <algorithm>
#include <boost/iterator/iterator_facade.hpp>

namespace Mantid {
namespace Geometry {

/** InfoIterator

Base to allow users of the Info objects (DetectorInfo etc) access to data
via a random access iterator.
*/
template <typename T, template <typename> class InfoItem>
class InfoIteratorBase
    : public boost::iterator_facade<InfoIteratorBase<T, InfoItem>,
                                    InfoItem<T> &,
                                    boost::random_access_traversal_tag> {

public:
  InfoIteratorBase(T &info, const size_t index) : m_item(info, index) {}

private:
  // Allow boost iterator access
  friend class boost::iterator_core_access;

  void advance(int64_t delta) {
    m_item.m_index =
        delta < 0 ? std::max(static_cast<uint64_t>(0),
                             static_cast<uint64_t>(m_item.m_index) + delta)
                  : std::min(m_item.infoSize(),
                             m_item.m_index + static_cast<size_t>(delta));
  }

  void increment() {
    if (m_item.m_index < m_item.infoSize()) {
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

  bool equal(const InfoIteratorBase<T, InfoItem> &other) const {
    return getIndex() == other.getIndex();
  }

  InfoItem<T> &dereference() const { return m_item; }

  uint64_t distance_to(const InfoIteratorBase<T, InfoItem> &other) const {
    return static_cast<uint64_t>(other.getIndex()) -
           static_cast<uint64_t>(getIndex());
  }

  mutable InfoItem<T> m_item;
};
} // namespace Geometry
} // namespace Mantid
#endif /* MANTID_GEOMETRY_INFOITERATORBASE_H_ */
