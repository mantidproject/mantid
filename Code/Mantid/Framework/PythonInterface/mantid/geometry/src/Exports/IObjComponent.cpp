#include "MantidGeometry/IObjComponent.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::IObjComponent;
using Mantid::Geometry::IComponent;
using namespace boost::python;

void export_IObjComponent()
{
  REGISTER_SHARED_PTR_TO_PYTHON(IObjComponent);

  class_< IObjComponent, boost::python::bases<IComponent>, boost::noncopyable>("IObjComponent", no_init)
    ;

}

