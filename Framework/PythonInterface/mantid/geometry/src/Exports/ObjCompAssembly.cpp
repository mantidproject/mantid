#include "MantidGeometry/Instrument/ObjCompAssembly.h"
#include "MantidPythonInterface/kernel/GetPointer.h"

#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::ICompAssembly;
using Mantid::Geometry::ObjCompAssembly;
using Mantid::Geometry::ObjComponent;
using namespace boost::python;

GET_POINTER_SPECIALIZATION(ObjCompAssembly)

void export_ObjCompAssembly() {
  register_ptr_to_python<boost::shared_ptr<ObjCompAssembly>>();

  class_<ObjCompAssembly, boost::python::bases<ICompAssembly, ObjComponent>,
         boost::noncopyable>("IObjCompAssembly", no_init);
}
