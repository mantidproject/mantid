#include "MantidGeometry/Crystal/CrystalStructure.h"
#include "MantidGeometry/Crystal/IsotropicAtomBraggScatterer.h"
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

std::vector<std::string> getScatterers(CrystalStructure &self) {
  CompositeBraggScatterer_sptr scatterers = self.getScatterers();

  std::vector<std::string> scattererStrings;
  scattererStrings.reserve(scatterers->nScatterers());

  for (size_t i = 0; i < scatterers->nScatterers(); ++i) {
    scattererStrings.push_back(
        getIsotropicAtomBraggScattererString(scatterers->getScatterer(i)));
  }

  return scattererStrings;
}
}

void export_CrystalStructure() {
  class_<CrystalStructure>("CrystalStructure", no_init)
      .def(init<const std::string &, const std::string &, const std::string &>(
          (arg("unitCell"), arg("spaceGroup"), arg("scatterers"))))
      .def("getUnitCell", &CrystalStructure::cell, arg("self"))
      .def("getSpaceGroup", &getSpaceGroup, arg("self"))
      .def("getScatterers", &getScatterers, arg("self"));
}
