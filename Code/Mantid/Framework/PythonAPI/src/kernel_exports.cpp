//
// Wrappers for classes in the Mantid::Kernel namespace
//
#include <boost/python.hpp>
#include <string>
#include <vector>
#include "MantidPythonAPI/kernel_exports.h"
//Kernel
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/BoundedValidator.h"
#include "MantidKernel/ArrayBoundedValidator.h"
#include "MantidKernel/MandatoryValidator.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/Unit.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/TimeSeriesProperty.h"

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
      .add_property("name", make_function(&Mantid::Kernel::Property::name, return_value_policy<copy_const_reference>()))
      .add_property("isValid", &Mantid::Kernel::Property::isValid)
      .add_property("value", &Mantid::Kernel::Property::value)
      .add_property("allowedValues", &Mantid::Kernel::Property::allowedValues)
      .add_property("direction", &Mantid::Kernel::Property::direction)
      .add_property("units", &Mantid::Kernel::Property::units)
      .add_property("isDefault", &Mantid::Kernel::Property::isDefault)
      ;


    // property with value - scalars
#define EXPORT_PROP_W_VALUE(type, suffix)   \
	class_<Mantid::Kernel::PropertyWithValue<type>, \
	       bases<Mantid::Kernel::Property>, boost::noncopyable>("PropertyWithValue_"#suffix, no_init) \
	           .add_property("value", make_function(&Mantid::Kernel::PropertyWithValue<type>::operator(), return_value_policy<copy_const_reference>())) \
	           ;
    EXPORT_PROP_W_VALUE(double,dbl);
    EXPORT_PROP_W_VALUE(int,int);
    EXPORT_PROP_W_VALUE(bool,bool);
#undef EXPORT_PROP_W_VALUE
// TODO template DLLExport class PropertyWithValue<std::string>;

    // property with value - vectors
#define EXPORT_PROP_W_VALUE_V(type, suffix)   \
	class_<Mantid::Kernel::PropertyWithValue<std::vector<type> >, \
	       bases<Mantid::Kernel::Property>, boost::noncopyable>("PropertyWithValue_"#suffix, no_init) \
	           .add_property("value", make_function(&Mantid::Kernel::PropertyWithValue<std::vector<type> >::operator(), return_value_policy<copy_const_reference>())) \
	           ;

    EXPORT_PROP_W_VALUE_V(double,dbl);
    EXPORT_PROP_W_VALUE_V(int,int);
    EXPORT_PROP_W_VALUE_V(long,long);
#undef EXPORT_PROP_W_VALUE_V
// TODO template DLLExport class PropertyWithValue<std::vector<std::string> >;

    // array property
#define EXPORT_ARRAY_PROP(type, suffix) \
    class_<Mantid::Kernel::ArrayProperty<type>, \
           bases<Mantid::Kernel::PropertyWithValue<std::vector<type> > >, boost::noncopyable>("ArrayProperty_"#suffix, no_init);

    EXPORT_ARRAY_PROP(double,dbl);
    EXPORT_ARRAY_PROP(int,int);
#undef EXPORT_ARRAY_PROP
// TODO template DLLExport class ArrayProperty<std::string>;

    class_<Mantid::Kernel::TimeSeriesPropertyStatistics>("TimeSeriesPropertyStatistics", no_init)
          .add_property("minimum", &Mantid::Kernel::TimeSeriesPropertyStatistics::minimum)
          .add_property("maximum", &Mantid::Kernel::TimeSeriesPropertyStatistics::maximum)
          .add_property("mean", &Mantid::Kernel::TimeSeriesPropertyStatistics::mean)
          .add_property("median", &Mantid::Kernel::TimeSeriesPropertyStatistics::median)
          .add_property("standard_deviation", &Mantid::Kernel::TimeSeriesPropertyStatistics::standard_deviation)
          .add_property("duration", &Mantid::Kernel::TimeSeriesPropertyStatistics::duration)
          ;

    class_<Mantid::Kernel::TimeSeriesProperty<double>, \
           bases<Mantid::Kernel::Property>, boost::noncopyable>("TimeSeriesProperty_dbl", no_init)
              .def("getStatistics", &Mantid::Kernel::TimeSeriesProperty<double>::getStatistics)
              .add_property("value", &Mantid::Kernel::TimeSeriesProperty<double>::valuesAsVector)
              .add_property("times", &Mantid::Kernel::TimeSeriesProperty<double>::timesAsVector)
    	      ;

    class_<Mantid::Kernel::DateAndTime>("DateAndTime", no_init)
        .def("__str__", &Mantid::Kernel::DateAndTime::to_ISO8601_string);
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
      ;

    EXPORT_BOUNDEDVALIDATOR(double,dbl);
    EXPORT_BOUNDEDVALIDATOR(int,int);
#undef EXPORT_BOUNDEDVALIDATOR      

#define EXPORT_IVALIDATOR_V(type,suffix)        \
    class_<Kernel::IValidator<std::vector<type> >, boost::noncopyable >("IValidator_"#suffix, no_init);

    EXPORT_IVALIDATOR_V(int,int);
    EXPORT_IVALIDATOR_V(double,dbl);
#undef EXPORT_IVALIDATOR_V

#define EXPORT_ARRAYBOUNDEDVALIDATOR(type,suffix) \
    class_<Kernel::ArrayBoundedValidator<type>, bases<Kernel::IValidator<std::vector<type> > > >("ArrayBoundedValidator_"#suffix) \
      .def("setLower", (void (Kernel::ArrayBoundedValidator<type>::*)(const type& value))&Kernel::ArrayBoundedValidator<type>::setLower )\
      .def("setUpper", (void (Kernel::ArrayBoundedValidator<type>::*)(const type& value))&Kernel::ArrayBoundedValidator<type>::setUpper )\
      ;

    EXPORT_ARRAYBOUNDEDVALIDATOR(int,int);
    EXPORT_ARRAYBOUNDEDVALIDATOR(double,dbl);
#undef EXPORT_ARRAYBOUNDEDVALIDATOR

    // Mandatory validators
#define EXPORT_MANDATORYVALIDATOR(type,suffix)\
    class_<Kernel::MandatoryValidator<type>, bases<Kernel::IValidator<type> > >("MandatoryValidator_"#suffix); \
      
    EXPORT_MANDATORYVALIDATOR(int,int);
    EXPORT_MANDATORYVALIDATOR(double,dbl);
    EXPORT_MANDATORYVALIDATOR(std::string,str);
#undef EXPORT_MANDATORYVALIDATOR

    //List validator
    class_<Kernel::ListValidator, bases<Kernel::IValidator<std::string> > >("ListValidator_str", init<std::vector<std::string> >())
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

  void export_instrumentinfo()
  {
    // Inform Python about a vector of such objects
    vector_proxy<InstrumentInfo>::wrap("stl_vector_instrumentinfo");

    class_<InstrumentInfo>("InstrumentInfo",no_init)
      //special methods
      .def("__str__", &InstrumentInfo::name)
      //standard methods
      .def("name", &InstrumentInfo::name)
      .def("shortName", &InstrumentInfo::shortName)
      .def("zeroPadding", &InstrumentInfo::zeroPadding)
      .def("techniques", &InstrumentInfo::techniques, return_value_policy<copy_const_reference>())
      ;
   }

  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(FacilityInfo_instrumentOverloads, Mantid::Kernel::FacilityInfo::Instrument, 0, 1)

  void export_facilityinfo()
  {
    class_<FacilityInfo>("FacilityInfo",no_init)
      .def("name", &FacilityInfo::name)
      .def("zeroPadding", &FacilityInfo::zeroPadding)
      .def("extensions", &FacilityInfo::extensions)
      .def("preferredExt", &FacilityInfo::preferredExtension)
      .def("instrument", &FacilityInfo::Instrument, FacilityInfo_instrumentOverloads()[return_value_policy<copy_const_reference>()])
      .def("instruments", (const std::vector<InstrumentInfo>& (FacilityInfo::*)() const)&FacilityInfo::Instruments, return_value_policy<copy_const_reference>())
      .def("instruments", (std::vector<InstrumentInfo> (FacilityInfo::*)(const std::string &) const)&FacilityInfo::Instruments)
      ;
  }

  BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(ConfigService_facilityOverloads, Mantid::PythonAPI::ConfigServiceWrapper::facility, 0, 1)

  void export_configservice()
  {
    class_<ConfigServiceWrapper>("ConfigService")
      // Special methods
      .def("__getitem__", &ConfigServiceWrapper::getString)
      .def("__setitem__", &ConfigServiceWrapper::setString)
      // Standard methods
      .def("facility", &ConfigServiceWrapper::facility, ConfigService_facilityOverloads())
      .def("welcomeMessage", &ConfigServiceWrapper::welcomeMessage)
      .def("getDataSearchDirs",&ConfigServiceWrapper::getDataSearchDirs)
      .def("setDataSearchDirs", (void (ConfigServiceWrapper::*)(const std::string &))&ConfigServiceWrapper::setDataSearchDirs)
      .def("setDataSearchDirs", (void (ConfigServiceWrapper::*)(const boost::python::list &))&ConfigServiceWrapper::setDataSearchDirs)
      .def("appendDataSearchDir", &ConfigServiceWrapper::appendDataSearchDir)
      .def("getInstrumentDirectory", &ConfigServiceWrapper::getInstrumentDirectory)
      .def("getUserFilename", &ConfigServiceWrapper::getUserFilename)
      .def("saveConfig", &ConfigServiceWrapper::saveConfig)
      ;
  }

  void export_units()
  {
    //
    register_ptr_to_python<Mantid::Kernel::Unit_sptr>();
    // Class Object & Methods
    class_<Mantid::Kernel::Unit, boost::noncopyable >("MantidUnit", no_init)
      .def("name", &Mantid::Kernel::Unit::unitID)
      .def("label", &Mantid::Kernel::Unit::label)
      .def("caption", &Mantid::Kernel::Unit::caption)
      ;
  }

  void export_kernel_namespace()
  {
    export_property();
    export_validators();
    export_logger();
    export_instrumentinfo();
    export_facilityinfo();
    export_configservice();
    export_units();
  }
  
  //@endcond

}
}
