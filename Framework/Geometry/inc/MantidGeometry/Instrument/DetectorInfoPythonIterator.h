#ifndef MANTID_GEOMETRY_DETECTORINFOPYTHONITERATOR_H_
#define MANTID_GEOMETRY_DETECTORINFOPYTHONITERATOR_H_

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/def.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/module.hpp>
#include <boost/python/reference_existing_object.hpp>

#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/DetectorInfoItem.h"
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"

using Mantid::Geometry::DetectorInfo;
using Mantid::Geometry::DetectorInfoItem;
using Mantid::Geometry::DetectorInfoIterator;

using namespace boost::python;

namespace Mantid {
namespace Geometry {

class DetectorInfoPythonIterator {
public:
  explicit DetectorInfoPythonIterator(const DetectorInfo &source)
      : m_begin(source.begin()), m_end(source.end()), m_current(*m_begin) {}

  const DetectorInfoItem &next() {
    if (m_begin == m_end) {
      objects::stop_iteration_error();
    }
    m_current = *m_begin++;
    return m_current;
  }

private:
  DetectorInfoIterator m_begin;
  DetectorInfoIterator m_end;
  DetectorInfoItem m_current;
};

} // namespace Geometry
} // namespace Mantid
#endif /* MANTID_GEOMETRY_DETECTORINFOPYTHONITERATOR_H_ */
