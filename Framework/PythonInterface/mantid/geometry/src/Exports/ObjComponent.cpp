#include "MantidGeometry/Instrument/ObjComponent.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::Component;
using Mantid::Geometry::IObjComponent;
using Mantid::Geometry::ObjComponent;
using namespace boost::python;

void export_ObjComponent() {
  class_<ObjComponent, boost::python::bases<IObjComponent, Component>,
         boost::noncopyable>("ObjComponent", no_init);
}
