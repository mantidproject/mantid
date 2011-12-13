#include "MantidGeometry/IObjComponent.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::IObjComponent;
using Mantid::Geometry::IComponent;
using namespace boost::python;

void export_IObjComponent()
{
  register_ptr_to_python<boost::shared_ptr<IObjComponent> >();

  class_< IObjComponent, boost::python::bases<IComponent>, boost::noncopyable>("IObjComponent", no_init)
    ;

}

