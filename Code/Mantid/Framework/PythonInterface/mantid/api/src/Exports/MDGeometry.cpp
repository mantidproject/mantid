#include "MantidAPI/MDGeometry.h"
#include "MantidPythonInterface/kernel/Policies/VectorToNumpy.h"

#include <boost/python/class.hpp>
#include <boost/python/list.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::API::MDGeometry;
using Mantid::Geometry::IMDDimension_const_sptr;
using Mantid::PythonInterface::Policies::VectorToNumpy;
using namespace boost::python;

namespace
{
  /**
   * Pass through method to convert a std::vector<IMDimension_sptr> to a Python list
   * of IMDimension objects
   * @param self The calling MDGeometry object
   * @return A python list of objects converted from the return of self.getNonIntegratedDimensions()
   */
  boost::python::list getNonIntegratedDimensionsAsPyList(const MDGeometry & self)
  {
    auto dimensions = self.getNonIntegratedDimensions();

    boost::python::list nonIntegrated;
    //auto converter = boost::python::to_python_value<IMDDimension_const_sptr>();
    for(auto it = dimensions.begin(); it != dimensions.end(); ++it)
    {
      //auto item = converter(*it);
      nonIntegrated.append(*it);
    }
    return nonIntegrated;
  }

}

void export_MDGeometry()
{
  class_<MDGeometry,boost::noncopyable>("MDGeometry", no_init)
    .def("getNumDims", &MDGeometry::getNumDims, "Returns the number of dimensions present")

    .def("getDimension", &MDGeometry::getDimension, (args("index")),
         "Returns the description of the dimension at the given index (starts from 0). Raises RuntimeError if index is out of range.")

    .def("getDimensionWithId", &MDGeometry::getDimensionWithId, (args("id")),
         "Returns the description of the dimension with the given id string. Raises ValueError if the string is not a known id.")

    .def("getDimensionIndexByName", &MDGeometry::getDimensionIndexByName, (args("name")),
         "Returns the index of the dimension with the given name. Raises RuntimeError if the name does not exist.")

    .def("getDimensionIndexById", &MDGeometry::getDimensionIndexById, (args("id")),
         "Returns the index of the dimension with the given ID. Raises RuntimeError if the name does not exist.")

    .def("getNonIntegratedDimensions", &getNonIntegratedDimensionsAsPyList,
         "Returns the description objects of the non-integrated dimension as a python list of IMDDimension.")

    .def("estimateResolution", &MDGeometry::estimateResolution, return_value_policy<VectorToNumpy>(),
         "Returns a numpy array containing the width of the smallest bin in each dimension")

    .def("getXDimension", &MDGeometry::getXDimension,
         "Returns the dimension description mapped to X")

    .def("getYDimension", &MDGeometry::getYDimension,
         "Returns the dimension description mapped to Y")

    .def("getZDimension", &MDGeometry::getZDimension,
          "Returns the dimension description mapped to Z")

    .def("getTDimension", &MDGeometry::getTDimension,
         "Returns the dimension description mapped to time")
    ;
}

