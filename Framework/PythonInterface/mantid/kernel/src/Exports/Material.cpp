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
using Mantid::PhysicalConstants::Atom;
using Mantid::PhysicalConstants::NeutronAtom;
using namespace boost::python;

namespace {
/**
 *
 * @param chemicalFormula the chemical formula of the material (compound)
 * @return a tuple consisting of a list of the elements and their proportions
 */
tuple chemicalFormula(Material &self) {
  std::string chemicalSymbols = self.name();
  Material::ChemicalFormula CF =
      Material::parseChemicalFormula(chemicalSymbols);
  list atoms, numberAtoms;
  for (size_t i = 0; i < CF.atoms.size(); i++) {
    atoms.append(CF.atoms[i]);
    numberAtoms.append(CF.numberAtoms[i]);
  }
  return make_tuple(atoms, numberAtoms);
}

/**
 *
 * @param rmm the relative molecular (formula) mass of this material
 * @return the relative molecular mass
 */
double relativeMolecularMass(Material &self) {
  std::string chemicalSymbols = self.name();
  Material::ChemicalFormula CF =
      Material::parseChemicalFormula(chemicalSymbols);
  double retval = 0.;
  for (size_t i = 0; i < CF.atoms.size(); i++) {
    retval += CF.atoms[i]->mass * CF.numberAtoms[i];
  }
  return retval;
}
}

void export_Material() {
  register_ptr_to_python<Material *>();
  register_ptr_to_python<boost::shared_ptr<Material>>();

  class_<Material, boost::noncopyable>("Material", no_init)
      .def("name", &Material::name, arg("self"),
           return_value_policy<copy_const_reference>(), "Name of the material")
      .add_property("numberDensity", make_function(&Material::numberDensity),
                    "Number density")
      .add_property("temperature", make_function(&Material::temperature),
                    "Temperature")
      .add_property("pressure", make_function(&Material::pressure), "Pressure")
      .def("cohScatterXSection",
           (double (Material::*)(double) const)(&Material::cohScatterXSection),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Coherent Scattering Cross-Section")
      .def("incohScatterXSection", (double (Material::*)(double)
                                        const)(&Material::incohScatterXSection),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Incoherent Scattering Cross-Section")
      .def("totalScatterXSection", (double (Material::*)(double)
                                        const)(&Material::totalScatterXSection),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Total Scattering Cross-Section")
      .def("absorbXSection",
           (double (Material::*)(double) const)(&Material::absorbXSection),
           (arg("self"),
            arg("lambda") = static_cast<double>(NeutronAtom::ReferenceLambda)),
           "Absorption Cross-Section")
      .def("chemicalFormula", &chemicalFormula, arg("self"), "Chemical Formula")
      .def("relativeMolecularMass", &relativeMolecularMass, arg("self"),
           "Relative Molecular Mass");
}
