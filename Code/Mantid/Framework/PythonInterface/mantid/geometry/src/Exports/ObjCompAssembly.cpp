#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"

#include <boost/python/class.hpp>

using Mantid::Geometry::ObjCompAssembly;
using Mantid::Geometry::ICompAssembly;
using Mantid::Geometry::ObjComponent;
using namespace boost::python;

void export_ObjCompAssembly()
{
  REGISTER_SHARED_PTR_TO_PYTHON(ObjCompAssembly);

  class_<ObjCompAssembly, boost::python::bases<ICompAssembly, ObjComponent>, boost::noncopyable>("IObjCompAssembly", no_init)
    ;
}

