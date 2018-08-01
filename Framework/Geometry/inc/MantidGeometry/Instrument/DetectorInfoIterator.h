
#ifndef MANTID_GEOMETRY_DETECTORINFOITERATOR_H_
#define MANTID_GEOMETRY_DETECTORINFOITERATOR_H_

#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/DetectorInfoItem.h"

#include <boost/iterator/iterator_facade.hpp>
#include <memory>

using DetectorInfo;

namespace Mantid {
namespace Geometry {
namespace Iterators {

class DetectorInfoIterator
    : public boost::iterator_facade<DetectorInfoIterator,
                                    const DetectorInfoItem &,
                                    boost::bidirectional_traversal_tag> {

private:
  friend class boost::iterator_core_access;

  DetectorInfoItem m_item;

  void increment() { m_item.incrementIndex(); }

  bool equal(const DetectorInfoIterator &other) const {
    return m_item.getIndex() == other.m_item.getIndex();
  }

  const DetectorInfoItem &dereference() const { return m_item; }

  void decrement() { m_item.decrementIndex(); }

  void advance(int64_t delta) { m_item.advance(delta); }

  uint64_t distance_to(const DetectorInfoIterator &other) const {
    return static_cast<uint64_t>(other.m_item.getIndex()) -
           static_cast<uint64_t>(m_item.getIndex());
  }

public:
  DetectorInfoIterator(const DetectorInfo &detectorInfo, const size_t index)
      : m_item(detectorInfo, index){};
};

} // namespace Iterators
} // namespace Geometry
} // namespace Mantid
#endif
