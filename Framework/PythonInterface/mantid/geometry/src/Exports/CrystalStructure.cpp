#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/Crystal/IsotropicAtomBraggScatterer.h"
#include <boost/algorithm/string/join.hpp>
#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using namespace boost::python;

namespace {
SpaceGroup_sptr getSpaceGroup(CrystalStructure &self) {
  return boost::const_pointer_cast<SpaceGroup>(self.spaceGroup());
}

std::vector<std::string> getScatterers(const CrystalStructure &self) {
  CompositeBraggScatterer_sptr scatterers = self.getScatterers();

  std::vector<std::string> scattererStrings;
  scattererStrings.reserve(scatterers->nScatterers());

  for (size_t i = 0; i < scatterers->nScatterers(); ++i) {
    scattererStrings.push_back(
        getIsotropicAtomBraggScattererString(scatterers->getScatterer(i)));
  }

  return scattererStrings;
}

std::string __str__implementation(const CrystalStructure &self) {
  std::stringstream ss;
  ss << "Crystal structure with:\n";
  ss << "Unit cell:";

  const auto cell = self.cell();
  ss << " a = " << cell.a();
  ss << " b = " << cell.b();
  ss << " c = " << cell.c();

  ss << " alpha = " << cell.alpha();
  ss << " beta = " << cell.beta();
  ss << " gamma = " << cell.gamma();

  ss << "\n";

  ss << "Centering: " << self.centering()->getName() << "\n";
  ss << "Space Group: " << self.spaceGroup()->hmSymbol() << "\n";
  ss << "Scatterers: " << boost::algorithm::join(getScatterers(self), ", ");

  return ss.str();
}

std::string __repr__implementation(const CrystalStructure &self) {
  std::stringstream ss;
  ss << "CrystalStructure(\"";

  const auto cell = self.cell();

  ss << cell.a() << " ";
  ss << cell.b() << " ";
  ss << cell.c() << " ";

  ss << cell.alpha() << " ";
  ss << cell.beta() << " ";
  ss << cell.gamma();
  ss << "\", ";

  ss << "\"" << self.spaceGroup()->hmSymbol() << "\", ";
  ss << "\"" << boost::algorithm::join(getScatterers(self), "; ") << "\"";

  ss << ")";

  return ss.str();
}
}

void export_CrystalStructure() {
  class_<CrystalStructure>("CrystalStructure", no_init)
      .def(init<const std::string &, const std::string &, const std::string &>(
          (arg("unitCell"), arg("spaceGroup"), arg("scatterers"))))
      .def("getUnitCell", &CrystalStructure::cell, arg("self"))
      .def("getSpaceGroup", &getSpaceGroup, arg("self"))
      .def("getScatterers", &getScatterers, arg("self"))
      .def("__str__", &__str__implementation)
      .def("__repr__", &__repr__implementation);
}
