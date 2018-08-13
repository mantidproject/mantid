#ifndef MANTID_GEOMETRY_DETECTORINFOITERATOR_H_
#define MANTID_GEOMETRY_DETECTORINFOITERATOR_H_

#include "MantidGeometry/Instrument/DetectorInfoItem.h"

#include <boost/iterator/iterator_facade.hpp>

using Mantid::Geometry::DetectorInfoItem;

namespace Mantid {
namespace Geometry {

/** DetectorInfoIterator

DetectorInfoIterator allows users of the DetectorInfo object access to data
via an iterator. The iterator works as a slice view in that the index is
incremented and all items accessible at that index are made available via the
iterator.

@author Bhuvan Bezawada, STFC
@date 2018

Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

This file is part of Mantid.

Mantid is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

Mantid is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

File change history is stored at: <https://github.com/mantidproject/mantid>
Code Documentation is available at: <http://doxygen.mantidproject.org>
*/

class MANTID_GEOMETRY_DLL DetectorInfoIterator
    : public boost::iterator_facade<DetectorInfoIterator,
                                    const DetectorInfoItem &,
                                    boost::random_access_traversal_tag> {

public:
  DetectorInfoIterator(const DetectorInfo &detectorInfo, const size_t index)
      : m_item(detectorInfo, index) {}

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
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_DETECTORINFOITERATOR_H_ */
