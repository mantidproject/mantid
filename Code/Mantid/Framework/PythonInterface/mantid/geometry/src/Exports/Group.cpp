
#include "MantidGeometry/Crystal/Group.h"

#include <boost/python/class.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/list.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::Geometry;
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

Group_sptr constructGroupFromString(const std::string &initializerString) {
  return boost::const_pointer_cast<Group>(
      GroupFactory::create<Group>(initializerString));
}

Group_sptr
constructGroupFromVector(const std::vector<SymmetryOperation> &symOps) {
  return boost::const_pointer_cast<Group>(GroupFactory::create<Group>(symOps));
}

Group_sptr constructGroupFromPythonList(const boost::python::list &symOpList) {
  std::vector<SymmetryOperation> operations;

  for (int i = 0; i < len(symOpList); ++i) {
    operations.push_back(
        boost::python::extract<SymmetryOperation>(symOpList[i]));
  }

  return boost::const_pointer_cast<Group>(
      GroupFactory::create<Group>(operations));
}
}

void export_Group() {

  register_ptr_to_python<boost::shared_ptr<Group>>();

  enum_<Group::CoordinateSystem>("CoordinateSystem")
      .value("Orthogonal", Group::Orthogonal)
      .value("Hexagonal", Group::Hexagonal);

  enum_<Group::GroupAxiom>("GroupAxiom")
      .value("Closure", Group::Closure)
      .value("Identity", Group::Identity)
      .value("Inversion", Group::Inversion)
      .value("Associativity", Group::Associativity);

  class_<Group, boost::noncopyable>("Group", no_init)
      .def("__init__", make_constructor(&constructGroupFromString),
           "Construct a group from the provided initializer string.")
      .def("__init__", make_constructor(&constructGroupFromVector),
           "Construct a group from the provided symmetry operation list.")
      .def("__init__", make_constructor(&constructGroupFromPythonList),
           "Construct a group from a python generated symmetry operation list.")
      .def("getOrder", &Group::order, "Returns the order of the group.")
      .def("getCoordinateSystem", &Group::getCoordinateSystem,
           "Returns the type of coordinate system to distinguish groups with "
           "hexagonal system definition.")
      .def("getSymmetryOperations", &Group::getSymmetryOperations,
           "Returns the symmetry operations contained in the group.")
      .def("getSymmetryOperationStrings", &getSymmetryOperationStrings,
           "Returns the x,y,z-strings for the contained symmetry operations.")
      .def("containsOperation", &Group::containsOperation,
           "Checks whether a SymmetryOperation is included in Group.")
      .def("isGroup", &Group::isGroup, "Checks whether the contained symmetry "
                                       "operations fulfill the group axioms.")
      .def("fulfillsAxiom", &Group::fulfillsAxiom,
           "Checks if the contained symmetry operations fulfill the specified "
           "group axiom.");
}
