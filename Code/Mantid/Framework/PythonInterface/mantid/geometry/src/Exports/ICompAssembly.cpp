#include "MantidGeometry/ICompAssembly.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Geometry::ICompAssembly;
using Mantid::Geometry::IComponent;
using namespace boost::python;

void export_ICompAssembly()
{

  register_ptr_to_python<boost::shared_ptr<ICompAssembly> >();

  class_<ICompAssembly, boost::python::bases<IComponent>, boost::noncopyable>("ICompAssembly", no_init)
    .def("nelements", &ICompAssembly::nelements, "Returns the number of elements in the assembly")
    .def("__getitem__", &ICompAssembly::operator[], "Return the component at the given index")
    ;

}

