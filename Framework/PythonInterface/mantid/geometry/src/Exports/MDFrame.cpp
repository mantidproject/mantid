#include "MantidGeometry/MDGeometry/MDFrame.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using namespace Mantid::Geometry;

using namespace boost::python;

void export_MDFrame() {

  using namespace Mantid::Geometry;

  register_ptr_to_python<boost::shared_ptr<MDFrame>>();

  class_<MDFrame, boost::noncopyable>("MDFrame", no_init)
      .def("getUnitLabel", &MDFrame::getUnitLabel)
      .def("name", &MDFrame::name);
}
