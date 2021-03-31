// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidTypes/SpectrumDefinition.h"

#include <boost/python/class.hpp>
#include <boost/python/tuple.hpp>

using Mantid::SpectrumDefinition;
using namespace boost::python;

// Helper function to convert a std::Pair to a Python tuple
boost::python::tuple toTuple(const SpectrumDefinition &self, const size_t index) {
  const std::pair<size_t, size_t> pair = self.operator[](index);
  return make_tuple(pair.first, pair.second);
}

void export_SpectrumDefinition() {
  class_<SpectrumDefinition>("SpectrumDefinition", no_init)

      .def("__getitem__", &toTuple, (arg("self"), arg("index")),
           "Returns the pair of detector index and time index at given index "
           "of spectrum definition.")

      .def("__len__", &SpectrumDefinition::size, arg("self"),
           "Returns the size of the SpectrumDefinition i.e. the number of "
           "detectors for the spectrum.")

      .def("size", &SpectrumDefinition::size, arg("self"),
           "Returns the size of the SpectrumDefinition i.e. the number of "
           "detectors for the spectrum.")

      .def("add", &SpectrumDefinition::add, (arg("self"), arg("detectorIndex"), arg("timeIndex")),
           "Adds a pair of detector index and time index to the spectrum "
           "definition.")

      .def("equals", &SpectrumDefinition::operator==, (arg("self"), arg("other")), "Compare spectrum definitions.");
}
