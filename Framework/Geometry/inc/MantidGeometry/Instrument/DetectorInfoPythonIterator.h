#ifndef MANTID_GEOMETRY_DETECTORINFOPYTHONITERATOR_H_
#define MANTID_GEOMETRY_DETECTORINFOPYTHONITERATOR_H_

#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/DetectorInfoItem.h"
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"

#include <boost/python/iterator.hpp>

using Mantid::Geometry::DetectorInfo;
using Mantid::Geometry::DetectorInfoItem;
using Mantid::Geometry::DetectorInfoIterator;
using namespace boost::python;

namespace Mantid {
namespace Geometry {

/** DetectorInfoPythonIterator

DetectorInfoPythonIterator is used to expose DetectorInfoIterator to the Python
side. From Python the user will be able to use more pythonic loop syntax to
access data such as:
- isMonitor()
- isMaksed()
- twoTheta()
- position()
- rotation()
without the need for indexes.
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

class DetectorInfoPythonIterator {
public:
  explicit DetectorInfoPythonIterator(const DetectorInfo &detectorInfo)
      : m_begin(detectorInfo.begin()), m_end(detectorInfo.end()),
        m_firstOrDone(true) {}

  const DetectorInfoItem &next() {
    if (!m_firstOrDone) {
      ++m_begin;
    } else {
      m_firstOrDone = false;
      objects::stop_iteration_error();
    }

    return *m_begin;
  }

private:
  DetectorInfoIterator m_begin;
  DetectorInfoIterator m_end;
  bool m_firstOrDone;
};

} // namespace Geometry
} // namespace Mantid

#endif /* MANTID_GEOMETRY_DETECTORINFOPYTHONITERATOR_H_ */
