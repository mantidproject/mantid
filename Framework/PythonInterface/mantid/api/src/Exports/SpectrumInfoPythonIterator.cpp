// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidPythonInterface/api/SpectrumInfoPythonIterator.h"

#include <boost/python/class.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/module.hpp>
#include <boost/python/reference_existing_object.hpp>

using Mantid::PythonInterface::SpectrumInfoPythonIterator;
using namespace boost::python;

// Export SpectrumInfoPythonIterator
void export_SpectrumInfoPythonIterator() {

  // Export to Python
  class_<SpectrumInfoPythonIterator>("SpectrumInfoPythonIterator", no_init)
      .def("__iter__", objects::identity_function())
      .def("__next__", &SpectrumInfoPythonIterator::next, return_value_policy<reference_existing_object>());
}
