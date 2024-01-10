// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidGeometry/Crystal/Group.h"
#include "MantidPythonInterface/core/Converters/PyObjectToV3D.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/list.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/self.hpp>

using Mantid::Geometry::Group;
using Mantid::Geometry::PointGroup;

using namespace boost::python;

GET_POINTER_SPECIALIZATION(PointGroup)

namespace //<unnamed>
{
using namespace Mantid::PythonInterface;

bool isEquivalent(const PointGroup &self, const object &hkl1, const object &hkl2) {
  return self.isEquivalent(Converters::PyObjectToV3D(hkl1)(), Converters::PyObjectToV3D(hkl2)());
}

boost::python::list getEquivalents(const PointGroup &self, const object &hkl) {
  const auto &equivalents = self.getEquivalents(Converters::PyObjectToV3D(hkl)());

  boost::python::list pythonEquivalents;
  for (const auto &equivalent : equivalents) {
    pythonEquivalents.append(equivalent);
  }

  return pythonEquivalents;
}

Mantid::Kernel::V3D getReflectionFamily(const PointGroup &self, const object &hkl) {
  return self.getReflectionFamily(Converters::PyObjectToV3D(hkl)());
}

std::string __repr__implementation(const PointGroup &self) {
  std::stringstream ss;
  ss << "PointGroupFactory.createPointGroup(\"";
  ss << self.getSymbol();
  ss << "\")";
  return ss.str();
}
} // namespace

void export_PointGroup() {
  register_ptr_to_python<std::shared_ptr<PointGroup>>();

  scope pointGroupScope = class_<PointGroup, boost::noncopyable>("PointGroup", no_init);

  enum_<PointGroup::CrystalSystem>("CrystalSystem")
      .value("Triclinic", PointGroup::CrystalSystem::Triclinic)
      .value("Monoclinic", PointGroup::CrystalSystem::Monoclinic)
      .value("Orthorhombic", PointGroup::CrystalSystem::Orthorhombic)
      .value("Tetragonal", PointGroup::CrystalSystem::Tetragonal)
      .value("Hexagonal", PointGroup::CrystalSystem::Hexagonal)
      .value("Trigonal", PointGroup::CrystalSystem::Trigonal)
      .value("Cubic", PointGroup::CrystalSystem::Cubic);

  enum_<PointGroup::LatticeSystem>("LatticeSystem")
      .value("Triclinic", PointGroup::LatticeSystem::Triclinic)
      .value("Monoclinic", PointGroup::LatticeSystem::Monoclinic)
      .value("Orthorhombic", PointGroup::LatticeSystem::Orthorhombic)
      .value("Tetragonal", PointGroup::LatticeSystem::Tetragonal)
      .value("Hexagonal", PointGroup::LatticeSystem::Hexagonal)
      .value("Rhombohedral", PointGroup::LatticeSystem::Rhombohedral)
      .value("Cubic", PointGroup::LatticeSystem::Cubic);

  class_<PointGroup, boost::noncopyable, bases<Group>>("PointGroup", no_init)
      .def("getName", &PointGroup::getName, arg("self"))
      .def("getHMSymbol", &PointGroup::getSymbol, arg("self"))
      .def("getCrystalSystem", &PointGroup::crystalSystem, arg("self"))
      .def("getLatticeSystem", &PointGroup::latticeSystem, arg("self"))
      .def("isEquivalent", &isEquivalent, (arg("self"), arg("hkl1"), arg("hkl2")),
           "Check whether the two HKLs are symmetrically equivalent.")
      .def("getEquivalents", &getEquivalents, (arg("self"), arg("hkl")),
           "Returns an array with all symmetry equivalents of the supplied "
           "HKL.")
      .def("getLauePointGroupSymbol", &PointGroup::getLauePointGroupSymbol, arg("self"))
      .def("getReflectionFamily", &getReflectionFamily, (arg("self"), arg("hkl")),
           "Returns the same HKL for all symmetry equivalents.")
      .def(str(self))
      .def("__repr__", &__repr__implementation);
}
