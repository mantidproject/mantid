
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

  boost::python::list getEquivalentPositions(SpaceGroup & self, const object& point)
  {
    const std::vector<Mantid::Kernel::V3D> &equivalents = self.getEquivalentPositions(Converters::PyObjectToV3D(point)());

    boost::python::list pythonEquivalents;
    for(auto it = equivalents.begin(); it != equivalents.end(); ++it) {
        pythonEquivalents.append(*it);
    }

    return pythonEquivalents;
  }

  std::vector<std::string> getSymmetryOperationStrings(SpaceGroup & self)
  {
      const std::vector<SymmetryOperation> &symOps = self.getSymmetryOperations();

      std::vector<std::string> pythonSymOps;
      for(auto it = symOps.begin(); it != symOps.end(); ++it) {
          pythonSymOps.push_back((*it).identifier());
      }

      return pythonSymOps;
  }

}

void export_SpaceGroup()
{
  register_ptr_to_python<boost::shared_ptr<const SpaceGroup> >();

  class_<SpaceGroup, boost::noncopyable, bases<Group> >("SpaceGroup", no_init)
          .def("order", &SpaceGroup::order)
          .def("getSymmetryOperationStrings", &getSymmetryOperationStrings)
          .def("number", &SpaceGroup::number)
          .def("hmSymbol", &SpaceGroup::hmSymbol)
          .def("getEquivalentPositions", &getEquivalentPositions, "Returns an array with all symmetry equivalents of the supplied HKL.")
          ;
}

