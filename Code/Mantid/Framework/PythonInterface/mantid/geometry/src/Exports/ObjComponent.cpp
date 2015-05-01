#include "MantidGeometry/Instrument/ObjComponent.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::ObjComponent;
using Mantid::Geometry::IObjComponent;
using Mantid::Geometry::Component;
using namespace boost::python;

// clang-format off
void export_ObjComponent()
// clang-format on
{
  class_<ObjComponent, boost::python::bases<IObjComponent, Component>, boost::noncopyable>("ObjComponent", no_init)
    ;
}

