//
// Wrappers for classes in the Mantid::Kernel namespace
//

#include <boost/python.hpp>
//Kernel
#include<MantidKernel/Property.h>
#include<MantidKernel/FileProperty.h>
#include<MantidKernel/BoundedValidator.h>
#include<MantidKernel/MandatoryValidator.h>
#include<MantidKernel/ListValidator.h>
#include<MantidKernel/Logger.h>

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
      .def("name", &Mantid::Kernel::Property::name, return_value_policy<copy_const_reference>())
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
  
  void export_validators()
  {
#define EXPORT_IVALIDATOR(type,suffix)				\
    class_<Kernel::IValidator<type>, boost::noncopyable >("IValidator_"#suffix, no_init);
    
    EXPORT_IVALIDATOR(int,int);
    EXPORT_IVALIDATOR(double,dbl);
    EXPORT_IVALIDATOR(std::string,int);
#undef EXPORT_IVALIDATOR

    // Bounded validators
#define EXPORT_BOUNDEDVALIDATOR(type,suffix)\
    class_<Kernel::BoundedValidator<type>, bases<Kernel::IValidator<type> > >("BoundedValidator_"#suffix) \
      .def(init<type,type>())\
      .def("setLower", (void (Kernel::BoundedValidator<type>::*)(const type& value))&Kernel::BoundedValidator<type>::setLower )\
      .def("setUpper", (void (Kernel::BoundedValidator<type>::*)(const type& value))&Kernel::BoundedValidator<type>::setUpper )\
      ;\

    EXPORT_BOUNDEDVALIDATOR(double,dbl);
    EXPORT_BOUNDEDVALIDATOR(int,int);
#undef EXPORT_BOUNDEDVALIDATOR      
    
    // Mandatory validators
#define EXPORT_MANDATORYVALIDATOR(type,suffix)\
    class_<Kernel::MandatoryValidator<type>, bases<Kernel::IValidator<type> > >("MandatoryValidator_"#suffix); \
      
    EXPORT_MANDATORYVALIDATOR(int,int);
    EXPORT_MANDATORYVALIDATOR(double,dbl);
    EXPORT_MANDATORYVALIDATOR(std::string,str);
#undef EXPORT_MANDATORYVALIDATOR

    //List validator
    class_<Kernel::ListValidator, bases<Kernel::IValidator<std::string> > >("ListValidator_str", init<std::vector<std::string> >())
//      .def(init<std::vector<std::string> >())
      .def("addAllowedValue", &Kernel::ListValidator::addAllowedValue)
      ;
  }

  void export_logger()
  {
    class_<Kernel::Logger, boost::noncopyable>("MantidLogger", no_init)
      .def("notice", (void(Kernel::Logger::*)(const std::string&))&Kernel::Logger::notice)
      .def("information", (void(Kernel::Logger::*)(const std::string&))&Kernel::Logger::information)
      .def("error", (void(Kernel::Logger::*)(const std::string&))&Kernel::Logger::error)
      ;
  }

  void export_kernel_namespace()
  {
    export_property();
    export_validators();
    export_logger();
  }
  
  //@endcond

}
}
