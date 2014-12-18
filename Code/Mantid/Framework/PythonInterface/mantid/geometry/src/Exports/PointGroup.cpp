
#include "MantidGeometry/Crystal/PointGroup.h"
#include "MantidPythonInterface/kernel/Converters/PyObjectToV3D.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/list.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::PointGroup;

using namespace boost::python;

namespace //<unnamed>
{
  using namespace Mantid::PythonInterface;

  bool isEquivalent(PointGroup & self, const object& hkl1, const object& hkl2)
  {
    return self.isEquivalent(Converters::PyObjectToV3D(hkl1)(), Converters::PyObjectToV3D(hkl2)());
  }

  boost::python::list getEquivalents(PointGroup & self, const object& hkl)
  {
    const std::vector<Mantid::Kernel::V3D> &equivalents = self.getEquivalents(Converters::PyObjectToV3D(hkl)());

    boost::python::list pythonEquivalents;
    for(auto it = equivalents.begin(); it != equivalents.end(); ++it) {
        pythonEquivalents.append(*it);
    }

    return pythonEquivalents;
  }

  Mantid::Kernel::V3D getReflectionFamily(PointGroup & self, const object& hkl)
  {
    return self.getReflectionFamily(Converters::PyObjectToV3D(hkl)());
  }

}

void export_PointGroup()
{
  register_ptr_to_python<boost::shared_ptr<PointGroup> >();

  scope pointGroupScope = class_<PointGroup, boost::noncopyable>("PointGroup", no_init);

  enum_<PointGroup::CrystalSystem>("CrystalSystem")
          .value("Triclinic", PointGroup::Triclinic)
          .value("Monoclinic", PointGroup::Monoclinic)
          .value("Orthorhombic", PointGroup::Orthorhombic)
          .value("Tetragonal", PointGroup::Tetragonal)
          .value("Hexagonal", PointGroup::Hexagonal)
          .value("Trigonal", PointGroup::Trigonal)
          .value("Cubic", PointGroup::Cubic);

  class_<PointGroup, boost::noncopyable>("PointGroup", no_init)
          .def("getName", &PointGroup::getName)
          .def("getSymbol", &PointGroup::getSymbol)
          .def("crystalSystem", &PointGroup::crystalSystem)
          .def("isEquivalent", &isEquivalent, "Check whether the two HKLs are symmetrically equivalent.")
          .def("getEquivalents", &getEquivalents, "Returns an array with all symmetry equivalents of the supplied HKL.")
          .def("getReflectionFamily", &getReflectionFamily, "Returns the same HKL for all symmetry equivalents.");
}

