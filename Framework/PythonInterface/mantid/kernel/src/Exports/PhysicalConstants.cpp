// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidKernel/PhysicalConstants.h"
#include <boost/python/class.hpp>

namespace {
class PhysicalConstants {};
} // namespace

void export_PhysicalConstants() {
  using namespace boost::python;

  class_<PhysicalConstants>("PhysicalConstants")
      .def_readonly("N_A", &Mantid::PhysicalConstants::N_A)
      .def_readonly("h", &Mantid::PhysicalConstants::h)
      .def_readonly("h_bar", &Mantid::PhysicalConstants::h_bar)
      .def_readonly("g", &Mantid::PhysicalConstants::g)
      .def_readonly("NeutronMass", &Mantid::PhysicalConstants::NeutronMass)
      .def_readonly("NeutronMassAMU", &Mantid::PhysicalConstants::NeutronMassAMU)
      .def_readonly("AtomicMassUnit", &Mantid::PhysicalConstants::AtomicMassUnit)
      .def_readonly("meV", &Mantid::PhysicalConstants::meV)
      .def_readonly("meVtoWavenumber", &Mantid::PhysicalConstants::meVtoWavenumber)
      .def_readonly("meVtoFrequency", &Mantid::PhysicalConstants::meVtoFrequency)
      .def_readonly("meVtoKelvin", &Mantid::PhysicalConstants::meVtoKelvin)
      .def_readonly("E_mev_toNeutronWavenumberSq", &Mantid::PhysicalConstants::E_mev_toNeutronWavenumberSq)
      .def_readonly("MuonLifetime", &Mantid::PhysicalConstants::MuonLifetime)
      .def_readonly("StandardAtmosphere", &Mantid::PhysicalConstants::StandardAtmosphere)
      .def_readonly("BoltzmannConstant", &Mantid::PhysicalConstants::BoltzmannConstant)
      .def_readonly("MuonGyromagneticRatio", &Mantid::PhysicalConstants::MuonGyromagneticRatio);
}
