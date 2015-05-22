
#include "MantidGeometry/Crystal/Group.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/list.hpp>

using Mantid::Geometry::Group;
using Mantid::Geometry::SymmetryOperation;

using namespace boost::python;

namespace {
    std::vector<std::string> getSymmetryOperationStrings(Group &self) {
      const std::vector<SymmetryOperation> &symOps = self.getSymmetryOperations();

      std::vector<std::string> pythonSymOps;
      for (auto it = symOps.begin(); it != symOps.end(); ++it) {
        pythonSymOps.push_back((*it).identifier());
      }

      return pythonSymOps;
    }
}

// clang-format off
void export_Group()
// clang-format on
{
  enum_<Group::CoordinateSystem>("CoordinateSystem")
      .value("Orthogonal", Group::Orthogonal)
      .value("Hexagonal", Group::Hexagonal);

  enum_<Group::GroupAxiom>("GroupAxiom")
      .value("Closure", Group::Closure)
      .value("Identity", Group::Identity)
      .value("Inversion", Group::Inversion)
      .value("Associativity", Group::Associativity);

  class_<Group>("Group")
      .def(init<std::string>("Construct a group from the provided initializer string."))
      .def(init<std::vector<SymmetryOperation> >("Construct a group from the provided symmetry operation list."))
      .def("getOrder", &Group::order, "Returns the order of the group.")
      .def("getCoordinateSystem", &Group::getCoordinateSystem, "Returns the type of coordinate system to distinguish groups with hexagonal system definition.")
      .def("getSymmetryOperations", &Group::getSymmetryOperations, "Returns the symmetry operations contained in the group.")
      .def("getSymmetryOperationStrings", &getSymmetryOperationStrings, "Returns the x,y,z-strings for the contained symmetry operations.")
      .def("containsOperation", &Group::containsOperation, "Checks whether a SymmetryOperation is included in Group.")
      .def("isGroup", &Group::isGroup, "Checks whether the contained symmetry operations fulfill the group axioms.")
      .def("fulfillAxiom", &Group::fulfillsAxiom, "Checks if the contained symmetry operations fulfill the specified group axiom.");
}
