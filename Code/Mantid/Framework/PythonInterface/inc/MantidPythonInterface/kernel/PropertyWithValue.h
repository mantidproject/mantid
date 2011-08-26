#ifndef MANTID_PYTHONINTERFACE_PROPERTY_HPP_
#define MANTID_PYTHONINTERFACE_PROPERTY_HPP_

#include "MantidKernel/PropertyWithValue.h"

/**
 * Define a macro to export PropertyWithValue template types
 */
#define EXPORT_PROP_W_VALUE(type, suffix)   \
  class_<Mantid::Kernel::PropertyWithValue<type>, \
         bases<Mantid::Kernel::Property>, boost::noncopyable>("PropertyWithValue"#suffix, no_init) \
             .add_property("value", make_function(&Mantid::Kernel::PropertyWithValue<type>::operator(), return_value_policy<copy_const_reference>())) \
             ;

#endif /* MANTID_PYTHONINTERFACE_PROPERTY_HPP_ */
