// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/geometry/DetectorInfoPythonIterator.h"
#include "MantidPythonInterface/core/VersionCompat.h"
#include <boost/python/class.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/module.hpp>

using Mantid::PythonInterface::DetectorInfoPythonIterator;
using namespace boost::python;

// Export DetectorInfoPythonIterator
void export_DetectorInfoPythonIterator() {

  // Export to Python
  class_<DetectorInfoPythonIterator>("DetectorInfoPythonIterator", no_init)
      .def("__iter__", objects::identity_function())
#ifdef IS_PY2K
      .def("next", &DetectorInfoPythonIterator::next)
#else
      .def("__next__", &DetectorInfoPythonIterator::next)
#endif
      ;
}
