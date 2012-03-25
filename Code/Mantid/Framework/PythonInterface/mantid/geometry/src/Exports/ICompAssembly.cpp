#include "MantidGeometry/ICompAssembly.h"
#include "MantidPythonInterface/kernel/SharedPtrToPythonMacro.h"
#include <boost/python/class.hpp>

using Mantid::Geometry::ICompAssembly;
using Mantid::Geometry::IComponent;
using namespace boost::python;

void export_ICompAssembly()
{

  REGISTER_SHARED_PTR_TO_PYTHON(ICompAssembly);

  class_<ICompAssembly, boost::python::bases<IComponent>, boost::noncopyable>("ICompAssembly", no_init)
    .def("nelements", &ICompAssembly::nelements, "Returns the number of elements in the assembly")
    .def("__getitem__", &ICompAssembly::operator[], "Return the component at the given index")
    ;

}

