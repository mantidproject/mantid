#include "MantidGeometry/Instrument/DetectorInfoIterator.h"

#include <boost/python/class.hpp>
#include <boost/python/init.hpp>
#include <boost/python/module.hpp>

using Mantid::Geometry::DetectorInfoIterator;
using namespace boost::python;

// Export DetectorInfoIterator
void export_DetectorInfoIterator() {

  // Export to Python
  class_<DetectorInfoIterator, boost::noncopyable>("DetectorInfoIterator",
                                                   no_init);
}
