// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Crystal/Group.h"
#include "MantidPythonInterface/core/Converters/PyObjectToMatrix.h"
#include "MantidPythonInterface/core/GetPointer.h"

#include <algorithm>
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/enum.hpp>
#include <boost/python/list.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/scope.hpp>
#include <boost/python/stl_iterator.hpp>

using namespace Mantid::Geometry;
using Mantid::Geometry::SymmetryOperation;
using Mantid::PythonInterface::Converters::PyObjectToMatrix;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(Group)

namespace {
std::vector<std::string> getSymmetryOperationStrings(const Group &self) {
  const auto &symOps = self.getSymmetryOperations();

  std::vector<std::string> pythonSymOps;
  pythonSymOps.reserve(symOps.size());
  std::transform(symOps.cbegin(), symOps.cend(), std::back_inserter(pythonSymOps),
                 [](const auto &symOp) { return symOp.identifier(); });

  return pythonSymOps;
}

Group_sptr constructGroupFromString(const std::string &initializerString) {
  return std::const_pointer_cast<Group>(GroupFactory::create<Group>(initializerString));
}

Group_sptr constructGroupFromVector(const std::vector<SymmetryOperation> &symOps) {
  return std::const_pointer_cast<Group>(GroupFactory::create<Group>(symOps));
}

Group_sptr constructGroupFromPythonList(const boost::python::list &symOpList) {
  std::vector<SymmetryOperation> operations;

  for (int i = 0; i < len(symOpList); ++i) {
    operations.emplace_back(boost::python::extract<SymmetryOperation>(symOpList[i]));
  }

  return std::const_pointer_cast<Group>(GroupFactory::create<Group>(operations));
}

bool isInvariantDefault(const Group &self, const boost::python::object &tensor) {
  return self.isInvariant(PyObjectToMatrix(tensor)());
}

bool isInvariantTolerance(const Group &self, const boost::python::object &tensor, double tolerance) {
  return self.isInvariant(PyObjectToMatrix(tensor)(), tolerance);
}
} // namespace

void export_Group() {

  register_ptr_to_python<std::shared_ptr<Group>>();

  enum_<Group::CoordinateSystem>("CoordinateSystem")
      .value("Orthogonal", Group::Orthogonal)
      .value("Hexagonal", Group::Hexagonal);

  enum_<Group::GroupAxiom>("GroupAxiom")
      .value("Closure", Group::Closure)
      .value("Identity", Group::Identity)
      .value("Inversion", Group::Inversion)
      .value("Associativity", Group::Associativity);

  class_<Group, boost::noncopyable>("Group", no_init)
      .def("__init__",
           make_constructor(&constructGroupFromString, default_call_policies(), (arg("symmetryOperationString"))),
           "Construct a group from the provided initializer string.")
      .def("__init__",
           make_constructor(&constructGroupFromVector, default_call_policies(), (arg("symmetryOperationVector"))),
           "Construct a group from the provided symmetry operation list.")
      .def("__init__",
           make_constructor(&constructGroupFromPythonList, default_call_policies(), (arg("symmetryOperationList"))),
           "Construct a group from a python generated symmetry operation list.")
      .def("getOrder", &Group::order, arg("self"), "Returns the order of the group.")
      .def("getCoordinateSystem", &Group::getCoordinateSystem, arg("self"),
           "Returns the type of coordinate system to distinguish groups with "
           "hexagonal system definition.")
      .def("getSymmetryOperations", &Group::getSymmetryOperations, arg("self"),
           return_value_policy<copy_const_reference>(), "Returns the symmetry operations contained in the group.")
      .def("getSymmetryOperationStrings", &getSymmetryOperationStrings, arg("self"),
           "Returns the x,y,z-strings for the contained symmetry operations.")
      .def("containsOperation", &Group::containsOperation, (arg("self"), arg("operation")),
           "Checks whether a SymmetryOperation is included in Group.")

      .def("isInvariant", &isInvariantDefault, (arg("self"), arg("tensor")),
           "Returns true if the tensor is not changed by the group's symmetry "
           "operations with a tolerance of 1e-8.")
      .def("isInvariant", &isInvariantTolerance, (arg("self"), arg("tensor"), arg("tolerance")),
           "Returns true if the tensor is not changed by the group's symmetry "
           "operations with the given tolerance.")
      .def("isGroup", &Group::isGroup, arg("self"),
           "Checks whether the contained symmetry "
           "operations fulfill the group axioms.")
      .def("fulfillsAxiom", &Group::fulfillsAxiom, (arg("self"), arg("axiom")),
           "Checks if the contained symmetry operations fulfill the specified "
           "group axiom.");
}
