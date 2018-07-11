#include "MantidGeometry/Instrument/DetectorInfo.h"

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/return_value_policy.hpp>

using Mantid::Geometry::DetectorInfo;
using namespace boost::python;

void export_DetectorInfo() {

  class_<DetectorInfo, boost::noncopyable>("DetectorInfo", no_init);

}
