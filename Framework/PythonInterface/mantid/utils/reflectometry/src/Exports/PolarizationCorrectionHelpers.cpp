// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidAlgorithms/PolarizationCorrections/PolarizationCorrectionsHelpers.h"
#include <boost/python/class.hpp>

namespace {
class SpinStatesORSO {};
} // namespace

void export_SpinStatesORSO() {
  using namespace boost::python;

  class_<SpinStatesORSO>("SpinStatesORSO")
      .def_readonly("PP", &Mantid::Algorithms::SpinStatesORSO::PP)
      .def_readonly("PM", &Mantid::Algorithms::SpinStatesORSO::PM)
      .def_readonly("MP", &Mantid::Algorithms::SpinStatesORSO::MP)
      .def_readonly("MM", &Mantid::Algorithms::SpinStatesORSO::MM)
      .def_readonly("PO", &Mantid::Algorithms::SpinStatesORSO::PO)
      .def_readonly("MO", &Mantid::Algorithms::SpinStatesORSO::MO)
      .def_readonly("LOG_NAME", &Mantid::Algorithms::SpinStatesORSO::LOG_NAME);
}
