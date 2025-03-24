// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/Material.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/tuple.hpp>
#include <numeric>

using Mantid::Kernel::Material;
using Mantid::PhysicalConstants::NeutronAtom;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Material)

namespace {
/**
 *
 * @param chemicalFormula the chemical formula of the material (compound)
 * @return a tuple consisting of a list of the elements and their proportions
 */
tuple chemicalFormula(const Material &self) {
  list atoms, numberAtoms;
  for (const auto &formulaUnit : self.chemicalFormula()) {
    atoms.append(formulaUnit.atom);
    numberAtoms.append(formulaUnit.multiplicity);
  }
  return make_tuple(atoms, numberAtoms);
}

bool toBool(const Material &self) {
  return (self.cohScatterXSection() != 0.) || (self.incohScatterXSection() != 0.) ||
         (self.totalScatterXSection() != 0.) || (self.absorbXSection() != 0.) || (self.cohScatterLength() != 0.) ||
         (self.incohScatterLength() != 0.) || (self.totalScatterLength() != 0.) ||
         (self.cohScatterLengthReal() != 0.) || (self.cohScatterLengthImg() != 0.) ||
         (self.incohScatterLengthReal() != 0.) || (self.incohScatterLengthImg() != 0.) ||
         (self.cohScatterLengthSqrd() != 0.) || (self.incohScatterLengthSqrd() != 0.) ||
         (self.totalScatterLengthSqrd() != 0.);
}

/**
 *
 * @param rmm the relative molecular (formula) mass of this material
 * @return the relative molecular mass
 */
double relativeMolecularMass(const Material &self) {
  const auto &formula = self.chemicalFormula();
  return std::accumulate(formula.cbegin(), formula.cend(), 0., [](double sum, const auto &formulaUnit) {
    return sum + formulaUnit.atom->mass * formulaUnit.multiplicity;
  });
}
} // namespace

void export_Material() {
  register_ptr_to_python<Material *>();
  register_ptr_to_python<std::shared_ptr<Material>>();

  class_<Material>("Material", no_init)
      .def("name", &Material::name, arg("self"), return_value_policy<copy_const_reference>(), "Name of the material")
      .add_property("numberDensity", make_function(&Material::numberDensity), "Number density in atoms per A^-3")
      .add_property("numberDensityEffective", make_function(&Material::numberDensityEffective),
                    "Effective number density in atoms per A^-3")
      .add_property("packingFraction", make_function(&Material::packingFraction),
                    "Packing fraction as a number, ideally, 0 to 1")
      .add_property("temperature", make_function(&Material::temperature), "Temperature")
      .add_property("pressure", make_function(&Material::pressure), "Pressure")
      .add_property("totalAtoms", make_function(&Material::totalAtoms), "Total number of atoms")
      .def("__bool__", &toBool, "Returns True if any of the scattering values are non-zero")
      .def("cohScatterXSection", (double (Material::*)() const)(&Material::cohScatterXSection), (arg("self")),
           "Coherent Scattering Cross-Section for the given wavelength in "
           "barns")
      .def("incohScatterXSection", (double (Material::*)() const)(&Material::incohScatterXSection), (arg("self")),
           "Incoherent Scattering Cross-Section for the given wavelength in "
           "barns")
      .def("totalScatterXSection", (double (Material::*)() const)(&Material::totalScatterXSection), (arg("self")),
           "Total Scattering Cross-Section for the given wavelength in barns")
      .def("absorbXSection", (double (Material::*)(double) const)(&Material::absorbXSection),
           (arg("self"), arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Absorption Cross-Section for the given wavelength in barns")
      .def("cohScatterLength", (double (Material::*)(double) const)(&Material::cohScatterLength),
           (arg("self"), arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Coherent Scattering Length for the given wavelength in fm")
      .def("incohScatterLength", (double (Material::*)(double) const)(&Material::incohScatterLength),
           (arg("self"), arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Incoherent Scattering Length for the given wavelength in fm")
      .def("totalScatterLength", (double (Material::*)(double) const)(&Material::totalScatterLength),
           (arg("self"), arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Total Scattering Length for the given wavelength in fm")
      .def("cohScatterLengthReal", (double (Material::*)(double) const)(&Material::cohScatterLengthReal),
           (arg("self"), arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Real part of Coherent Scattering Length for the given wavelength "
           "in fm")
      .def("cohScatterLengthImg", (double (Material::*)(double) const)(&Material::cohScatterLengthImg),
           (arg("self"), arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Imaginary part of Coherent Scattering Length for the given "
           "wavelength "
           "in fm")
      .def("incohScatterLengthReal", (double (Material::*)(double) const)(&Material::incohScatterLengthReal),
           (arg("self"), arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Real part of Incoherent Scattering Length for the given wavelength "
           "in fm")
      .def("incohScatterLengthImg", (double (Material::*)(double) const)(&Material::incohScatterLengthImg),
           (arg("self"), arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Imaginary part of Incoherent Scattering Length for the given "
           "wavelength "
           "in fm")

      .def("cohScatterLengthSqrd", (double (Material::*)(double) const)(&Material::cohScatterLengthSqrd),
           (arg("self"), arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Coherent Scattering Length Squared <b>^2 for the given wavelength "
           "in fm^2")
      .def("incohScatterLengthSqrd", (double (Material::*)(double) const)(&Material::incohScatterLengthSqrd),
           (arg("self"), arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Incoherent Scattering Length Squared <b>^2 for the given "
           "wavelength in fm^2")
      .def("totalScatterLengthSqrd", (double (Material::*)(double) const)(&Material::totalScatterLengthSqrd),
           (arg("self"), arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Total Scattering Length Squared <b^2> for the given wavelength in "
           "fm^2")
      .def("chemicalFormula", &chemicalFormula, arg("self"),
           "Chemical formula as a tuple of two lists: the first one contains "
           "the Atom object the second their multiplicities within the "
           "formula.")
      .def("relativeMolecularMass", &relativeMolecularMass, arg("self"), "Relative Molecular Mass");
}
