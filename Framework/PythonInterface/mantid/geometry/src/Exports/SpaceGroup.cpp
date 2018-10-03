// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidGeometry/Crystal/Group.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"
#include "MantidPythonInterface/kernel/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/list.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/self.hpp>

using Mantid::Geometry::Group;
using Mantid::Geometry::SpaceGroup;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(SpaceGroup)

namespace //<unnamed>
{
using namespace Mantid::PythonInterface;

boost::python::list getEquivalentPositions(SpaceGroup &self,
                                           const object &point) {
  const std::vector<Mantid::Kernel::V3D> &equivalents =
      self.getEquivalentPositions(Converters::PyObjectToV3D(point)());

  boost::python::list pythonEquivalents;
  for (const auto &equivalent : equivalents) {
    pythonEquivalents.append(equivalent);
  }

  return pythonEquivalents;
}

bool isAllowedReflection(SpaceGroup &self, const object &hkl) {
  const Mantid::Kernel::V3D &hklV3d = Converters::PyObjectToV3D(hkl)();

  return self.isAllowedReflection(hklV3d);
}

Mantid::Geometry::Group_sptr getSiteSymmetryGroup(SpaceGroup &self,
                                                  const object &position) {
  const Mantid::Kernel::V3D &posV3d = Converters::PyObjectToV3D(position)();

  Mantid::Geometry::Group_sptr group =
      boost::const_pointer_cast<Group>(self.getSiteSymmetryGroup(posV3d));

  return group;
}

std::string __repr__implementation(const SpaceGroup &self) {
  std::stringstream ss;
  ss << "SpaceGroupFactory.createSpaceGroup(\"";
  ss << self.hmSymbol();
  ss << "\")";
  return ss.str();
}
} // namespace

void export_SpaceGroup() {
  register_ptr_to_python<boost::shared_ptr<SpaceGroup>>();

  class_<SpaceGroup, boost::noncopyable, bases<Group>>("SpaceGroup", no_init)
      .def("getNumber", &SpaceGroup::number, arg("self"))
      .def("getHMSymbol", &SpaceGroup::hmSymbol, arg("self"))
      .def("getEquivalentPositions", &getEquivalentPositions,
           (arg("self"), arg("point")),
           "Returns an array with all symmetry equivalents of the supplied "
           "HKL.")
      .def("isAllowedReflection", &isAllowedReflection,
           (arg("self"), arg("hkl")),
           "Returns True if the supplied reflection is allowed with respect to "
           "space group symmetry operations.")
      .def("isAllowedUnitCell", &SpaceGroup::isAllowedUnitCell,
           (arg("self"), arg("cell")),
           "Returns true if the metric of the cell "
           "is compatible with the space group.")
      .def("getPointGroup", &SpaceGroup::getPointGroup, arg("self"),
           "Returns the point group of the space group.")
      .def("getSiteSymmetryGroup", &getSiteSymmetryGroup,
           (arg("self"), arg("position")),
           "Returns the site symmetry group for supplied point coordinates.")
      .def(str(self))
      .def("__repr__", &__repr__implementation);
}
