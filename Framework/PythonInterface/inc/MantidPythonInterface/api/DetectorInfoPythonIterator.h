// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_DETECTORINFOPYTHONITERATOR_H_
#define MANTID_PYTHONINTERFACE_DETECTORINFOPYTHONITERATOR_H_

#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidGeometry/Instrument/DetectorInfoItem.h"
#include "MantidGeometry/Instrument/DetectorInfoIterator.h"

#include <boost/python/iterator.hpp>

using Mantid::Geometry::DetectorInfo;
using Mantid::Geometry::DetectorInfoItem;
using Mantid::Geometry::DetectorInfoIterator;
using namespace boost::python;

namespace Mantid {
namespace PythonInterface {

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
*/

class DetectorInfoPythonIterator {
public:
  explicit DetectorInfoPythonIterator(DetectorInfo &detectorInfo)
      : m_begin(detectorInfo.begin()), m_end(detectorInfo.end()),
        m_firstOrDone(true) {}

  DetectorInfoItem<DetectorInfo> next() {
    if (!m_firstOrDone)
      ++m_begin;
    else
      m_firstOrDone = false;
    if (m_begin == m_end) {
      m_firstOrDone = true;
      objects::stop_iteration_error();
    }
    return *m_begin;
  }

private:
  DetectorInfoIterator<DetectorInfo> m_begin;
  DetectorInfoIterator<DetectorInfo> m_end;
  bool m_firstOrDone;
};

} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_DETECTORINFOPYTHONITERATOR_H_ */
