// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidKernel/UnitLabel.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidPythonInterface/core/GetPointer.h"
#include <boost/shared_ptr.hpp>

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_internal_reference.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(IMDDimension)

GNU_DIAG_OFF("strict-aliasing")

namespace {
/**
 * @param self A reference to the calling object
 * @return A plain-text string giving the units
 */
std::string getUnitsAsStr(IMDDimension &self) {
  return self.getUnits().ascii();
}

/**
 * @brief getMDFrame
 * @param self : A reference to the calling object
 * @return cloned MDFrame wrapped as a shared pointer.
 */
boost::shared_ptr<MDFrame> getMDFrame(const IMDDimension &self) {
  return boost::shared_ptr<MDFrame>(self.getMDFrame().clone());
}
} // namespace

//--------------------------------------------------------------------------------------
// Deprecated function
//--------------------------------------------------------------------------------------
/**
 * @param self Reference to the calling object
 * @return name of the dimension.
 */
std::string getName(IMDDimension &self) {
  PyErr_Warn(PyExc_DeprecationWarning,
             ".getName() is deprecated. Use .name instead.");
  return self.getName();
}

void export_IMDDimension() {
  register_ptr_to_python<boost::shared_ptr<IMDDimension>>();

  class_<IMDDimension, boost::noncopyable>("IMDDimension", no_init)
      .def("getName", &getName, arg("self"),
           "Return the name of the dimension "
           "as can be displayed along the "
           "axis")
      .add_property("name", &IMDDimension::getName,
                    "Return the name of the dimension as can be displayed "
                    "along the axis")
      .def("getMaximum", &IMDDimension::getMaximum, arg("self"),
           "Return the maximum extent of this dimension")
      .def("getMinimum", &IMDDimension::getMinimum, arg("self"),
           "Return the maximum extent of this dimension")
      .def("getNBins", &IMDDimension::getNBins, arg("self"),
           "Return the number of bins dimension have (an integrated has one). "
           "A axis directed along dimension would have getNBins+1 axis points.")
      .def("getNBoundaries", &IMDDimension::getNBoundaries, arg("self"),
           "Return the number of bins boundaries (axis points) dimension have "
           "(an integrated has two). "
           "A axis directed along dimension would have getNBins+1 axis points.")
      .def("getX", &IMDDimension::getX, (arg("self"), arg("ind")),
           "Return coordinate of the axis at the given index")
      .def("getBinWidth", &IMDDimension::getBinWidth, arg("self"),
           "Return the width of each bin.")
      .def("getDimensionId", &IMDDimension::getDimensionId, arg("self"),
           return_value_policy<copy_const_reference>(),
           "Return a short name which identify the dimension among other "
           "dimension."
           "A dimension can be usually find by its ID and various  ")
      .def("getUnits", &getUnitsAsStr, arg("self"),
           "Return the units associated with this dimension.")
      .def("getMDFrame", &getMDFrame, arg("self"),
           "Return the multidimensional frame for this dimension.");
}
