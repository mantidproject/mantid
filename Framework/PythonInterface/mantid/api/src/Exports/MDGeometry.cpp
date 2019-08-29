// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/MDGeometry.h"
#include "MantidAPI/Workspace.h"
#include "MantidPythonInterface/core/Policies/RemoveConst.h"
#include "MantidPythonInterface/core/Policies/VectorToNumpy.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::API::MDGeometry;
using Mantid::PythonInterface::Policies::RemoveConstSharedPtr;
using Mantid::PythonInterface::Policies::VectorToNumpy;
using namespace boost::python;

namespace {
/**
 * Pass through method to convert a std::vector<IMDimension_sptr> to a Python
 * list
 * of IMDimension objects
 * @param self The calling MDGeometry object
 * @return A python list of objects converted from the return of
 * self.getNonIntegratedDimensions()
 */
boost::python::list getNonIntegratedDimensionsAsPyList(const MDGeometry &self) {
  auto dimensions = self.getNonIntegratedDimensions();

  boost::python::list nonIntegrated;
  for (auto &dimension : dimensions) {
    nonIntegrated.append(
        boost::const_pointer_cast<Mantid::Geometry::IMDDimension>(dimension));
  }
  return nonIntegrated;
}
} // namespace

void export_MDGeometry() {
  class_<MDGeometry, boost::noncopyable>("MDGeometry", no_init)
      .def("getNumDims", &MDGeometry::getNumDims, arg("self"),
           "Returns the number of dimensions present")

      .def("getDimension", &MDGeometry::getDimension,
           (arg("self"), arg("index")),
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the description of the :class:`~mantid.api.IMDDimension` "
           "at the given index "
           "(starts from 0). Raises RuntimeError if index is out of range.")

      .def("getDimensionWithId", &MDGeometry::getDimensionWithId,
           (arg("self"), arg("id")),
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the description of the :class:`~mantid.api.IMDDimension` "
           "with the given id string. "
           "Raises ValueError if the string is not a known id.")

      .def("getDimensionIndexByName", &MDGeometry::getDimensionIndexByName,
           (arg("self"), arg("name")),
           "Returns the index of the dimension with the given "
           "name. Raises RuntimeError if the name does not "
           "exist.")

      .def("getDimensionIndexById", &MDGeometry::getDimensionIndexById,
           (arg("self"), arg("id")),
           "Returns the index of the :class:`~mantid.api.IMDDimension` with "
           "the given "
           "ID. Raises RuntimeError if the name does not exist.")

      .def("getNonIntegratedDimensions", &getNonIntegratedDimensionsAsPyList,
           arg("self"),
           "Returns the description objects of the non-integrated dimension as "
           "a python list of :class:`~mantid.api.IMDDimension`.")

      .def("estimateResolution", &MDGeometry::estimateResolution, arg("self"),
           return_value_policy<VectorToNumpy>(),
           "Returns a numpy array containing the width of the smallest bin in "
           "each dimension")

      .def("getXDimension", &MDGeometry::getXDimension, arg("self"),
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the :class:`~mantid.api.IMDDimension` description mapped "
           "to X")

      .def("getYDimension", &MDGeometry::getYDimension, arg("self"),
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the :class:`~mantid.api.IMDDimension` description mapped "
           "to Y")

      .def("getZDimension", &MDGeometry::getZDimension, arg("self"),
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the :class:`~mantid.api.IMDDimension` description mapped "
           "to Z")

      .def("getTDimension", &MDGeometry::getTDimension, arg("self"),
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the :class:`~mantid.api.IMDDimension` description mapped "
           "to time")

      .def("getGeometryXML", &MDGeometry::getGeometryXML, arg("self"),
           "Returns an XML representation, as a string, of the geometry of the "
           "workspace")

      .def("getBasisVector",
           (const Mantid::Kernel::VMD &(MDGeometry::*)(size_t) const) &
               MDGeometry::getBasisVector,
           (arg("self"), arg("index")),
           return_value_policy<copy_const_reference>(),
           "Returns a :class:`~mantid.kernel.VMD` object defining the basis "
           "vector for the specified "
           "dimension")

      .def("hasOriginalWorkspace", &MDGeometry::hasOriginalWorkspace,
           (arg("self"), arg("index")),
           "Returns True if there is a source workspace at the given index")

      .def("numOriginalWorkspaces", &MDGeometry::numOriginalWorkspaces,
           arg("self"), "Returns the number of source workspaces attached")

      .def("getOriginalWorkspace", &MDGeometry::getOriginalWorkspace,
           (arg("self"), arg("index")),
           "Returns the source workspace attached at the given index")

      .def("getOrigin",
           (const Mantid::Kernel::VMD &(MDGeometry::*)() const) &
               MDGeometry::getOrigin,
           arg("self"), return_value_policy<copy_const_reference>(),
           "Returns the vector of the origin (in the original workspace) that "
           "corresponds to 0,0,0... in this workspace")

      .def("getNumberTransformsFromOriginal",
           &MDGeometry::getNumberTransformsFromOriginal, arg("self"),
           "Returns the number of transformations from original workspace "
           "coordinate systems")

      .def("getNumberTransformsToOriginal",
           &MDGeometry::getNumberTransformsToOriginal, arg("self"),
           "Returns the number of transformations to original workspace "
           "coordinate systems")

      ;
}
