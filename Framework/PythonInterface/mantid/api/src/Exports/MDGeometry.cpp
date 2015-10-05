#include "MantidAPI/MDGeometry.h"
#include "MantidPythonInterface/kernel/Policies/RemoveConst.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"
#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/list.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::API::MDGeometry;
using Mantid::Geometry::IMDDimension_const_sptr;
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
  for (auto it = dimensions.begin(); it != dimensions.end(); ++it) {
    nonIntegrated.append(
        boost::const_pointer_cast<Mantid::Geometry::IMDDimension>(*it));
  }
  return nonIntegrated;
}
}

void export_MDGeometry() {
  class_<MDGeometry, boost::noncopyable>("MDGeometry", no_init)
      .def("getNumDims", &MDGeometry::getNumDims,
           "Returns the number of dimensions present")

      .def("getDimension", &MDGeometry::getDimension, (args("index")),
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the description of the dimension at the given index "
           "(starts from 0). Raises RuntimeError if index is out of range.")

      .def("getDimensionWithId", &MDGeometry::getDimensionWithId, (args("id")),
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the description of the dimension with the given id string. "
           "Raises ValueError if the string is not a known id.")

      .def("getDimensionIndexByName", &MDGeometry::getDimensionIndexByName,
           (args("name")), "Returns the index of the dimension with the given "
                           "name. Raises RuntimeError if the name does not "
                           "exist.")

      .def("getDimensionIndexById", &MDGeometry::getDimensionIndexById,
           (args("id")), "Returns the index of the dimension with the given "
                         "ID. Raises RuntimeError if the name does not exist.")

      .def("getNonIntegratedDimensions", &getNonIntegratedDimensionsAsPyList,
           "Returns the description objects of the non-integrated dimension as "
           "a python list of IMDDimension.")

      .def("estimateResolution", &MDGeometry::estimateResolution,
           return_value_policy<VectorToNumpy>(),
           "Returns a numpy array containing the width of the smallest bin in "
           "each dimension")

      .def("getXDimension", &MDGeometry::getXDimension,
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the dimension description mapped to X")

      .def("getYDimension", &MDGeometry::getYDimension,
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the dimension description mapped to Y")

      .def("getZDimension", &MDGeometry::getZDimension,
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the dimension description mapped to Z")

      .def("getTDimension", &MDGeometry::getTDimension,
           return_value_policy<RemoveConstSharedPtr>(),
           "Returns the dimension description mapped to time")

      .def("getGeometryXML", &MDGeometry::getGeometryXML,
           "Returns an XML representation, as a string, of the geometry of the "
           "workspace")

      .def("getBasisVector",
           (const Mantid::Kernel::VMD &(MDGeometry::*)(size_t) const) &
               MDGeometry::getBasisVector,
           (args("index")), return_value_policy<copy_const_reference>(),
           "Returns a VMD object defining the basis vector for the specified "
           "dimension")

      .def("hasOriginalWorkspace", &MDGeometry::hasOriginalWorkspace,
           (args("index")),
           "Returns True if there is a source workspace at the given index")

      .def("numOriginalWorkspaces", &MDGeometry::numOriginalWorkspaces,
           "Returns the number of source workspaces attached")

      .def("getOriginalWorkspace", &MDGeometry::getOriginalWorkspace,
           (args("index")),
           "Returns the source workspace attached at the given index")

      .def("getOrigin", (const Mantid::Kernel::VMD &(MDGeometry::*)() const) &
                            MDGeometry::getOrigin,
           return_value_policy<copy_const_reference>(),
           "Returns the vector of the origin (in the original workspace) that "
           "corresponds to 0,0,0... in this workspace")

      .def("getNumberTransformsFromOriginal",
           &MDGeometry::getNumberTransformsFromOriginal,
           "Returns the number of transformations from original workspace "
           "coordinate systems")

      .def("getNumberTransformsToOriginal",
           &MDGeometry::getNumberTransformsToOriginal,
           "Returns the number of transformations to original workspace "
           "coordinate systems")

      ;
}
