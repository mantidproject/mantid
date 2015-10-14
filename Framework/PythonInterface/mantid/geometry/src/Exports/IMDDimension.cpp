#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidGeometry/MDGeometry/MDFrame.h"
#include "MantidKernel/UnitLabel.h"
#include "MantidKernel/WarningSuppressions.h"
#include <boost/shared_ptr.hpp>

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>
#include <boost/python/return_internal_reference.hpp>
#include <boost/python/copy_const_reference.hpp>

using namespace Mantid::Geometry;
using namespace boost::python;

// clang-format off
GCC_DIAG_OFF(strict-aliasing)
// clang-format on

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
}

void export_IMDDimension() {
  register_ptr_to_python<boost::shared_ptr<IMDDimension>>();

  class_<IMDDimension, boost::noncopyable>("IMDDimension", no_init)
      .def("getName", &IMDDimension::getName, "Return the name of the "
                                              "dimension as can be displayed "
                                              "along the axis")
      .def("getMaximum", &IMDDimension::getMaximum,
           "Return the maximum extent of this dimension")
      .def("getMinimum", &IMDDimension::getMinimum,
           "Return the maximum extent of this dimension")
      .def("getNBins", &IMDDimension::getNBins,
           "Return the number of bins dimension have (an integrated has one). "
           "A axis directed along dimension would have getNBins+1 axis points.")
      .def("getX", &IMDDimension::getX,
           "Return coordinate of the axis at the given index")
      .def("getDimensionId", &IMDDimension::getDimensionId,
           "Return a short name which identify the dimension among other "
           "dimension."
           "A dimension can be usually find by its ID and various  ")
      .def("getUnits", &getUnitsAsStr,
           "Return the units associated with this dimension.")
      .def("getMDFrame", &getMDFrame,
           "Return the multidimensional frame for this dimension.");
}
