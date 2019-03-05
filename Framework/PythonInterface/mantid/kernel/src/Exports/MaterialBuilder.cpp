// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/MaterialBuilder.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/implicit.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Kernel::MaterialBuilder;
using namespace boost::python;

void export_MaterialBuilder() {
  register_ptr_to_python<MaterialBuilder *>();
  register_ptr_to_python<boost::shared_ptr<MaterialBuilder>>();

  enum_<MaterialBuilder::NumberDensityUnit>("NumberDensityUnit")
      .value("Atoms", MaterialBuilder::NumberDensityUnit::Atoms)
      .value("FormulaUnits", MaterialBuilder::NumberDensityUnit::FormulaUnits)
      .export_values();

  class_<MaterialBuilder, boost::noncopyable>("MaterialBuilder")
      .def("setName", &MaterialBuilder::setName, return_self<>(),
           (arg("self"), arg("name")),
           "Set the string name given to the "
           "material. Defaults to the chemical "
           "formula.")
      .def("setFormula", &MaterialBuilder::setFormula, return_self<>(),
           (arg("self"), arg("formula")),
           "Set the chemical formula of the material")
      .def("setAtomicNumber", &MaterialBuilder::setAtomicNumber,
           return_self<>(), (arg("self"), arg("atomicNumber")),
           "Set the atomic number of the material")
      .def("setMassNumber", &MaterialBuilder::setMassNumber, return_self<>(),
           (arg("self"), arg("massNumber")),
           "Set the mass number of the material")
      .def("setNumberDensity", &MaterialBuilder::setNumberDensity,
           return_self<>(), (arg("self"), arg("rho")),
           "Set the number density of the material in atoms (default) or "
           "formula units per Angstrom^3")
      .def("setNumberDensityUnit", &MaterialBuilder::setNumberDensityUnit,
           return_self<>(), (arg("self"), arg("unit")),
           "Change the number density units from atoms per Angstrom^3 to the "
           "desired unit")
      .def("setZParameter", &MaterialBuilder::setZParameter, return_self<>(),
           (arg("self"), arg("zparam")),
           "Set the number of formula units in a unit cell")
      .def("setUnitCellVolume", &MaterialBuilder::setUnitCellVolume,
           return_self<>(), (arg("self"), arg("cellVolume")),
           "Set the unit cell volume of the material")
      .def("setMassDensity", &MaterialBuilder::setMassDensity, return_self<>(),
           (arg("self"), arg("massDensity")),
           "Set the mass density of the material in g / cc")
      .def("setTotalScatterXSection", &MaterialBuilder::setTotalScatterXSection,
           return_self<>(), (arg("self"), arg("xsec")),
           "Set the total scattering cross section of the material in barns")
      .def("setCoherentXSection", &MaterialBuilder::setCoherentXSection,
           return_self<>(), (arg("self"), arg("xsec")),
           "Set the coherent cross section of the material in barns")
      .def("setIncoherentXSection", &MaterialBuilder::setIncoherentXSection,
           return_self<>(), (arg("self"), arg("xsec")),
           "Set the incoherent cross section of the material in barns")
      .def("setAbsorptionXSection", &MaterialBuilder::setAbsorptionXSection,
           return_self<>(), (arg("self"), arg("xsec")),
           "Set the absorption cross section of the material in barns")
      .def("build", &MaterialBuilder::build,
           return_value_policy<return_by_value>(), (arg("self")),
           "Build the new mantid.kernel.Material object from the current set "
           "of options");
}
