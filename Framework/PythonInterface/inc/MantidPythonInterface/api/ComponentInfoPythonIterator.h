// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_PYTHONINTERFACE_COMPONENTINFOPYTHONITERATOR_H_
#define MANTID_PYTHONINTERFACE_COMPONENTINFOPYTHONITERATOR_H_

#include "MantidGeometry/Instrument/ComponentInfo.h"
#include "MantidGeometry/Instrument/ComponentInfoItem.h"
#include "MantidGeometry/Instrument/ComponentInfoIterator.h"

#include <boost/python/iterator.hpp>

using Mantid::Geometry::ComponentInfo;
using Mantid::Geometry::ComponentInfoItem;
using Mantid::Geometry::ComponentInfoIterator;
using namespace boost::python;

namespace Mantid {
namespace PythonInterface {

/** ComponentInfoPythonIterator

ComponentInfoPythonIterator is used to expose ComponentInfoIterator to the
Python side.
*/

class ComponentInfoPythonIterator {
public:
  explicit ComponentInfoPythonIterator(ComponentInfo &detectorInfo)
      : m_begin(detectorInfo.begin()), m_end(detectorInfo.end()),
        m_firstOrDone(true) {}

  ComponentInfoItem<ComponentInfo> next() {
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
  ComponentInfoIterator<ComponentInfo> m_begin;
  ComponentInfoIterator<ComponentInfo> m_end;
  bool m_firstOrDone;
};

} // namespace PythonInterface
} // namespace Mantid

#endif /* MANTID_PYTHONINTERFACE_COMPONENTINFOPYTHONITERATOR_H_ */
