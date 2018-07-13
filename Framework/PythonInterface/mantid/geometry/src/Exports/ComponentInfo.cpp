#include "MantidGeometry/Instrument/ComponentInfo.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::ComponentInfo;
using namespace boost::python;

void export_ComponentInfo() {
  // WARNING SpectrumInfo is work in progress and not ready for exposing more of
  // its functionality to Python, and should not yet be used in user scripts. DO
  // NOT ADD EXPORTS TO OTHER METHODS without contacting the team working on
  // Instrument-2.0.
  class_<ComponentInfo, boost::noncopyable>("ComponentInfo", no_init);
}
