#include <boost/python/class.hpp>
#include <boost/python/module.hpp>
#include <boost/python/init.hpp>

#include "MantidGeometry/Instrument/DetectorInfoIterator.h"

using Mantid::Geometry::DetectorInfoIterator;
using namespace boost::python;

void export_DetectorInfoIterator() {

  // Export to Python
  class_<DetectorInfoIterator, boost::noncopyable>("DetectorInfoIterator", no_init);
}
