#include "MantidGeometry/MDGeometry/IMDDimension.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"

#include <boost/python/class.hpp>


using Mantid::Geometry::IMDDimension;
using Mantid::Geometry::IMDDimension_sptr;
using namespace boost::python;

void export_IMDDimension()
{
  REGISTER_SHARED_PTR_TO_PYTHON(IMDDimension);

  class_< IMDDimension, boost::noncopyable >("IMDDimension", no_init)
      .def("getName", &IMDDimension::getName, "Return the name of the dimension as can be displayed along the axis")
      .def("getMaximum", &IMDDimension::getMaximum, "Return the maximum extent of this dimension")
      .def("getMinimum", &IMDDimension::getMinimum, "Return the maximum extent of this dimension")
      .def("getNBins", &IMDDimension::getNBins, "Return the number of bins dimension have (an integrated has one). "
           "A axis directed along dimension would have getNBins+1 axis points.")
      .def("getX", &IMDDimension::getX, "Return coordinate of the axis at the given index")
      .def("getDimensionId", &IMDDimension::getDimensionId, "Return a short name which identify the dimension among other dimension."
           "A dimension can be usually find by its ID and various  ")
      .def("getUnits",  &IMDDimension::getDimensionId, "Return the units associated with this dimension.")
      ;
}

