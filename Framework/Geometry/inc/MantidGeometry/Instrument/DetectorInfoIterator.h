
#ifndef MANTID_GEOMETRY_DETECTORINFOITERATOR_H_
#define MANTID_GEOMETRY_DETECTORINFOITERATOR_H_

//#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/DetectorInfoItem.h"

#include <boost/iterator/iterator_facade.hpp>
#include <memory>

namespace Mantid {
namespace Geometry {

class DetectorInfo;

class MANTID_GEOMETRY_DLL DetectorInfoIterator
    : public boost::iterator_facade<DetectorInfoIterator,
                                    const DetectorInfoItem &,
                                    boost::bidirectional_traversal_tag> {

public:
  DetectorInfoIterator(const DetectorInfo &detectorInfo, const size_t index)
    : m_item(detectorInfo, index), m_detectorInfo(detectorInfo), m_index(index) {}

  DetectorInfoIterator(const DetectorInfoIterator&detInfoIterator) : 
    m_index(detInfoIterator.m_index), m_detectorInfo(detInfoIterator.m_detectorInfo), m_item(detInfoIterator.m_detectorInfo, detInfoIterator.m_index) {}

private:
  friend class boost::iterator_core_access;

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

  DetectorInfoItem m_item;
  DetectorInfo m_detectorInfo;
  size_t m_index;
};

} // namespace Geometry
} // namespace Mantid

#endif
