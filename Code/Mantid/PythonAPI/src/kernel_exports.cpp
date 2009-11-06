//
// Wrappers for classes in the Mantid::Kernel namespace
//

#include <boost/python.hpp>
//Kernel
#include<MantidKernel/Property.h>

#include <MantidPythonAPI/stl_proxies.h>

namespace Mantid
{
namespace PythonAPI
{
  using namespace boost::python;
  
  //@cond

  void export_property()
  {
    
    //Pointer 
    register_ptr_to_python<Mantid::Kernel::Property*>();
    //Vector
    vector_proxy<Mantid::Kernel::Property*>::wrap("stl_vector_property");
    
    class_< Mantid::Kernel::Property, boost::noncopyable>("Property", no_init)
      .def("isValid", &Mantid::Kernel::Property::isValid)
      .def("value", &Mantid::Kernel::Property::value)
      .def("allowedValues", &Mantid::Kernel::Property::allowedValues)
      .def("direction", &Mantid::Kernel::Property::direction)
      ;
  }

  void export_kernel_namespace()
  {
    export_property();
  }
  
  //@endcond

}
}
