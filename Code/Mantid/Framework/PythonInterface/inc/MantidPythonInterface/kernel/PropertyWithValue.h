#ifndef MANTID_PYTHONINTERFACE_PROPERTY_HPP_
#define MANTID_PYTHONINTERFACE_PROPERTY_HPP_

#include "MantidKernel/PropertyWithValue.h"

#include <boost/python/bases.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/copy_const_reference.hpp>

/**
 * Define a macro to export PropertyWithValue template types
 */
#define EXPORT_PROP_W_VALUE(type)   \
  class_<Mantid::Kernel::PropertyWithValue<type>, \
     boost::python::bases<Mantid::Kernel::Property>, boost::noncopyable>("PropertyWithValue_"#type, boost::python::no_init) \
   .add_property("value", make_function(&Mantid::Kernel::PropertyWithValue<type>::operator(), \
       boost::python::return_value_policy<boost::python::copy_const_reference>())) \
   ;

#endif /* MANTID_PYTHONINTERFACE_PROPERTY_HPP_ */
