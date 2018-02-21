#include "MantidPythonInterface/kernel/GetPointer.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/Atom.h"
#include "MantidKernel/NeutronAtom.h"
#include <boost/python/tuple.hpp>
#include <boost/python/list.hpp>
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/register_ptr_to_python.hpp>

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
tuple chemicalFormula(Material &self) {
  list atoms, numberAtoms;
  for (const auto &formulaUnit : self.chemicalFormula()) {
    atoms.append(formulaUnit.atom);
    numberAtoms.append(formulaUnit.multiplicity);
  }
  return make_tuple(atoms, numberAtoms);
}

/**
 *
 * @param rmm the relative molecular (formula) mass of this material
 * @return the relative molecular mass
 */
double relativeMolecularMass(Material &self) {
  double retval = 0.;
  for (const auto &formulaUnit : self.chemicalFormula()) {
    retval += formulaUnit.atom->mass * formulaUnit.multiplicity;
  }
  return retval;
}
}

void export_Material() {
  register_ptr_to_python<Material *>();
  register_ptr_to_python<boost::shared_ptr<Material>>();

  class_<Material>("Material", no_init)
      .def("name", &Material::name, arg("self"),
           return_value_policy<copy_const_reference>(), "Name of the material")
      .add_property("numberDensity", make_function(&Material::numberDensity),
                    "Number density in atoms per A^-3")
      .add_property("temperature", make_function(&Material::temperature),
                    "Temperature")
      .add_property("pressure", make_function(&Material::pressure), "Pressure")
      .def("cohScatterXSection",
           (double (Material::*)(double) const)(&Material::cohScatterXSection),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Coherent Scattering Cross-Section for the given wavelength in "
           "barns")
      .def("incohScatterXSection", (double (Material::*)(double)
                                        const)(&Material::incohScatterXSection),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Incoherent Scattering Cross-Section for the given wavelength in "
           "barns")
      .def("totalScatterXSection", (double (Material::*)(double)
                                        const)(&Material::totalScatterXSection),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Total Scattering Cross-Section for the given wavelength in barns")
      .def("absorbXSection",
           (double (Material::*)(double) const)(&Material::absorbXSection),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Absorption Cross-Section for the given wavelength in barns")

      .def("cohScatterLength",
           (double (Material::*)(double) const)(&Material::cohScatterLength),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Coherent Scattering Length for the given wavelength in fm")
      .def("incohScatterLength",
           (double (Material::*)(double) const)(&Material::incohScatterLength),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Incoherent Scattering Length for the given wavelength in fm")
      .def("totalScatterLength",
           (double (Material::*)(double) const)(&Material::totalScatterLength),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Total Scattering Length for the given wavelength in fm")

      .def("cohScatterLengthReal", (double (Material::*)(double)
                                        const)(&Material::cohScatterLengthReal),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Real part of Coherent Scattering Length for the given wavelength "
           "in fm")
      .def("cohScatterLengthImg",
           (double (Material::*)(double) const)(&Material::cohScatterLengthImg),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Imaginary part of Coherent Scattering Length for the given "
           "wavelength "
           "in fm")
      .def("incohScatterLengthReal", (double (Material::*)(double) const)(
                                         &Material::incohScatterLengthReal),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Real part of Incoherent Scattering Length for the given wavelength "
           "in fm")
      .def("incohScatterLengthImg", (double (Material::*)(double) const)(
                                        &Material::incohScatterLengthImg),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Imaginary part of Incoherent Scattering Length for the given "
           "wavelength "
           "in fm")
      .def("cohScatterLengthSqrd", (double (Material::*)(double)
                                        const)(&Material::cohScatterLengthSqrd),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Coherent Scattering Length Squared <b>^2 for the given wavelength "
           "in fm^2")
      .def("incohScatterLengthSqrd", (double (Material::*)(double) const)(
                                         &Material::incohScatterLengthSqrd),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Incoherent Scattering Length Squared <b>^2 for the given "
           "wavelength in fm^2")
      .def("totalScatterLengthSqrd", (double (Material::*)(double) const)(
                                         &Material::totalScatterLengthSqrd),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Total Scattering Length Squared <b^2> for the given wavelength in "
           "fm^2")

      .def("chemicalFormula", &chemicalFormula, arg("self"), "Chemical Formula")
      .def("relativeMolecularMass", &relativeMolecularMass, arg("self"),
           "Relative Molecular Mass");
}
