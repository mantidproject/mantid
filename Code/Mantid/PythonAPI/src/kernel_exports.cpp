//
// Wrappers for classes in the Mantid::Kernel namespace
//

#include <boost/python.hpp>
//Kernel
#include<MantidKernel/Property.h>
#include<MantidKernel/FileProperty.h>

#include <MantidPythonAPI/stl_proxies.h>

namespace Mantid
{
namespace PythonAPI
{
  using namespace boost::python;
  using namespace Mantid::Kernel;
  //@cond

  void export_property()
  {
    
    //Pointer 
    register_ptr_to_python<Mantid::Kernel::Property*>();
    //Vector
    vector_proxy<Mantid::Kernel::Property*>::wrap("stl_vector_property");

    //Direction
    enum_<Direction::Type>("Direction")
      .value("Input", Direction::Input)
      .value("Output", Direction::Output)
      .value("InOut", Direction::InOut)
      .value("None", Direction::None)
      ;
    
    class_< Mantid::Kernel::Property, boost::noncopyable>("Property", no_init)
      .def("isValid", &Mantid::Kernel::Property::isValid)
      .def("value", &Mantid::Kernel::Property::value)
      .def("allowedValues", &Mantid::Kernel::Property::allowedValues)
      .def("direction", &Mantid::Kernel::Property::direction)
      ;

    //FileProperty enum
    enum_<FileProperty::FileAction>("FileAction")
      .value("Save", FileProperty::Save)
      .value("OptionalSave", FileProperty::OptionalSave)
      .value("Load", FileProperty::Load)
      .value("OptionalLoad", FileProperty::OptionalLoad)
      ;
  }

  void export_kernel_namespace()
  {
    export_property();
  }
  
  //@endcond

}
}
