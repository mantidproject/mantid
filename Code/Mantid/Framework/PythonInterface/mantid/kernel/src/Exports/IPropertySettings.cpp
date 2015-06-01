#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/IPropertyManager.h"
#include <boost/python/class.hpp>
#include <boost/python/register_ptr_to_python.hpp>

using Mantid::Kernel::IPropertySettings;
using namespace boost::python;

void export_IPropertySettings()
{
  register_ptr_to_python<IPropertySettings*>();

  class_<IPropertySettings,boost::noncopyable>("IPropertySettings", no_init)
    .def("isEnabled", &IPropertySettings::isEnabled,
         "Is the property to be shown as enabled in the GUI. Default true.")

    .def("isVisible", &IPropertySettings::isVisible,
         "Is the property to be shown in the GUI? Default true.")
         ;
}

