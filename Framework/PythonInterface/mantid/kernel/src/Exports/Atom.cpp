#include "MantidKernel/Atom.h"
#include "MantidKernel/NeutronAtom.h"
#include "MantidKernel/PhysicalConstants.h"
#include <boost/python/class.hpp>
#include <boost/python/dict.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/make_function.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::PhysicalConstants::Atom;
using Mantid::PhysicalConstants::getAtom;
using namespace boost::python;

namespace {
/**
 * The neutron cross-sections for this atom
 * @return a dict of the neutron cross-sections
 */
dict neutron(Atom &self) {
  dict retval;
  retval["coh_scatt_xs"] = self.neutron.coh_scatt_xs;
  retval["inc_scatt_xs"] = self.neutron.inc_scatt_xs;
  retval["tot_scatt_xs"] = self.neutron.tot_scatt_xs;
  retval["abs_xs"] = self.neutron.abs_scatt_xs;

  retval["coh_scatt_length_real"] = self.neutron.coh_scatt_length_real;
  retval["coh_scatt_length_img"] = self.neutron.coh_scatt_length_img;
  retval["inc_scatt_length_real"] = self.neutron.inc_scatt_length_real;
  retval["inc_scatt_length_img"] = self.neutron.inc_scatt_length_img;

  retval["tot_scatt_length"] = self.neutron.tot_scatt_length;
  retval["coh_scatt_length"] = self.neutron.coh_scatt_length;
  retval["inc_scatt_length"] = self.neutron.inc_scatt_length;

  return retval;
}
/**
 * Constructor for Python Atom class, from element symbol or atomic number
 * and (optional) element mass
 * @param symbol :: Element symbol
 * @param a_number :: Mass number of isotope (number of nucleons)
 * @param z_number :: Atomic number of element (number of protons)
 * @return a dict of the neutron cross-sections
 */
static boost::shared_ptr<Atom> setAtom(const std::string &symbol,
                                       const uint16_t a_number = 0,
                                       const uint16_t z_number = 0) {
  if (z_number > 0) {
    Atom atom = getAtom(z_number, a_number);
    return boost::shared_ptr<Atom>(new Atom(atom));
  }
  // Returns Hydrogen by default
  else if (symbol.empty()) {
    Atom atom = getAtom(1, 0);
    return boost::shared_ptr<Atom>(new Atom(atom));
  } else {
    Atom atom = getAtom(symbol, a_number);
    return boost::shared_ptr<Atom>(new Atom(atom));
  }
}
} // namespace

void export_Atom() {
  register_ptr_to_python<Atom *>();
  register_ptr_to_python<boost::shared_ptr<Atom>>();

  class_<Atom, boost::noncopyable>("Atom", no_init) // No default constructor
      .def("__init__",
           make_constructor(
               &setAtom, default_call_policies(),
               (arg("symbol") = "", arg("a_number") = 0, arg("z_number") = 0)),
           "Constructor for Atom class")
      .def_readonly("symbol", &Atom::symbol, "The element symbol of this atom")
      .def_readonly("z_number", &Atom::z_number,
                    "The atomic number (number of protons) of this atom")
      .def_readonly("a_number", &Atom::a_number,
                    "The mass number (number of nucleons) of this atom")
      .def_readonly("abundance", &Atom::abundance, "The abundance of this atom")
      .def_readonly("mass", &Atom::mass,
                    "The relative atomic mass of this atom")
      .def_readonly("mass_density", &Atom::mass_density,
                    "The mass density of this atom in g/cm^3")
      .def_readonly("number_density", &Atom::number_density,
                    "The number density of this atom in cm^-3")
      .def("neutron", &neutron, arg("self"),
           "Neutron cross-section information for this atom");
}
