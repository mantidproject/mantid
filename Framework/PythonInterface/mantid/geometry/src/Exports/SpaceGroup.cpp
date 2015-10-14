
#include "MantidGeometry/Crystal/Group.h"
#include "MantidGeometry/Crystal/SpaceGroup.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::Group;
using Mantid::Geometry::SpaceGroup;
using Mantid::Geometry::SymmetryOperation;

using namespace boost::python;

namespace //<unnamed>
    {
using namespace Mantid::PythonInterface;

boost::python::list getEquivalentPositions(SpaceGroup &self,
                                           const object &point) {
  const std::vector<Mantid::Kernel::V3D> &equivalents =
      self.getEquivalentPositions(Converters::PyObjectToV3D(point)());

  boost::python::list pythonEquivalents;
  for (auto it = equivalents.begin(); it != equivalents.end(); ++it) {
    pythonEquivalents.append(*it);
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
}

void export_SpaceGroup() {
  register_ptr_to_python<boost::shared_ptr<SpaceGroup>>();

  class_<SpaceGroup, boost::noncopyable, bases<Group>>("SpaceGroup", no_init)
      .def("getNumber", &SpaceGroup::number)
      .def("getHMSymbol", &SpaceGroup::hmSymbol)
      .def("getEquivalentPositions", &getEquivalentPositions,
           "Returns an array with all symmetry equivalents of the supplied "
           "HKL.")
      .def("isAllowedReflection", &isAllowedReflection,
           "Returns True if the supplied reflection is allowed with respect to "
           "space group symmetry operations.")
      .def("getPointGroup", &SpaceGroup::getPointGroup,
           "Returns the point group of the space group.")
      .def("getSiteSymmetryGroup", &getSiteSymmetryGroup,
           "Returns the site symmetry group for supplied point coordinates.");
}
